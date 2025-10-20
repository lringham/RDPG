#include "HalfEdgeMesh.h"
#include "DefaultShader.h"
#include "Utils.h"
#include "Mat3.h"
#include "Quaternion.h"
#include "Triangle.h"

#include <iostream>
#include <fstream>
#include <limits>
#include <algorithm>
#include <stack>
#include <unordered_set>
#include <cmath>


HalfEdgeMesh::HalfEdgeMesh()
{
    domainType = SimulationDomain::DomainType::MESH;
    LOG("Creating HalfEdgeMesh");
}

HalfEdgeMesh::HalfEdgeMesh(const Drawable& drawable)
{
    domainType = SimulationDomain::DomainType::MESH;
    init(drawable);
}

void HalfEdgeMesh::init(const Drawable& drawable)
{
    const size_t numVertices = drawable.positions_.size();
    if (numVertices == 0)
    {
        std::cout << "Cannot create HalfEdgeMesh from Drawable. Drawable has no vertices!" << std::endl;
        return;
    }
    if (drawable.indices_.size() % 3 != 0)
    {
        std::cout << "Number of indices_ is not a multiple of 3. Only Triangle meshes supported." << std::endl;
        return;
    }

    //============== reset mesh state =================
    edges.clear();
    faces.clear();
    vertices.clear();
    initialized_ = false;

    prevP_.resize(1, Vec3(0.f, 0.f, 0.f));
    cells1.resize(numVertices);
    cells2.resize(numVertices);
    positions_.resize(numVertices, Vec3(0, 0, 0));
    normals_.resize(numVertices, Vec3(0, 1, 0));
    textureCoords_.resize(numVertices, Vec2(0, 0));
    colors_.resize(numVertices, Vec4(1.f, 1.f, 1.f));
    //=================================================

    textureCoords_ = drawable.textureCoords_;
    normals_ = drawable.normals_;
    if (drawable.colors_.size() == numVertices) // Only copy if there are enough colors_
        colors_ = drawable.colors_;

    //Create edges, faces, vertices
    for (size_t i = 0; i < drawable.indices_.size() - 2; i += 3)
    {
        unsigned i0 = drawable.indices_[i];
        unsigned i1 = drawable.indices_[i + 1];
        unsigned i2 = drawable.indices_[i + 2];

        // Get Vertices
        Vertex* v0 = vertexExists(i0) ? vertices[i0] : createVertex(drawable.positions_[i0], i0);
        Vertex* v1 = vertexExists(i1) ? vertices[i1] : createVertex(drawable.positions_[i1], i1);
        Vertex* v2 = vertexExists(i2) ? vertices[i2] : createVertex(drawable.positions_[i2], i2);

        // Get Edges
        Edge* e01 = edgeExists(v0, v1) ? getEdge(v0, v1) : createEdge(v0, v1);
        Edge* e12 = edgeExists(v1, v2) ? getEdge(v1, v2) : createEdge(v1, v2);
        Edge* e20 = edgeExists(v2, v0) ? getEdge(v2, v0) : createEdge(v2, v0);

        // Create Face
        createFace(e01, e12, e20);
    }
    createBoundaryEdges();
    calculateBoundaryVertices();
    calculateDualAreas();
    calculateFaceNormals();
    calculateVertexNormals();

    // Init gradient lines graphics
    for (size_t i = 0; i < faces.size(); ++i)
        gradientLines.addLine(Vec3(0, 0, 0), Vec3(1,0,0), Vec3(0.2f, 0.2f, 0.2f), Vec3(1, 0, 0));

    // Create openGL representation
    initVBOs();

    // Print mesh info
    printInfo();

    ASSERT(!validateMesh(), "Mesh is invalid");
}

bool HalfEdgeMesh::isInitialized()
{
    return initialized_;
}

void HalfEdgeMesh::setBoundaryColor(float r, float g, float b)
{
    for (Vertex* v : vertices)
        if (v->isBoundary)
            colors_[v->index].set(r, g, b);

    updateColorVBO();
}

void HalfEdgeMesh::doInit(int numMorphs)
{
    //
    for (int i = 0; i < ATTRIB_COUNT; ++i)
        dirtyAttributes[(MeshAttributeNames)i].indices = std::set<unsigned>();

    cellsToUpdate.clear();
    dirtyAttributes[EDGE_ATTRB].indices.clear();
    dirtyAttributes[FACE_ATTRB].indices.clear();

    //
    if (shader_ == nullptr)
        shader_ = std::make_unique<DefaultShader>();

    numMorphs_ = numMorphs;

    paintingMorph.resize(numMorphs_, false);
    paintingDiffDir.resize(numMorphs_, false);
    showingMorph.resize(numMorphs_, false);
    showingDiffDir.resize(numMorphs_, false);
    showingGrad.resize(numMorphs_, false);
    clearMValue.resize(numMorphs_, 0.1f);
    paintValue.resize(numMorphs_, 0.f);
    L.resize(numMorphs_ * vertices.size(), 0.f);

    paintValue[0] = .1f;
    showingMorph[0] = true;

    tangents.clear();
    tangents.resize(numMorphs_);
    for (int i = 0; i < numMorphs_; ++i)
        tangents[i].resize(faces.size(), Vec3(1.f, 0.f, 0.f));

    for (int j = 0; j < faces.size(); ++j)
        for (int i = 0; i < numMorphs_; ++i)
            tangents[i][j] = getFaceCentroid(faces[j]);

    diffusionTensors.clear();
    diffusionTensors.resize(numMorphs);
    for (int i = 0; i < numMorphs; ++i)
        diffusionTensors[i].resize(faces.size(), 1.f);

    // Update tensor visualization
    anisotropicDiffusionTensor.clearBuffers();
    anisotropicDiffusionTensor.destroyVBOs();
    for (unsigned i = 0; i < faces.size(); ++i)
    {
        anisotropicDiffusionTensor.addLine(
            Vec3(0, 0, 0), Vec3(0, 0, 0),
            Vec3(0, 0, 0), Vec3(0, 1, 0));
    }
    anisotropicDiffusionTensor.finalize();

    cells1.clear();
    cells1.resize(positions_.size());
    for (auto& c : cells1)
    {
        c.vals.resize(numMorphs_);
        c.isConstVal.resize(numMorphs_, false);
    }

    cells2.clear();
    cells2.resize(positions_.size());
    for (auto& c : cells2)
    {
        c.vals.resize(numMorphs_);
        c.isConstVal.resize(numMorphs_, false);
    }

    for (auto& e : edges)
    {
        e->cotacotb.clear();
        e->cotacotb.resize(numMorphs_);
		e->diffVec.clear();
		e->diffVec.resize(numMorphs_);	
		e->nranVec.clear();
		e->nranVec.resize(numMorphs_);
    }

    for (auto& v : vertices)
    {
        v->cotacotb.clear();
        v->cotacotb.resize(numMorphs_);
    }

    for (auto& f : faces) 
    {
        f->clearDirty();
        f->veclambda1.clear();
        f->veclambda1.resize(numMorphs_);
        f->veclambda2.clear();
        f->veclambda2.resize(numMorphs_);
    }

    projectAniVecs();
    updateDiffusionCoefs();

    bvh_.build(*this);

    initialized_ = true;
}

void HalfEdgeMesh::doRecalculateParameters()
{
    for (auto& f : faces)
        f->clearDirty();

    updateDiffusionCoefs();
}

//Rivara and Inostroza adaptive subdivision scheme
// Rivara  M.  and  Inostroza  P.  
// Using  Longest-side  Bisection  Techniques  for  the Automatic Refinement of Delaunay Triangulations.
// The 4th International Meshing Roundtable, Sandia National Laboratories, pp.335-346, October 1995
void HalfEdgeMesh::subdivideFace(Face* f)
{
    // Find the longest edge, e in face f
    // Split f at e and the opposite vertex
    // Find neighbouring face, f2, along e
    // If f2's longest face is not e, subdivide f2 and check again
    // else subdivide f2 along e and return	

    Edge* e = getLongestEdge(f);
    Edge* ePair = e->pair();
    bool isBoundaryEdge = e->isBoundary;

    splitFaceAt(f, e);

    Face* f2 = nullptr;
    bool subdividing = !isBoundaryEdge;
    while (subdividing)
    {
        f2 = ePair->face();
        if (f2 != nullptr)
        {
            if (getLongestEdge(f2) != ePair)
                subdivideFace(f2);
            else
            {
                splitFaceAt(f2, ePair);
                subdividing = false;
            }
        }
        else
            subdividing = false;
    }
}

void HalfEdgeMesh::precomputeGradCoefs()
{
    for (Face* f : faces)
        precomputeGradCoef(f);
}

std::vector<float> HalfEdgeMesh::gaussianCurvatures() const
{
    int i = 0;
    std::vector<float> curvatures(vertices.size());
    for (auto v : vertices)
    {
        float curvature = 0;
        Edge* start = v->edge();
        Edge* current = start;
        do
        {
            if (current->face() != nullptr)
                curvature += current->angle;
            current = current->pair()->next();
        } while (start != current);

        // TODO: handle boundary edges
        // Possible solution... if v is on a boundary
        // find the total angle projected onto a plane 
        // and use that instead of 2PI
        // or use PI like stack overflow suggests

        curvature = (Math::PI2 - curvature) / v->area;
        curvatures[i++] = curvature;
    }
    return curvatures;
}

void HalfEdgeMesh::precomputeGradCoef(Face* f)
{
    int i = f->edge()->origin()->index;
    int j = f->edge()->destination()->index;
    int k = f->edge()->next()->destination()->index;

    float area = (1.f / (f->area * 2.f));
    gradCoefs[f] = std::pair<Vec3, Vec3>(
        area * rotate((positions_[i] - positions_[k]), Math::degToRad(90.f), f->normal),		// e_ki 
        area * rotate((positions_[j] - positions_[i]), Math::degToRad(90.f), f->normal));		// e_ij
}

void HalfEdgeMesh::growAndSubdivide(Vec3& growth, float maxFaceArea, bool subdivisionEnabled, size_t stepCount)
{
    // Grow
    if (growthMode == SimulationDomain::GrowthMode::AnimationGrowth)
        animation_.updatePositionsFromUVs(positions_, stepCount);
    else if (growthMode == SimulationDomain::GrowthMode::LinearAreaGrowth)
    {
        float area = getTotalArea();
        float targetArea = area + growth.x;
        float factor = sqrt(targetArea / area);

        for (Vec3& p : positions_)
            p = p * factor;
    }
    else if (growthMode == SimulationDomain::GrowthMode::PercentageGrowth)
    {
        for (Vec3& p : positions_)
            p = p + p * growth;
    }
    else if (growthMode == SimulationDomain::GrowthMode::MorphogenGrowth)
    {
        size_t i = 0;
        for (Vec3& p : positions_)
        {        
            const auto& normal = normals_[i];
            float growthPos = getReadFromCells()[i][0] * growth.x;
            float growthNeg = getReadFromCells()[i][1] * growth.y;

            if (std::isfinite(growthPos) && std::isfinite(growthNeg))
                p = p + normal * (growthPos - growthNeg);

            i++;
        }
    }

    // Find faces that are too big
    const unsigned FACE_COUNT = static_cast<unsigned>(faces.size());
    bool subdivdeHappened = false;
    if (subdivisionEnabled)
    {
        std::vector<unsigned> fatFaces;
        for (unsigned i = 0; i < FACE_COUNT; ++i)
            if (faces[i]->area > maxFaceArea && std::isfinite(faces[i]->area))
                fatFaces.push_back(i);

        // Subdivide faces
        subdivdeHappened = fatFaces.size() > 0;
        for (unsigned i : fatFaces)
            // Need to check if we still need to subdivide face 
            // because the recursive subdiv algorithm may have already
            // subdivided some previously fat faces
            if (faces[i]->area >= maxFaceArea) 
                subdivideFace(faces[i]);
    }

    if (subdivdeHappened)
    {           
        destroyVBOs();
        initVBOs();
        bvh_.build(*this);
    }
    else
    {
        updatePositionVBO();
        updateTextureVBO();
        updateNormalVBO();        
    }

    // Update areas and contangents
    if (!isGPUEnabled)
    {
        calculateAngles();
        calculateNormals();
        projectAniVecs();
        calculateDualAreas();
        updateDiffusionCoefs();
    }

    // Update tangents
    ASSERT(anisotropicDiffusionTensor.numLines() == faces.size(), "tensor vec num != num faces");
    ASSERT(gradientLines.numLines() == faces.size(), "gradient num != num faces");


    if (subdivdeHappened)
    {
        anisotropicDiffusionTensor.destroyVBOs();
        anisotropicDiffusionTensor.initVBOs();
    }

    // Update gradient
    gradientLines.destroyVBOs();
    gradientLines.initVBOs();

    // Update edge data
    if (isGPUEnabled && subdivdeHappened)
    {
        for (unsigned i = 0; i < FACE_COUNT; ++i)
        {
            Face* f = faces[i];
            if (f->dirty())
            {
                dirtyAttributes[FACE_ATTRB].indices.insert(i);
                f->clearDirty();
            }
        }

        for (unsigned i = 0; i < edges.size(); ++i)
        {
            Edge* e = edges[i];
            if (e->dirty())
            {
                dirtyAttributes[EDGE_ATTRB].indices.insert(i);
                e->clearDirty();
            }
        }
    }
}

HalfEdgeMesh::Vertex* HalfEdgeMesh::createVertex(const Vec3& pos, const unsigned index)
{
    positions_[index] = pos;

    //Resize vertices to fit
    if (index >= vertices.size())
        vertices.resize(index + 1);

    ASSERT(!vertexExists(index), "Vertex cannot be created if it already exists");

    vertices[index] = new Vertex(index);
    return vertices[index];
}

HalfEdgeMesh::Vertex* HalfEdgeMesh::createVertex(const Vec3& pos)
{    
    positions_.emplace_back(pos);
    normals_.emplace_back(1.f, 0.f, 0.f);
    colors_.emplace_back(1.f, 1.f, 1.f, 0.f);

    cells1.emplace_back(numMorphs_);
    cells2.emplace_back(numMorphs_);

    if (textureCoords_.size() > 0)
        textureCoords_.emplace_back(0.f, 0.f);

    for (int i = 0; i < numMorphs_; ++i)
        L.emplace_back(0.f);

    unsigned i = static_cast<unsigned>(positions_.size()-1);

    //Resize vertices to fit
    if (i >= vertices.size())
        vertices.resize(i + 1);

    ASSERT(!vertexExists(i), "Vertex cannot be created if it already exists");

    vertices[i] = new Vertex(i);
    vertices[i]->cotacotb.resize(numMorphs_);

    return vertices[i];
}

HalfEdgeMesh::Edge* HalfEdgeMesh::createEdge(Vertex* v0, Vertex* v1)
{
    ASSERT(!edgeExists(v0->index, v1->index), "Edge already exists");

    Edge* e01 = new Edge();
    e01->index = static_cast<int>(edges.size());
    e01->key = edgeKey(v0->index, v1->index);
    edgeKeyIndexMap[e01->key] = e01->index;

    e01->setOrigin(v0);
    e01->setDestination(v1);

    e01->diffVec.resize(numMorphs_);
    e01->cotacotb.resize(numMorphs_);
    e01->nranVec.resize(numMorphs_);

    v0->setEdge(e01);
    edges.push_back(e01);

    if (edgeExists(v1, v0))
        associateNeighbours(v0->index, v1->index);

    return e01;
}

HalfEdgeMesh::Face* HalfEdgeMesh::createFace(Edge* e01, Edge* e12, Edge* e20)
{
    unsigned faceIndex = static_cast<unsigned>(faces.size());
    Face* f = new Face();
    f->index = faceIndex;
    faces.push_back(f);

    f->firstIndexIndex = static_cast<unsigned>(indices_.size());
    indices_.emplace_back(e01->origin()->index);
    indices_.emplace_back(e12->origin()->index);
    indices_.emplace_back(e20->origin()->index);

    join(e01, e12, e20, f);

    return f;
}

void HalfEdgeMesh::createBoundaryEdges()
{
    // Find all edges with no pair and create one
    std::vector<Edge*> boundaryEdges;

    size_t edgeCount = edges.size();
    for (size_t i = 0; i < edgeCount; ++i)
    {
        auto& e = edges[i];
        if (e->pair() == nullptr)
        {
            Edge* ePair = createEdge(e->destination(), e->origin());
            ePair->isBoundary = true;
            ePair->pair()->isBoundary = true;
            boundaryEdges.emplace_back(ePair);
        }
    }

    // join boundary edges
    for (Edge* e : boundaryEdges)
    {
        // Find all edges coming from e's destination
        std::vector<Edge*> comingFrom;
        for (Edge* n : boundaryEdges)
            if (e->destination() == n->origin())
                comingFrom.emplace_back(n);

        if (comingFrom.size() == 1)
        {
            e->setNext(comingFrom[0]);
            continue;
        }
        else
        {
            LOG("Non-manifold meshes not supported");
            ASSERT(false, "Non-manifold meshes not supported");
        }
    }
}

//    _____        ___ ___ 
//   0     2      0   1   2 
HalfEdgeMesh::Edge* HalfEdgeMesh::splitEdge(Edge* e02)
{
    unsigned i0 = e02->origin()->index;		// Get original start index
    unsigned i1 = 0;
    unsigned i2 = e02->destination()->index; // Get next index

    Vertex* v0 = e02->origin();
    Vertex* v1 = nullptr;
    Vertex* v2 = e02->destination();
    Edge* e20 = e02->pair();
    Edge* e01 = nullptr;
    Edge* e12 = nullptr;
    dirtyAttributes[VERTEX_ATTRB].indices.insert(v0->index);
    dirtyAttributes[VERTEX_ATTRB].indices.insert(v2->index);

    // If the edge we are splitting is on the boundary, we also need to split it's pair 
    if (e02->isBoundary)
    {
        // Point boundary edges to the right spot
        Edge* start = e20;
        Edge* current = e20;
        do
        {
            current = current->next();
        } while (current->next() != start);

        // Perform splitting
        v1 = createVertex(lerp(.5f, positions_[i0], positions_[i2]));
        i1 = v1->index;
        dirtyAttributes[VERTEX_ATTRB].indices.insert(i1);

        for (int i = 0; i < numMorphs_; ++i)
        {
            cells1[i1][i] = (cells1[i0][i] + cells1[i2][i]) * 0.5f;
            cells2[i1][i] = cells1[i1][i];
        }

        e01 = e02;
        Edge* e10 = e20;
        e12 = createEdge(v1, e02->destination());
        Edge* e21 = createEdge(e20->origin(), v1);

        edgeKeyIndexMap.erase(e01->key);
        e01->key = edgeKey(v0->index, v1->index);
        edgeKeyIndexMap[e01->key] = e01->index;
        e01->setOrigin(v0);
        e01->setDestination(v1);
        associateNeighbours(e01);

        edgeKeyIndexMap.erase(e10->key);
        e10->key = edgeKey(v1->index, v0->index);
        edgeKeyIndexMap[e10->key] = e10->index;
        e10->setOrigin(v1);
        e10->setDestination(v0);
        associateNeighbours(e10);

        e01->setNext(e12);
        e12->setNext(e02->next());
        e21->setNext(e10);
        e12->setFace(e02->face());
        e21->setFace(e20->face());

        e01->isBoundary = true;
        e12->isBoundary = true;
        e21->isBoundary = true;
        e10->isBoundary = true;

        v1->isBoundary = true;

        current->setNext(e21);
        return e01;
    }
    else
    {
        Edge* e21 = nullptr;

        // If the current edge has a valid pair edge. ie. one that
        // points to the same vertex that the current edge starts from.
        // or if the current edge has no pair at all, create a new vertex
        if (e02->pair()->origin() == e02->destination() && e02->pair()->destination() == e02->origin())
        {
            v1 = createVertex(lerp(.5f, positions_[i0], positions_[i2]));
            i1 = v1->index;
            dirtyAttributes[VERTEX_ATTRB].indices.insert(i1);

            for (int i = 0; i < numMorphs_; ++i)
            {
                cells1[i1][i] = (cells1[i0][i] + cells1[i2][i]) * 0.5f;
                cells2[i1][i] = cells1[i1][i];
            }

            normals_[i1] = normalize(normals_[i0] + normals_[i2]);
            e21 = e20;
        }
        // Current edge is invalid so we can pair it up
        // with it's opposite edge.
        else
        {
            i1 = e02->pair()->destination()->index;
            v1 = vertices[i1];
            e21 = e20->next();
        }

        // Assign values to vertices and edges
        e20 = e02->pair();
        e01 = e02;
        e12 = createEdge(v1, e02->destination());

        v1->setEdge(e12);
        v1->isBoundary = e02->isBoundary;

        edgeKeyIndexMap.erase(e01->key);
        e01->key = edgeKey(v0->index, v1->index);
        edgeKeyIndexMap[e01->key] = e01->index;
        e01->setOrigin(v0);
        e01->setDestination(v1);
        e01->origin()->setEdge(e01);
        e01->setPair(e20);
        e01->setNext(e12);
        associateNeighbours(e01);

        e12->setFace(e02->face());
        e12->isBoundary = e02->isBoundary;
        e12->setOrigin(v1);
        e12->setDestination(e02->destination());
        e12->origin()->setEdge(e12);
        e12->setPair(e21);
        e12->setNext(e02->next());

        e20->setPair(e01);

        return e01;
    }
}

void HalfEdgeMesh::laplacian(unsigned index, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap)
{
    for (size_t i = 0; i < numMorphs_; ++i)
        *(lap + i) = 0;

    Vertex* v = vertices[index];
    Edge* start = v->edge();
    Edge* current = start;
#if DIFFUSION_EQNS==0
    do
    {
        for (int i = 0; i < numMorphs_; ++i)
            lap[i] += current->cotacotb[i] * readFromCells.at(current->destination()->index)[i];
        current = current->pair()->next();
    } while (start != current);

    for (int i = 0; i < numMorphs_; ++i)
        *(lap + i) += v->cotacotb[i] * readFromCells.at(index)[i];

    ASSERT(v->area != 0, "Area associated with vertex is 0");

    for (int i = 0; i < numMorphs_; ++i)
        *(lap + i) /= v->area;

#elif DIFFUSION_EQNS==1
    float Ac, Bc, Cc;
    do
    {
        Edge* ePair = current->pair();
        for (int i = 0; i < numMorphs_; ++i) {
            // compute mass flow rate per half edge
            Ac = readFromCells.at(current->next()->destination()->index)[i];
            Bc = readFromCells.at(current->origin()->index)[i];
            Cc = readFromCells.at(current->destination()->index)[i];
            *(lap + i) -= dot(Vec3(Ac, Bc, Cc), current->diffVec[i]);
            // and the paired half edge
            Ac = readFromCells.at(ePair->next()->destination()->index)[i];
            Bc = readFromCells.at(ePair->origin()->index)[i];
            Cc = readFromCells.at(ePair->destination()->index)[i];
            *(lap + i) += dot(Vec3(Ac, Bc, Cc), ePair->diffVec[i]);
        }
        current = current->pair()->next();
    } while (start != current);

    ASSERT(v->area != 0, "Area associated with vertex is 0");

    for (int i = 0; i < numMorphs_; ++i)
        *(lap + i) /= v->area;
#else
    std::cerr << "Error: diffusion equations undefined\n";
#endif
}

void HalfEdgeMesh::laplacianCLE(unsigned index, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap, std::vector<float>::iterator lapNoise)
{
	for (size_t i = 0; i < numMorphs_; ++i) {
		*(lap + i) = 0;
		*(lapNoise + i) = 0;
	}

	Vertex * v = vertices[index];
	Edge * start = v->edge();
	Edge * current = start;
	
	Vec3 ABC, sqrtABC, veinABC;
	do
	{
		Edge* ePair = current->pair();
		for (int i = 0; i < numMorphs_; ++i) {

			if (veinMorphIndex_ == -1 || i == veinMorphIndex_) {
				// compute mass flow rate per half edge
				ABC.x = readFromCells.at(current->next()->destination()->index)[i];
				ABC.y = readFromCells.at(current->origin()->index)[i];
				ABC.z = readFromCells.at(current->destination()->index)[i];
				*(lap + i) -= dot(ABC, current->diffVec[i]);
				// and its noise term
				sqrtABC.x = std::sqrt(ABC.x * std::abs(current->diffVec[i].x));
				sqrtABC.y = std::sqrt(ABC.y * std::abs(current->diffVec[i].y));
				sqrtABC.z = std::sqrt(ABC.z * std::abs(current->diffVec[i].z));
				*(lapNoise + i) -= dot(sqrtABC, current->nranVec[i]);

				// then, the paired half edge
				ABC.x = readFromCells.at(ePair->next()->destination()->index)[i];
				ABC.y = readFromCells.at(ePair->origin()->index)[i];
				ABC.z = readFromCells.at(ePair->destination()->index)[i];
				*(lap + i) += dot(ABC, ePair->diffVec[i]);
				// and its noise term
				sqrtABC.x = std::sqrt(ABC.x * std::abs(ePair->diffVec[i].x));
				sqrtABC.y = std::sqrt(ABC.y * std::abs(ePair->diffVec[i].y));
				sqrtABC.z = std::sqrt(ABC.z * std::abs(ePair->diffVec[i].z));
				*(lapNoise + i) += dot(sqrtABC, ePair->nranVec[i]);
			}
			else {
				float scale;

				veinABC.x = readFromCells.at(current->next()->destination()->index)[veinMorphIndex_];
				veinABC.y = readFromCells.at(current->origin()->index)[veinMorphIndex_];
				veinABC.z = readFromCells.at(current->destination()->index)[veinMorphIndex_];
				//if (veinABC.y > 0.f && (veinABC.x > 0.f || veinABC.z > 0.f))
				if (veinABC.y > 0.f && veinABC.z > 0.f)
					scale = veinDiffusionScale_;
				else
					scale = petalDiffusionScale_;

				// compute mass flow rate per half edge
				ABC.x = readFromCells.at(current->next()->destination()->index)[i];
				ABC.y = readFromCells.at(current->origin()->index)[i];
				ABC.z = readFromCells.at(current->destination()->index)[i];
				*(lap + i) -= scale * dot(ABC, current->diffVec[i]);
				// and its noise term
				sqrtABC.x = std::sqrt(ABC.x * std::abs(current->diffVec[i].x));
				sqrtABC.y = std::sqrt(ABC.y * std::abs(current->diffVec[i].y));
				sqrtABC.z = std::sqrt(ABC.z * std::abs(current->diffVec[i].z));
				*(lapNoise + i) -= scale * dot(sqrtABC, current->nranVec[i]);


				veinABC.x = readFromCells.at(ePair->next()->destination()->index)[veinMorphIndex_];
				veinABC.y = readFromCells.at(ePair->origin()->index)[veinMorphIndex_];
				veinABC.z = readFromCells.at(ePair->destination()->index)[veinMorphIndex_];
				//if (veinABC.y > 0.f && (veinABC.x > 0.f || veinABC.z > 0.f))
				if (veinABC.y > 0.f && veinABC.z > 0.f)
					scale = veinDiffusionScale_;
				else
					scale = petalDiffusionScale_;

				// then, the paired half edge
				ABC.x = readFromCells.at(ePair->next()->destination()->index)[i];
				ABC.y = readFromCells.at(ePair->origin()->index)[i];
				ABC.z = readFromCells.at(ePair->destination()->index)[i];
				*(lap + i) += scale * dot(ABC, ePair->diffVec[i]);
				// and its noise term
				sqrtABC.x = std::sqrt(ABC.x * std::abs(ePair->diffVec[i].x));
				sqrtABC.y = std::sqrt(ABC.y * std::abs(ePair->diffVec[i].y));
				sqrtABC.z = std::sqrt(ABC.z * std::abs(ePair->diffVec[i].z));
				*(lapNoise + i) += scale * dot(sqrtABC, ePair->nranVec[i]);

			}
		}
		current = current->pair()->next();
	} while (start != current);

	ASSERT(v->area != 0, "Area associated with vertex is 0");

	for (int i = 0; i < numMorphs_; ++i) {
		*(lap + i) /= v->area;
		*(lapNoise + i) /= v->area;
	}
}

void HalfEdgeMesh::generateRandomNumbers(unsigned index)
{
	Vertex* v = vertices[index];
	Edge* start = v->edge();
	Edge* current = start;
	do
	{
		for (int i = 0; i < numMorphs_; ++i) {
			current->nranVec[i].x = nran();
			current->nranVec[i].y = nran();
			current->nranVec[i].z = nran();
		}
		current = current->pair()->next();
	} while (start != current);
}

void HalfEdgeMesh::computeVecLambda(Face* f)
{
    // Update face
    for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
    {
        float t0 = diffusionTensors[morphIndex].t0_[f->index], t1 = diffusionTensors[morphIndex].t1_[f->index];
        if (prepatternInfo0.enabled && prepatternInfo0.targetIndex == morphIndex)
        {
            t0 = Utils::lerp(
                prepatternInfo0.lowVal, prepatternInfo0.highVal,
                pow(getFaceMorph(faces[f->index], prepatternInfo0.sourceIndex), prepatternInfo0.exponent));
            if (prepatternInfo0.squared)
                t0 *= t0;
        }

        if (prepatternInfo1.enabled && prepatternInfo1.targetIndex == morphIndex)
        {
            t1 = Utils::lerp(
                prepatternInfo1.lowVal, prepatternInfo1.highVal,
                pow(getFaceMorph(faces[f->index], prepatternInfo1.sourceIndex), prepatternInfo1.exponent));
            if (prepatternInfo1.squared)
                t1 *= t1;
        }

        Vec3 gradient = getFaceTangent(f, morphIndex);
        f->veclambda1[morphIndex] = t0 * gradient;
        f->veclambda2[morphIndex] = t1 * cross(gradient, f->normal);
    }

    Edge* e = f->edge();
    do
    {
        int Bi = e->origin()->index;
        int Ci = e->destination()->index;
        int Ai = e->next()->destination()->index;

        Vec3 A = positions_[Ai];
        Vec3 B = positions_[Bi];
        Vec3 C = positions_[Ci];

        Vec3 AB = B - A;
        Vec3 BC = C - B;
        Vec3 CA = A - C;

        Vec3 N = cross(AB, BC);

        for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
        {
            e->diffVec[morphIndex] = Vec3(0, 0, 0);
        }

        if (f != nullptr)
        {
            for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
            {
                Vec3 vec1n = normalize(f->veclambda1[morphIndex]);
                Vec3 vec2n = normalize(f->veclambda2[morphIndex]);

                float a = dot(-0.5f * dot(cross(BC, -1.f * N) / dot(N, N), vec1n) * f->veclambda1[morphIndex], BC * e->cotan);
                a += dot(-0.5f * dot(cross(BC, -1.f * N) / dot(N, N), vec2n) * f->veclambda2[morphIndex], BC * e->cotan);

                float b = dot(-0.5f * dot(cross(CA, -1.f * N) / dot(N, N), vec1n) * f->veclambda1[morphIndex], BC * e->cotan);
                b += dot(-0.5f * dot(cross(CA, -1.f * N) / dot(N, N), vec2n) * f->veclambda2[morphIndex], BC * e->cotan);

                float c = dot(-0.5f * dot(cross(AB, -1.f * N) / dot(N, N), vec1n) * f->veclambda1[morphIndex], BC * e->cotan);
                c += dot(-0.5f * dot(cross(AB, -1.f * N) / dot(N, N), vec2n) * f->veclambda2[morphIndex], BC * e->cotan);

                e->diffVec[morphIndex] = Vec3(a, b, c);
            }
        }

        e = e->next();
    } while(e != f->edge());
}

void HalfEdgeMesh::computeVecLambdas()
{
    for (Face* f : faces)
        computeVecLambda(f);

 /*   for (Face* f : faces)
    {
        for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
        {
            float t0 = diffusionTensors[morphIndex].t0_[f->index], t1 = diffusionTensors[morphIndex].t1_[f->index];
            if (prepatternInfo0.enabled && prepatternInfo0.targetIndex == morphIndex)
            {
                t0 = Utils::lerp(
                    prepatternInfo0.lowVal, prepatternInfo0.highVal,
                    pow(getFaceMorph(faces[f->index], prepatternInfo0.sourceIndex), prepatternInfo0.exponent));
                if (prepatternInfo0.squared)
                    t0 *= t0;
            }

            if (prepatternInfo1.enabled && prepatternInfo1.targetIndex == morphIndex)
            {
                t1 = Utils::lerp(
                    prepatternInfo1.lowVal, prepatternInfo1.highVal,
                    pow(getFaceMorph(faces[f->index], prepatternInfo1.sourceIndex), prepatternInfo1.exponent));
                if (prepatternInfo1.squared)
                    t1 *= t1;
            }

            Vec3 gradient = getFaceTangent(f, morphIndex);
            f->veclambda1[morphIndex] = t0 * gradient;
            f->veclambda2[morphIndex] = t1 * cross(gradient, f->normal);
        }
    }

	
	// precompute diffusion vector for each half edge
	for (Edge* e : edges) 
    {
		int Bi = e->origin()->index;
		int Ci = e->destination()->index;
		int Ai = e->next()->destination()->index;

		Vec3 A = positions_[Ai];
		Vec3 B = positions_[Bi];
		Vec3 C = positions_[Ci];

		Vec3 AB = B - A;
		Vec3 BC = C - B;
		Vec3 CA = A - C;

		Vec3 N = cross(AB, BC);

		for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex) 
        {
			e->diffVec[morphIndex] = Vec3(0, 0, 0);
		}

		Face* f = e->face();
		if (f != nullptr) 
        {
			for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex) 
            {
				Vec3 vec1n = normalize(f->veclambda1[morphIndex]);
				Vec3 vec2n = normalize(f->veclambda2[morphIndex]);

				float a = dot(-0.5f * dot(cross(BC, -1.f * N) / dot(N, N), vec1n) * f->veclambda1[morphIndex], BC * e->cotan);
				a += dot(-0.5f * dot(cross(BC, -1.f * N) / dot(N, N), vec2n) * f->veclambda2[morphIndex], BC * e->cotan);

				float b = dot(-0.5f * dot(cross(CA, -1.f * N) / dot(N, N), vec1n) * f->veclambda1[morphIndex], BC * e->cotan);
				b += dot(-0.5f * dot(cross(CA, -1.f * N) / dot(N, N), vec2n) * f->veclambda2[morphIndex], BC * e->cotan);

				float c = dot(-0.5f * dot(cross(AB, -1.f * N) / dot(N, N), vec1n) * f->veclambda1[morphIndex], BC * e->cotan);
				c += dot(-0.5f * dot(cross(AB, -1.f * N) / dot(N, N), vec2n) * f->veclambda2[morphIndex], BC * e->cotan);

				e->diffVec[morphIndex] = Vec3(a, b, c);
			}
		}
	}
    */
}

void HalfEdgeMesh::gradient(unsigned i, int morphIndex, std::vector<Cell>& readFromCells, Vec3& grad)
{
    grad.set(0, 0, 0);
    gradientFace(vertices[i]->edge()->face(), morphIndex, readFromCells, grad);
}

bool HalfEdgeMesh::hideAnisoVec(int) const
{
    return false; // TODO: implement me
}

void HalfEdgeMesh::updateGradLines()
{
    for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
    {
        if (showingGrad[morphIndex])
        {
            for (Face* face : faces)
            {
                Vec3 grad;
                auto start = getFaceCentroid(face);
                gradientFace(face, morphIndex, getReadFromCells(), grad);
                gradientLines.positions_[face->index * 2]     = 0.001f * face->normal + start;
                gradientLines.positions_[face->index * 2 + 1] = 0.001f * face->normal + start + normalize(grad) * vecLength;
            }
            gradientLines.updatePositionVBO();
            break;
        }
    }
}

void HalfEdgeMesh::updateDiffDirLines()
{
    for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
    {
        if (showingDiffDir[morphIndex])
        {
            for (size_t i = 0; i < faces.size(); ++i)
            {
                auto face = faces[i];
                auto start = getFaceCentroid(face);
                anisotropicDiffusionTensor.positions_[i * 2]     = 0.001f * face->normal + start;
                anisotropicDiffusionTensor.positions_[i * 2 + 1] = 0.001f * face->normal + start + tangents[morphIndex][i] * vecLength;
            }
            anisotropicDiffusionTensor.updatePositionVBO();
            break;
        }
    }
}

void HalfEdgeMesh::gradientFace(Face* f, int morphIndex, std::vector<Cell>& readFromCells, Vec3& faceGrad) const
{
    const std::pair<Vec3, Vec3>& gradVecs = gradCoefs.at(f);
    int i = f->edge()->origin()->index;
    int j = f->edge()->destination()->index;
    int k = f->edge()->next()->destination()->index;
    faceGrad =
        (readFromCells.at(j)[morphIndex] - readFromCells.at(i)[morphIndex]) * gradVecs.first +
        (readFromCells.at(k)[morphIndex] - readFromCells.at(i)[morphIndex]) * gradVecs.second;
}

bool HalfEdgeMesh::isBoundary(unsigned i)
{
    return vertices[i]->isBoundary;
}

float HalfEdgeMesh::getCotanWeights(unsigned v0, unsigned v1, int morphIndex)
{
    if (v0 != v1)
        return getEdge(v0, v1)->cotacotb[morphIndex];
    else
        return vertices[v0]->cotacotb[morphIndex];
}

float HalfEdgeMesh::getEdgeCotanWeight(unsigned edgeIndex, int morphIndex)
{
    return edges[edgeIndex]->cotacotb[morphIndex];
}

float HalfEdgeMesh::getArea(unsigned v) const
{
    return vertices[v]->area;
}

Vec3 HalfEdgeMesh::getPosition(unsigned i) const
{
    return positions_[i];
}

Vec3 HalfEdgeMesh::getNormal(unsigned i) const
{
    return normals_[i];
}

float HalfEdgeMesh::getTotalArea() const
{
    float area = 0;
    for (Face* f : faces)
        area += f->area;
    return area;
}

Vec3 HalfEdgeMesh::getFaceTangent(Face* f, int morphIndex)
{
    return tangents[morphIndex][f->index];
}

#if DIFFUSION_EQNS==0
void HalfEdgeMesh::calculateCotangent(Vertex* v, const int morphIndex)
{
    ASSERT(v != nullptr, "Vertex is nullptr");
    // Create tensor
    auto calcD = [&](unsigned faceIndex)
    {
        Mat3 DMat;
        float t0 = diffusionTensors[morphIndex].t0_[faceIndex], t1 = diffusionTensors[morphIndex].t1_[faceIndex];

        if (prepatternInfo0.enabled && prepatternInfo0.targetIndex == morphIndex)
        {
            t0 = Utils::lerp(
                prepatternInfo0.lowVal, prepatternInfo0.highVal, 
                pow(getFaceMorph(faces[faceIndex], prepatternInfo0.sourceIndex), prepatternInfo0.exponent));
            if (prepatternInfo0.squared)
                t0 *= t0;
        }

        if (prepatternInfo1.enabled && prepatternInfo1.targetIndex == morphIndex)
        {
            t1 = Utils::lerp(
                prepatternInfo1.lowVal, prepatternInfo1.highVal,
                pow(getFaceMorph(faces[faceIndex], prepatternInfo1.sourceIndex), prepatternInfo1.exponent));
            if (prepatternInfo1.squared)
                t1 *= t1;
        }

        DMat.setCol(0, t0, 0, 0);
        DMat.setCol(1, 0, t1, 0);
        DMat.setCol(2, 0, 0, 1);

        return DMat;
    };

    auto DT = [](const Vec3& norm, const Vec3& dir, const Mat3& D)
    {
        //90 degree rotation matrix
        Mat3 R = RotationMat3(Math::degToRad(90), norm);
        Mat3 RT = transpose(R);

        // Transform from triangle frame to standard basis for comparison against diag tensor
        Mat3 Q;
        Q.setCol(0, normalize(dir));
        Q.setCol(1, normalize(norm.cross(Q.col(0))));
        Q.setCol(2, normalize(Q.col(0).cross(Q.col(1))));
        Mat3 QT = transpose(Q);

        return RT * Q * D * QT * R;
    };

    float alpha, beta, theta;
    float Gamma, gamma, Xi, xi;
    Vec3 norm, dir;

    // precompute inner product of neighbouring bases
    unsigned i = v->index;
    Edge* start = v->edge();
    Edge* current = start;
    {
        unsigned j, k;
        do // For each edge, calculate inner product 
        {
            j = current->destination()->index;
            if (current->face() != nullptr)
            {
                k = current->next()->destination()->index;
                alpha = current->next()->next()->angle;
                beta = current->pair()->next()->next()->angle;
                {
                    Face* face = current->face();
                    norm = normalize(face->normal);
                    dir = getFaceTangent(face, morphIndex);
                    dir = normalize(norm.cross(dir).cross(norm));

                    Vec3 e_i = positions_[i] - positions_[k];
                    Vec3 e_j = positions_[k] - positions_[j];

                    Mat3 D = calcD(face->index);
                    Vec3 Dtei = DT(norm, dir, D) * e_i;
                    Gamma = Dtei.length() / e_i.length();
                    float value = dot(Dtei, -1.f * e_j) / (Dtei.length() * e_j.length());
                    value = std::clamp(value, -1.f, 1.f);
                    gamma = acos(value);

                    current->cotacotb[morphIndex] = .5f * Gamma * (cosf(gamma) / sinf(alpha));
                }

                // Handle adjacent face
                if (current->pair()->face() != nullptr)
                {
                    Face* face = current->pair()->face();
                    k = current->pair()->next()->destination()->index;
                    norm = normalize(face->normal);
                    dir = getFaceTangent(face, morphIndex);
                    dir = normalize(norm.cross(dir).cross(norm));

                    Vec3 e_l = positions_[i] - positions_[k];
                    Vec3 e_m = positions_[k] - positions_[j];

                    Mat3 D = calcD(face->index);
                    Vec3 Dtel = DT(norm, dir, D) * e_l;
                    Xi = Dtel.length() / e_l.length();
                    float value = dot(Dtel, -1.f * e_m) / (Dtel.length() * e_m.length());
                    value = std::clamp(value, -1.f, 1.f);
                    xi = acos(value);

                    current->cotacotb[morphIndex] += .5f * Xi * (cosf(xi) / sinf(beta));
                }
            }
            else
            {
                Face* face = current->pair()->face();
                k = current->pair()->next()->destination()->index;
                beta = current->pair()->next()->next()->angle;

                norm = normalize(face->normal);
                dir = getFaceTangent(face, morphIndex);
                dir = normalize(norm.cross(dir).cross(norm));

                Vec3 e_l = positions_[i] - positions_[k];
                Vec3 e_m = positions_[k] - positions_[j];

                Mat3 D = calcD(face->index);

                Vec3 Dtel = DT(norm, dir, D) * e_l;
                Xi = Dtel.length() / e_l.length();
                float value = dot(Dtel, -1.f * e_m) / (Dtel.length() * e_m.length());
                value = std::clamp(value, -1.f, 1.f);
                xi = acos(value);

                current->cotacotb[morphIndex] = .5f * Xi * (cosf(xi) / sinf(beta));
            }

            current = current->pair()->next();
        } while (start != current);
    }

    // precompute innerproduct of basis with itself
    {
        start = v->edge();
        current = start;
        v->cotacotb[morphIndex] = 0.f;
        unsigned j, k;
        do // For each face, calculate cotangent
        {
            j = current->destination()->index;
            k = current->next()->destination()->index;
            Face* face = current->face();
            if (face != nullptr)
            {
                alpha = current->next()->next()->angle;
                theta = current->next()->angle;

                norm = normalize(face->normal);
                dir = getFaceTangent(face, morphIndex);
                dir = normalize(norm.cross(dir).cross(norm));

                Mat3 D = calcD(face->index);

                Vec3 e_j = positions_[k] - positions_[j];

                v->cotacotb[morphIndex] += dot(DT(norm, dir, D) * e_j / e_j.length(), e_j / e_j.length()) * ((1.f / tanf(alpha) + 1.f / tanf(theta))); // calculate I
            }
            current = current->pair()->next();
        } while (start != current);
        v->cotacotb[morphIndex] *= -.5f;
    }
#else
void HalfEdgeMesh::calculateCotangent(Vertex* v, const int)
{
    // TODO: cotangents need to be computed only if the mesh changes...are they?

    // compute cotangent for each half edge	
    unsigned i = v->index;
    Edge* start = v->edge();
    Edge* current = start;
    unsigned j, k;
    do
    {
        j = current->destination()->index;
        if (current->face() != nullptr)
        {
            k = current->next()->destination()->index;
            {
                Vec3 el1 = positions_[k] - positions_[i];
                Vec3 el2 = positions_[k] - positions_[j];
                float dote = dot(el1, el2);
                float cross_len = cross(el1, el2).length();
                current->cotan = dote / cross_len;
            }
        }
        else
        {
            current->cotan = 0;
        }
        current = current->pair()->next();
    } while (start != current);
#endif
}

void HalfEdgeMesh::updateDiffusionCoefs()
{
    calculateCotangents();
#if DIFFUSION_EQNS==1
    computeVecLambdas();
#endif
}

void HalfEdgeMesh::calculateCotangents()
{
    for (size_t i = 0; i < vertices.size(); ++i)
        for (int j = 0; j < numMorphs_; ++j)
        {
            calculateCotangent(vertices[i], j);
        }
}

void HalfEdgeMesh::clearDiffusion(int morphIndex)
{
    for (unsigned i = 0; i < faces.size(); ++i)
    {
        diffusionTensors[morphIndex].t0_[i] = paintTensorVals[0];
        diffusionTensors[morphIndex].t1_[i] = paintTensorVals[1];

        if (isGPUEnabled)
            dirtyAttributes[TANGENT_ATTRIB].indices.insert(i);
    }
}

void HalfEdgeMesh::calculateBoundaryVertices()
{
    for (Vertex* v : vertices)
    {
        Edge* start = v->edge();
        Edge* current = start;
        v->isBoundary = false;

        do
        {
            current = current->pair()->next();
            if (current->face() == nullptr)
            {
                v->isBoundary = true;
                break;
            }
        } while (start != current);
    }
}

void HalfEdgeMesh::associateNeighbours(unsigned i0, unsigned i1)
{
    if (edgeExists(i0, i1) && edgeExists(i1, i0))
    {
        Edge* e = getEdge(i0, i1);
        Edge* pair = getEdge(i1, i0);
        e->setPair(pair);
        pair->setPair(e);
    }
}

void HalfEdgeMesh::associateNeighbours(Edge* e)
{
    unsigned i0 = e->origin()->index;
    unsigned i1 = e->next()->origin()->index;
    associateNeighbours(i0, i1);
}

std::vector<HalfEdgeMesh::Edge*> HalfEdgeMesh::getEdgesByRadius(unsigned i, float radius)
{
    std::vector<HalfEdgeMesh::Edge*> foundEdges;
    std::vector<unsigned> neighbours;
    std::vector<unsigned> visit;
    visit.push_back(i);
    neighbours.push_back(i);

    Vec3 v = positions_[i];
    do
    {
        visit.erase(visit.begin());

        Edge* startE = vertices[visit[0]]->edge();
        Edge* e = startE;
        do
        {
            foundEdges.push_back(e);

            unsigned j = e->destination()->index;
            if (length(v - positions_[j]) <= radius &&
                std::find(visit.begin(), visit.end(), j) == visit.end() &&
                std::find(neighbours.begin(), neighbours.end(), j) == neighbours.end())
            {
                neighbours.push_back(j);
                visit.push_back(j);
            }
            e = e->pair()->next();
        } while (e != startE);
    } while (visit.size() > 0);
    return foundEdges;
}

std::vector<unsigned> HalfEdgeMesh::getNeighboursByRadius(unsigned i, float radius)
{
    std::vector<unsigned> neighbours;
    std::vector<unsigned> visit;
    visit.push_back(i);
    neighbours.push_back(i);

    Vec3 v = positions_[i];
    do
    {
        Edge* startE = vertices[visit[0]]->edge();
        Edge* e = startE;
        visit.erase(visit.begin());
        do
        {
            unsigned j = e->destination()->index;
            if (length(v - positions_[j]) <= radius &&
                std::find(visit.begin(), visit.end(), j) == visit.end() &&
                std::find(neighbours.begin(), neighbours.end(), j) == neighbours.end())
            {
                neighbours.push_back(j);
                visit.push_back(j);
            }
            e = e->pair()->next();
        } while (e != startE);
    } while (visit.size() > 0);

    return neighbours;
}

std::vector<unsigned> HalfEdgeMesh::getFaceNeighboursByRadius(unsigned faceIndex, float radius)
{
    std::vector<unsigned> neighbours;
    std::vector<unsigned> visit;
    visit.push_back(faceIndex);
    neighbours.push_back(faceIndex);

    Vec3 v = getFaceCentroid(faces[faceIndex]);
    do
    {
        Edge* startE = faces[visit[0]]->edge();
        Edge* e = startE;
        visit.erase(visit.begin());
        do
        {
            if (!e->isBoundary)
            {
                unsigned j = e->pair()->face()->index;
                if (length(v - getFaceCentroid(faces[j])) <= radius &&                      // neighbour is in radius
                    std::find(visit.begin(), visit.end(), j) == visit.end() &&              // neighbour not to be visited
                    std::find(neighbours.begin(), neighbours.end(), j) == neighbours.end()) // neighour not visited
                {
                    neighbours.push_back(j);
                    visit.push_back(j);
                }
            }
            e = e->next();
        } while (e != startE);

    } while (visit.size() > 0);

    return neighbours;
}

std::set<unsigned> HalfEdgeMesh::getBoundaryCellsIndices()
{
    std::set<unsigned> foundIndices;
    for (auto& v : vertices)
        if (v->isBoundary)
            foundIndices.insert(v->index);
    return foundIndices;
}

std::vector<unsigned> HalfEdgeMesh::getNeighbours(unsigned i, unsigned order)
{
    std::vector<unsigned> foundNeighbours;
    std::vector<unsigned> visited;
    std::vector<unsigned> visit;

    if (order == 0)
        return foundNeighbours;

    visit.push_back(i);
    getNeighbours(order, visit, visited, foundNeighbours);
    return foundNeighbours;
}

void HalfEdgeMesh::getNeighbours(unsigned order, std::vector<unsigned>& visit, std::vector<unsigned>& visited, std::vector<unsigned>& neighbours)
{
    if (order == 0)
        return;

    std::vector<unsigned> newVisits;
    while (visit.size() > 0)
    {
        unsigned i = visit[0];
        Vertex* v = vertices[i];
        Edge* start = v->edge();
        Edge* current = start;

        //for each neighbour
        do
        {
            unsigned j = current->destination()->index;

            // if not visited
            if (std::find(visited.begin(), visited.end(), j) == visited.end())
            {

                // and not queued to visit then add to visit
                if (std::find(visit.begin(), visit.end(), j) == visit.end())
                    newVisits.push_back(j);

                // if not a neighbour add it to neighbours
                if (std::find(neighbours.begin(), neighbours.end(), j) == neighbours.end())
                    neighbours.push_back(j);
            }
            current = current->pair()->next();

        } while (current != start);

        visited.push_back(i);
        visit.erase(std::find(visit.begin(), visit.end(), i));
    }
    visit = newVisits;
    getNeighbours(order - 1, visit, visited, neighbours);
}

Vec3 HalfEdgeMesh::getFaceNormal(unsigned i)
{
    return faces[i]->normal;
}

Vec3 HalfEdgeMesh::getFaceCentroid(Face* f)
{
    auto i0 = f->edge()->origin()->index;
    auto i1 = f->edge()->destination()->index;
    auto i2 = f->edge()->next()->destination()->index;
    return (positions_[i0] + positions_[i1] + positions_[i2]) * .33333333f;
}

float HalfEdgeMesh::getFaceMorph(Face* f, int morphIndex)
{
    auto& cells = getReadFromCells();
    auto i0 = f->edge()->origin()->index;
    auto i1 = f->edge()->destination()->index;
    auto i2 = f->edge()->next()->destination()->index;
    return (cells[i0][morphIndex] + cells[i1][morphIndex] + cells[i2][morphIndex]) / 3.f;
}

HalfEdgeMesh::EdgeKey HalfEdgeMesh::edgeKey(uint32_t a, uint32_t b)
{
    return ((uint64_t)a << 32) | b;
}

bool HalfEdgeMesh::vertexExists(unsigned i)
{
    if (i >= vertices.size())
        return false;
    return vertices[i] != nullptr;
}

bool HalfEdgeMesh::edgeExists(Vertex* v0, Vertex* v1)
{
    return edgeExists(v0->index, v1->index);
}

bool HalfEdgeMesh::edgeExists(uint32_t i0, uint32_t i1)
{
    EdgeKey key = edgeKey(i0, i1);
    return edgeKeyIndexMap.count(key) > 0;
}

HalfEdgeMesh::Edge* HalfEdgeMesh::getEdge(Vertex* v0, Vertex* v1)
{
    return getEdge(v0->index, v1->index);
}

HalfEdgeMesh::Edge* HalfEdgeMesh::getEdge(unsigned i0, unsigned i1)
{
    return edges.at(edgeKeyIndexMap.at(edgeKey(i0, i1)));
}

HalfEdgeMesh::Vertex* HalfEdgeMesh::getVertex(unsigned i)
{
    ASSERT(i < vertices.size(), "Index out of bounds");
    return vertices[i];
}

float getArea(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& centre)
{
    float a1 = ((p1 - p0) * .5f).areaBetween(centre - p0);
    float a2 = ((p2 - p0) * .5f).areaBetween(centre - p0);
    return a1 + a2;
}

void HalfEdgeMesh::join(Edge* e0, Edge* e1, Edge* e2, Face* f)
{
    unsigned i0 = e0->origin()->index;
    unsigned i1 = e1->origin()->index;
    unsigned i2 = e2->origin()->index;

    indices_[f->firstIndexIndex] = i0;
    indices_[f->firstIndexIndex + 1] = i1;
    indices_[f->firstIndexIndex + 2] = i2;

    e0->origin()->setEdge(e0);
    e1->origin()->setEdge(e1);
    e2->origin()->setEdge(e2);

    e0->setNext(e1);
    e1->setNext(e2);
    e2->setNext(e0);

    e0->setDestination(e1->origin());
    e1->setDestination(e2->origin());
    e2->setDestination(e0->origin());

    e0->setFace(f);
    e1->setFace(f);
    e2->setFace(f);

    e0->angle = acuteAngleBetween(positions_[i1] - positions_[i0], positions_[i2] - positions_[i0]);
    e1->angle = acuteAngleBetween(positions_[i0] - positions_[i1], positions_[i2] - positions_[i1]);
    e2->angle = acuteAngleBetween(positions_[i0] - positions_[i2], positions_[i1] - positions_[i2]);

    f->area = areaBetween(positions_[i1] - positions_[i0], positions_[i2] - positions_[i0]);
    f->circumcentre = calculateCentre(i0, i1, i2);
    f->setEdge(e0);
    f->normal = normalize(cross(positions_[i1] - positions_[i0], positions_[i2] - positions_[i0]));

    if (e0->pair() != nullptr && e0->face() != nullptr && e0->pair()->face() != nullptr)
    {
        e0->isBoundary = false;
        e0->pair()->isBoundary = false;
    }
    if (e1->pair() != nullptr && e1->face() != nullptr && e1->pair()->face() != nullptr)
    {
        e1->isBoundary = false;
        e1->pair()->isBoundary = false;
    }
    if (e2->pair() != nullptr && e2->face() != nullptr && e2->pair()->face() != nullptr)
    {
        e2->isBoundary = false;
        e2->pair()->isBoundary = false;
    }

    precomputeGradCoef(f);
}

HalfEdgeMesh::Edge* HalfEdgeMesh::getLongestEdge(Face* f)
{
    unsigned i0 = f->edge()->origin()->index;
    unsigned i1 = f->edge()->next()->origin()->index;
    unsigned i2 = f->edge()->next()->next()->origin()->index;
    float e0Len = length(positions_[i1] - positions_[i0]);
    float e1Len = length(positions_[i2] - positions_[i1]);
    float e2Len = length(positions_[i0] - positions_[i2]);

    if (e0Len > e1Len)
        if (e0Len > e2Len)
            return f->edge();
        else
            return f->edge()->next()->next();
    else if (e1Len > e2Len)
        return f->edge()->next();
    else
        return f->edge()->next()->next();
}

void HalfEdgeMesh::calculateAngles()
{
    for (Face* f : faces)
    {
        Edge* e01 = f->edge();
        Edge* e12 = f->edge()->next();
        Edge* e20 = f->edge()->next()->next();
        unsigned i0 = e01->origin()->index;
        unsigned i1 = e12->origin()->index;
        unsigned i2 = e20->origin()->index;
        e01->angle = acuteAngleBetween(positions_[i1] - positions_[i0], positions_[i2] - positions_[i0]);
        e12->angle = acuteAngleBetween(positions_[i0] - positions_[i1], positions_[i2] - positions_[i1]);
        e20->angle = acuteAngleBetween(positions_[i0] - positions_[i2], positions_[i1] - positions_[i2]);
    }
}

void HalfEdgeMesh::calculateDualAreas()
{
    calculateFaceAreas();
    unsigned vertCount = static_cast<unsigned>(vertices.size());
    for (unsigned i = 0; i < vertCount; ++i)
        calculateDualArea(i);
}

void HalfEdgeMesh::calculateDualArea(unsigned i)
{
    calculateDualArea(vertices[i]);
}

void HalfEdgeMesh::calculateDualArea(Vertex* v)
{
    if (v == nullptr)
        return;

    Edge* start = v->edge();
    Edge* current = start;

    v->area = 0;
    do
    {
        if (current->face() != nullptr)
        {
            float area = 0;
            if (thirdArea_)
            {
                area = current->face()->area / 3.f;
            }
            else
            {
                unsigned i0 = v->index;
                unsigned i1 = current->next()->origin()->index;
                unsigned i2 = current->next()->next()->origin()->index;
                area = getArea(positions_[i0], positions_[i1], positions_[i2], calculateCentre(i0, i1, i2));
            }
            v->area += area;
        }
        current = current->pair()->next();
    } while (start != current);
}

void HalfEdgeMesh::calculateFaceAreas()
{
    for (Face* f : faces)
        calculateFaceArea(f);
}

void HalfEdgeMesh::calculateFaceArea(Face* f)
{
    unsigned i0 = f->edge()->origin()->index;
    unsigned i1 = f->edge()->next()->origin()->index;
    unsigned i2 = f->edge()->next()->next()->origin()->index;
    f->area = areaBetween(positions_[i1] - positions_[i0], positions_[i2] - positions_[i0]);
}

void HalfEdgeMesh::calculateFaceNormals()
{
    for (Face* f : faces)
        calculateFaceNormal(f);
}

Vec3 HalfEdgeMesh::calculateCentre(Face* f)
{
    Edge* e = f->edge();
    int i0 = e->origin()->index;
    int i1 = e->next()->origin()->index;
    int i2 = e->next()->origin()->index;

    const Vec3& v0 = positions_[i0];
    const Vec3& v1 = positions_[i1];
    const Vec3& v2 = positions_[i2];

    Vec3 centre = circumcentre(v0, v1, v2);
    if (!pointInTriangle(v0, v1, v2, centre))
        centre = barycentre(v0, v1, v2);
    return centre;
}

Vec3 HalfEdgeMesh::calculateCentre(unsigned i0, unsigned i1, unsigned i2)
{
    const Vec3& v0 = positions_[i0];
    const Vec3& v1 = positions_[i1];
    const Vec3& v2 = positions_[i2];

    Vec3 centre = circumcentre(v0, v1, v2);
    if (!pointInTriangle(v0, v1, v2, centre))
        centre = barycentre(v0, v1, v2);
    return centre;
}

void HalfEdgeMesh::calculateFaceNormal(Face* f)
{
    unsigned i0 = f->edge()->origin()->index;
    unsigned i1 = f->edge()->next()->origin()->index;
    unsigned i2 = f->edge()->next()->next()->origin()->index;
    f->normal = normalize(cross(positions_[i1] - positions_[i0], positions_[i2] - positions_[i0]));
}

void HalfEdgeMesh::calculateVertexNormals()
{
    for (Vertex* v : vertices)
        calculateVertexNormal(v);
}

// Assumes face normals_ have been already calculated and normalized
void HalfEdgeMesh::calculateVertexNormal(Vertex* v)
{
    if (v == nullptr)
        return;

    // Sum all Angles around vertex
    Edge* start = v->edge();
    Edge* current = start;
    unsigned i = v->index;
    normals_[i].zero();

    do
    {
        if (current->face() != nullptr)
        {
            Vertex* v1 = current->next()->origin();
            Vertex* v2 = current->next()->next()->origin();
            float angle = acuteAngleBetween(positions_[v1->index] - positions_[v->index], positions_[v2->index] - positions_[v->index]);
            normals_[i] = normals_[i] + current->face()->normal * angle;
        }
        current = current->pair()->next();
    } while (current != start);

    normals_[i].normalize();
}

void HalfEdgeMesh::calculateNormals()
{
    calculateFaceNormals();
    calculateVertexNormals();
    updateNormalVBO();
}

HalfEdgeMesh::Vertex* HalfEdgeMesh::splitFaceAt(Face* f, Edge* e20)
{
    /* Split face at e_02 produces new edge e_31
             1	 	    1	 
            / \	 	   /|\	 
           /   \	  / | \	 
          /_____\    /__|__\  
         0       2  0   3   2 
    */

    ASSERT(f->edge() != nullptr, "Invalid face");
    ASSERT(f == e20->face(), "Edge does not belong to face");

    //cellsToUpdate.push_back({ e20->destination()->index, {}});
    //cellsToUpdate.push_back({ e20->next()->destination()->index, {}});
    //cellsToUpdate.push_back({ e20->origin()->index, {}});

    Edge* e01 = e20->next();
    Edge* e12 = e01->next();
    Edge* e23 = splitEdge(e20);

    cellsToUpdate[e23->destination()->index] = NewCell{{e23->origin()->index, e01->origin()->index}};

    animation_.addVertex(e23->destination()->index, e23->origin()->index, e01->origin()->index);

    Edge* e30 = e23->next();
    Edge* e13 = createEdge(e12->origin(), e30->origin()); // e13
    Edge* e31 = createEdge(e30->origin(), e12->origin()); // e31

    e13->isBoundary = false;
    e31->isBoundary = false;
    e13->setOrigin(e12->origin());
    e31->setOrigin(e30->origin());

    Face* f1 = createFace(e31, e12, e23);
    join(e01, e13, e30, f);

    dirtyAttributes[FACE_ATTRB].indices.insert(f->index);
    dirtyAttributes[FACE_ATTRB].indices.insert(f1->index);

    for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
    {
        diffusionTensors[morphIndex].t0_.emplace_back(
            diffusionTensors[morphIndex].t0_[f->index]);
        diffusionTensors[morphIndex].t1_.emplace_back(
            diffusionTensors[morphIndex].t1_[f->index]);
        f1->veclambda1 = f->veclambda1;
        f1->veclambda2 = f->veclambda2;

        auto tangent = tangents[morphIndex][f->index];
        tangents[morphIndex].push_back(tangent);
    }

    auto tangent = tangents[0][f->index];
    auto center = getFaceCentroid(f1);
    auto dir = center + tangent;
    gradientLines.addLine(center, dir, Vec3(0, 0, 0), Vec3(1, 0, 0));
    anisotropicDiffusionTensor.addLine(center, dir, Vec3(0, 0, 0), Vec3(0, 1, 0));

    // try to match new edges with neighbours
    associateNeighbours(e13);
    associateNeighbours(e23);
    associateNeighbours(e30);

    return e30->origin();
}

void HalfEdgeMesh::paint(int faceIndex, Vec3 p, float paintRadius)
{
    if (faceIndex < 0 || static_cast<size_t>(faceIndex) >= faces.size())
        return;

    auto v0i = faces[faceIndex]->edge()->origin()->index;
    auto v1i = faces[faceIndex]->edge()->next()->origin()->index;
    auto v2i = faces[faceIndex]->edge()->next()->next()->origin()->index;

    Vec3 v0 = (transform_.getTransformMat() * Vec4(positions_[v0i], 1)).xyz();
    Vec3 v1 = (transform_.getTransformMat() * Vec4(positions_[v1i], 1)).xyz();
    Vec3 v2 = (transform_.getTransformMat() * Vec4(positions_[v2i], 1)).xyz();
        
    float e0Len = (v0 - p).length();
    float e1Len = (v1 - p).length();
    float e2Len = (v2 - p).length();

    p = (transform_.getInvTransformMat() * Vec4(p, 1)).xyz(); // Put p back into model space

    // Find closest vertex
    unsigned ind = 0;
    if (e0Len < e1Len && e0Len < e2Len)
        ind = v0i;
    else if (e1Len < e0Len && e1Len < e2Len)
        ind = v1i;
    else
        ind = v2i;

    // Check if painting diff dir
    int paintingDiffDirIndex = -1;
    for (int i = 0; i < numMorphs_; ++i)
        if (paintingDiffDir[i])
            paintingDiffDirIndex = i;

    // Find neighbour indices_
    std::vector<unsigned> neighbours;
    if (paintingDiffDirIndex >= 0)
        neighbours = getFaceNeighboursByRadius(faceIndex, paintRadius);
    else
        neighbours = getNeighboursByRadius(ind, paintRadius);

    // Update values at indices_
    for (unsigned i : neighbours)
    {
        if (paintingDiffDirIndex >= 0)
        {
            Vec3 T = normalize(p - *prevP_.begin());
            T = T.cross(faces[i]->normal);
            T = faces[i]->normal.cross(T);
            if (p.length() != 0 && T.length() != 0)
            {
                tangents[paintingDiffDirIndex][i] = T;
                setDiffTensor(i, paintTensorVals[0], paintTensorVals[1], paintingDiffDirIndex);

#if DIFFUSION_EQNS==1
                computeVecLambda(faces[i]);
#else
                calculateCotangent(faces[i]->edge()->origin(), paintingDiffDirIndex);
                calculateCotangent(faces[i]->edge()->destination(), paintingDiffDirIndex);
                calculateCotangent(faces[i]->edge()->next()->origin(), paintingDiffDirIndex);
#endif
            }

            if (isGPUEnabled)
            {
                dirtyAttributes[TANGENT_ATTRIB].indices.insert(i);

                dirtyAttributes[VERTEX_ATTRB].indices.insert(faces[i]->edge()->origin()->index);
                dirtyAttributes[VERTEX_ATTRB].indices.insert(faces[i]->edge()->destination()->index);
                dirtyAttributes[VERTEX_ATTRB].indices.insert(faces[i]->edge()->next()->origin()->index);
            }

        }
        else if (selectCells)
        {
            if (alternateControlMode)
                selectedCells.erase(i);
            else
                selectedCells.insert(i);
        }
        else
        {
            float s = length(p - positions_[i]) / paintRadius;
            for (int j = 0; j < numMorphs_; ++j)
            {
                if (alternateControlMode)
                {
                    if (paintingMorph[j])
                    {
                        float val = s * clearMValue[j] + (1.f - s) * clearMValue[j];
                        cells1[i][j] = val;
                        cells2[i][j] = val;
                    }
                }
                else
                {
                    if (paintingMorph[j])
                    {
                        float val = s * clearMValue[j] + (1.f - s) * clearMValue[j];
                        cells1[i][j] += val;
                        cells2[i][j] += val;
                        
                        if (cells1[i][j] < 0.f)
                            cells1[i][j] = 0.f;
                        
                        if (cells2[i][j] < 0.f)
                            cells2[i][j] = 0.f;
                    }
                }
            }
            dirtyAttributes[CELL1_ATTRIB].indices.insert(i);
            dirtyAttributes[CELL2_ATTRIB].indices.insert(i);
        }
    }
    selectedCell = ind;
    prevP_.push_back(p);
    if (prevP_.size() > 5)
        prevP_.erase(prevP_.begin());
}

int HalfEdgeMesh::raycast(const Vec3& dir, const Vec3& origin, float& t0) const
{
    Ray ray((transform_.getInvTransformMat() * Vec4(origin)).xyz(),
            (transform_.getInvTransformMat() * Vec4(dir)).xyz());
    int result = -1;
    if (bvh_.raycast(ray))
    {
        result = ray.triIndex_;
        t0 = ray.t_;
    }
    return result;
}

void HalfEdgeMesh::printInfo()
{
    unsigned numVerts = static_cast<unsigned>(vertices.size());
    unsigned numEdges = static_cast<unsigned>(edges.size()) / 2;
    unsigned numFaces = static_cast<unsigned>(faces.size());
    int euler = numVerts - numEdges + numFaces;

    std::cout << "Half Edge Mesh Info" << std::endl;
    std::cout << "--------------------------" << std::endl;
    std::cout << " Vertices " << numVerts << std::endl;
    std::cout << " Faces " << numFaces << std::endl;
    std::cout << " Edges " << numEdges << std::endl;
    std::cout << " Half Edges " << edges.size() << std::endl;
    std::cout << " Euler " << euler << std::endl;
}

float HalfEdgeMesh::getArea(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& centre)
{
    float a1 = ((p1 - p0) * .5f).areaBetween(centre - p0);
    float a2 = ((p2 - p0) * .5f).areaBetween(centre - p0);
    return a1 + a2;
}

void HalfEdgeMesh::saveToObj(const std::string& filename)
{
    std::ofstream objFile;
    objFile.open(filename, std::ofstream::out);
    std::vector<unsigned> nullsBefore(vertices.size(), 0);
    unsigned nulls = 0;
    unsigned i = 0;
    for (Vertex* v : vertices)
    {
        if (v == nullptr)
        {
            nulls++;
            nullsBefore[i++] = nulls;
            continue;
        }

        nullsBefore[i++] = nulls;

        Vec3 p = positions_[v->index];
        objFile << "v " << p.x << " " << p.y << " " << p.z << std::endl;
    }

    for (Vertex* v : vertices)
    {
        if (v == nullptr)
            continue;

        Vec3 n = normals_[v->index];
        objFile << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
    }

    for (Face* f : faces)
    {
        Vertex* v0 = f->edge()->origin();
        Vertex* v1 = f->edge()->next()->origin();
        Vertex* v2 = f->edge()->next()->next()->origin();
        unsigned i0 = v0->index;
        unsigned i1 = v1->index;
        unsigned i2 = v2->index;

        objFile << "f ";
        objFile << i0 - nullsBefore[i0] + 1 << "//" << i0 - nullsBefore[i0] + 1 << " ";
        objFile << i1 - nullsBefore[i1] + 1 << "//" << i1 - nullsBefore[i1] + 1 << " ";
        objFile << i2 - nullsBefore[i2] + 1 << "//" << i2 - nullsBefore[i2] + 1 << std::endl;
    }
    objFile.close();
}

float HalfEdgeMesh::avgFaceSize()
{
    if (faces.size() == 0)
        return 0.f;

    float avgFaceSize = 0.f;
    for (Face* f : faces)
    {
        unsigned i0 = f->edge()->origin()->index;
        unsigned i1 = f->edge()->next()->origin()->index;
        unsigned i2 = f->edge()->next()->next()->origin()->index;
        avgFaceSize += areaBetween(positions_[i1] - positions_[i0], positions_[i2] - positions_[i0]);
    }
    avgFaceSize = avgFaceSize / faces.size();
    return avgFaceSize;
}

void HalfEdgeMesh::saveToPly(const std::string& filename, bool saveVeins)
{        
    std::ofstream plyFile;
    plyFile.open(filename, std::ofstream::out);

    plyFile << R"(ply
format ascii 1.0
comment Created by LRDS - https://www.leeringham.com/
)";

    //bool hasTextureCoords = textureCoords_.size() > 0;

    plyFile << "element vertex " << vertices.size() << "\n";

    plyFile << "property float x\n";
    plyFile << "property float y\n";
    plyFile << "property float z\n";
    plyFile << "property float nx\n";
    plyFile << "property float ny\n";
    plyFile << "property float nz\n";
    plyFile << "property float s\n";
    plyFile << "property float t\n";
    plyFile << "property uchar red\n";
    plyFile << "property uchar green\n";
    plyFile << "property uchar blue\n";

    plyFile << "element face " << faces.size() << "\n";

    plyFile << R"(property list uchar uint vertex_indices_
end_header
)";

    // Write data
    std::vector<unsigned> nullsBefore(vertices.size(), 0);
    unsigned nulls = 0;
    size_t i = 0;
    for (Vertex* v : vertices)
    {
        if (v == nullptr)
        {
            nulls++;
            nullsBefore[i++] = nulls;
            continue;
        }
        nullsBefore[i++] = nulls;

        unsigned char r = 0, g = 0, b = 0;
        if (saveVeins && veinMorphIndex_ >= 0)
        {
            float morphVal = getReadFromCells()[v->index][veinMorphIndex_];
            r = static_cast<unsigned char>(morphVal * 255);
            g = r;
            b = r;
        }
        else
            colorMapOutside.sample(colors_[v->index].r / normCoef, r, g, b);

        Vec3 p = positions_[v->index];
        Vec3 n = normals_[v->index];
        Vec2 t = textureCoords_[v->index];
        plyFile << p.x  << " " << p.y << " " << p.z << " "
                << n.x  << " " << n.y << " " << n.z << " "
                << t.u_ << " " << t.v_ << " ";

        //if (hasTextureCoords)
        //    plyFile << t.u_ << " " << t.v_ << " ";

        plyFile << (int)r << " " << (int)g << " " << (int)b << "\n";
    }

    for (Face* f : faces)
    {
        Vertex* v0 = f->edge()->origin();
        Vertex* v1 = f->edge()->next()->origin();
        Vertex* v2 = f->edge()->next()->next()->origin();
        unsigned i0 = v0->index;
        unsigned i1 = v1->index;
        unsigned i2 = v2->index;

        plyFile << "3 ";
        plyFile << i0 - nullsBefore[i0] << " ";
        plyFile << i1 - nullsBefore[i1] << " ";
        plyFile << i2 - nullsBefore[i2] << "\n";
    }

    plyFile.close();
}

void HalfEdgeMesh::exportTexture(unsigned char* pixels, int width, int height)
{
    if (textureCoords_.size() == 0)
    {
        LOG("No texture coordinates found when saving texture!");
        return;
    }

    std::vector<int> originalClosestIndices(width * height, -1);
    std::vector<int> closestIndices(width * height, -1);
    for (HalfEdgeMesh::Face* f : faces)
    {
        unsigned i0 = f->edge()->origin()->index;
        unsigned i1 = f->edge()->next()->origin()->index;
        unsigned i2 = f->edge()->next()->next()->origin()->index;

        Vec2 t0 = textureCoords_[f->firstIndexIndex];
        Vec2 t1 = textureCoords_[f->firstIndexIndex + 1];
        Vec2 t2 = textureCoords_[f->firstIndexIndex + 2];

        int minX = static_cast<int>(Utils::min(Utils::min(t0.u_, t1.u_), t2.u_) * (width - 1));
        int minY = static_cast<int>(Utils::min(Utils::min(t0.v_, t1.v_), t2.v_) * (height - 1));
        int maxX = static_cast<int>(Utils::max(Utils::max(t0.u_, t1.u_), t2.u_) * (width - 1));
        int maxY = static_cast<int>(Utils::max(Utils::max(t0.v_, t1.v_), t2.v_) * (height - 1));

        float c0 = colors_[i0].r;
        float c1 = colors_[i1].r;
        float c2 = colors_[i2].r;

        float u = 0.f, v = 0.f, w = 0.f;
        unsigned char r = 0, g = 0, b = 0;
        int j = 0;
        for (int x = minX; x <= maxX; ++x)
        {
            for (int y = minY; y <= maxY; ++y)
            {
                j = x + (height - 1 - y) * width;
                if (pointInTriangle(
                    Vec3(t0.x_, t0.y_, 0.f),
                    Vec3(t1.x_, t1.y_, 0.f),
                    Vec3(t2.x_, t2.y_, 0.f),
                    Vec3(x / (width - 1.f), y / (height - 1.f), 0.f),
                    u, v, w))
                {
                    colorMapOutside.sample((c0 * w + c1 * u + c2 * v) / normCoef, r, g, b);
                    pixels[j * 3] = r;
                    pixels[j * 3 + 1] = g;
                    pixels[j * 3 + 2] = b;
                    originalClosestIndices[j] = j * 3;
                }
            }
        }
    }

    auto checkAdjPixel = [](int p_i, int x, int y, int x1, int y1, int width, int height, std::vector<int>& closestIndices, std::vector<int>& originalClosestIndices, float& foundDist)
    {
        if (x1 < 0 || x1 >= width || y1 < 0 || y1 >= height)
            return;

        int index = (x1 + y1 * width);
        if (originalClosestIndices[index] != -1)
        {
            float dx = float(x - x1);
            float dy = float(y - y1);
            float dist = sqrt(dx * dx + dy * dy);
            if (dist < foundDist)
            {
                foundDist = dist;
                closestIndices[p_i] = index * 3;
            }
        }
    };

    // Assign colors_ to background pixels to avoid texture seams
    int x, y;
    const int order = 10;
    ASSERT(closestIndices.size() <= std::numeric_limits<size_t>::max(), "Texture too large");
    for (int p_i = 0; p_i < static_cast<int>(closestIndices.size()); ++p_i)
    {
        // Skip colored pixels
        if (originalClosestIndices[p_i] != -1)
            continue;

        // Find closest color
        x = p_i % width;
        y = p_i / width;
        float foundDist = std::numeric_limits<float>::max();
        for (int i = 1; i <= order; ++i)
        {
            int offset = (i * 2 + 1) / 2;
            for (int j = -offset; j <= offset; ++j)
            {
                checkAdjPixel(p_i, x, y, x + j, y - offset, width, height, closestIndices, originalClosestIndices, foundDist);
                checkAdjPixel(p_i, x, y, x + j, y + offset, width, height, closestIndices, originalClosestIndices, foundDist);
                checkAdjPixel(p_i, x, y, x - offset, y + j, width, height, closestIndices, originalClosestIndices, foundDist);
                checkAdjPixel(p_i, x, y, x + offset, y + j, width, height, closestIndices, originalClosestIndices, foundDist);
            }
        }
    }

    // Copy found colors_ to image
    for (size_t p_i = 0; p_i < closestIndices.size(); ++p_i)
    {
        if (closestIndices[p_i] >= 0)
        {
            pixels[p_i * 3] = pixels[closestIndices[p_i]];
            pixels[p_i * 3 + 1] = pixels[closestIndices[p_i] + 1];
            pixels[p_i * 3 + 2] = pixels[closestIndices[p_i] + 2];
        }
    }
}

void HalfEdgeMesh::projectAniVecs(const Vec3& guessVec)
{
    for(int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
    { 
        for (Face* f : faces)
        {
            Vec3 N = f->normal;
            Vec3 T = cross(N, normalize(guessVec));
            tangents[morphIndex][f->index] = normalize(cross(T, N));
            if (isGPUEnabled)
                dirtyAttributes[TANGENT_ATTRIB].indices.insert(f->index);
        }
    }
    //if (selectedCells.size() > 0)
    //{
    //    for (int j = 0; j < numMorphs_; ++j)
    //    {
    //        for (unsigned i : selectedCells)
    //        {
    //            Vec3 N = getNormal(i);
    //            Vec3 T = cross(N, normalize(guessVec));
    //            tangents[j][i] = normalize(cross(T, N));

    //            if (isGPUEnabled)
    //                dirtyAttributes[TANGENT_ATTRIB].indices_.insert(i);
    //        }
    //    }
    //}
    //else
    //{
    //    for (int j = 0; j < numMorphs_; ++j)
    //    {
    //        for (unsigned i = 0; i < getCellCount(); ++i)
    //        {
    //            Vec3 N = getNormal(i);
    //            Vec3 T = cross(N, normalize(guessVec));
    //            tangents[j][i] = normalize(cross(T, N));

    //            if (isGPUEnabled)
    //                dirtyAttributes[TANGENT_ATTRIB].indices_.insert(i);
    //        }
    //    }
    //}
}

void HalfEdgeMesh::projectAniVecs(const Vec3& guessVec, int morphIndex)
{
    for (Face* f : faces)
    {
        Vec3 N = f->normal;
        Vec3 T = cross(N, normalize(guessVec));
        tangents[morphIndex][f->index] = normalize(cross(T, N));
        if (isGPUEnabled)
            dirtyAttributes[TANGENT_ATTRIB].indices.insert(f->index);
    }

    //if (selectedCells.size() > 0)
    //{
    //    for (unsigned i : selectedCells)
    //    {
    //        Vec3 N = getNormal(i);
    //        Vec3 T = cross(N, normalize(guessVec));
    //        tangents[morphIndex][i] = normalize(cross(T, N));

    //        if (isGPUEnabled)
    //            dirtyAttributes[TANGENT_ATTRIB].indices_.insert(i);
    //    }
    //}
    //else
    //{
    //    for (unsigned i = 0; i < getCellCount(); ++i)
    //    {
    //        Vec3 N = getNormal(i);
    //        Vec3 T = cross(N, normalize(guessVec));
    //        tangents[morphIndex][i] = normalize(cross(T, N));

    //        if (isGPUEnabled)
    //            dirtyAttributes[TANGENT_ATTRIB].indices_.insert(i);
    //    }
    //}
}

void HalfEdgeMesh::projectAniVecs()
{
    for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
    {
        for (Face* f : faces)
        {
            Vec3 N = f->normal;
            Vec3 T = cross(N, tangents[morphIndex][f->index]);
            tangents[morphIndex][f->index] = normalize(cross(T, N));
            if (isGPUEnabled)
                dirtyAttributes[TANGENT_ATTRIB].indices.insert(f->index);
        }
    }
}

void HalfEdgeMesh::copyGradientToDiffusion(int morphIndex, int gradIndex)
{
    Vec3 grad;
    auto& cells = getReadFromCells();
    
    for (unsigned i = 0; i < faces.size(); ++i)
    {
        gradientFace(faces[i], gradIndex, cells, grad);
        tangents[morphIndex][faces[i]->index] = normalize(grad);
        if (isGPUEnabled)
            dirtyAttributes[TANGENT_ATTRIB].indices.insert(faces[i]->index);
    }
    calculateCotangents();
}

bool HalfEdgeMesh::validateMesh()
{
#ifdef DEBUG
    // Create vector of edges
    // All edges need a pair, next, origin, and destination
    for (Edge* e : edges)
    {
        if (e->pair() == nullptr)
        {
            std::cout << "Mesh Invalid: all edges need a pair" << std::endl;
            return true;
        }

        if (e->next() == nullptr)
        {
            std::cout << "Mesh Invalid: all edges need a next" << std::endl;
            return true;
        }

        if (e->origin() == nullptr)
        {
            std::cout << "Mesh Invalid: all edges need an origin" << std::endl;
            return true;
        }

        if (e->destination() == nullptr)
        {
            std::cout << "Mesh Invalid: all edges need a destination" << std::endl;
            return true;
        }

        if (e->face() == e->pair()->face() && e->face() != nullptr)
        {
            std::cout << "Mesh Invalid: edge and pair have the same face" << std::endl;
            return true;
        }

        // Edges cannot have same orign and destination
        if (e->origin() == e->destination())
        {
            std::cout << "Mesh Invalid: edges cannot have same orign and destination" << std::endl;
            return true;
        }
    }

    // All edges should get back to self by following next
    int maxJumps = static_cast<int>(edges.size());
    int jumps = 0;
    for (Edge* e : edges)
    {
        jumps = 0;
        Edge* n = e->next();
        while (e != n && jumps <= maxJumps)
        {
            n = n->next();
            jumps++;
        }

        if (jumps > maxJumps)
        {
            std::cout << "Mesh Invalid: could not get back to self by following next edge" << std::endl;
            return true;
        }
    }

    //if closed, all edges should reach themselfs in two jumps
    for (Edge* e : edges)
    {
        if (e != e->next()->next()->next())
        {
            LOG("Mesh is open");
        }
    }

    // All faces need an edge
    for (Face* f : faces)
    {
        if (f->edge() == nullptr)
        {
            std::cout << "Mesh Invalid: all faces need an edge" << std::endl;
            return true;
        }

        if (f->edge()->face() != f)
        {
            std::cout << "Mesh Invalid: Face's edge and edge's face do not match" << std::endl;
            return true;
        }
    }

    for (Vertex* v : vertices)
    {
        if (v == nullptr)
            continue;

        if (v != vertices[v->index])
        {
            std::cout << "Mesh Invalid: Vertex is not at the index it thinks it is" << std::endl;
            return true;
        }

        if (v->edge()->origin() != v)
        {
            std::cout << "Mesh Invalid: Vertice's edge's origin is not the vertex" << std::endl;
            return true;
        }
    }
#endif
    return false;
}
