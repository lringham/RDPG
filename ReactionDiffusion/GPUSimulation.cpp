#include "GPUSimulation.h"
#include "SimulationLoader.h"

#include <string>
#include <vector>
#include <map>


GPUSimulation::GPUSimulation(SimulationDomain* domain, 
    Parameters params, 
    const std::map<std::string, std::string>& includes,
    std::vector<Simulation::InitConditions> initConditions, 
    std::vector<Simulation::BoundaryConditions> boundaryConditions,
    std::vector<Simulation::InitParams> initParams, 
    std::map<std::string, int> morphIndexMap,
	bool stochastic) :
    Simulation(domain, params, "GPU", initConditions, boundaryConditions, initParams, morphIndexMap, 1)
{
	isStochastic = stochastic;
    isGPUEnabled = true;
    domain->isGPUEnabled = true;
    domain->normCoef = 1.f;

    this->includes = includes;
    if (domain->isDomainType(SimulationDomain::DomainType::MESH))
#if DIFFUSION_EQNS==1
        cs = std::make_unique<MeshComputeShader>(this, includes, "meshComputeShaderStochastic.glsl");
#else
		cs = std::make_unique<MeshComputeShader>(this, includes, "meshComputeShader.glsl");
#endif
    else if (domain->isDomainType(SimulationDomain::DomainType::GRID))
        cs = std::make_unique<GridComputeShader>(this, includes);
    else
        return;

    cs->createSSBOs(static_cast<GLint>(morphIndexMap.size()));
    initSim();
}

void GPUSimulation::doSimulate()
{
    cs->bindBuffers();
    
    if (precCotanWeights)
    {
        cs->precompCotans();
        precCotanWeights = false;
    }
	cs->calcFluxes();
    cs->calcSimulate();
    cs->swapState();
}

void GPUSimulation::growAndSubdivide()
{
    cs->updateRAM();
    domain->growAndSubdivide(growth, maxFaceArea, subdivisionEnabled_, stepCount);
    cs->bindBuffers();

    if (domain->cellsToUpdate.size() > 0)
    {
        // For each new cell, get neighbouring params and calc param for new cell
        for (auto& newCell : domain->cellsToUpdate)
        {
            if (newCell.second.neighbours.size() == 0)
                continue;

            unsigned i = newCell.second.neighbours[0];
            for (auto& p : paramsMap)
            {
                if (p.contains(i))
                {
                    p.indices_.insert(newCell.first);
                    domain->dirtyAttributes[domain->PARAM_ATTRB].indices.insert(newCell.first);
                    for (auto& param : p.params_)
                        domain->dirtyAttributes[domain->PARAM_ATTRB].attributes.push_back(param.second);
                    break;
                }
            }
        }

        for (auto& newCell : domain->cellsToUpdate)
        {
            domain->dirtyAttributes[domain->VERTEX_ATTRB].indices.insert(newCell.first);
            domain->dirtyAttributes[domain->TANGENT_ATTRIB].indices.insert(newCell.first);
            domain->dirtyAttributes[domain->CELL1_ATTRIB].indices.insert(newCell.first);
            domain->dirtyAttributes[domain->CELL2_ATTRIB].indices.insert(newCell.first);
        }
        domain->cellsToUpdate.clear();

        cs->updateGPU();
    }

    cs->calcGrow(domain->getTotalArea(), domain->getGrowthMode());
    cs->calcDomainAttribs();

    updateColorsFromRam = true;
    updateColors();
}

void GPUSimulation::updateDiffusionCoefs()
{
    precCotanWeights = true;
}

void GPUSimulation::initSim()
{}

void GPUSimulation::doReloadSim()
{
    doReloadPDEs();
}

void GPUSimulation::doReloadPDEs()
{
    updateRAM();

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

    std::vector<std::string> morphogens;
    for (size_t i = 0; i < morphIndexMap.size(); ++i)
    {
        for (const auto& pair : morphIndexMap)
        {
            if (pair.second == (int) i)
                morphogens.push_back(pair.first);
        }
    }

    std::string simType = isStochastic ? "stochastic GPU" : "GPU";
    std::string customModel = "";
    rawRDModel = "";
    if (!SimulationLoader::parseRDModel(simType, customModel, rawRDModel, configFileLines, morphogens))
    {
        LOG("Failed to load rdModel");
        return;
    }

	bool veins = veinMorph != "" && (domain->veinDiffusionScale_ != 1.f || domain->petalDiffusionScale_ != 1.f);
    includes["#include \"model\""] = SimulationLoader::createModelSubroutine(morphogens, customModel, domain->domainTypeStr(), isStochastic, veins, paramsMap[0].params_);

    cs = nullptr;
    if (domain->isDomainType(SimulationDomain::DomainType::MESH))
#if DIFFUSION_EQNS==1
		cs = std::make_unique<MeshComputeShader>(this, includes, "meshComputeShaderStochastic.glsl");
#else
		cs = std::make_unique<MeshComputeShader>(this, includes, "meshComputeShader.glsl");
#endif
    else if (domain->isDomainType(SimulationDomain::DomainType::GRID))
        cs = std::make_unique<GridComputeShader>(this, includes);
    else
        return;

    cs->createSSBOs(static_cast<GLint>(morphIndexMap.size()));
    initSim();
    isGPUEnabled = true;
    domain->isGPUEnabled = true;
    domain->normCoef = 1.f;
}

void GPUSimulation::updateColors()
{
    if (updateColorsFromRam)
    {
        domain->update();
        domain->updateColorVBO();
        updateColorsFromRam = false;
    }

    // Update gradients
    if (!ramUpToDate())
    {
        bool showingGrad = false;
        for (bool sg : domain->showingGrad)
            showingGrad = showingGrad || sg;
        if (showingGrad)
            updateRAM();
    }

    domain->updateGradLines();
    domain->updateDiffDirLines();

    domain->shader_->setTexture("colorMapInside", domain->colorMapInside.getTextureID());
    domain->shader_->setTexture("colorMapOutside", domain->colorMapOutside.getTextureID());
    domain->shader_->setTexture("background", domain->background_.id());
    domain->shader_->setUniform1f("normCoef", domain->normCoef);
    domain->shader_->setUniform1f("backgroundThreshold", domain->backgroundThreshold_);
    domain->shader_->setUniform1f("backgroundOffset", domain->backgroundOffset_);
    domain->shader_->setUniform1i("useBackgroundTexture", domain->background_.id() != 0);

    if (isGPUEnabled && !gpuUpToDate())
        updateGPU();
}

void GPUSimulation::updateGPU()
{
    LOG("updating gpu");
    cs->updateGPU();
    gpuUpToDate(true);
}

void GPUSimulation::updateRAM()
{
    LOG("updating ram");
    cs->updateRAM();
    domain->update();
    ramUpToDate(true);
}