#pragma once
#include "Camera.h"
#include "SimulationDomain.h"
#include "Simulation.h"
#include "CmdArgsParser.h"

#include <vector>
#include <string>
#include <map>
#include <any>


class SimulationLoader
{
public:
    SimulationLoader() = default;

    bool loadSim(std::string filename, SimulationDomain*& d, Simulation*& s, Camera& camera, const CmdArgsParser& cmdArgsParser);
    static std::string createModelSubroutine(const std::vector<std::string>& morphogens, std::string customModel, const std::string& domainstr, bool stochastic, bool veins, const Parameters& params);
    static std::string createModelFunc(const std::vector<std::string>& morphogens, std::string customModel, Parameters params);
    static std::string convertToGPUCode(std::string source);
    static std::string convertToCPUCode(std::string source);
    static bool parseRDModel(std::string& simType, std::string& customModelStr, std::string& rawModelStr, const std::vector<std::string>& configFileLines, const std::vector<std::string>& morphogens);
    static bool parseBoundaryConditions(std::vector<Simulation::BoundaryConditions>& boundaryConditions, std::string& rawModelStr, const std::vector<std::string>& configFileLines);
    static bool parseInitialConditions(std::vector<Simulation::InitConditions>& initConditions, std::string& rawParamsStr, const std::vector<std::string>& configFileLines);
    static bool parseParams(std::vector<Simulation::InitParams>& initParams, std::string& rawInitCondsStr, const std::vector<std::string>& configFileLines);

private:
    void parseSimulation(
        const std::string& simName,
        Simulation*& s,
        SimulationDomain*& d,
        Parameters& globalParamMap,
        const std::map<std::string, std::string>& includes,
        std::vector<Simulation::InitConditions> initConditions,
        std::vector<Simulation::BoundaryConditions> boundaryConditions,
        std::vector<Simulation::InitParams> initParams,
        std::map<std::string, int> morphIndexMap,
        size_t numThreads);

    bool parseDomain(
        const std::string& domName,
        SimulationDomain*& d,
        Parameters globalParamMap,
        const std::vector<std::string>& colorMaps);

    std::string rawInitConds = "";
    std::string rawRDModel = "";
    std::string rawBCs = "";
    std::string rawParams = "";
};