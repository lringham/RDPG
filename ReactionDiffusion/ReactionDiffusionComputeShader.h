#pragma once
#include "ComputeShader.h"
#include "Simulation.h"


class ReactionDiffusionComputeShader
    : public ComputeShader
{
public:
    ReactionDiffusionComputeShader(const std::map<std::string, std::string>& includes, const std::string& shaderFilename)
        : ComputeShader(includes, shaderFilename)
    {}

    virtual void createSSBOs(const GLint NUM_MORPHS) = 0;
    virtual void bindBuffers() = 0;
    virtual void updateGPU() = 0;
    virtual void updateRAM() = 0;
    virtual void updateUniforms() = 0;
    virtual void swapState() = 0;
    virtual void calcSimulate() = 0;
	virtual void calcFluxes() = 0;
    virtual void calcGrow(float totalArea, int growthMode) = 0;
    virtual void calcDomainAttribs() = 0;
    virtual void precompCotans() {};

    Simulation* sim = nullptr;
    GLuint simLoc = 0, calcGrowLoc = 0, calcFluxesLoc = 0;
    std::string subroutineName = "";
};