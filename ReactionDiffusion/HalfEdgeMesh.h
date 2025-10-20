#pragma once
#include "SimulationDomain.h"
#include "Ply.h"
#include "BVH.h"
#include "Animation.h"

#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>


class HalfEdgeMesh : 
    public SimulationDomain
{
public:
    struct Edge;
    struct Vertex;
    using EdgeKey = uint64_t;

    struct Face
    {
    private:
        Edge* edge_;
        bool dirty_ = true;

    public:
        Vec3 normal;
        Vec3 circumcentre;
        std::vector<Vec3> veclambda1, veclambda2;
        float area = 0.f;
        unsigned firstIndexIndex = 0;
        unsigned index = 0;

        bool dirty() { return dirty_; }
        void clearDirty() { dirty_ = false; }
        Edge* edge() { return edge_; }
        void setEdge(Edge* e)
        {
            dirty_ = true;
            edge_ = e;
        }

        Face(Edge* edge = nullptr, float area = 0, Vec3 normal = Vec3(0, 0, 0), Vec3 circumcentre = Vec3(0, 0, 0))
            : edge_(edge), normal(normal), circumcentre(circumcentre), area(area), firstIndexIndex(0)
        {}
    };

    struct Edge
    {
    private:
        Vertex* origin_ = nullptr;
        Vertex* destination_ = nullptr;
        Edge* next_ = nullptr;
        Edge* pair_ = nullptr;
        Face* face_ = nullptr;
        bool dirty_ = true;

    public:
        float angle = 0.f;
        std::vector<float> cotacotb;
        std::vector<Vec3> diffVec;
        std::vector<Vec3> nranVec;

        EdgeKey key = 0;
		float cotan = 0.f;
        unsigned index = 0;
        bool isBoundary = false;

        bool dirty() { return dirty_; }
        void clearDirty() { dirty_ = false; }
        Vertex* origin() { return origin_; }
        Vertex* destination() { return destination_; }
        Edge* next() { return next_; }
        Edge* pair() { return pair_; }
        Face* face() { return face_; }

        void setOrigin(Vertex* v)
        {
            origin_ = v;
            dirty_ = true;
        }
        void setDestination(Vertex* v)
        {
            destination_ = v;
            dirty_ = true;
        }
        void setNext(Edge* e)
        {
            next_ = e;
            dirty_ = true;
        }
        void setPair(Edge* e)
        {
            pair_ = e;
            dirty_ = true;
        }
        void setFace(Face* f)
        {
            face_ = f;
            dirty_ = true;
        }
    };

    struct Vertex
    {
    private:
        Edge* edge_ = nullptr;
        bool dirty_ = true;

    public:
        bool dirty() { return dirty_; }
        void clearDirty() { dirty_ = false; }
        Edge* edge() { return edge_; }

        void setEdge(Edge* e)
        {
            dirty_ = true;
            edge_ = e;
        }

        std::vector<float> cotacotb;
        float area = 0.f;
        unsigned index = 0;
        bool isBoundary = false;

        Vertex(unsigned index = 0, Edge * edge = nullptr, float area = 0, bool isBoundary = true) :
            edge_(edge), area(area), isBoundary(isBoundary), index(index)
        {}
    };

    // Inhereted methods
    void laplacian(unsigned i, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap) override;
	void laplacianCLE(unsigned i, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap, std::vector<float>::iterator lapNoise) override;
	void computeVecLambdas();
    void computeVecLambda(Face* f);
	void generateRandomNumbers(unsigned i) override;
    void gradient(unsigned i, int morphIndex, std::vector<Cell>& readFromCells, Vec3& grad) override;
    bool isBoundary(unsigned i) override;
    std::vector<unsigned> getNeighbours(unsigned i, unsigned order = 1) override;
    std::vector<unsigned> getNeighboursByRadius(unsigned i, float radius);
    std::vector<unsigned> getFaceNeighboursByRadius(unsigned i, float radius);
    std::set<unsigned> getBoundaryCellsIndices() override;
    std::vector<Edge*> getEdgesByRadius(unsigned i, float radius);
    bool hideAnisoVec(int i) const override;
    void gradientFace(Face* f, int morphIndex, std::vector<Cell>& readFromCells, Vec3& faceGrad) const;
    void growAndSubdivide(Vec3& growth, float maxFaceArea, bool subdivisionEnabled, size_t stepCount) override;
    float getTotalArea() const override;
    float getCotanWeights(unsigned v0, unsigned v1, int morphIndex);
    float getEdgeCotanWeight(unsigned edgeIndex, int morphIndex);
    float getArea(unsigned v) const override;
    Vec3 getPosition(unsigned i) const override;
    Vec3 getNormal(unsigned) const override;
    void updateDiffusionCoefs() override;
    void doInit(int numMorphs) override;
    void doRecalculateParameters() override;
    void exportTexture(unsigned char* pixels, int width, int height) override;    
    void updateGradLines() override;
    void updateDiffDirLines() override;

    void projectAniVecs(const Vec3& guessVec) override;
    void projectAniVecs(const Vec3& guessVec, int morphIndex) override;
    void projectAniVecs() override;
    void copyGradientToDiffusion(int morphIndex, int gradIndex) override;
    void clearDiffusion(int morphIndex) override;
    void paint(int faceIndex, Vec3 p, float paintRadius) override;

    HalfEdgeMesh();
    HalfEdgeMesh(const Drawable& drawable);
    virtual ~HalfEdgeMesh() = default;
    void init(const Drawable& drawable);
    bool isInitialized();
    void printInfo();
    bool validateMesh();
    void saveToObj(const std::string& filename);
    void saveToPly(const std::string& filename, bool saveVeins = false);

    // Creation
    Vertex* createVertex(const Vec3& pos);
    Vertex* createVertex(const Vec3& pos, const unsigned index);
    Edge* createEdge(Vertex* v0, Vertex* v1);
    Face* createFace(Edge* e01, Edge* e12, Edge* e20);
    void createBoundaryEdges();

    // Query
    bool vertexExists(unsigned i0);
    bool edgeExists(Vertex* v0, Vertex* v1);
    bool edgeExists(uint32_t i0, uint32_t i1);

    // Association
    void join(Edge* e0, Edge* e1, Edge* e2, Face* f);
    void associateNeighbours(unsigned i0, unsigned i1);
    void associateNeighbours(Edge* e);
    EdgeKey edgeKey(uint32_t a, uint32_t b);

    // Mesh Calculations
    Vec3 calculateCentre(Face* f);
    Vec3 calculateCentre(unsigned i0, unsigned i1, unsigned i2);
    void calculateAngles();
    void calculateDualAreas();
    void calculateDualArea(unsigned i);
    void calculateDualArea(Vertex* v);
    void calculateFaceAreas();
    void calculateFaceArea(Face* f);
    void calculateCotangents();
    void calculateCotangent(Vertex* v, const int morphIndex);
    void calculateNormals();
    void calculateVertexNormal(Vertex* v);
    void calculateVertexNormals();
    void calculateFaceNormals();
    void calculateFaceNormal(Face* f);
    void calculateBoundaryVertices();

    void precomputeGradCoef(Face* f);
    void precomputeGradCoefs();
    std::vector<float> gaussianCurvatures() const;

    int raycast(const Vec3& dir, const Vec3& origin, float& t0) const override;
    float avgFaceSize();
    Edge* splitEdge(Edge* e);
    Vertex* splitFaceAt(Face* f, Edge* e20);
    void subdivideFace(Face* f);

    // Getters
    Vertex* getVertex(unsigned i);
    Edge* getEdge(unsigned i0, unsigned i1);
    Edge* getEdge(Vertex* v0, Vertex* v1);
    float getArea(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& centre);
    void getNeighbours(unsigned order, std::vector<unsigned>& visit, std::vector<unsigned>& visited, std::vector<unsigned>& neighbours);
    Vec3 getFaceNormal(unsigned i);
    Vec3 getFaceCentroid(Face* f);
    float getFaceMorph(Face * f, int morphIndex);
    Vec3 getFaceTangent(Face* f, int morphIndex);
    Edge* getLongestEdge(Face* f);

    // Setters
    void setBoundaryColor(float r, float g, float b);

    // Members
    std::vector<Edge*> edges;
    std::vector<Face*> faces;
    std::vector<Vertex*> vertices;
    std::unordered_map<EdgeKey, int> edgeKeyIndexMap;
    std::unordered_map<Face*, std::pair<Vec3, Vec3>> gradCoefs;

    bool initialized_ = false;
    bool thirdArea_ = true;
    std::vector<Vec3> prevP_;
    BVH bvh_;
    Animation animation_;
};