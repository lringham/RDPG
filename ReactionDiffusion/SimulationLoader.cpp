#include "SimulationLoader.h"
#include "ResourceLocations.h"
#include "GPUSimulation.h"
#include "Mesh.h"
#include "Grid.h"
#include "ObjModel.h"
#include "Ply.h"
#include "Utils.h"
#include "Textures.h"

#include <iostream>
#include <sstream>
#include <tuple>


std::string SimulationLoader::createModelSubroutine(const std::vector<std::string>& morphogens, std::string customModel, const std::string& domainstr, bool stochastic, bool veins, const Parameters& params)
{
    bool isAGrid = Utils::sToLower(domainstr) == "grid";
    bool isAMesh = !isAGrid;

    // Add model morph variables
    std::string modelSubroutine = "subroutine (calculateSub) void model(uint gid)\n{\n";

    for (unsigned i = 0; i < morphogens.size(); ++i)
        modelSubroutine += "\t const float " + Utils::sToLower(morphogens[i]) + " = cells1[gid].vals[" + morphogens[i] + "];\n";

    for (auto& p : params)
        if (p.first != "growing" && p.first != "growthTickLimit")
            modelSubroutine += "\tfloat " + p.first + " = params[gid]." + p.first + ";\n";
    
	if (!stochastic)
		modelSubroutine += "\tfloat L[MORPHOGEN_COUNT];\n\tlap(gid, L);";
	else 
    {
		if (!veins)
			modelSubroutine += "\tfloat L[MORPHOGEN_COUNT];\n\tfloat LNoise[MORPHOGEN_COUNT];\n\tlap_noise(gid, L, LNoise);";
		else
			modelSubroutine += "\tfloat L[MORPHOGEN_COUNT];\n\tfloat LNoise[MORPHOGEN_COUNT];\n\tlap_noise_veins(gid, L, LNoise);";
	}

    for (unsigned i = 0; i < morphogens.size(); ++i)
        customModel += "\tif(isConstVal[gid].vals[" + morphogens[i] + "])\n\t\tcells2[gid].vals[" + morphogens[i] + "] = " + Utils::sToLower(morphogens[i]) + ";\n";

    // Add model output
    if (isAMesh)
        modelSubroutine += customModel + "\tif(outMorphIndex > -1)\n\t\toutValues[gid].colors[0] = cells2[gid].vals[outMorphIndex];\n}\n";
    else if (isAGrid)
        modelSubroutine += customModel + "\timageStore(outputTexture, ivec2(gl_GlobalInvocationID.yx), vec4(cells2[gid].vals[outMorphIndex]));\n}\n";

    return modelSubroutine;
}

std::string SimulationLoader::createModelFunc(const std::vector<std::string>& morphogens, std::string customModel, Parameters params)
{
    std::string modelSubroutine = "";

    modelSubroutine = R"(
#include <vector>
#include <set>
#include <map>
#include <string>
#include <cmath>

using Parameters = std::map<std::string, float>;

struct Cell
{
	std::vector<float> vals;
	std::vector<bool> isConstVal;

	Cell() = default;

	explicit Cell(int morphCount)
		: vals(morphCount, 0.f)
	{}

	float operator [](int i) const
	{
		return vals[i];
	}

	float &operator [](int i)
	{
		return vals[i];
	}

};

using ThreadWork = std::vector<std::tuple<Parameters, std::vector<unsigned>>>;

extern "C"
{
#ifdef _WIN32
	__declspec(dllexport) 
#endif
    void simulate(
		std::vector<Cell>& readFrom,
		std::vector<Cell>& writeTo,
		std::vector<float>& L,
		ThreadWork& threadWork,
		const size_t MORPH_COUNT,
        const size_t stepCount)
    {         
        using std::pow;
        auto mix = [](float v0, float v1, float s) {
            return (1.f-s) * v0 + s * v1;
        };
        auto clamp = [](float x, float minv, float maxv) {
            return x < minv ? minv : x > maxv ? maxv : x;
        };
)";

    for (unsigned i = 0; i < morphogens.size(); ++i)
    {
        modelSubroutine += "\t\t#define " + morphogens[i] + " " + std::to_string(i) + "\n";
    }

    for (auto& p : params)
        if (p.first != "growing" && p.first != "growthTickLimit")
            modelSubroutine += "\t\tfloat " + p.first + " = 0;\n";

    modelSubroutine += "\t\tfor (auto& workTuple : threadWork)\n";
    modelSubroutine += "\t\t{\n";
    modelSubroutine += "\t\t\tauto& params = std::get<0>(workTuple);\n";
    for (auto& p : params)
        if (p.first != "growing" && p.first != "growthTickLimit")
            modelSubroutine += "\t\t\t" + p.first + " = params.at(\"" + p.first + "\");\n";

    modelSubroutine += "\t\t\tfor (unsigned gid : std::get<1>(workTuple))\n\t\t\t{\n";

    for (unsigned i = 0; i < morphogens.size(); ++i)
        modelSubroutine += "\t\t\t\t const float " + Utils::sToLower(morphogens[i]) + " = readFrom[gid][" + morphogens[i] + "];\n";

    modelSubroutine += R"(
)";

    modelSubroutine += customModel;

    for (unsigned i = 0; i < morphogens.size(); ++i)
    {
        modelSubroutine += "\t\t\t\tif(readFrom[gid].isConstVal[" + morphogens[i] + "])\n";
        modelSubroutine += "\t\t\t\t\twriteTo[gid][" + morphogens[i] + "] = " + Utils::sToLower(morphogens[i]) + ";\n";
    }

    modelSubroutine += "\t\t\t}\n\t\t}\n\t}\n}";
    return modelSubroutine;
}

std::string SimulationLoader::convertToGPUCode(std::string source)
{
    Utils::findAndReplace(source, "new[", "cells2[gid].vals[");
    Utils::findAndReplace(source, "params.", "params[gid].");
    return source;
}

std::string SimulationLoader::convertToCPUCode(std::string source)
{
    Utils::findAndReplace(source, "new[", "\t\t\twriteTo[gid][");
    Utils::findAndReplace(source, "params.", "");
    return source;
}

bool SimulationLoader::loadSim(std::string filename, SimulationDomain*& d, Simulation*& s, Camera& camera, const CmdArgsParser& cmdArgsParser)
{
    std::ifstream file;
    if (filename == "")
    {  
#ifdef _WIN32	    
        if (Utils::doesFileExist("./SimConfig.txt"))
#endif
        {
            filename = "./SimConfig.txt";
        }
    }

    if (filename != "")
    {
        std::cout << "Loading: " << filename << "\n";
        file.open(filename);
        if (!file.is_open())
        {
            LOG("Failed to load simfile [" + filename + "]");
            return false;
        }
    }

    std::vector<Simulation::InitParams> initParams;
    std::vector<Simulation::BoundaryConditions> boundaryConditions;
    std::vector<Simulation::InitConditions> initConditions;
    
    Parameters globalParamMap;

    std::string simType;
    std::string simFile;
    std::string domainStr;
    std::string growthMode;
    std::vector<std::string> colorMaps(2, "");

    std::string backgroundTexture = "";
    bool maxFaceAreaFound = false;
    bool cameraMatFound = false;
    Vec3 camPosition, camLookAt;
    bool modelMatFound = false;
    Vec3 modelX(1, 0, 0), modelY(0, 1, 0), modelZ(0, 0, 1);
    bool growing = false;
    float growthX = 0.f, growthY = 0.f, growthZ = 0.f;
    unsigned long long growthTickLimit = 0;
    long long pauseAt = 0, exitAt = 0;
    float maxFaceArea = 1.f;

    bool hasParams = false;
    bool hasInitialConditions = false;
    bool hasBoundaryConditions = false;

    std::string veinMorph = "";
	float veinDiffusionScale = 1.f;
	float petalDiffusionScale = 1.f;

    bool prepatternInfo0enabled = false;
    bool prepatternInfo0UseSquare = false;
    std::string prepatternInfo0targetIndex = "";
    std::string prepatternInfo0sourceIndex = "";
    float prepatternInfo0lowVal = 1.f;
    float prepatternInfo0highVal = 1.f;
    float prepatternInfo0exponent = 1.f;

    bool prepatternInfo1enabled = false;
    bool prepatternInfo1UseSquare = false;
    std::string prepatternInfo1targetIndex = "";
    std::string prepatternInfo1sourceIndex = "";
    float prepatternInfo1lowVal = 1.f;
    float prepatternInfo1highVal = 1.f;
    float prepatternInfo1exponent = 1.f;

    // custom model parameters
    std::string customModel;
    std::vector<std::string> morphogens;
    std::map<std::string, std::string> includes;
    std::map<std::string, int> morphIndexMap;

    // Read file
    std::vector<std::string> configFileLines;
    {
        std::string line = "";
        if (file.is_open())
        {
            while (std::getline(file, line))
                if(line != "")
                    configFileLines.push_back(line);
            file.close();
        }
    }

    for (size_t i = 0; i < configFileLines.size(); ++i)
    {    
        std::string line = configFileLines[i];

        // Skip empty lines, comments, non valid lines
        if (line.length() == 0 || line[0] == '#' || line.find(":") == std::string::npos)
            continue;

        // Strip end comments
        if (line.find("#") != std::string::npos)
            line = line.substr(0, line.find("#"));

        // Parse key values
        std::string label;
        std::string value;
        size_t loc = static_cast<unsigned>(line.find(":"));
        if (loc != std::string::npos)
        {
            std::string substring = line.substr(0, loc);
            label = Utils::trim(substring);

            substring = line.substr(loc + 1);
            value = Utils::trim(substring);
        }

        // 
        if (label == "domain")
            domainStr = value;
        else if (label == "ModelsPath")
            ResourceLocations::instance().modelsPath(value);
        else if (label == "ColorMapsPath")
            ResourceLocations::instance().colorMapsPath(value);
        else if (label == "ShadersPath")
            ResourceLocations::instance().shadersPath(value);
        else if (label == "simFile")
            simFile = value;
        else if (label == "colorMap")
        {
            colorMaps[0] = value;
            colorMaps[1] = value;
        }
        else if (label == "colorMapOutside")
        {
            colorMaps[0] = value;
        }
        else if (label == "colorMapInside")
        {
            colorMaps[1] = value;
        }
        else if (label == "backgroundTexture")
        {
            backgroundTexture = value;
        }
        else if (label == "camera")
        {
            const char* data = value.data();
            char* extra;
            camPosition.x = strtof(data, &extra);
            camPosition.y = strtof(extra, &extra);
            camPosition.z = strtof(extra, &extra);
            camLookAt.x = strtof(extra, &extra);
            camLookAt.y = strtof(extra, &extra);
            camLookAt.z = strtof(extra, nullptr);
            cameraMatFound = true;
        }
        else if (label == "model")
        {
            const char* data = value.data();
            char* extra;
            modelX.x = strtof(data, &extra);
            modelX.y = strtof(extra, &extra);
            modelX.z = strtof(extra, &extra);
            modelY.x = strtof(extra, &extra);
            modelY.y = strtof(extra, &extra);
            modelY.z = strtof(extra, &extra);
            modelZ.x = strtof(extra, &extra);
            modelZ.y = strtof(extra, &extra);
            modelZ.z = strtof(extra, nullptr);
            modelMatFound = true;
        }
        else if (label == "morphogens")
        {
            size_t pos = 0;
            while ((pos = value.find(",")) != std::string::npos)
            {
                std::string substring = value.substr(0, pos);
                morphogens.push_back(Utils::trim(substring));

                substring = value.substr(pos + 1, value.size());
                value = Utils::sToUpper(substring);
            }

            std::string substring = value.substr(0, value.size());
            morphogens.push_back(
                Utils::sToUpper(
                    Utils::trim(substring))
            );
        }
        else if (label == "growing")
            growing = Utils::sToLower(value) == "true";
        else if (label == "growthX")
            growthX = strtof(value.data(), nullptr);
        else if (label == "growthY")
            growthY = strtof(value.data(), nullptr);
        else if (label == "growthZ")
            growthZ = strtof(value.data(), nullptr);
        else if (label == "growthTickLimit")
            growthTickLimit = strtol(value.data(), nullptr, 10);
        else if (label == "maxFaceArea") 
        {
            maxFaceArea = strtof(value.data(), nullptr); 
            maxFaceAreaFound = true;
        }
        else if (label == "growthMode")
            growthMode = Utils::sToLower(value);
        else if (label == "pauseAt")
            pauseAt = strtol(value.data(), nullptr, 10);
        else if (label == "exitAt")
            exitAt = strtol(value.data(), nullptr, 10);
        else if (label == "prepatternEnabledT0")
            prepatternInfo0enabled = strtol(value.data(), nullptr, 10) != 0;
        else if (label == "prepatternTargetT0")
            prepatternInfo0targetIndex = value;
        else if (label == "prepatternSourceT0")
            prepatternInfo0sourceIndex = value;
        else if (label == "prepatternLowValT0")
            prepatternInfo0lowVal = strtof(value.data(), nullptr);
        else if (label == "prepatternHighValT0")
            prepatternInfo0highVal = strtof(value.data(), nullptr);
        else if (label == "prepatternExponentT0")
            prepatternInfo0exponent = strtof(value.data(), nullptr);
        else if (label == "prepatternUseSquareT0")
            prepatternInfo0UseSquare = strtol(value.data(), nullptr, 10) != 0;
        else if (label == "prepatternEnabledT1")
            prepatternInfo1enabled = strtol(value.data(), nullptr, 10) != 0;
        else if (label == "prepatternTargetT1")
            prepatternInfo1targetIndex = value;
        else if (label == "prepatternSourceT1")
            prepatternInfo1sourceIndex = value;
        else if (label == "prepatternLowValT1")
            prepatternInfo1lowVal = strtof(value.data(), nullptr);
        else if (label == "prepatternHighValT1")
            prepatternInfo1highVal = strtof(value.data(), nullptr);
        else if (label == "prepatternExponentT1")
            prepatternInfo1exponent = strtof(value.data(), nullptr);
        else if (label == "prepatternUseSquareT1")
            prepatternInfo1UseSquare = strtol(value.data(), nullptr, 10) != 0;
        else if (label == "veinMorph")
            veinMorph = value;
		else if (label == "veinDiffusionScale")
			veinDiffusionScale = strtof(value.data(), nullptr);
		else if (label == "petalDiffusionScale")
			petalDiffusionScale = strtof(value.data(), nullptr);
        else if (label == "params" || label == "initialConditions" || label == "boundaryConditions" || label == "rdModel")
        {            
            if (label == "params")
                hasParams = true;
            if (label == "initialConditions")
                hasInitialConditions = true;
            if (label == "boundaryConditions")
                hasBoundaryConditions = true;
            if (label == "rdModel")
            {
                if (cmdArgsParser.hasOption("SimDevice"))
                    simType = cmdArgsParser.getOption("SimDevice");
                else
                    simType = value;
            }

            if (i+1 == configFileLines.size() || configFileLines[i+1].find_first_of('{', 0) == std::string::npos)
                continue;

            // Skip these to be parsed later
            std::string subline = value;
            bool startSeen = false;
            int bracketCount = 0;
            do
            {
                for (char c : subline)
                {
                    if (c == '{')
                    {
                        if (!startSeen)
                            startSeen = true;
                        bracketCount++;
                    }
                    else if (c == '}')
                        bracketCount--;
                }

                if((bracketCount != 0 || !startSeen) && i < configFileLines.size())
                    subline = configFileLines[++i];

            } while (bracketCount != 0 || !startSeen);
            i--;
        }
        else
            globalParamMap[label] = (float)stof(value);
    }

    if (hasParams && !parseParams(initParams, rawParams, configFileLines))
    {
        LOG("Failed to load params");
        return false;
    }

    if (hasInitialConditions && !parseInitialConditions(initConditions, rawInitConds, configFileLines))
    {
        LOG("Failed to load initialConditions");
        return false;
    }

    if (hasBoundaryConditions && !parseBoundaryConditions(boundaryConditions, rawBCs, configFileLines))
    {
        LOG("Failed to load boundaryConditions");
        return false;
    }
    
    if (!parseRDModel(simType, customModel, rawRDModel, configFileLines, morphogens))
    {
        LOG("Failed to load rdModel");
        return false;
    }

    // Check for valid domain and simulation
    if (simType != "" && domainStr == "")
    {
        std::cout << "No domain specified.\n";
        return false;
    }

    if (domainStr != "" && simType == "")
    {
        std::cout << "No simulation specified.\n";
        return false;
    }

    if (ResourceLocations::instance().modelsPath() == "")
        ResourceLocations::instance().modelsPath(cmdArgsParser.getOption("ModelsPath"));
    if (ResourceLocations::instance().colorMapsPath() == "")
        ResourceLocations::instance().colorMapsPath(cmdArgsParser.getOption("ColorMapsPath"));
    if (ResourceLocations::instance().shadersPath() == "")
        ResourceLocations::instance().shadersPath(cmdArgsParser.getOption("ShadersPath"));

    for (unsigned i = 0; i < morphogens.size(); ++i)
        morphIndexMap[morphogens[i]] = i;

    if (simType == "CPU" || simType == "GPU" || simType == "stochastic CPU" || simType == "stochastic GPU")
    {
        Parameters params;
        if (initParams.size() > 0)
            params = initParams[0].params;
        else
            params = globalParamMap;

        if (simType == "CPU" || simType == "stochastic CPU") 
        { 
            std::string modelSubroutine = createModelFunc(morphogens, customModel, params);

			if (simType == "stochastic CPU")
			{
				// add LNoise variable
				std::string replace1("std::vector<float>& L,");
				size_t pos1 = modelSubroutine.find(replace1);
				modelSubroutine.replace(pos1, replace1.length(), "std::vector<float>& L, std::vector<float>& LNoise,");
				// add nran to function definition
				std::string replace2("size_t stepCount)");
				size_t pos2 = modelSubroutine.find(replace2);
				modelSubroutine.replace(pos2, replace2.length(), "size_t stepCount,float(&nran)(void))");
			}

            std::ofstream modelCPPFile("PDES.cpp");
            if (modelCPPFile.is_open())
            {
                modelCPPFile << modelSubroutine;
                modelCPPFile.flush();
                modelCPPFile.close();
            }
        }
        else if (simType == "GPU" || simType == "stochastic GPU")
        {
            // Defines 
            std::string defines = std::string("#define MORPHOGEN_COUNT ") + std::to_string(morphogens.size()) + std::string("\n");
            for (unsigned i = 0; i < morphogens.size(); ++i)
                defines += "#define " + morphogens[i] + " " + std::to_string(i) + "\n";
            includes["#include \"defines\""] = defines;

            // Parameters
            std::string paramStruct = "struct Parameters\n{\n";
            for (auto& p : params)
                if (p.first != "growing" && p.first != "growthTickLimit")
                    paramStruct += "\tfloat " + p.first + ";\n";
            paramStruct += "};\n";
            includes["#include \"parameters\""] = paramStruct;

            // Model
			bool veins = veinMorph != "" && (veinDiffusionScale != 1.f || petalDiffusionScale != 1.f);
            std::string modelSubroutine = createModelSubroutine(morphogens, customModel, domainStr, simType == "stochastic GPU", veins, params);
            includes["#include \"model\""] = modelSubroutine;
        }
    }

    if (!parseDomain(domainStr, d, globalParamMap, colorMaps))
    {
        std::cout << "Failed to load [" + domainStr + "]\n";
        return false;
    }

    if (veinMorph != "") 
        d->setVeinMorphIndex(morphIndexMap.at(veinMorph), veinDiffusionScale, petalDiffusionScale);

    size_t numThreads = 0;
    if (cmdArgsParser.hasOption("Threads"))
        numThreads = std::atoi(cmdArgsParser.getOption("Threads").c_str());
    
    if (numThreads > 1 && (simType == "stochastic GPU" || simType == "GPU"))
        LOG("GPU computation does not use more than 1 thread.");

    parseSimulation(simType, s, d, globalParamMap, includes, initConditions, boundaryConditions, initParams, morphIndexMap, numThreads);

    
    d->growing = growing;
    s->setGrowthTickLimit(growthTickLimit);
    
    if (!maxFaceAreaFound) 
    {
        if (auto* hem = dynamic_cast<HalfEdgeMesh*>(d))
            maxFaceArea = (float) d->getTotalArea() / (float) hem->faces.size();
        else
            maxFaceArea = (float) d->getTotalArea() / (float) d->getCellCount();
    }

    s->maxFaceArea = maxFaceArea;
    s->growth.x = growthX;
    s->growth.y = growthY;
    s->growth.z = growthZ;
    s->filename = filename;
    s->veinMorph = veinMorph;
    if (pauseAt != 0)
    {
        s->pauseStepCount = static_cast<int>(pauseAt);
        s->pauseAt = true;
    }
    if (exitAt != 0)
    {
        s->exitStepCount = static_cast<int>(exitAt);
        s->exitAt = true;
    }

    if (simFile != "" && !s->loadSim(simFile))
    {
        std::cout << "Failed to load [" + simFile + "]\n";
        return false;
    }

    if (growthMode != "")
    {
        if (growthMode == "animation")
            d->growthMode = SimulationDomain::GrowthMode::AnimationGrowth;
        else if (growthMode == "linear")
            d->growthMode = SimulationDomain::GrowthMode::LinearAreaGrowth;
        else if (growthMode == "morphogen")
            d->growthMode = SimulationDomain::GrowthMode::MorphogenGrowth;
        else if (growthMode == "percentage")
            d->growthMode = SimulationDomain::GrowthMode::PercentageGrowth;
    }

    camera.fitTo(calculateBounds(*d));
    if (cameraMatFound)
    {
        camera.position_ = camPosition;
        camera.lookAt_ = camLookAt;
    }
    if (modelMatFound)
    {
        d->transform_.setOrientation(modelX, modelY, modelZ);
    }

    if(prepatternInfo0enabled)
    { 
        d->prepatternInfo0.enabled = prepatternInfo0enabled;
        d->prepatternInfo0.targetIndex = s->morphIndexMap[prepatternInfo0targetIndex];
        d->prepatternInfo0.sourceIndex = s->morphIndexMap[prepatternInfo0sourceIndex];
        d->prepatternInfo0.lowVal = prepatternInfo0lowVal;
        d->prepatternInfo0.highVal = prepatternInfo0highVal;
        d->prepatternInfo0.exponent = prepatternInfo0exponent;
        d->prepatternInfo0.squared = prepatternInfo0UseSquare;
    }

    if (prepatternInfo1enabled)
    { 
        d->prepatternInfo1.enabled = prepatternInfo1enabled;
        d->prepatternInfo1.targetIndex = s->morphIndexMap[prepatternInfo1targetIndex];
        d->prepatternInfo1.sourceIndex = s->morphIndexMap[prepatternInfo1sourceIndex];
        d->prepatternInfo1.lowVal = prepatternInfo1lowVal;
        d->prepatternInfo1.highVal = prepatternInfo1highVal;
        d->prepatternInfo1.exponent = prepatternInfo1exponent;
        d->prepatternInfo1.squared = prepatternInfo1UseSquare;
    }
    if (prepatternInfo0enabled || prepatternInfo1enabled)
        s->updateDiffusionCoefs();

    // Load background texture
    {
        Image bgImage = Utils::loadImage(backgroundTexture);
        if (bgImage.data_.size() > 0) 
        {
            d->background_ = Textures::create2DTexture(bgImage, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_RGBA);
            d->backgroundImage_ = bgImage;
            LOG("Loaded backgroundTexture: " << backgroundTexture);
        }
    }

    // Print location information
    LOG("Models Location:\t\t" << ResourceLocations::instance().modelsPath());
    LOG("ColorMapsPath Location:\t" << ResourceLocations::instance().colorMapsPath());
    LOG("ShadersPath Location:\t\t" << ResourceLocations::instance().shadersPath());

    s->rawInitConds = rawInitConds;
    s->rawBCs = rawBCs;
    s->rawRDModel = rawRDModel;

    return true;
}

bool SimulationLoader::parseDomain(const std::string& domName, SimulationDomain*& d, Parameters globalParamMap, const std::vector<std::string>& colorMaps)
{
    std::string name = Utils::sToLower(domName);
    if (name == "mesh")
        d = new HalfEdgeMesh(Mesh((int)globalParamMap["xRes"], (int)globalParamMap["yRes"], globalParamMap["width"], globalParamMap["height"]));
    else if (name.find(".obj") != std::string::npos)
    {
        ObjModel model(ResourceLocations::instance().modelsPath() + name);

        if (!model.isLoaded())
            return false;

        d = new HalfEdgeMesh(model);
    }
    else if (name.find(".ply") != std::string::npos)
    {
        Ply model(ResourceLocations::instance().modelsPath() + name);

        if (!model.isLoaded())
            return false;

        d = new HalfEdgeMesh(model);
    }
    else if (name == "grid")
        d = new Grid((int)globalParamMap["xRes"], (int)globalParamMap["yRes"], globalParamMap["cellSize"]);
    // default domain
    else if (name == "")
    {
        d = new HalfEdgeMesh(Mesh(50, 50, 5, 5));
    }
    else
    {
        std::cout << "Invalid Domain: " << domName << std::endl;
        return false;
    }

    auto cOutside = colorMaps[0] != "" ? colorMaps[0] : colorMaps[1];
    auto cInside = colorMaps[1] != "" ? colorMaps[1] : colorMaps[0];

    d->colorMapOutside.init(cOutside, ResourceLocations::instance().colorMapsPath());
    d->colorMapInside.init(cInside, ResourceLocations::instance().colorMapsPath());
    return true;
}

void SimulationLoader::parseSimulation(
    const std::string& simName,
    Simulation*& s,
    SimulationDomain*& d,
    Parameters& globalParamMap,
    const std::map<std::string, std::string>& includes,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap,
    size_t numThreads)
{
    auto safeCopy = [](Parameters& params1, Parameters& params2, std::string key)
    {
        if (params2.count(key) > 0)
            params1[key] = params2[key];
    };

    Parameters params;
    if (simName == "DecayDiffusion")
    {
        morphIndexMap.clear();
        morphIndexMap["A"] = 0;

        safeCopy(globalParamMap, params, "Da");
        safeCopy(globalParamMap, params, "Ka");
        safeCopy(globalParamMap, params, "dt");
        safeCopy(globalParamMap, params, "fixedIndex");

        s = new DecayDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap);
    }
    else if (simName == "GrayScott")
    {
        morphIndexMap.clear();
        morphIndexMap["A"] = 0;
        morphIndexMap["S"] = 1;

        safeCopy(globalParamMap, params, "Da");
        safeCopy(globalParamMap, params, "Ds");
        safeCopy(globalParamMap, params, "f");
        safeCopy(globalParamMap, params, "k");
        safeCopy(globalParamMap, params, "dt");

        s = new GrayScottReactionDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap);
    }
    else if (simName == "Kondo")
    {
        morphIndexMap.clear();
        morphIndexMap["A"] = 0;
        morphIndexMap["S"] = 1;

        safeCopy(globalParamMap, params, "c1");
        safeCopy(globalParamMap, params, "c2");
        safeCopy(globalParamMap, params, "c3");
        safeCopy(globalParamMap, params, "c4");
        safeCopy(globalParamMap, params, "c5");
        safeCopy(globalParamMap, params, "ga");
        safeCopy(globalParamMap, params, "gs");
        safeCopy(globalParamMap, params, "Da");
        safeCopy(globalParamMap, params, "Ds");
        safeCopy(globalParamMap, params, "Ds");
        safeCopy(globalParamMap, params, "dt");

        s = new KondoReactionDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap);
    }
    else if (simName == "Turing")
    {
        morphIndexMap.clear();
        morphIndexMap["U"] = 0;
        morphIndexMap["V"] = 1;

        safeCopy(globalParamMap, params, "a");
        safeCopy(globalParamMap, params, "b");
        safeCopy(globalParamMap, params, "p_v");
        safeCopy(globalParamMap, params, "u_u");
        safeCopy(globalParamMap, params, "Du");
        safeCopy(globalParamMap, params, "Dv");
        safeCopy(globalParamMap, params, "s");
        safeCopy(globalParamMap, params, "dt");
        safeCopy(globalParamMap, params, "uSat");
        safeCopy(globalParamMap, params, "vSat");

        s = new TuringReactionDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap);
    }
    else if (simName == "GiererMeinhardt")
    {
        morphIndexMap.clear();
        morphIndexMap["U"] = 0;
        morphIndexMap["V"] = 1;

        safeCopy(globalParamMap, params, "Du");
        safeCopy(globalParamMap, params, "Dv");
        safeCopy(globalParamMap, params, "p_u");
        safeCopy(globalParamMap, params, "p_v");
        safeCopy(globalParamMap, params, "u_u");
        safeCopy(globalParamMap, params, "s_v");
        safeCopy(globalParamMap, params, "s_u");
        safeCopy(globalParamMap, params, "k");
        safeCopy(globalParamMap, params, "dt");

        s = new GiererMeinhardtReactionDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap);
    }
    else if (simName == "OsterMurray")
    {
        morphIndexMap.clear();
        morphIndexMap["A"] = 0;
        morphIndexMap["S"] = 1;

        safeCopy(globalParamMap, params, "D");
        safeCopy(globalParamMap, params, "N");
        safeCopy(globalParamMap, params, "s");
        safeCopy(globalParamMap, params, "r");
        safeCopy(globalParamMap, params, "a");
        safeCopy(globalParamMap, params, "dt");

        s = new OsterMurrayReactionDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap);
    }
    else if (simName == "ActivatorInhibitor")
    {
        morphIndexMap.clear();
        morphIndexMap["A"] = 0;
        morphIndexMap["H"] = 1;

        safeCopy(globalParamMap, params, "p");
        safeCopy(globalParamMap, params, "p_a");
        safeCopy(globalParamMap, params, "p_h");
        safeCopy(globalParamMap, params, "u_a");
        safeCopy(globalParamMap, params, "u_h");
        safeCopy(globalParamMap, params, "Da");
        safeCopy(globalParamMap, params, "Dh");
        safeCopy(globalParamMap, params, "dt");

        s = new ActivatorInhibitorReactionDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap);
    }
    else if (simName == "GPU" || simName == "stochastic GPU")
    {
        s = new GPUSimulation(d, globalParamMap, includes, initConditions, boundaryConditions, initParams, morphIndexMap, simName == "stochastic GPU");
    }
    else if (simName == "CPU")
    {
        params = globalParamMap;
        s = new CustomReactionDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap, numThreads);
    }
	else if (simName == "stochastic CPU")
	{
	    params = globalParamMap;
	    s = new StochasticCustomReactionDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap, numThreads);
	}
    else if (simName == "") // Default simulation 
    {
        morphIndexMap.clear();
        morphIndexMap["A"] = 0;
        morphIndexMap["S"] = 1;

        params["dt"] = 0.5f;
        params["Da"] = 0.0005f;
        params["Ds"] = 0.001f;
        params["f"] = 0.03f;
        params["k"] = 0.063f;

        s = new GrayScottReactionDiffusion(d, params, initConditions, boundaryConditions, initParams, morphIndexMap);
    }
    else
        std::cout << "Invalid Simulation: " << simName << std::endl;
}

bool SimulationLoader::parseBoundaryConditions(std::vector<Simulation::BoundaryConditions>& boundaryConditions, std::string& rawBCsStr, const std::vector<std::string>& configFileLines)
{
    size_t i = 0;
    for (; i < configFileLines.size(); ++i)
    {
        if (configFileLines[i].find("boundaryConditions") == 0)
        {
            rawBCsStr += configFileLines[i] + "\n";
            break;
        }
    }

    bool startSeen = false;
    bool endSeen = false;
    size_t openBracketCount = 0;

    for (i = i + 1; i < configFileLines.size(); ++i)
    {
        if (endSeen)
            break;

        std::string line = configFileLines[i];
        rawBCsStr += line + "\n";

        if (line[0] == '#' || line == "")
            continue;

        size_t pos = line.find("#");
        if (pos != std::string::npos)
            line = line.substr(0, pos);

        pos = line.find("{");
        if (pos != std::string::npos)
        {
            openBracketCount++;
            if (!startSeen)
                line.replace(pos, 1, "");
            startSeen = true;
        }

        pos = line.find("}");
        if (pos != std::string::npos)
        {
            openBracketCount--;
            if (openBracketCount == 0)
            {
                endSeen = true;
                line.replace(pos, 1, "");
            }
        }

        std::string label;
        std::string value;
        size_t loc = line.find(":");
        if (loc != line.size() - 1)
        {
            std::string substring = line.substr(0, loc);
            label = Utils::trim(substring);

            substring = line.substr(loc + 1);
            value = Utils::sToLower(Utils::trim(substring));
        }

        // Parse indices
        if (Utils::sToLower(label) == "indices")
        {
            boundaryConditions.emplace_back(value);
        }
        else if (label != "")
        {
            ASSERT(boundaryConditions.size() > 0, "Indices need to go first in boundary conditions");
            ASSERT(value.compare("neumann") == 0 || value.compare("dirichlet") == 0, "Invalid boundary condition. Expected neumann or dirichlet");
            boundaryConditions.back().morpMap[label] = value;
        }
    }
    if (!endSeen || !startSeen)
    {
        std::cout << "Invalid parameters. Are you missing a { or }?\n";
        return false;
    }

    return true;
}

bool SimulationLoader::parseRDModel(std::string& simType, std::string& customModelStr, std::string& rawModelStr, const std::vector<std::string>& configFileLines, const std::vector<std::string>& morphogens)
{
    if (simType != "GPU" && simType != "CPU" && simType != "stochastic CPU" && simType != "stochastic GPU")
        return true;

    bool startSeen = false;
    bool endSeen = false;
    size_t openBracketCount = 0;

    // Find rdModel label
    size_t i = 0;    
    for(; i < configFileLines.size(); ++i)
    {
        if (configFileLines[i].find("rdModel") == 0)
        {
            rawModelStr += configFileLines[i] + "\n";
            break;
        }
    }
    
    // Parse and convert model
    for (i = i + 1; i < configFileLines.size(); ++i)
    {
        if (endSeen)
            break;

        std::string line = configFileLines[i];
        rawModelStr += line + "\n";

        if (line[0] == '#')
            continue;

        size_t pos = line.find("#");
        if (pos != std::string::npos)
            line = line.substr(0, pos);

        pos = line.find("{");
        if (pos != std::string::npos)
        {
            openBracketCount++;
            if (!startSeen)
                line.replace(pos, 1, "");
            startSeen = true;
        }

        pos = line.find("}");
        if (pos != std::string::npos)
        {
            openBracketCount--;
            if (openBracketCount == 0)
            {
                endSeen = true;
                line.replace(pos, 1, "");
            }
        }

        if (simType == "GPU" || simType == "stochastic GPU")
        {
            if (line.find("Morphogens") != std::string::npos || line.find("Prepattern") != std::string::npos)
                line = "";

            for (auto& morphName : morphogens)
            {
                auto dMorph = Utils::sToLower(morphName) + "'";
                if (line.find(dMorph) != std::string::npos)
                {
                    Utils::findAndReplace(line, dMorph, "cells2[gid].vals[" + morphName + "]");
                    auto splitRes = Utils::split(line, "=");
                    if (splitRes.size() >= 2)
                    {
                        splitRes[1] = Utils::sToLower(morphName) + " + (" + splitRes[1];
                    }
                    line = Utils::join(splitRes, "=");

                    splitRes = Utils::split(line, ";");
                    if (splitRes.size() >= 2)
                    {
                        splitRes[0] = splitRes[0] + ") * dt";
                    }
                    line = Utils::join(splitRes, ";");
;                }
            }

            Utils::findAndReplace(line, "new[", "cells2[gid].vals[");
            Utils::findAndReplace(line, "params.", "params[gid].");

            if (line.find("diffusion(") != std::string::npos)
            {
                auto splitRes = Utils::split(line, "diffusion(");
                for (size_t j = 1; j < splitRes.size(); ++j)
                {
                    auto splitRes2 = Utils::split(splitRes[j], ")");
                    if (splitRes2.size() >= 2)
                    {
                        splitRes2[1] = Utils::sToUpper(splitRes2[0]) + "]" + splitRes2[1];
                        splitRes2.erase(std::begin(splitRes2));
                    }
                    splitRes[j] = Utils::join(splitRes2, ")");
                }
                line = Utils::join(splitRes, "L[");
            }
        }
        else if (simType == "CPU")
        {
            if (line.find("Morphogens") != std::string::npos || line.find("Prepattern") != std::string::npos)
                line = "";

            for (auto& morphName : morphogens)
            {
                auto dMorph = Utils::sToLower(morphName) + "'";
                if (line.find(dMorph) != std::string::npos)
                {
                    Utils::findAndReplace(line, Utils::sToLower(morphName) + "'", "\t\t\twriteTo[gid][" + morphName + "]");

                    auto splitRes = Utils::split(line, "=");
                    if (splitRes.size() >= 2)
                    {
                        splitRes[1] = Utils::sToLower(morphName) + " + (" + splitRes[1];
                    }
                    line = Utils::join(splitRes, "=");

                    splitRes = Utils::split(line, ";");
                    if (splitRes.size() >= 2)
                    {
                        splitRes[0] = splitRes[0] + ") * dt";
                    }
                    line = Utils::join(splitRes, ";");
                }
            }

            Utils::findAndReplace(line, "new[", "\t\t\twriteTo[gid][");
            Utils::findAndReplace(line, "params.", "");
            Utils::findAndReplace(line, "L[", "L[gid * MORPH_COUNT + "); 

            if (line.find("diffusion(") != std::string::npos)
            {
                auto splitRes = Utils::split(line, "diffusion(");
                for (size_t j = 1; j < splitRes.size(); ++j)
                {
                    auto splitRes2 = Utils::split(splitRes[j], ")");
                    if(splitRes2.size() >= 2)
                    { 
                        splitRes2[1] = Utils::sToUpper(splitRes2[0]) + "]" + splitRes2[1];
                        splitRes2.erase(std::begin(splitRes2));
                    }
                    splitRes[j] = Utils::join(splitRes2, ")");
                }
                line = Utils::join(splitRes, "L[gid * MORPH_COUNT + ");
            }
        }
		else if (simType == "stochastic CPU")
		{
			Utils::findAndReplace(line, "new[", "\t\t\twriteTo[gid][");
			Utils::findAndReplace(line, "params.", "");
			Utils::findAndReplace(line, "L[", "L[gid * MORPH_COUNT + ");
			Utils::findAndReplace(line, "LNoise[", "LNoise[gid * MORPH_COUNT + ");
		}

        customModelStr += line + "\n";
    }

    // Sanity check brackets
    if (!endSeen || !startSeen)
    {
        std::cout << "Invalid custom model. Are you missing a { or }?\n";
        return false;
    }
    else
        return true;
}

bool SimulationLoader::parseParams(std::vector<Simulation::InitParams>& initParams, std::string& rawParamsStr, const std::vector<std::string>& configFileLines)
{
    // Find params label
    size_t i = 0;
    for (; i < configFileLines.size(); ++i)
    {
        if (configFileLines[i].find("params") == 0)    
        {
            rawParamsStr += configFileLines[i] + "\n";
            break;
        }
    }

    bool startSeen = false;
    bool endSeen = false;
    size_t openBracketCount = 0;
    size_t curCondIndex = 0;

    for (i = i + 1; i < configFileLines.size(); ++i)
    {
        if (endSeen)
            break;

        std::string line = configFileLines[i];
        rawParamsStr += line + "\n";

        if (line[0] == '#')
            continue;

        size_t pos = line.find("#");
        if (pos != std::string::npos)
            line = line.substr(0, pos);

        pos = line.find("{");
        if (pos != std::string::npos)
        {
            openBracketCount++;
            if (!startSeen)
                line.replace(pos, 1, "");
            startSeen = true;
        }

        pos = line.find("}");
        if (pos != std::string::npos)
        {
            openBracketCount--;
            if (openBracketCount == 0)
            {
                endSeen = true;
                line.replace(pos, 1, "");
            }
        }

        std::string label;
        std::string value;
        unsigned loc = static_cast<unsigned>(line.find(":"));
        if (loc != line.size() - 1)
        {
            std::string substring = line.substr(0, loc);
            label = Utils::trim(substring);

            substring = line.substr(loc + 1);
            value = Utils::trim(substring);
        }

        // Parse indices
        if (label == "indices")
        {
            curCondIndex = initParams.size();
            initParams.emplace_back(value);
        }
        else if (label != "")
        {
            if (initParams.size() == curCondIndex)
                initParams.emplace_back("all");

            if (label == "radius")
                initParams[curCondIndex].radius = std::atoi(Utils::trim(value).c_str());
            else
                initParams[curCondIndex].params[label] = stof(value);
        }
    }

    if (!endSeen || !startSeen)
    {
        std::cout << "Invalid parameters. Are you missing a { or }?\n";
        return false;
    }
    return true;
}

bool SimulationLoader::parseInitialConditions(std::vector<Simulation::InitConditions>& initConditions, std::string& rawInitCondsStr, const std::vector<std::string>& configFileLines)
{
    size_t i = 0;
    for (; i < configFileLines.size(); ++i)
    {
        if (configFileLines[i].find("initialConditions") == 0)
        {
            rawInitCondsStr += configFileLines[i] + "\n";
            break;
        }
    }

    bool startSeen = false;
    bool endSeen = false;
    size_t openBracketCount = 0;
    size_t curCondIndex = 0;

    for (i = i + 1; i < configFileLines.size(); ++i)
    {
        if (endSeen)
            break;

        std::string line = configFileLines[i];
        rawInitCondsStr += line + "\n";

        if (line[0] == '#')
            continue;

        size_t pos = line.find("#");
        if (pos != std::string::npos)
            line = line.substr(0, pos);

        pos = line.find("{");
        if (pos != std::string::npos)
        {
            openBracketCount++;
            if (!startSeen)
                line.replace(pos, 1, "");
            startSeen = true;
        }

        pos = line.find("}");
        if (pos != std::string::npos)
        {
            openBracketCount--;
            if (openBracketCount == 0)
            {
                endSeen = true;
                line.replace(pos, 1, "");
            }
        }

        std::string label;
        std::string value;
        unsigned loc = static_cast<unsigned>(line.find(":"));
        if (loc != line.size() - 1)
        {
            std::string substring = line.substr(0, loc);
            label = Utils::trim(substring);

            substring = line.substr(loc + 1);
            value = Utils::trim(substring);
        }

        if (label == "indices")
        {
            curCondIndex = initConditions.size();
            initConditions.emplace_back(value);
        }
        else if (label != "")
        {
            if (initConditions.size() == curCondIndex)
                initConditions.emplace_back("all");

            if (label == "radius")
                initConditions[curCondIndex].radius = std::atoi(Utils::trim(value).c_str());
            else
                initConditions[curCondIndex].morpMap[label] = value;
        }
    }

    if (!endSeen || !startSeen)
    {
        std::cout << "Invalid initialConditions. Are you missing a { or }?\n";
        return false;
    }
    return true;
}
