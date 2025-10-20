#include "Vec3.h"
#include "Simulation.h"
#include "Utils.h"
#include "DefaultShader.h"
#include "SimulationLoader.h"
#include "Ply.h"
#include "Nran.h"

#include <fstream>
#include <iostream>
#include <cfloat>
#include <sstream>
#include <algorithm>


Simulation::Simulation(
    SimulationDomain* domain,
    Parameters params,
    const std::string& name,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap,
    size_t threadCount,
    Vec3 growth, 
    float maxFaceArea)
    : domain(domain), growth(growth), maxFaceArea(maxFaceArea), name(name), 
    MORPH_COUNT(static_cast<int>(morphIndexMap.size())), 
    threadPool_(threadCount ? threadCount : std::thread::hardware_concurrency())
{
    LOG("Using " + std::to_string(threadPool_.getNumThreads()) + " threads");
    domain->init(MORPH_COUNT);
    lap.resize(domain->getCellCount() * MORPH_COUNT);

    this->morphIndexMap = morphIndexMap;
    growthCounter.setDuration(100);
    stepCount = 0;
    outMorphIndex = 0;

    // Parse initial conditions
    initConditions_ = initConditions;
    populateCells(initConditions_, morphIndexMap);

    boundaryConditions_ = boundaryConditions;
    setBoundaryConditions(boundaryConditions, morphIndexMap);

    // Parse parameters
    if (initParams.size() > 0)
    {
        for (auto& param : initParams)
        {
            paramsMap.emplace_back(param.params);
            auto indices = parseIndices(param.indices, param.radius);
            for (auto i : indices)
                paramsMap.back().indices_.insert(i);
        }
        originalParams = paramsMap[0].params_;
    }
    else
    {
        if (params.count("growthTickLimit") > 0)
            params.erase("growthTickLimit");

        originalParams = params;
        paramsMap.emplace_back(params);
        for (unsigned i = 0; i < domain->getCellCount(); ++i)
            paramsMap[0].indices_.insert(i);
    }

    if (domain->hasVeins())
        domain->updateVeinValues();

    initSim();
    computeThreadWork();
}

void Simulation::simulate()
{
    if (!paused && isGPUEnabled && !gpuUpToDate())
        updateGPU();

    doSimulate();

    if (domain->growing && growthCounter.countElapsedAndReset())
        growAndSubdivide();

    domain->swap();
    stepCount++;

    if (isGPUEnabled)
        ramUpToDate(false);
}

void Simulation::growAndSubdivide()
{
    domain->growAndSubdivide(growth, maxFaceArea, subdivisionEnabled_, stepCount);

    if (domain->cellsToUpdate.size() > 0)
    {
        // TODO: add to StochasticCustomReactionDiffusion lap_noise.resize(domain->getCellCount() * MORPH_COUNT);
        lap.resize(domain->getCellCount() * MORPH_COUNT);

        // For each new cell, get neighbouring params and calc param for new cell
        for (auto& cellToUpdate : domain->cellsToUpdate)
        {
            unsigned i = cellToUpdate.second.neighbours[0];
            for (auto& p : paramsMap)
                if (p.contains(cellToUpdate.first))
                    p.indices_.erase(cellToUpdate.first);

            for (auto& p : paramsMap)
            {
                if (p.contains(i))
                {
                    p.indices_.insert(cellToUpdate.first);
                    break;
                }
            }
        }
        domain->cellsToUpdate.clear();

        if (!isGPUEnabled)
            computeThreadWork();
    }
}

void Simulation::updateDiffusionCoefs()
{
    domain->updateDiffusionCoefs();
}

void Simulation::initSim()
{
    growthCounter.setDuration(growthTickLimit);
}

void Simulation::updateColors()
{
    domain->update();
    domain->updateColorVBO();

    domain->updateGradLines();
    domain->updateDiffDirLines();
}

void Simulation::populateCells(std::vector<Simulation::InitConditions> initConditions, std::map<std::string, int> initMorphIndexMap)
{
    // Noise and randomness
    std::vector<Cell>& cells1 = domain->getReadFromCells();
    std::vector<Cell>& cells2 = domain->getWriteToCells();
    std::mt19937 gen(4);

    // Default initial conditions gray-scott
    if (initConditions.size() == 0)
    {
        Simulation::InitConditions ic0("all");
        ic0.morpMap["A"] = "0";
        ic0.morpMap["S"] = "1";

        Simulation::InitConditions ic1("rand(10)");
        ic1.radius = 1;
        ic1.morpMap["A"] = "1";
        ic1.morpMap["S"] = "1";

        initConditions.push_back(ic0);
        initConditions.push_back(ic1);
    }

    // For each initial condition
    for (auto& cond : initConditions)
    {
        // Find indices to set
        std::vector<int> indices = parseIndices(cond.indices, cond.radius);
        for (auto& morphPair : cond.morpMap)
        {
            // Set initial morphogen value
            for (auto i : indices)
            {
                float val = parseVal(morphPair.second, gen);
                cells1[i].vals[initMorphIndexMap[morphPair.first]] = val;
                cells2[i].vals[initMorphIndexMap[morphPair.first]] = val;
            }
        }
    }
}

void Simulation::setBoundaryConditions(std::vector<Simulation::BoundaryConditions> boundaryConditions, std::map<std::string, int> initMorphIndexMap)
{
    // Noise and randomness
    std::vector<Cell>& cells1 = domain->getReadFromCells();
    std::vector<Cell>& cells2 = domain->getWriteToCells();

    for (auto& cond : boundaryConditions)
    {
        std::vector<int> indices = parseIndices(cond.indices);
        for (auto i : indices)
        {
            for (auto& morphPair : cond.morpMap)
            {
                if (morphPair.second == "dirichlet")
                {
                    cells1[i].isConstVal[initMorphIndexMap[morphPair.first]] = true;
                    cells2[i].isConstVal[initMorphIndexMap[morphPair.first]] = true;
                }
            }
        }
    }
}

void Simulation::pause(bool state)
{
    paused = state;
    if (!paused && isGPUEnabled && !gpuUpToDate())
        updateGPU();
}

bool Simulation::isPaused()
{
    return paused;
}

void Simulation::updateRAM()
{
    std::cout << " updating ram" << std::endl;
    ramUpToDate(true);
}

bool Simulation::gpuUpToDate()
{
    return gpuUpToDateFlag;
}

bool Simulation::ramUpToDate()
{
    return ramUpToDateFlag;
}

void Simulation::gpuUpToDate(bool state)
{
    gpuUpToDateFlag = state;
}

void Simulation::ramUpToDate(bool state)
{
    ramUpToDateFlag = state;
}

ParamIndexPair& Simulation::getParamIndexPair(const Parameters& params, bool& newPairCreated)
{
    for (ParamIndexPair& pip : paramsMap)
    {
        bool match = true;
        for (auto p : pip.params_)
        {
            if (p.second != params.at(p.first))
            {
                match = false;
                break;
            }
        }
        if (match)
        {
            newPairCreated = false;
            return pip;
        }
    }
    newPairCreated = true;
    paramsMap.emplace_back(params);
    return paramsMap.back();
}

ParamIndexPair& Simulation::getParamIndexPair(unsigned index)
{
    for (ParamIndexPair& p : paramsMap)
    {
        if (p.contains(index))
            return p;
    }
    return paramsMap.back();
}

void Simulation::setGrowthTickLimit(unsigned long long newGrowthTickLimit)
{
    this->growthTickLimit = newGrowthTickLimit;
    this->growthCounter.setDuration(newGrowthTickLimit);
}

unsigned long long Simulation::getGrowthTickLimit() const
{
    return growthTickLimit;
}

bool Simulation::updateParams(Parameters params)
{
    bool anyNewParamCreated = false;
    // If no cells are selected, treat that as all cells selected 
    // and temporarily update selected cell vector accordingly
    bool noSelectedCells = domain->selectedCells.size() == 0;
    for (unsigned i = 0; noSelectedCells && i < domain->getCellCount(); ++i)
        domain->selectedCells.insert(i);

    // For each selected cells index, get and update
    // associated oldParams with the newParams. 
    // Finally, Remove empty parameter index pairs
    for (unsigned i : domain->selectedCells)
    {
        // get old cell parameters
        ParamIndexPair& oldPip = getParamIndexPair(i);
        oldPip.indices_.erase(i);

        // update old params
        Parameters newParameters = oldPip.params_;
        for (auto& p : params)
            newParameters[p.first] = p.second;

        if (isGPUEnabled)
        {
            domain->dirtyAttributes[domain->PARAM_ATTRB].indices.insert(i);
            for (auto& p : newParameters)
                domain->dirtyAttributes[domain->PARAM_ATTRB].attributes.emplace_back(p.second);
        }

        // put cell into the proper paramsMapEntry
        bool newParamCreated = false;
        ParamIndexPair& pip = getParamIndexPair(newParameters, newParamCreated);
        anyNewParamCreated = anyNewParamCreated || newParamCreated;
        pip.indices_.insert(i);

        // remove empty params
        paramsMap.erase(std::remove_if(begin(paramsMap), end(paramsMap), [](auto& p)
            {
                return !p.used();
            }), end(paramsMap));
    }

    // remove selected cells that were added temporarily
    if (noSelectedCells)
        domain->selectedCells.clear();

    // update thread work
    computeThreadWork();

    return anyNewParamCreated;
}

void Simulation::computeThreadWork()
{
    if (domain->getCellCount() == 0)
        return;

    threadWork_.clear();
    auto tempParamsMap = paramsMap;
    size_t indicesPerThread = (size_t)std::ceil((float)domain->getCellCount() / (float)threadPool_.getNumThreads());
    size_t numIndicesNeeded = indicesPerThread;
    for (int i = 0; i < threadPool_.getNumThreads(); ++i)
    {
        ThreadWorks threadWorks;
        numIndicesNeeded = indicesPerThread;

        while (numIndicesNeeded > 0 && tempParamsMap.size() > 0)
        {
            ThreadWork threadWork;
            ParamIndexPair& params = tempParamsMap[0];
            std::vector<unsigned> indices(params.indices_.begin(), params.indices_.end());

            size_t numIndicesToCopy = std::min(numIndicesNeeded, indices.size());
            threadWork = {params.params_, std::vector(indices.begin(), indices.begin() + numIndicesToCopy)};
            numIndicesNeeded -= numIndicesToCopy;

            for (auto index : std::get<1>(threadWork))
                params.indices_.erase(index);

            if (params.indices_.size() == 0)
                tempParamsMap.erase(tempParamsMap.begin());

            threadWorks.push_back(threadWork);
        }
        threadWork_.push_back(threadWorks);
    }

#ifdef DEBUG
    ASSERT(threadWork_.size() == threadPool_.getNumThreads(), "Invalid number of thread work array elements");
    std::set<unsigned> indices;
    for (auto& threadworks : threadWork_)
    {
        for(auto threadwork : threadworks)
        { 
            for(auto i : std::get<1>(threadwork))
                indices.insert(i);
        }
    }

    ASSERT(indices.size() == domain->getCellCount(), "Invalid number of indices in thread work array");

    std::vector<unsigned> indicesVec(indices.begin(), indices.end());
    std::sort(std::begin(indicesVec), std::end(indicesVec));
    for (auto i = 1; i < indicesVec.size(); ++i)
        ASSERT(indicesVec[i] - indicesVec[i-1] == 1, "Indices missing from thread work array");
#endif
}

void Simulation::updateGPU()
{
    std::cout << " updating gpu" << std::endl;

    //// Map the color buffer
    //glBindBuffer(GL_ARRAY_BUFFER, domain->colID);

    //Vec4* colors = (Vec4*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    //if (colors == nullptr)
    //	std::cout << "Cannot map buffer" << std::endl;

    //// update
    //for (size_t i = 0; i < domain->cells1.size(); ++i)
    //{
    //	float a = domain->cells1[i][0];
    //	float s = domain->cells1[i][1];

    //	colors[i].x = a;
    //	colors[i].y = s;
    //	colors[i].z = a;
    //	colors[i].a = s;
    //}

    //// Unmap
    //glUnmapBuffer(GL_ARRAY_BUFFER);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);

    gpuUpToDate(true);
}

//bool Simulation::saveSim(const std::string& path, const std::string& fileName)
//{
//    return saveEditorSettings(path) && saveConcentrations(path, fileName);
//}

bool Simulation::saveEditorSettings(const std::string& path)
{
    std::ofstream editorSettingsFile(path + "EditorSettings.txt");
    if (editorSettingsFile.is_open())
    {
        editorSettingsFile << "normCoef: " << domain->normCoef << "\n";

        int visibleMorph = -1;
        for (size_t i = 0; i < domain->showingMorph.size(); ++i)
            if (domain->showingMorph[i])
                visibleMorph = static_cast<int>(i);
        editorSettingsFile << "visibleMorph: " << visibleMorph << "\n";
        editorSettingsFile << "backgroundThreshold: " << domain->backgroundThreshold_ << "\n";
        editorSettingsFile << "backgroundOffset: " << domain->backgroundOffset_ << "\n";
        editorSettingsFile.close();
        return true;
    }
    else
        return false;
}

bool Simulation::recording() const
{
    return outputScreens || outputTextures || outputPlys;
}

bool Simulation::saveConcentrations(const std::string& path, const std::string& fileName)
{
    std::vector<Cell>& writeToCells = domain->getWriteToCells();
    std::string line;
    std::ofstream file(path + fileName);

    if (file.is_open())
    {
        file << writeToCells.size() << "\n";
        file << domain->tangents[0].size() << "\n";
        file << MORPH_COUNT << "\n";

        if (isGPUEnabled)
            updateRAM();

        for (size_t i = 0; i < writeToCells.size(); ++i)
        {
            for (int j = 0; j < MORPH_COUNT; ++j)
                file << writeToCells[i][j] << " ";
            for (int j = 0; j < MORPH_COUNT; ++j)
                file << 1.f << " ";
            file << "\n";
        }

        for (size_t i = 0; i < domain->tangents[0].size(); ++i)
        {
            for (int j = 0; j < MORPH_COUNT; ++j)
                file << domain->tangents[j][i].x << " " << domain->tangents[j][i].y << " " << domain->tangents[j][i].z << " ";
            for (int j = 0; j < MORPH_COUNT; ++j)
                file << domain->diffusionTensors[j].t0_[i] << " " << domain->diffusionTensors[j].t1_[i] << " ";

            file << "\n";
        }
        file.close();
        return true;
    }
    else
        return false;
}

bool Simulation::loadSim(const std::string& fileName)
{
    this->initalFilename = fileName;
    std::ifstream file(fileName);
    if (file.is_open())
    {
        std::vector<Cell>& writeToCells = domain->getWriteToCells();
        std::vector<Cell>& readFromCells = domain->getReadFromCells();

        size_t cellCount = 0;
        size_t tangentCount = 0;
        size_t numMorphs = 0;

        // Read "header"
        file >> cellCount;
        file >> tangentCount;
        file >> numMorphs;

        // Resize buffers
        writeToCells.reserve(cellCount);
        readFromCells.reserve(cellCount);
        for (size_t i = 0; i < cellCount; ++i)
        {
            writeToCells[i].vals.reserve(numMorphs);
            readFromCells[i].vals.reserve(numMorphs);
        }

        domain->tangents.resize(numMorphs);
        domain->diffusionTensors.resize(numMorphs);

        // Read per vertex values: morphs and diff scale coefs
        for (size_t i = 0; i < cellCount; ++i)
        {
            for (int j = 0; j < numMorphs; ++j)
                file >> writeToCells[i][j];
            readFromCells[i] = writeToCells[i];

            float diffScale = 1.f;
            for (int j = 0; j < numMorphs; ++j)
                file >> diffScale;
        }

        // Read per face values: vectors and lambdas
        for (size_t i = 0; i < tangentCount; ++i)
        {
            Vec3 tensor;
            float t0 = 0;
            float t1 = 0;

            for (int j = 0; j < numMorphs; ++j)
            {
                file >> tensor.x >> tensor.y >> tensor.z;
                domain->tangents[j][i] = tensor;
            }
            
            for (int j = 0; j < numMorphs; ++j)
            {
                file >> t0 >> t1;
                domain->diffusionTensors[j].t0_[i] = t0;
                domain->diffusionTensors[j].t1_[i] = t1;
            }
        }
        file.close();

        domain->destroyVBOs();
        domain->initVBOs();
        domain->recalculateParameters();

        if (isGPUEnabled)
        {
            for (unsigned i = 0; i < domain->getCellCount(); ++i)
            {
                domain->dirtyAttributes[domain->CELL1_ATTRIB].indices.insert(i);
                domain->dirtyAttributes[domain->CELL2_ATTRIB].indices.insert(i);
            }

            for(unsigned i = 0; i < domain->tangents.size(); ++i)
                domain->dirtyAttributes[domain->TANGENT_ATTRIB].indices.insert(i);

            updateGPU();
            domain->update();
            updateColorsFromRam = true;
        }
        return true;
    }
    else
        return false;
}

void Simulation::shouldReloadSim(bool value)
{
    shouldReloadSim_ = value;
}

void Simulation::shouldReloadPDEs(bool value)
{
    shouldReloadPDEs_ = value;
}

bool Simulation::reloadSim()
{
    if (this->initalFilename != "")
    {
        bool res = loadSim(initalFilename);
        doReloadSim();
        return res;
    }
    else
    {
        doReloadSim();
        domain->init(domain->numMorphs_);
        populateCells(initConditions_, morphIndexMap);

        if (!isGPUEnabled)
            updateColors();
    }

    if (isGPUEnabled)
    {
        ramUpToDate(true);
        for (unsigned i = 0; i < domain->getCellCount(); ++i)
        {
            domain->dirtyAttributes[domain->CELL1_ATTRIB].indices.insert(i);
            domain->dirtyAttributes[domain->CELL2_ATTRIB].indices.insert(i);
        }
        updateColorsFromRam = true;
        domain->update();
        updateColors();
        updateGPU();
    }
    stepCount = 0;
    return true;
}

bool Simulation::reloadPDEs()
{
    doReloadPDEs();
    return true;
}

std::vector<int> Simulation::parseIndices(std::string indicesStr, int radius)
{
    std::set<int> indicesSet;
    auto parseHyphen = [&](std::string& indicesStr, int radius, std::set<int>& indices)
    {
        size_t pos;
        if ((pos = static_cast<int>(indicesStr.find("-"))) != std::string::npos)
        {
            int start = std::atoi(indicesStr.substr(0, pos).c_str());
            int end = std::atoi(indicesStr.substr(pos + 1, indicesStr.size() - pos - 1).c_str());

            for (int i = start; i <= end; ++i)
            {
                if (radius > 0)
                {
                    auto neighbours = domain->getNeighbours(i, radius);
                    indices.insert(neighbours.begin(), neighbours.end());
                }
                indices.insert(i);
            }
        }
    };

    auto parseRand = [&](std::string& indicesStr, int radius, std::set<int>& indices)
    {
        size_t pos;
        if ((pos = indicesStr.find("rand(")) != std::string::npos)
        {
            std::mt19937 gen(5);

            size_t start = pos + 5;
            size_t end = indicesStr.find(")");
            if (end == std::string::npos)
                return;

            int count = 0;
            std::string substr = indicesStr.substr(start, end - start);
            std::string countStr = Utils::trim(substr);
            count = std::atoi(indicesStr.substr(start, end).c_str());
            std::uniform_int_distribution<> dist(0, domain->getCellCount() - 1);

            for (int i = 0; i < count; ++i)
            {
                int j = dist(gen);
                if (radius > 0)
                {
                    auto neighbours = domain->getNeighbours(j, radius);
                    indices.insert(neighbours.begin(), neighbours.end());
                }
                indices.insert(j);
            }
        }
    };

    auto parseAll = [&](std::set<int>& indices)
    {
        for (unsigned i = 0; i < domain->getCellCount(); ++i)
            indices.insert(i);
    };

    auto parseBoundary = [&](std::set<int>& indices)
    {
        auto boundary = domain->getBoundaryCellsIndices();
        for (auto i : boundary)
            indices.insert(i);
    };

    auto parsedIndices = Utils::split(indicesStr, ",");
    for (std::string& entry : parsedIndices)
    {
        size_t pos;
        entry = Utils::trim(entry);
        if ((pos = entry.find("-")) != std::string::npos)
            parseHyphen(entry, radius, indicesSet);
        else if ((pos = entry.find("rand(")) != std::string::npos)
            parseRand(entry, radius, indicesSet);
        else if ((pos = entry.find("all")) != std::string::npos)
            parseAll(indicesSet);
        else if ((pos = entry.find("boundary")) != std::string::npos)
            parseBoundary(indicesSet);
        else
            indicesSet.insert(std::atoi(entry.c_str()));
    }

    std::vector<int> indices(indicesSet.begin(), indicesSet.end());
    return indices;
}

float Simulation::parseVal(std::string valStr, std::mt19937& gen)
{
    std::vector<std::string> terms;
    size_t pos;
    float result = 0.f;
    if ((pos = valStr.find("+")) != std::string::npos)
    {
        std::string substring = valStr.substr(0, pos);
        terms.push_back(
            Utils::trim(substring));

        substring = valStr.substr(pos + 1, valStr.size() - (pos + 1));
        terms.push_back(Utils::trim(substring));
    }
    else
        terms.push_back(valStr);

    for (auto term : terms)
    {
        if ((pos = term.find("rand(")) != std::string::npos)
        {
            term = term.substr(pos + 5, term.size() - (pos + 4));
            term = term.substr(0, term.size() - 1);

            size_t commaPos = term.find(",");

            if (commaPos != std::string::npos)
            {
                float lower = static_cast<float>(std::atof(term.substr(0, commaPos).c_str()));
                float upper = static_cast<float>(std::atof(term.substr(commaPos + 1, term.size() - commaPos).c_str()));
                std::uniform_real_distribution<float> dist(lower, upper);
                result += dist(gen);
            }
        }
        else
        {
            result += static_cast<float>(std::atof(term.c_str()));
        }
    }

    return result;
}

std::string indicesToString(std::set<unsigned>& indices)
{
    std::string indStr = "indices: ";
    size_t numIndices = indices.size();
    size_t i = 0;
    if (numIndices > 0)
    {
        unsigned start = *indices.begin();
        if (numIndices == 1)
            indStr += std::to_string(start) + "\n";
        else
        {
            unsigned prev = 0;
            for (unsigned cur : indices)
            {
                if (i++ != 0)
                {
                    // end of range seen
                    if (cur != prev + 1)
                    {
                        // range length is 1
                        if (prev == start)
                        {
                            indStr += std::to_string(start) + ", ";
                            start = cur;
                        }
                        // range is longer than 1
                        else
                        {
                            indStr += std::to_string(start) + " - " + std::to_string(prev) + ", ";
                            start = cur;
                        }
                    }
                }
                prev = cur;
            }

            // end reached, print final range
            if (prev == start)
                indStr += std::to_string(start);
            else
                indStr += std::to_string(start) + " - " + std::to_string(prev);
        }
    }
    return indStr;
}

//================================================================================
CustomReactionDiffusion::CustomReactionDiffusion(
    SimulationDomain* domain,
    Parameters params,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap,
    const size_t threadCount) :
    Simulation(domain, params, "CPU", initConditions, boundaryConditions, initParams, morphIndexMap, threadCount)
{

#ifdef _WIN32	    
    if (!Utils::doesFileExist(Utils::BAT_FILENAME))
        Utils::createBatFile();    
    system(Utils::BAT_FILENAME);
    dynamicLibLoader.loadLib("PDES.dll");
#elif __APPLE__
    std::ofstream makeFile("rdpg.mak");
    makeFile << "all:\n";
    makeFile << "\tclang++ -std=c++11 -O3 -arch x86_64 -dynamiclib -flat_namespace PDES.cpp -o PDES.so\n";
    makeFile.close();
    system("make -f rdpg.mak");
    dynamicLibLoader.loadLib("PDES.so");
#elif __linux__
    system("g++ -fPIC -shared PDES.cpp -o PDES.so");    
    dynamicLibLoader.loadLib("PDES.so");
#endif

    customSimFunc = dynamicLibLoader.loadFunc<Simulate>("simulate");
}

void CustomReactionDiffusion::doReloadSim()
{
    doReloadPDEs();
}

void CustomReactionDiffusion::doReloadPDEs()
{
    // Read file lines
    std::ifstream simFile(filename);
    std::vector<std::string> configFileLines;
    {
        std::string line = "";
        if (simFile.is_open())
        {
            while (std::getline(simFile, line))
                configFileLines.push_back(line);
            simFile.close();
        }
        else
        {
            LOG("Failed to reload PDES");
            return;
        }
    }

    // Calc morphs
    std::vector<std::string> morphogens;
    for (size_t i = 0; i < morphIndexMap.size(); ++i)
    {
        for (const auto& pair : morphIndexMap)
        {
            if (pair.second == (int) i)
                morphogens.push_back(pair.first);
        }
    }

    // parse RD model
    std::string simType = "CPU";
    std::string customModel = "";
    rawRDModel = "";
    if (!SimulationLoader::parseRDModel(simType, customModel, rawRDModel, configFileLines, morphogens))
    {
        LOG("Failed to load rdModel");
        return;
    }

    std::vector<Simulation::InitParams> initParams;
    std::string rawParamsStr;
    SimulationLoader::parseParams(initParams, rawParamsStr, configFileLines);
    std::string modelSubroutine = SimulationLoader::createModelFunc(morphogens, customModel, initParams[0].params);

    std::ofstream modelCPPFile("PDES.cpp");
    if (modelCPPFile.is_open())
    {
        modelCPPFile << modelSubroutine;
        modelCPPFile.flush();
        modelCPPFile.close();
    }

    dynamicLibLoader.freeLib();
#ifdef _WIN32	    
    if (!Utils::doesFileExist(Utils::BAT_FILENAME))
        Utils::createBatFile();
    system(Utils::BAT_FILENAME);
    dynamicLibLoader.loadLib("PDES.dll");
#elif __APPLE__
    system("make -f rdpg.mak");
    dynamicLibLoader.loadLib("PDES.so");
#elif __linux__
    system("g++ -fPIC -shared PDES.cpp -o PDES.so");
    dynamicLibLoader.loadLib("PDES.so");
#endif
    customSimFunc = nullptr;
    customSimFunc = dynamicLibLoader.loadFunc<Simulate>("simulate");
}

void CustomReactionDiffusion::doSimulate()
{
    const size_t CELL_COUNT = static_cast<int>(domain->getCellCount());
    std::vector<Cell>& readFromVec = domain->getReadFromCells();
    std::vector<Cell>& writeToVec = domain->getWriteToCells();

    // Evaluate PDE
    std::vector<std::future<void>> futures;
    size_t indicesPerThread = (size_t)std::ceil((float)CELL_COUNT / threadPool_.getNumThreads());
    size_t start = 0;
    size_t end = 0;

    for (size_t threadID = 0; threadID < threadPool_.getNumThreads(); ++threadID)
    {
        start = threadID * indicesPerThread;
        end = std::min(start + indicesPerThread, CELL_COUNT);
        size_t numMorphs = MORPH_COUNT;
        size_t numSteps = stepCount;

        futures.emplace_back(threadPool_.enqueue([&, start, end, numMorphs, numSteps, threadID]() {
            ThreadWorks& threadWork = threadWork_[threadID];
            for (auto& workTuple : threadWork)
            {
                // Compute laplacian
                for (unsigned i : std::get<1>(workTuple))
                    domain->laplacian(i, readFromVec, lap.begin() + i * MORPH_COUNT);
            }

            // Compute PDEs
            customSimFunc(readFromVec, writeToVec, lap, threadWork, numMorphs, numSteps);

            }));
    }

    for (auto& task : futures)
        task.wait();
}

//================================================================================
StochasticCustomReactionDiffusion::StochasticCustomReactionDiffusion(
	SimulationDomain* domain,
	Parameters params,
	std::vector<Simulation::InitConditions> initConditions,
	std::vector<Simulation::BoundaryConditions> boundaryConditions,
	std::vector<Simulation::InitParams> initParams,
	std::map<std::string, int> morphIndexMap,
	const size_t threadCount) :
	Simulation(domain, params, "CPU", initConditions, boundaryConditions, initParams, morphIndexMap, threadCount)
{

#ifdef _WIN32	    
	if (!Utils::doesFileExist(Utils::BAT_FILENAME))
		Utils::createBatFile();
	system(Utils::BAT_FILENAME);
	dynamicLibLoader.loadLib("PDES.dll");
#elif __APPLE__
	std::ofstream makeFile("rdpg.mak");
	makeFile << "all:\n";
	makeFile << "\tclang++ -std=c++11 -O3 -arch x86_64 -dynamiclib -flat_namespace PDES.cpp -o PDES.so\n";
	makeFile.close();
	system("make -f rdpg.mak");
	dynamicLibLoader.loadLib("PDES.so");
#elif __linux__
	system("g++ -fPIC -shared PDES.cpp -o PDES.so");
	dynamicLibLoader.loadLib("PDES.so");
#endif

	customSimFunc = dynamicLibLoader.loadFunc<Simulate>("simulate");

	lap_noise.resize(domain->getCellCount() * MORPH_COUNT);
}

void StochasticCustomReactionDiffusion::doReloadSim()
{
	doReloadPDEs();
}

void StochasticCustomReactionDiffusion::doReloadPDEs()
{
	// Read file lines
	std::ifstream simFile(filename);
	std::vector<std::string> configFileLines;
	{
		std::string line = "";
		if (simFile.is_open())
		{
			while (std::getline(simFile, line))
				configFileLines.push_back(line);
			simFile.close();
		}
		else
		{
			LOG("Failed to reload PDES");
			return;
		}
	}

	// Calc morphs
	std::vector<std::string> morphogens;
	for (size_t i = 0; i < morphIndexMap.size(); ++i)
	{
		for (const auto& pair : morphIndexMap)
		{
			if (pair.second == (int)i)
				morphogens.push_back(pair.first);
		}
	}

	// parse RD model
	std::string simType = "CPU";
	std::string customModel = "";
	rawRDModel = "";
	if (!SimulationLoader::parseRDModel(simType, customModel, rawRDModel, configFileLines, morphogens))
	{
		LOG("Failed to load rdModel");
		return;
	}

	std::vector<Simulation::InitParams> initParams;
	std::string rawParamsStr;
	SimulationLoader::parseParams(initParams, rawParamsStr, configFileLines);
	std::string modelSubroutine = SimulationLoader::createModelFunc(morphogens, customModel, initParams[0].params);

	std::ofstream modelCPPFile("PDES.cpp");
	if (modelCPPFile.is_open())
	{
		modelCPPFile << modelSubroutine;
		modelCPPFile.flush();
		modelCPPFile.close();
	}

	dynamicLibLoader.freeLib();
#ifdef _WIN32	    
	if (!Utils::doesFileExist(Utils::BAT_FILENAME))
		Utils::createBatFile();
	system(Utils::BAT_FILENAME);
	dynamicLibLoader.loadLib("PDES.dll");
#elif __APPLE__
	system("make -f rdpg.mak");
	dynamicLibLoader.loadLib("PDES.so");
#elif __linux__
	system("g++ -fPIC -shared PDES.cpp -o PDES.so");
	dynamicLibLoader.loadLib("PDES.so");
#endif
	customSimFunc = nullptr;
	customSimFunc = dynamicLibLoader.loadFunc<Simulate>("simulate");
}

void StochasticCustomReactionDiffusion::doSimulate()
{
	const size_t CELL_COUNT = static_cast<int>(domain->getCellCount());
	std::vector<Cell>& readFromVec = domain->getReadFromCells();
	std::vector<Cell>& writeToVec = domain->getWriteToCells();

	std::vector<std::future<void>> futures;
	size_t indicesPerThread = (size_t)std::ceil((float)CELL_COUNT / threadPool_.getNumThreads());
	size_t start = 0;
	size_t end = 0;
	
	// generate same random numbers for each half edge
	for (size_t threadID = 0; threadID < threadPool_.getNumThreads(); ++threadID)
	{
		start = threadID * indicesPerThread;
		end = std::min(start + indicesPerThread, CELL_COUNT);

		futures.emplace_back(threadPool_.enqueue([&, start, end, threadID]() {
			// thread_local necessary?
			thread_local std::random_device randomDevice;
			thread_local std::mt19937 randomGen(randomDevice());
			thread_local std::uniform_int_distribution<unsigned int> uIntRan;
			seed_nran(uIntRan(randomDevice));
			//thread_local NormalDist normalRandom(uIntRan(randomDevice));
			for (size_t i = start; i < end; ++i)
			{
				// generate N(0,1) for each half edge, but this should be changed to compute mass flow rate...
				//domain->generateRandomNumbers(i, normalRandom);
				domain->generateRandomNumbers((unsigned) i);
			}
		}));
	}

	for (auto& task : futures)
		task.wait();

	// Evaluate PDE
	for (size_t threadID = 0; threadID < threadPool_.getNumThreads(); ++threadID)
	{
		start = threadID * indicesPerThread;
		end = std::min(start + indicesPerThread, CELL_COUNT);
		size_t numMorphs = MORPH_COUNT;
		size_t numSteps = stepCount;

		futures.emplace_back(threadPool_.enqueue([&, start, end, numMorphs, numSteps, threadID]() {
			ThreadWorks& threadWork = threadWork_[threadID];
			for (auto& workTuple : threadWork)
			{
				// Compute laplacian
				for (unsigned i : std::get<1>(workTuple))
					domain->laplacianCLE(i, readFromVec, lap.begin() + i * MORPH_COUNT, lap_noise.begin() + i * MORPH_COUNT);
			}

			// Compute PDEs
			thread_local std::random_device randomDevice;
			thread_local std::mt19937 randomGen(randomDevice());
			thread_local std::uniform_int_distribution<unsigned int> uIntRan;
			//thread_local NormalDist normalRandom(uIntRan(randomDevice));
			seed_nran(uIntRan(randomDevice));
			//customSimFunc(readFromVec, writeToVec, lap, lap_noise, threadWork, numMorphs, numSteps, normalRandom);
			customSimFunc(readFromVec, writeToVec, lap, lap_noise, threadWork, numMorphs, numSteps, nran);
		}));
	}

	for (auto& task : futures)
		task.wait();
}

//================================================================================
GrayScottReactionDiffusion::GrayScottReactionDiffusion(SimulationDomain* domain,
    Parameters params,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap) :
    Simulation(domain, params, "GrayScott", initConditions, boundaryConditions, initParams, morphIndexMap)
{}

void GrayScottReactionDiffusion::doSimulate()
{
    std::vector<Cell>& readFromCells = domain->getReadFromCells();
    std::vector<Cell>& writeToCells = domain->getWriteToCells();

    float f, k, dt, Da, Ds;
    int A = morphIndexMap["A"];
    int S = morphIndexMap["S"];
    std::vector<std::future<void>> futures;
    for (auto& paramPair : paramsMap)
    {
        Parameters& params = paramPair.params_;
        f = params["f"];
        k = params["k"];
        dt = params["dt"];
        Da = params["Da"];
        Ds = params["Ds"];

        size_t indicesPerThread = (size_t)std::ceil((float)paramPair.indices_.size() / threadPool_.getNumThreads());
        size_t start = 0;
        size_t end = 0;
        for (size_t threadID = 0; threadID < threadPool_.getNumThreads(); ++threadID)
        {
            start = threadID * indicesPerThread;
            end = std::min(start + indicesPerThread, paramPair.indices_.size());
            futures.emplace_back(threadPool_.enqueue([&, Da, Ds, dt, f, k, start, end, threadID]() {
                for (size_t i = start; i < end; ++i)
                {
                    domain->laplacian((unsigned) i, readFromCells, lap.begin() + i * MORPH_COUNT);

                    float a = readFromCells[i][A];
                    float s = readFromCells[i][S];
                    float saa = s * a * a;
                    writeToCells[i][A] = a + (Da * lap[i * MORPH_COUNT + A] + saa - (k + f) * a) * dt;
                    writeToCells[i][S] = s + (Ds * lap[i * MORPH_COUNT + S] - saa + f * (1.f - s)) * dt;
                }
                }));
        }
    }

    for (auto& task : futures)
        task.wait();
}

//================================================================================

OsterMurrayReactionDiffusion::OsterMurrayReactionDiffusion(SimulationDomain* domain,
    Parameters params,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap) :
    Simulation(domain, params, "OsterMurray", initConditions, boundaryConditions, initParams, morphIndexMap)
{}

void OsterMurrayReactionDiffusion::doSimulate()
{
    std::vector<Cell>& readFromCells = domain->getReadFromCells();
    std::vector<Cell>& writeToCells = domain->getWriteToCells();
    float n, c;

    Vec3 gradN, gradC;
    int U = morphIndexMap["U"];
    int V = morphIndexMap["V"];
    float a, D, s, r, N, dt;

    for (auto& paramPair : paramsMap)
    {
        Parameters& params = paramPair.params_;
        a = params["a"];
        D = params["D"];
        s = params["s"];
        r = params["r"];
        N = params["N"];
        dt = params["dt"];

        for (unsigned i : paramPair.indices_)
        {
            n = readFromCells[i][U];
            c = readFromCells[i][V];

            domain->laplacian(i, readFromCells, lap.begin() + i * MORPH_COUNT);
            domain->gradient(i, U, readFromCells, gradN);
            domain->gradient(i, V, readFromCells, gradC);

            writeToCells[i][U] = n + (D * lap[i * MORPH_COUNT + U] - a * n * lap[i * MORPH_COUNT + V] - a * gradN.dot(gradC) + s * r * n * (N - n)) * dt;
            writeToCells[i][V] = c + (lap[i * MORPH_COUNT + V] + s * (n / (1.f + n) - c)) * dt;
        }
    }
}

//================================================================================
DecayDiffusion::DecayDiffusion(SimulationDomain* domain,
    Parameters params,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap) :
    Simulation(domain, params, "DecayDiffusion", initConditions, boundaryConditions, initParams, morphIndexMap)
{}

void DecayDiffusion::doSimulate()
{
    std::vector<Cell>& readFromCells = domain->getReadFromCells();
    std::vector<Cell>& writeToCells = domain->getWriteToCells();

    int A = morphIndexMap["A"];
    unsigned fixedIndex = 0;
    float Da = 0.f, dt = 0.f, Ka = 0.f;

    
    for (auto& paramPair : paramsMap)
    {
        Parameters& params = paramPair.params_;
        Ka = params["Ka"];
        Da = params["Da"];
        dt = params["dt"];
        fixedIndex = (int)params["fixedIndex"];

        for (unsigned i : paramPair.indices_)
        {
            domain->laplacian(i, readFromCells, lap.begin() + i * MORPH_COUNT);

            if (i != fixedIndex)
            {
                writeToCells[i][A] = readFromCells[i][A] + (Da * lap[i * MORPH_COUNT + A] - Ka * readFromCells[i][A]) * dt;
            }
        }
    }
}

//================================================================================
GiererMeinhardtReactionDiffusion::GiererMeinhardtReactionDiffusion(SimulationDomain* domain,
    Parameters params,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap) :
    Simulation(domain, params, "GiererMeinhardt", initConditions, boundaryConditions, initParams, morphIndexMap)
{}

void GiererMeinhardtReactionDiffusion::doSimulate()
{
    std::vector<Cell>& readFromCells = domain->getReadFromCells();
    std::vector<Cell>& writeToCells = domain->getWriteToCells();


    float u, v;
    float uu, uuv;
    int U = morphIndexMap["U"];
    int V = morphIndexMap["V"];
    float k, u_u, s_u, s_v, p_u, p_v, Du, Dv, dt;

    //domain->laplacians(readFromCells);
    for (auto& paramPair : paramsMap)
    {
        Parameters& params = paramPair.params_;
        k = params["k"];
        u_u = params["u_u"];
        s_u = params["s_u"];
        s_v = params["s_v"];
        p_u = params["p_u"];
        p_v = params["p_v"];
        Du = params["Du"];
        Dv = params["Dv"];
        dt = params["dt"];

        for (unsigned i : paramPair.indices_)
        {
            u = readFromCells[i][U];
            v = readFromCells[i][V];

            domain->laplacian(i, readFromCells, lap.begin() + i * MORPH_COUNT);

            uu = u * u;
            uuv = uu * v;
            writeToCells[i][U] = u + (Du * lap[i * MORPH_COUNT + U] + p_u * uuv / (1.f + k * uu) - u_u * u + s_u) * dt;
            writeToCells[i][V] = v + (Dv * lap[i * MORPH_COUNT + V] - p_v * uuv / (1.f + k * uu) + s_v) * dt;
        }
    }
}

//================================================================================
KondoReactionDiffusion::KondoReactionDiffusion(SimulationDomain* domain,
    Parameters params,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap) :
    Simulation(domain, params, "Kondo", initConditions, boundaryConditions, initParams, morphIndexMap)
{}

void KondoReactionDiffusion::doSimulate()
{
    std::vector<Cell>& readFromCells = domain->getReadFromCells();
    std::vector<Cell>& writeToCells = domain->getWriteToCells();

    float a, s;

    float F, G, c1, c2, c3, c4, c5, dt, Da, Ds, ga, gs;
    int A = morphIndexMap["A"];
    int S = morphIndexMap["S"];

    for (auto& paramPair : paramsMap)
    {
        Parameters& params = paramPair.params_;
        c1 = params["c1"];
        c2 = params["c2"];
        c3 = params["c3"];
        c4 = params["c4"];
        c5 = params["c5"];
        dt = params["dt"];
        Da = params["Da"];
        Ds = params["Ds"];
        ga = params["ga"];
        gs = params["gs"];

        for (unsigned i : paramPair.indices_)
        {
            a = readFromCells[i][A];
            s = readFromCells[i][S];
            F = c1 * a + c2 * s + c3;
            G = c4 * a + c5;

            if (F < 0.f)
                F = 0.f;
            else if (F > .18f)
                F = .18f;

            if (G < 0.f)
                G = 0.f;
            else if (G > .5)
                G = .5f;

            domain->laplacian(i, readFromCells, lap.begin() + i * MORPH_COUNT);

            writeToCells[i][A] = a + (F + Da * lap[i * MORPH_COUNT + A] - ga * a) * dt;
            writeToCells[i][S] = s + (G + Ds * lap[i * MORPH_COUNT + S] - gs * s) * dt;
        }
    }
}

//================================================================================
TuringReactionDiffusion::TuringReactionDiffusion(SimulationDomain* domain,
    Parameters params,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap) :
    Simulation(domain, params, "Turing", initConditions, boundaryConditions, initParams, morphIndexMap)
{}

void TuringReactionDiffusion::doSimulate()
{
    const std::vector<Cell>& readFromCells = domain->getReadFromCells();
    std::vector<Cell>& writeToCells = domain->getWriteToCells();

    float u, v;
    float a, b, s, Du, Dv, dt, uSat, vSat, p_v, u_u;
    bool useUSat, useVSat;
    int U = morphIndexMap["U"], V = morphIndexMap["V"];

    for (auto& paramPair : paramsMap)
    {
        Parameters& params = paramPair.params_;
        a = params["a"];
        b = params["b"];
        s = params["s"];
        p_v = params["p_v"];
        u_u = params["u_u"];
        Du = params["Du"];
        Dv = params["Dv"];
        dt = params["dt"];
        uSat = params.count("uSat") > 0 ? params["uSat"] : 0.f;
        vSat = params.count("vSat") > 0 ? params["vSat"] : 0.f;
        useUSat = uSat != 0.f;
        useVSat = vSat != 0.f;

        for (unsigned i : paramPair.indices_)
        {
            u = readFromCells[i][U];
            v = readFromCells[i][V];

            domain->laplacian(i, readFromCells, lap.begin() + i * MORPH_COUNT);

            writeToCells[i][U] = u + (s * (a - u * v - u_u)     + Du * lap[i * MORPH_COUNT + U]) * dt;
            writeToCells[i][V] = v + (s * (u * v - v - b + p_v) + Dv * lap[i * MORPH_COUNT + V]) * dt;

            if (writeToCells[i][U] < 0.f)
                writeToCells[i][U] = 0.f;
            else if (useUSat && writeToCells[i][U] > uSat)
                writeToCells[i][U] = uSat;

            if (writeToCells[i][V] < 0.f)
                writeToCells[i][V] = 0.f;
            else if (useVSat && writeToCells[i][V] > vSat)
                writeToCells[i][V] = vSat;
        }
    }
}

//================================================================================
ActivatorInhibitorReactionDiffusion::ActivatorInhibitorReactionDiffusion(SimulationDomain* domain,
    Parameters params,
    std::vector<Simulation::InitConditions> initConditions,
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams,
    std::map<std::string, int> morphIndexMap) :
    Simulation(domain, params, "ActivatorInhibitor", initConditions, boundaryConditions, initParams, morphIndexMap)
{}

void ActivatorInhibitorReactionDiffusion::doSimulate()
{
    std::vector<Cell>& readFromCells = domain->getReadFromCells();
    std::vector<Cell>& writeToCells = domain->getWriteToCells();


    float a, h;
    float p, p_a, p_h, u_h, u_a, Da, Dh, dt, aa;
    int A = morphIndexMap["A"], H = morphIndexMap["H"];

    for (auto& paramPair : paramsMap)
    {
        Parameters& params = paramPair.params_;
        p = params["p"];
        p_a = params["p_a"];
        p_h = params["p_h"];
        u_a = params["u_a"];
        u_h = params["u_h"];
        Da = params["Da"];
        Dh = params["Dh"];
        dt = params["dt"];

        for (unsigned i : paramPair.indices_)
        {
            a = readFromCells[i][A];
            h = readFromCells[i][H];
            aa = a * a;

            domain->laplacian(i, readFromCells, lap.begin() + i * MORPH_COUNT);

            writeToCells[i][A] = a + (p * (aa / h + p_a) - u_a * a + Da * lap[i * MORPH_COUNT + A]) * dt;
            writeToCells[i][H] = h + (p * aa + p_h - u_h * h       + Dh * lap[i * MORPH_COUNT + H]) * dt;
        }
    }
}
