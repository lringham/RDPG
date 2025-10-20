#pragma once
#include "Simulation.h"
#include "MeshComputeShader.h"
#include "GridComputeShader.h"
#include "DefaultShader.h"
#include "Utils.h"
#include "Vec4.h"

#include <random>


class GPUSimulation : 
    public Simulation
{
    std::unique_ptr<ReactionDiffusionComputeShader> cs;
    std::map<std::string, std::string> includes;

	bool isStochastic;

public:
    GPUSimulation(
        SimulationDomain* domain,
        Parameters params,
        const std::map<std::string, std::string>& includes,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap,
		bool stochastic);

    void doSimulate() override;
    void growAndSubdivide();
    void updateDiffusionCoefs() override;
    void initSim() override;
    void doReloadSim() override;
    void doReloadPDEs() override;
    void updateColors() override;
    void updateGPU() override;
    void updateRAM() override;

    bool precCotanWeights = false;
};