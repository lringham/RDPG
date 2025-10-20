#pragma once
#include "ReactionDiffusionComputeShader.h"
#include "Grid.h"
#include "Textures.h"


class GridComputeShader
    : public ReactionDiffusionComputeShader
{
public:
    GridComputeShader(Simulation* sim, const std::map<std::string, std::string>& includes) :
        ReactionDiffusionComputeShader(includes, 
R"(#version 460

// local workgroup size
layout(local_size_x = 16,
    local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform image2D outputTexture;

#include "defines"

//=========================
// Structs
//=========================
#include "parameters"

struct Position
{
    float x, y, z;
};

struct CellData
{
    float vals[MORPHOGEN_COUNT];
};

struct IsConstVal
{
    bool vals[MORPHOGEN_COUNT];
};

struct AnisoVecs
{
    Position vec[MORPHOGEN_COUNT];
};

struct Tensor
{
    float dxx[MORPHOGEN_COUNT];
    float dxy_yx[MORPHOGEN_COUNT];
    float dyy[MORPHOGEN_COUNT];
};

//=========================
// SSBOs
//=========================
#define stdlayout std430,

layout(stdlayout binding = 8) buffer TensorSSBO
{
    Tensor tensors[];
};
layout(stdlayout binding = 9) buffer Cell1SSBO
{
    CellData cells1[];
};
layout(stdlayout binding = 10) buffer Cell2SSBO
{
    CellData cells2[];
};
layout(stdlayout binding = 16) buffer diffScaleCoefsSSBO
{
    CellData diffScaleCoefs[];
};
layout(stdlayout binding = 17) buffer paramsSSBO
{
    Parameters params[];
};
layout(stdlayout binding = 18) buffer isConstValSSBO
{
    IsConstVal isConstVal[];
};

//=========================
// Uniforms
//=========================
uniform int outMorphIndex;
uniform vec3 growthRate;
uniform int stepCount;
uniform int xRes;
uniform int yRes;
uniform float areaScale;

//=========================
// Methods
//=========================

void indexToVec2(uint gid, inout uint x, inout uint y)
{
    x = gid / yRes;
    y = gid % yRes;
}

uint vec2ToIndex(uint x, uint y)
{
    return y + x * yRes;
}

void lap(uint gid, inout float L[MORPHOGEN_COUNT])
{
    for (int i = 0; i < MORPHOGEN_COUNT; ++i)
        L[i] = 0;

    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    // avoid branching for performance
    uint left = uint(x != 0);
    uint right = uint(x + 1 != xRes);
    uint up = uint(y + 1 != yRes);
    uint down = uint(y != 0);

    uint i_x1y0, i_x1y2;
    i_x1y0 = gid - down;
    i_x1y2 = gid + up;

    uint i_x0y0, i_x0y1, i_x0y2;
    i_x0y0 = i_x1y0 - yRes * left;
    i_x0y1 = gid - yRes * left;
    i_x0y2 = i_x1y2 - yRes * left;

    uint i_x2y0, i_x2y1, i_x2y2;
    i_x2y0 = i_x1y0 + right * yRes;
    i_x2y1 = gid + right * yRes;
    i_x2y2 = i_x1y2 + right * yRes;

    float invAreaScale = 1.f / areaScale;
    float m = 0;
    float dxx = 1;
    float dyy = 1;
    float dxy_yx = 0;

    for (int i = 0; i < MORPHOGEN_COUNT; ++i)
    {
        dxx = tensors[gid].dxx[i];
        dxy_yx = tensors[gid].dxy_yx[i];
        dyy = tensors[gid].dyy[i];

        m = -2.f * cells1[gid].vals[i] * (dxx + dyy);

        m -= cells1[i_x0y0].vals[i] * dxy_yx;
        m += cells1[i_x0y1].vals[i] * dxx;
        m += cells1[i_x0y2].vals[i] * dxy_yx;

        m += cells1[i_x1y0].vals[i] * dyy;
        m += cells1[i_x1y2].vals[i] * dyy;

        m += cells1[i_x2y0].vals[i] * dxy_yx;
        m += cells1[i_x2y1].vals[i] * dxx;
        m -= cells1[i_x2y2].vals[i] * dxy_yx;

        L[i] = m * invAreaScale;
        // L[i] = diffScaleCoefs[gid].vals[i] * m * invAreaScale;
    }
}

//=========================
// Subroutines
//=========================
subroutine void calculateSub(uint gid); 		// Subroutine Signature				
subroutine uniform calculateSub calculate;

#include "model"

//=========================
// Main
//=========================
void main()
{
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    // avoid going out of bounds if invocation size is larger than image size
    if (x < xRes && y < yRes)
        calculate(vec2ToIndex(x, y));
})")
    {
        this->sim = sim;
        grid = dynamic_cast<Grid*>(sim->domain);
        ASSERT(grid != nullptr, "Invalid domain used in grid compute shader");
        this->subroutineName = "model";
    }

    void createSSBOs(const GLint NUM_MORPHS) override
    {
        LOG("creating ssbos");
        std::vector<float> data(2 * NUM_MORPHS * grid->getCellCount(), 0.f);
        {
            size_t i = 0;
            const auto& cells = grid->getReadFromCells();
            for (auto& c : cells)
                for (int j = 0; j < 2; ++j) // space for both old and new values
                    for (int morph = 0; morph < NUM_MORPHS; ++morph)
                        data[i++] = c.vals[morph];
        }

        // Initialize SSBOs ========================================================================
        // load initial morphogens
        LOG("Loading data to GPU...");
        const unsigned CELL_COUNT = grid->getCellCount();
        const unsigned NUM_PARAMS = static_cast<unsigned>(sim->originalParams.size());

        GLint size = 0;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
        size /= 20; //FIXME: arbitrary choice to not run out of memory
        {	// load initial morphogens
            std::vector<GLfloat> cells(size / sizeof(GLfloat));

            // first array of cells		
            for (unsigned i = 0; i < CELL_COUNT; ++i)
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    cells[i * NUM_MORPHS + j] = grid->cells1[i].vals[j];
            cells1.init(cells);

            // second array of cells
            for (unsigned i = 0; i < CELL_COUNT; ++i)
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    cells[i * NUM_MORPHS + j] = grid->cells2[i].vals[j];
            cells2.init(cells);
        }
        {	// load params
            std::vector<GLfloat> paramsVals(size / sizeof(GLfloat));
            for (auto paramsMapEntry : sim->paramsMap)
            {
                for (size_t i : paramsMapEntry.indices_)
                {
                    size_t j = 0;
                    for (auto p : paramsMapEntry.params_)
                    {
                        paramsVals[i * NUM_PARAMS + j] = p.second;
                        j++;
                    }
                }
            }
            params.init(paramsVals);
        }
        {
            int k = 0;
            std::vector<GLfloat> anisoCoefsVals(size / sizeof(GLfloat));
            for (unsigned i = 0; i < CELL_COUNT; ++i)
            {
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    anisoCoefsVals[k++] = grid->anisoCoefs_[j][i].dxx;
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    anisoCoefsVals[k++] = grid->anisoCoefs_[j][i].dxy_yx;
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    anisoCoefsVals[k++] = grid->anisoCoefs_[j][i].dyy;
            }
            anisoCoefs.init(anisoCoefsVals);
        }
        {
            std::vector<GLint> isConstValVals(size / sizeof(GLint), 0);
            for (unsigned i = 0; i < CELL_COUNT; ++i)
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    isConstValVals[i * NUM_MORPHS + j] = grid->cells1[i].isConstVal[j] ? 1 : 0;
            isConstVal.init(isConstValVals);
        }

        glUseProgram(program);

        simLoc = glGetSubroutineIndex(program, GL_COMPUTE_SHADER, subroutineName.data());
        bindBuffers();
        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &simLoc);

        updateRAM();
        grid->update();
        grid->updateColorVBO();
        glUseProgram(0);

        sim->ramUpToDate(true);
        sim->gpuUpToDate(true);
    }

    void updateGPU() override
    {
        const unsigned CELL_COUNT = grid->getCellCount();
        const unsigned NUM_MORPHS = static_cast<unsigned>(grid->numMorphs_);

        {	// update morphogens
            if (sim->domain->dirtyAttributes[sim->domain->CELL1_ATTRIB].indices.size() > 0)
            {
                LOG("Updating cell1 morphs");
                cells1.map();
                for (auto i : sim->domain->dirtyAttributes[sim->domain->CELL1_ATTRIB].indices)
                    for (unsigned j = 0; j < NUM_MORPHS; ++j)
                        cells1[i * NUM_MORPHS + j] = grid->cells1[i].vals[j];
                cells1.unmap();
                sim->domain->dirtyAttributes[sim->domain->CELL1_ATTRIB].clear();
            }

            if (sim->domain->dirtyAttributes[sim->domain->CELL2_ATTRIB].indices.size() > 0)
            {
                LOG("Updating cell2 morphs");
                cells2.map();
                for (auto i : sim->domain->dirtyAttributes[sim->domain->CELL2_ATTRIB].indices)
                    for (unsigned j = 0; j < NUM_MORPHS; ++j)
                        cells2[i * NUM_MORPHS + j] = grid->cells2[i].vals[j];
                cells2.unmap();
                sim->domain->dirtyAttributes[sim->domain->CELL2_ATTRIB].clear();
            }

            if (sim->domain->dirtyAttributes[sim->domain->BOUNDARY_ATTRIB].indices.size() > 0)
            {
                LOG("Updating boundary conditions");
                isConstVal.map();
                for (auto i : sim->domain->dirtyAttributes[sim->domain->BOUNDARY_ATTRIB].indices)
                    for (unsigned j = 0; j < NUM_MORPHS; ++j)
                        isConstVal[i * NUM_MORPHS + j] = grid->cells1[i].isConstVal[j];
                isConstVal.unmap();
                sim->domain->dirtyAttributes[sim->domain->BOUNDARY_ATTRIB].clear();
            }
        }
        {	// load params
            LOG("Updating parameters");
            const int PARAM_COUNT = static_cast<int>(sim->originalParams.size());
            params.map();

            // update all params 
            for (auto& p : sim->paramsMap)
            {
                for (auto& i : p.indices_)
                {
                    int j = 0;
                    for (auto& param : p.params_)
                    {
                        params[i * PARAM_COUNT + j] = param.second;
                        j++;
                    }
                }
            }

            params.unmap();
            sim->domain->dirtyAttributes[sim->domain->PARAM_ATTRB].clear();
        }
        {
            int k = 0;
            anisoCoefs.map();
            for (unsigned i = 0; i < CELL_COUNT; ++i)
            {
                for (unsigned j = 0; j < NUM_MORPHS; ++j)
                    anisoCoefs[k++] = grid->anisoCoefs_[j][i].dxx;
                for (unsigned j = 0; j < NUM_MORPHS; ++j)
                    anisoCoefs[k++] = grid->anisoCoefs_[j][i].dxy_yx;
                for (unsigned j = 0; j < NUM_MORPHS; ++j)
                    anisoCoefs[k++] = grid->anisoCoefs_[j][i].dyy;
            }
            anisoCoefs.unmap();
        }
    }

    void updateRAM() override
    {
        LOG("update ram");
        const unsigned CELL_COUNT = grid->getCellCount();
        const unsigned NUM_MORPHS = static_cast<unsigned>(grid->numMorphs_);

        { // update morphogens 
            // first array of cells
            {
                cells1.map();
                for (unsigned i = 0; i < CELL_COUNT; ++i)
                    for (unsigned j = 0; j < NUM_MORPHS; ++j)
                        grid->cells1[i].vals[j] = cells1[i * NUM_MORPHS + j];
                cells1.unmap();
            }

            // second array of cells
            {
                cells2.map();
                for (unsigned i = 0; i < CELL_COUNT; ++i)
                    for (unsigned j = 0; j < NUM_MORPHS; ++j)
                        grid->cells2[i].vals[j] = cells2[i * NUM_MORPHS + j];
                cells2.unmap();
            }
        }
    }

    void bindBuffers() override
    {
        glUseProgram(program);

        glActiveTexture(GL_TEXTURE0);
        glBindImageTexture(0, grid->morphTexture_.id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &simLoc);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, anisoCoefs.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cell1Base, cells1.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cell2Base, cells2.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, params.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 18, isConstVal.GLid());

        updateUniforms();
    }

    void calcSimulate() override
    {
        static const int LOCAL_X = 16;
        static const int LOCAL_Y = 16;

        int x = grid->getXRes() / LOCAL_X + 1;
        int y = grid->getYRes() / LOCAL_Y + 1;

        dispatch(x, y);
        wait();
    }

	void calcFluxes() override
	{
        // todo implement
	}

    void swapState() override
    {
        if (cell1Base == 9)
        {
            cell1Base = 10;
            cell2Base = 9;
        }
        else
        {
            cell1Base = 9;
            cell2Base = 10;
        }
    }

    void calcGrow(float, int) override
    {}

    void calcDomainAttribs() override
    {}

    void updateUniforms() override
    {
        glUniform1i(getUniformLoc("outMorphIndex"), sim->outMorphIndex);
        glUniform1i(getUniformLoc("stepCount"), (GLint)sim->stepCount);
        glUniform1i(getUniformLoc("xRes"), grid->getXRes());
        glUniform1i(getUniformLoc("yRes"), grid->getYRes());
        glUniform1f(getUniformLoc("areaScale"), grid->getArea(0));
    }

private:
    int cell1Base = 9, cell2Base = 10;
    Grid* grid = nullptr;

    GPUBuffer<GLfloat> cells1;
    GPUBuffer<GLfloat> cells2;
    GPUBuffer<GLfloat> params;
    GPUBuffer<GLfloat> anisoCoefs;
    GPUBuffer<GLint> isConstVal;
};