#pragma once
#include "ReactionDiffusionComputeShader.h"
#include "HalfEdgeMesh.h"

#include <array>


class MeshComputeShader
    : public ReactionDiffusionComputeShader
{
private:
    static const int LOCAL_WORK_GROUP_SIZE = 12;

    struct Vertex
    {
        GLint index;
        GLint edgeIndex;
        GLfloat dualArea;
    };

    struct HalfEdge
    {
        GLint nextIndex;
        GLint pairIndex;
        GLint ccw;
        GLint faceIndex;
        GLint origIndex;
        GLfloat angle;
    };

    struct Face
    {
        GLint edgeIndex;
        GLfloat area;
        GLfloat nx, ny, nz;
    };

    struct DebugStruct
    {
        GLfloat x;
        GLfloat y;
        GLfloat z;
    };

    GPUBuffer<Vertex> vertex;
    GPUBuffer<HalfEdge> halfEdge;
    GPUBuffer<Face> face;
    GPUBuffer<GLfloat> cells1;
    GPUBuffer<GLfloat> cells2;
#if DIFFUSION_EQNS==1
	GPUBuffer<GLfloat> eNormalRands;
#else
	GPUBuffer<GLfloat> vCotans;
#endif
    GPUBuffer<GLfloat> eCotans;
    //GPUBuffer<GLfloat> t0;
    //GPUBuffer<GLfloat> t1;
	GPUBuffer<GLfloat> t0_t1;
	//GPUBuffer<GLfloat> fluxes;
	GPUBuffer<GLfloat> diffVectors;
    GPUBuffer<GLfloat> tangents;
    GPUBuffer<GLfloat> params;
    GPUBuffer<GLint> isConstVal;

public:
    MeshComputeShader(Simulation* sim, const std::map<std::string, std::string>& includes, const std::string& shaderFilename) :
        ReactionDiffusionComputeShader(includes, shaderFilename)
    {
        this->sim = sim;
        this->subroutineName = "model";
        mesh = dynamic_cast<HalfEdgeMesh*>(sim->domain);
        ASSERT(mesh != nullptr, "Error: invalid sim. Must be on a half edge mesh");
    }

    ~MeshComputeShader()
    {}

    void updateUniforms() override
    {
        glUniform1i(getUniformLoc("prepatternInfo0.enabled"), sim->domain->prepatternInfo0.enabled ? 1 : 0);
        glUniform1i(getUniformLoc("prepatternInfo0.targetIndex"), sim->domain->prepatternInfo0.targetIndex);
        glUniform1i(getUniformLoc("prepatternInfo0.sourceIndex"), sim->domain->prepatternInfo0.sourceIndex);
        glUniform1f(getUniformLoc("prepatternInfo0.lowVal"), sim->domain->prepatternInfo0.lowVal);
        glUniform1f(getUniformLoc("prepatternInfo0.highVal"), sim->domain->prepatternInfo0.highVal);
        glUniform1f(getUniformLoc("prepatternInfo0.exponent"), sim->domain->prepatternInfo0.exponent);
        glUniform1i(getUniformLoc("prepatternInfo0.squared"), sim->domain->prepatternInfo0.squared ? 1 : 0);
        
        glUniform1i(getUniformLoc("prepatternInfo1.enabled"), sim->domain->prepatternInfo1.enabled ? 1 : 0);
        glUniform1i(getUniformLoc("prepatternInfo1.targetIndex"), sim->domain->prepatternInfo1.targetIndex);
        glUniform1i(getUniformLoc("prepatternInfo1.sourceIndex"), sim->domain->prepatternInfo1.sourceIndex);
        glUniform1f(getUniformLoc("prepatternInfo1.lowVal"), sim->domain->prepatternInfo1.lowVal);
        glUniform1f(getUniformLoc("prepatternInfo1.highVal"), sim->domain->prepatternInfo1.highVal);
        glUniform1f(getUniformLoc("prepatternInfo1.exponent"), sim->domain->prepatternInfo1.exponent);
        glUniform1i(getUniformLoc("prepatternInfo1.squared"), sim->domain->prepatternInfo1.squared ? 1 : 0);

        glUniform1i(getUniformLoc("vertCount"), static_cast<GLint>(sim->domain->cells1.size()));
        glUniform1i(getUniformLoc("outMorphIndex"), sim->outMorphIndex);
		glUniform1i(getUniformLoc("veinMorphIndex"), sim->domain->veinMorphIndex_);
		glUniform1f(getUniformLoc("veinDiffusionScale"), sim->domain->veinDiffusionScale_);
		glUniform1f(getUniformLoc("petalDiffusionScale"), sim->domain->petalDiffusionScale_);
        glUniform3f(getUniformLoc("growthRate"), sim->growth.x, sim->growth.y, sim->growth.z);
        glUniform1i(getUniformLoc("stepCount"), (GLint)sim->stepCount);
    }

    void bindBuffers() override
    {
        glUseProgram(program);

        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &simLoc);
		glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &calcFluxesLoc);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, sim->domain->norID_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sim->domain->posID_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, sim->domain->colID_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, vertex.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, halfEdge.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, face.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cell1Base, cells1.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, cell2Base, cells2.GLid());
#if DIFFUSION_EQNS==1
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, eNormalRands.GLid());
#else
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, vCotans.GLid());
#endif       
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, eCotans.GLid());
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, t0.GLid());
        //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, t1.GLid());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, t0_t1.GLid());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, diffVectors.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, tangents.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, params.GLid());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 18, isConstVal.GLid()); // TODO: 16 seems to be the maximum number of storage buffers...could some of them be combined?

        updateUniforms();
    }

    void swapState()
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

    void printIDs()
    {
        std::cout <<
            "vertexID\t[" << vertex.GLid() <<
            "]\nhalfEdgeID\t[" << halfEdge.GLid() <<
            "]\nfaceID\t\t[" << face.GLid() <<
            "]\ncells1ID\t[" << cells1.GLid() <<
            "]\ncells2ID\t[" << cells2.GLid() <<
            "]\ntangentsID\t[" << tangents.GLid() <<
            "]\nparamsID\t[" << params.GLid() << "]\n";
    }

    void createSSBOs(const GLint NUM_MORPHS) override
    {
        // Create SSBOs ========================================================================
        LOG("Loading data to GPU...");
        int cellsCount = static_cast<int>(mesh->vertices.size());
        int edgesCount = static_cast<int>(mesh->edges.size());
        int facesCount = static_cast<int>(mesh->faces.size());
        const int NUM_PARAMS = static_cast<int>(sim->originalParams.size());

        // Create the buffer objects
        GLint size = 0;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
        size /= 20; // arbitrary choice to not run out of memory

        // Populate buffers
        {	// Half-Edge mesh vertices
            std::vector<Vertex> vertices(size / sizeof(Vertex));
            for (int i = 0; i < cellsCount; ++i)
            {
                vertices[i].index = mesh->vertices[i]->index;
                vertices[i].edgeIndex = mesh->vertices[i]->edge()->index;
            }
            vertex.init(vertices);
        }
        {	// Half-Edge mesh edges
            std::vector<HalfEdge> edges(size / sizeof(HalfEdge));
            for (int i = 0; i < edgesCount; ++i)
            {
                edges[i].nextIndex = mesh->edges[i]->next()->index;
                edges[i].pairIndex = mesh->edges[i]->pair()->index;
                edges[i].ccw = mesh->edges[i]->pair()->next()->index;
                edges[i].faceIndex = mesh->edges[i]->face() == nullptr ? -1 : mesh->edges[i]->face()->index;
                edges[i].origIndex = mesh->edges[i]->origin()->index;
            }
            halfEdge.init(edges);
        }

        {	// Half-Edge mesh faces
            std::vector<Face> faces(size / sizeof(Face));
			for (int i = 0; i < facesCount; ++i) {
				faces[i].edgeIndex = mesh->faces[i]->edge()->index;
			}
            face.init(faces);
        }
        {	// load initial morphogens
            std::vector<GLfloat> cells(size / sizeof(GLfloat));

            // first array of cells		
            for (int i = 0; i < cellsCount; ++i)
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    cells[i * NUM_MORPHS + j] = mesh->cells1[i].vals[j];
            cells1.init(cells);

            // second array of cells
            for (int i = 0; i < cellsCount; ++i)
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    cells[i * NUM_MORPHS + j] = mesh->cells2[i].vals[j];
            cells2.init(cells);
        }

#if DIFFUSION_EQNS==1
		{ // init normal random variables
			std::vector<GLfloat> eNormalRandsVals(size / sizeof(GLfloat));
			eNormalRands.init(eNormalRandsVals);
	}
#else
        { // load vCotans
            std::vector<GLfloat> vCotansVals(size / sizeof(GLfloat));

            for (int i = 0; i < cellsCount; ++i)
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    vCotansVals[i * NUM_MORPHS + j] = mesh->vertices[i]->cotacotb[j];
			vCotans.init(vCotansVals);
		}
#endif
        { // load eCotans
            std::vector<GLfloat> eCotansVals(size / sizeof(GLfloat));
			for (int i = 0; i < edgesCount; ++i)
				for (GLint j = 0; j < NUM_MORPHS; ++j)
#if DIFFUSION_EQNS==0
					eCotansVals[i * NUM_MORPHS + j] = mesh->edges[i]->cotacotb[j];
#else
					eCotansVals[i * NUM_MORPHS + j] = mesh->edges[i]->cotan; // TODO: we do not need cotangent for each morphogen
#endif
            eCotans.init(eCotansVals);
        }
		/*
        { // load tangents
            // t0
            std::vector<GLfloat> t(size / sizeof(GLfloat));
            for (GLint j = 0; j < NUM_MORPHS; ++j)
                for (int i = 0; i < mesh->diffusionTensors[j].t0_.size(); ++i)
                    t[i * NUM_MORPHS + j] = mesh->diffusionTensors[j].t0_[i];
            t0.init(t);

            // t1
            for (GLint j = 0; j < NUM_MORPHS; ++j)
                for (int i = 0; i < mesh->diffusionTensors[j].t1_.size(); ++i)
                    t[i * NUM_MORPHS + j] = mesh->diffusionTensors[j].t1_[i];
            t1.init(t);
        }
		*/
		{ // load t0 and t1
			std::vector<GLfloat> ts(size / sizeof(GLfloat));
			int offset = 0;
			for (int i = 0; i < facesCount; ++i) {
				for (GLint j = 0; j < NUM_MORPHS; ++j) {
					ts[offset + 0] = mesh->diffusionTensors[j].t0_[i];
					ts[offset + 1] = mesh->diffusionTensors[j].t1_[i];
					ts[offset + 2] = 0.;
					offset += 3;
				}
			}
			t0_t1.init(ts);
		}
		{ // load diffusion vectors
			std::vector<GLfloat> diffvectors(size / sizeof(GLfloat));
			// QQQ TODO could load diff vectors from CPU computation...
			diffVectors.init(diffvectors);
		}
        {	// load tangents
            std::vector<GLfloat> tangentsVal(size / sizeof(GLfloat));
            int offset = 0;
            for (int i = 0; i < facesCount; ++i)
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                {
                    Vec3& t = mesh->tangents[j][i];
                    tangentsVal[offset++] = t.x;
                    tangentsVal[offset++] = t.y;
                    tangentsVal[offset++] = t.z;
                }
            tangents.init(tangentsVal);
        }
        {	// load params
            std::vector<GLfloat> paramsVals(size / sizeof(GLfloat));
            for (auto paramsMapEntry : sim->paramsMap)
            {
                for (int i : paramsMapEntry.indices_)
                {
                    int j = 0;
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
            std::vector<GLint> isConstValVals(size / sizeof(GLint), 0);
            for (int i = 0; i < cellsCount; ++i)
                for (GLint j = 0; j < NUM_MORPHS; ++j)
                    isConstValVals[i * NUM_MORPHS + j] = mesh->cells1[i].isConstVal[j] ? 1 : 0;
            isConstVal.init(isConstValVals);
        }

        // Initialize SSBOs ========================================================================
        glUseProgram(program);

        simLoc = glGetSubroutineIndex(program, GL_COMPUTE_SHADER, subroutineName.data());
		calcFluxesLoc = glGetSubroutineIndex(program, GL_COMPUTE_SHADER, "calculateFlux_pwp");
        calcVertAnglesNormalsAreasLoc = glGetSubroutineIndex(program, GL_COMPUTE_SHADER, "calculateVertAnglesNormalsAreas");
        calcFaceNormalsAreasLoc = glGetSubroutineIndex(program, GL_COMPUTE_SHADER, "calculateFaceNormalsAreas");
        calcCotangentsLoc = glGetSubroutineIndex(program, GL_COMPUTE_SHADER, "calculateCotangents");
		calcCotangents_pwpLoc = glGetSubroutineIndex(program, GL_COMPUTE_SHADER, "calculateCotangents_pwp");
		calcDiffVectorsLoc = glGetSubroutineIndex(program, GL_COMPUTE_SHADER, "calculateDiffVectors");
        calcGrowLoc = glGetSubroutineIndex(program, GL_COMPUTE_SHADER, "grow");

        bindBuffers();
        calcFaceNormalsAreas();
        calcVertAnglesNormalsAreas();
#if DIFFUSION_EQNS==0
		calcCotangents();
#else
		calcCotangents_pwp();
		calcDiffVectors();
#endif

        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &simLoc);

        updateRAM();
        mesh->update();
        mesh->updateColorVBO();
        glUseProgram(0);

        sim->ramUpToDate(true);
        sim->gpuUpToDate(true);
    }

    void updateGPU() override
    {
        // Update VBOs
        mesh->updateNormalVBO();
        mesh->updatePositionVBO();
        mesh->updateTextureVBO();

        const int NUM_MORPHS = mesh->numMorphs_;

        if (sim->domain->dirtyAttributes[sim->domain->VERTEX_ATTRB].indices.size() > 0)
        {
            LOG("Updating vertex index and dual");
            vertex.map();
            for (auto i : sim->domain->dirtyAttributes[sim->domain->VERTEX_ATTRB].indices)
            {
                vertex[i].index = mesh->vertices[i]->index;
                vertex[i].edgeIndex = mesh->vertices[i]->edge()->index;
            }
            vertex.unmap();
            sim->domain->dirtyAttributes[sim->domain->VERTEX_ATTRB].clear();
        }

        if (sim->domain->dirtyAttributes[sim->domain->EDGE_ATTRB].indices.size() > 0)
        {
            LOG("Updating edge connections");
            halfEdge.map();
            for (auto i : sim->domain->dirtyAttributes[sim->domain->EDGE_ATTRB].indices)
            {
                halfEdge[i].nextIndex = mesh->edges[i]->next()->index;
                halfEdge[i].pairIndex = mesh->edges[i]->pair()->index;
                halfEdge[i].ccw = mesh->edges[i]->pair()->next()->index;
                halfEdge[i].faceIndex = mesh->edges[i]->face() == nullptr ? -1 : mesh->edges[i]->face()->index;
                halfEdge[i].origIndex = mesh->edges[i]->origin()->index;
            }
            halfEdge.unmap();
        }

        if (sim->domain->dirtyAttributes[sim->domain->FACE_ATTRB].indices.size() > 0)
        {
            LOG("Updating face index, area, normals");
            face.map();
            for (auto i : sim->domain->dirtyAttributes[sim->domain->FACE_ATTRB].indices)
                face[i].edgeIndex = mesh->faces[i]->edge()->index;
            face.unmap();
            sim->domain->dirtyAttributes[sim->domain->FACE_ATTRB].clear();
        }

        if (sim->domain->dirtyAttributes[sim->domain->CELL1_ATTRIB].indices.size() > 0)
        {
            LOG("Updating cell1 morphs");
            cells1.map();
            for (auto i : sim->domain->dirtyAttributes[sim->domain->CELL1_ATTRIB].indices)
                for (int j = 0; j < NUM_MORPHS; ++j)
                    cells1[i * NUM_MORPHS + j] = mesh->cells1[i].vals[j];
            cells1.unmap();
            sim->domain->dirtyAttributes[sim->domain->CELL1_ATTRIB].clear();
        }

        if (sim->domain->dirtyAttributes[sim->domain->BOUNDARY_ATTRIB].indices.size() > 0)
        {
            LOG("Updating boundary conditions");
            isConstVal.map();
            for (auto i : sim->domain->dirtyAttributes[sim->domain->BOUNDARY_ATTRIB].indices)
                for (int j = 0; j < NUM_MORPHS; ++j)
                    isConstVal[i * NUM_MORPHS + j] = mesh->cells1[i].isConstVal[j];
            isConstVal.unmap();
            sim->domain->dirtyAttributes[sim->domain->BOUNDARY_ATTRIB].clear();
        }

        if (sim->domain->dirtyAttributes[sim->domain->CELL2_ATTRIB].indices.size() > 0)
        {
            LOG("Updating cell2 morphs");
            cells2.map();
            for (auto i : sim->domain->dirtyAttributes[sim->domain->CELL2_ATTRIB].indices)
                for (int j = 0; j < NUM_MORPHS; ++j)
                    cells2[i * NUM_MORPHS + j] = mesh->cells2[i].vals[j];
            cells2.unmap();
            sim->domain->dirtyAttributes[sim->domain->CELL2_ATTRIB].clear();
        }

        if (mesh->dirtyAttributes[sim->domain->TANGENT_ATTRIB].indices.size() > 0)
        {
            // TODO: optimize this
            LOG("Updating tangent directions");
            tangents.map();
            int offset = 0;
            for (int i = 0; i < mesh->tangents[0].size(); ++i)
                for (GLint j = 0; j < mesh->tangents.size(); ++j)
                {
                    Vec3& t = mesh->tangents[j][i];
                    tangents[offset] = t.x;
                    tangents[offset + 1] = t.y;
                    tangents[offset + 2] = t.z;
                    offset += 3;
                }
            tangents.unmap();

            // update diffusion tensors
			LOG("Updating t0 and t1");
			t0_t1.map();
			offset = 0;
			for (int i = 0; i < mesh->faces.size(); ++i) {
				for (GLint j = 0; j < NUM_MORPHS; ++j) {
					t0_t1[offset + 0] = mesh->diffusionTensors[j].t0_[i];
					t0_t1[offset + 1] = mesh->diffusionTensors[j].t1_[i];
					t0_t1[offset + 2] = 0.;
					offset += 3;
				}
			}
			t0_t1.unmap();

            sim->domain->dirtyAttributes[sim->domain->TANGENT_ATTRIB].clear();
        }

        if (sim->domain->dirtyAttributes[sim->domain->PARAM_ATTRB].indices.size() > 0)
        {
            LOG("Updating parameters");
            const int PARAM_COUNT = static_cast<int>(sim->originalParams.size());
            params.map();

            // update only dirty vertices
            //int count = 0;
            //for (auto i : sim->domain->dirtyMeshAttributes[sim->domain->PARAM_ATTRB].indices)
            //{
            //    for (int j = 0; j < PARAM_COUNT; ++j)
            //        params[i * PARAM_COUNT + j] = sim->domain->dirtyMeshAttributes[sim->domain->PARAM_ATTRB].attributes[count * PARAM_COUNT + j];
            //    count++;
            //}

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

        bindBuffers();
        calcFaceNormalsAreas();
        calcVertAnglesNormalsAreas();
#if DIFFUSION_EQNS==0
		calcCotangents();
#else
		calcCotangents_pwp();
		calcDiffVectors();
#endif
    }

    void updateRAM() override
    {
        const int NUM_MORPHS = mesh->numMorphs_;
        int cellsCount = static_cast<int>(mesh->vertices.size());
        int facesCount = static_cast<int>(mesh->faces.size());
		
		{ // Half-Edge mesh vertices
			// update dual areas
			vertex.map();
			for (int i = 0; i < cellsCount; ++i)
				mesh->vertices[i]->area = vertex[i].dualArea;
			vertex.unmap();
			/* Replaced separate t0 and t1
						// update t0 / t1
						t0.map();
						for (int j = 0; j < NUM_MORPHS; ++j)
							for (unsigned i = 0; i < mesh->diffusionTensors[j].t0_.size(); ++i)
								mesh->diffusionTensors[j].t0_[i] = t0[i * NUM_MORPHS + j];
						t0.unmap();

						t1.map();
						for (int j = 0; j < NUM_MORPHS; ++j)
							for (unsigned i = 0; i < mesh->diffusionTensors[j].t1_.size(); ++i)
								mesh->diffusionTensors[j].t1_[i] = t1[i * NUM_MORPHS + j];
						t1.unmap();
			*/
			// update t0 / t1
			t0_t1.map();
			for (int i = 0; i < facesCount; ++i)
				for (int j = 0; j < NUM_MORPHS; ++j) {
					mesh->diffusionTensors[j].t0_[i] = t0_t1[i * NUM_MORPHS * 3 + j * 3 + 0];
					mesh->diffusionTensors[j].t1_[i] = t0_t1[i * NUM_MORPHS * 3 + j * 3 + 1];
				}
			t0_t1.unmap();
		}

        { // Half-Edge mesh faces
            face.map();
            for (int i = 0; i < facesCount; ++i)
            {
                mesh->faces[i]->area = face[i].area;
                mesh->faces[i]->normal.x = face[i].nx;
                mesh->faces[i]->normal.y = face[i].ny;
                mesh->faces[i]->normal.z = face[i].nz;
            }
            face.unmap();
        }
        { // update morphogens 
            // first array of cells
            {
                cells1.map();
				for (int i = 0; i < cellsCount; ++i) {
					for (int j = 0; j < NUM_MORPHS; ++j) {
						mesh->cells1[i].vals[j] = cells1[i * NUM_MORPHS + j];
					}
				}
                cells1.unmap();
            }

            // second array of cells
            {
                cells2.map();
                for (int i = 0; i < cellsCount; ++i)
					for (int j = 0; j < NUM_MORPHS; ++j) {
						mesh->cells2[i].vals[j] = cells2[i * NUM_MORPHS + j];
					}
                cells2.unmap();
            }
        }

        mesh->update();
    }

    void calcSimulate() override
    {
        GLint vertSize = static_cast<GLint>(sim->domain->cells1.size());
        glUniform1i(getUniformLoc("vertCount"), vertSize);
        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &simLoc);
        dispatch(1 + vertSize / LOCAL_WORK_GROUP_SIZE);
        wait();
    }

	void calcFluxes() override
	{
		GLint vertSize = static_cast<GLint>(sim->domain->cells1.size());
		glUniform1i(getUniformLoc("vertCount"), vertSize);
		glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &calcFluxesLoc);
		dispatch(1 + vertSize / LOCAL_WORK_GROUP_SIZE);
		wait();
	}

    void calcDomainAttribs() override
    {
        calcFaceNormalsAreas();
        calcVertAnglesNormalsAreas();
#if DIFFUSION_EQNS==0
		calcCotangents();
#else
		calcCotangents_pwp();
		calcDiffVectors();
#endif
    }

    void calcGrow(float totalArea, int growthMode) override
    {
        GLint vertSize = static_cast<GLint>(sim->domain->cells1.size());
        glUniform1i(getUniformLoc("vertCount"), vertSize);
        glUniform1i(getUniformLoc("growthMode"), growthMode);

        if (growthMode == 1)
            glUniform1f(getUniformLoc("linearGrowthFactor"), sqrt((totalArea + sim->growth.x) / totalArea));

        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &calcGrowLoc);
        dispatch(1 + vertSize / LOCAL_WORK_GROUP_SIZE);
        wait();
    }

    void calcFaceNormalsAreas()
    {
        GLint faceSize = static_cast<GLint>(mesh->faces.size());
        glUniform1i(getUniformLoc("vertCount"), faceSize);
        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &calcFaceNormalsAreasLoc);
        dispatch(1 + faceSize / LOCAL_WORK_GROUP_SIZE);
        wait();
    }

    void calcVertAnglesNormalsAreas()
    {
        GLint vertSize = static_cast<GLint>(sim->domain->cells1.size());
        glUniform1i(getUniformLoc("vertCount"), vertSize);
        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &calcVertAnglesNormalsAreasLoc);
        dispatch(1 + vertSize / LOCAL_WORK_GROUP_SIZE);
        wait();
    }

    void calcCotangents()
    {
        GLint vertSize = static_cast<GLint>(sim->domain->cells1.size());
        glUniform1i(getUniformLoc("vertCount"), vertSize);
        glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &calcCotangentsLoc);
        dispatch(1 + vertSize / LOCAL_WORK_GROUP_SIZE);
        wait();
    }

	void calcCotangents_pwp()
	{
		GLint vertSize = static_cast<GLint>(sim->domain->cells1.size());
		glUniform1i(getUniformLoc("vertCount"), vertSize);
		glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &calcCotangents_pwpLoc);
		dispatch(1 + vertSize / LOCAL_WORK_GROUP_SIZE);
		wait();
	}

	void calcDiffVectors()
	{
		GLint edgeSize = static_cast<GLint>(mesh->edges.size());
		glUniform1i(getUniformLoc("vertCount"), edgeSize);
		glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 1, &calcDiffVectorsLoc);
		dispatch(1 + edgeSize / LOCAL_WORK_GROUP_SIZE);
		wait();
	}

    virtual void precompCotans() override
    {
#if DIFFUSION_EQNS==0
        calcCotangents();
#else
		calcCotangents_pwp();
		calcDiffVectors();
#endif
    };

protected:
    HalfEdgeMesh* mesh;

private:
    GLuint calcVertAnglesNormalsAreasLoc, calcFaceNormalsAreasLoc, calcCotangentsLoc;
	GLuint calcCotangents_pwpLoc, calcDiffVectorsLoc;
    GLuint debugID;

    int cell1Base = 9, cell2Base = 10;
};
