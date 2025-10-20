#pragma once
#include "SimulationDomain.h"
#include "Counter.h"
#include "ThreadPool.h"

#include <vector>
#include <random>


using Cell = SimulationDomain::Cell;
using Parameters = std::map<std::string, float>;
struct ParamIndexPair
{
    bool used()
    {
        return indices_.size() > 0;
    }

    bool contains(unsigned index)
    {
        return indices_.count(index) > 0;
    }

    void update(const Parameters& newParams)
    {
        for (auto& p : newParams)
            params_[p.first] = p.second;
    }

    ParamIndexPair(Parameters params) :
        params_(params)
    {}

    ParamIndexPair() = default;

    std::set<unsigned> indices_;
    Parameters params_;
};

class GUI;
class Simulation
{
public:
    struct InitConditions
    {
        std::string indices;
        int radius = 0;
        std::map<std::string, std::string> morpMap;

        InitConditions(std::string indices) :
            indices(indices)
        {}
    };

    struct BoundaryConditions
    {
        std::string indices = "boundary";
        std::map<std::string, std::string> morpMap;

        BoundaryConditions(std::string indices = "boundary") :
            indices(indices)
        {}
    };

    struct InitParams
    {
        std::string indices;
        int radius = 0;
        Parameters params;

        InitParams(std::string indices) :
            indices(indices)
        {}
    };

    Simulation(
        SimulationDomain* domain,
        Parameters params,
        const std::string& name,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap,
        size_t threadCount = 0,
        Vec3 growth = Vec3(0), 
        float maxFaceArea = .01f);

    virtual ~Simulation() = default;

    void simulate();
    void populateCells(std::vector<Simulation::InitConditions> initConditions, std::map<std::string, int> morphIndexMap);
    void setBoundaryConditions(std::vector<Simulation::BoundaryConditions> boundaryConditions, std::map<std::string, int> morphIndexMap);
    virtual void initSim();
    virtual void updateColors();
    virtual void growAndSubdivide();
    virtual void updateDiffusionCoefs();

    //bool saveSim(const std::string& path, const std::string& fileName);
    bool saveConcentrations(const std::string& path, const std::string& fileName);
    bool saveEditorSettings(const std::string& path);
    bool recording() const;
    bool loadSim(const std::string& fileName);
    bool reloadSim();
    bool reloadPDEs();

    void shouldReloadSim(bool value);
    void shouldReloadPDEs(bool value);
    virtual void doReloadSim() = 0;
    virtual void doReloadPDEs() = 0;

    void pause(bool state);
    bool isPaused();
    virtual void updateGPU();
    virtual void updateRAM();
    bool gpuUpToDate();
    bool ramUpToDate();
    void gpuUpToDate(bool state);
    void ramUpToDate(bool state);
    bool updateParams(Parameters params);
    ParamIndexPair& getParamIndexPair(unsigned index);
    void setGrowthTickLimit(unsigned long long growthTickLimit);
    unsigned long long getGrowthTickLimit() const;
    void computeThreadWork();

    SimulationDomain* domain;
    Counter growthCounter;
    Vec3 growth;
    float maxFaceArea;

    std::map<std::string, int> morphIndexMap;
    std::vector<ParamIndexPair> paramsMap;
    Parameters originalParams;
    bool isGPUEnabled = false;
    bool pauseAt = false, exitAt = false;
    bool subdivisionEnabled_ = true;

    std::string name;
    std::string initalFilename;
    std::string filename;
    std::string veinMorph;
    std::string rawInitConds;
    std::string rawBCs;
    std::string rawRDModel;
    std::vector<Simulation::InitConditions> initConditions_;
    std::vector<Simulation::BoundaryConditions> boundaryConditions_;

    bool createTextureOnExit = false, createModelOnExit = false, outputTextures = false, outputPlys = false, outputScreens = false;
    int pauseStepCount = 0;
    int exitStepCount = 0;
    int screenshotsPerSim = 0;
    int outMorphIndex = 0;
    int MORPH_COUNT = 0;
    char videoPath[256] = ".";
    bool updateColorsFromRam = false;
    int stepCount = 0;
    bool shouldReloadSim_ = false;
    bool shouldReloadPDEs_ = false;
    ThreadPool threadPool_;

protected:
    std::vector<int> parseIndices(std::string indicesStr, int radius = 0);
    float parseVal(std::string valStr, std::mt19937& mt);

    virtual void doSimulate() = 0;

    ParamIndexPair& getParamIndexPair(const Parameters& params, bool& newPairCreated);

    bool paused = false;
    unsigned long long growthTickLimit = 0;
    std::vector<float> lap;
    using ThreadWork = std::tuple<Parameters, std::vector<unsigned>>;
    using ThreadWorks = std::vector<ThreadWork>;
    std::vector<ThreadWorks> threadWork_;

private:
    bool gpuUpToDateFlag = false;
    bool ramUpToDateFlag = false;
};

std::string indicesToString(std::set<unsigned>& indices);

//================================================================================
#include "DynamicLibLoader.h"
class CustomReactionDiffusion : public Simulation
{
    typedef void(*Simulate)(
        std::vector<Cell>& readFrom,
        std::vector<Cell>& writeTo,
        std::vector<float>& L,
        ThreadWorks& threadWork,
        const size_t MORPH_COUNT,
        const size_t stepCount
        );

    Simulate customSimFunc;
    DynamicLibLoader dynamicLibLoader;

public:
    CustomReactionDiffusion(SimulationDomain* domain,
        Parameters params,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap,
        const size_t threadCount);
    void doSimulate() override;
    void doReloadPDEs() override;
    void doReloadSim() override;
};

//================================================================================
class StochasticCustomReactionDiffusion : 
    public Simulation //TODO: inherit from public CustomReactionDiffusion
{
	typedef void(*Simulate)(
		std::vector<Cell>& readFrom,
		std::vector<Cell>& writeTo,
		std::vector<float>& L,
		std::vector<float>& Lnoise,
		ThreadWorks& threadWork,
		const size_t MORPH_COUNT,
		const size_t stepCount,
		float(&nran)(void)
		);

	Simulate customSimFunc;
	DynamicLibLoader dynamicLibLoader;
	std::vector<float> lap_noise;

public:
	StochasticCustomReactionDiffusion(SimulationDomain* domain,
		Parameters params,
		std::vector<Simulation::InitConditions> initConditions,
		std::vector<Simulation::BoundaryConditions> boundaryConditions,
		std::vector<Simulation::InitParams> initParams,
		std::map<std::string, int> morphIndexMap,
		const size_t threadCount);
	void doSimulate() override;
	void doReloadPDEs() override;
	void doReloadSim() override;
};

//================================================================================
class GrayScottReactionDiffusion : public Simulation
{
public:
    GrayScottReactionDiffusion(SimulationDomain* domain,
        Parameters params,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap);
    void doSimulate() override;
    void doReloadSim() override {}
    void doReloadPDEs() override {}
};

//================================================================================
class OsterMurrayReactionDiffusion : public Simulation
{
public:
    OsterMurrayReactionDiffusion(SimulationDomain* domain,
        Parameters params,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap);
    void doSimulate() override;
    void doReloadSim() override {}
    void doReloadPDEs() override {}
};

//================================================================================
class DecayDiffusion : public Simulation
{
public:
    DecayDiffusion(SimulationDomain* domain,
        Parameters params,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap);
    void doSimulate() override;
    void doReloadSim() override {}
    void doReloadPDEs() override {}
};

//================================================================================
class GiererMeinhardtReactionDiffusion : public Simulation
{
public:
    GiererMeinhardtReactionDiffusion(SimulationDomain* domain,
        Parameters params,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap);
    void doSimulate() override;
    void doReloadSim() override {}
    void doReloadPDEs() override {}
};

//================================================================================
class KondoReactionDiffusion : public Simulation
{
public:
    KondoReactionDiffusion(SimulationDomain* domain,
        Parameters params,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap);
    void doSimulate() override;
    void doReloadSim() override {}
    void doReloadPDEs() override {}
};

//================================================================================
class TuringReactionDiffusion : public Simulation
{
public:
    TuringReactionDiffusion(SimulationDomain* domain,
        Parameters params,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap);
    void doSimulate() override;
    void doReloadSim() override {}
    void doReloadPDEs() override {}
};

//================================================================================
class ActivatorInhibitorReactionDiffusion : public Simulation
{
public:
    ActivatorInhibitorReactionDiffusion(SimulationDomain* domain,
        Parameters params,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap);
    void doSimulate() override;
    void doReloadSim() override {}
    void doReloadPDEs() override {}
};
