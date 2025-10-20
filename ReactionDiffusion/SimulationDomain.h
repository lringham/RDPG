#pragma once
#include "Drawable.h"
#include "Lines.h"
#include "ColorMap.h"
#include "Nran.h"
#include "LOG.h"
#include "Animation.h"
#include "Textures.h"

#include <vector>
#include <set>


#define DIFFUSION_EQNS 1 // 0 - for diffusion equations based on Andreux et al. 2014, 1 - for new equations from Ringham et al. 2021
// FIXME NOTE: this define is not a good way to switch between equations. It is sometimes necessary to rebuild solution if you change the define value. 

struct PrepatternInfo
{
    int targetIndex = 0;
    int sourceIndex = 0;
    float lowVal = 1.f;
    float highVal = 1.f;
    float exponent = 1.f;
    bool enabled = false;
    bool squared = false;
};

class SimulationDomain 
    : public Drawable
{
public:
    enum class DomainType
    {
        MESH, GRID, NONE
    };

    bool isDomainType(DomainType type) const
    {
        return type == domainType;
    }

    std::string domainTypeStr() const
    {
        if (isDomainType(DomainType::MESH))
            return "mesh";
        else if (isDomainType(DomainType::GRID))
            return "grid";
        else if (isDomainType(DomainType::NONE))
            return "none";
        return "unknown";
    }

    struct Cell
    {
        std::vector<float> vals;
        std::vector<bool> isConstVal;

        Cell() {};

        explicit Cell(int morphCount)
            : vals(morphCount, 0.f), isConstVal(morphCount, false)
        {}

        float operator[](int i) const
        {
            return vals[i];
        }

        float& operator[](int i)
        {
            return vals[i];
        }
    };

    struct NewCell
    {
        std::vector<unsigned> neighbours;
    };

    struct DiffusionTensor
    {
        std::vector<float> t0_;
        std::vector<float> t1_;

        explicit DiffusionTensor(int morphCount)
            : t0_(morphCount, 1.f), t1_(morphCount, 1.f)
        {}

        DiffusionTensor() = default;
        
        void resize(size_t size, float value = 1.f)
        {
            t0_.resize(size, value);
            t1_.resize(size, value);
        }

        std::tuple<float, float> getTensor(size_t i) const
        {
            return std::make_tuple(t0_[i], t1_[i]);
        }
    };

    SimulationDomain() = default;
    virtual ~SimulationDomain() = default;

    virtual void laplacian(unsigned i, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap) = 0;
	virtual void laplacianCLE(unsigned i, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap, std::vector<float>::iterator lapNoise) = 0;
	virtual void generateRandomNumbers(unsigned i) = 0;
    virtual void gradient(unsigned i, int morphIndex, std::vector<Cell>& readFromCells, Vec3& grad) = 0;
    virtual bool isBoundary(unsigned i) = 0;
    virtual std::set<unsigned> getBoundaryCellsIndices() = 0;
    virtual std::vector<unsigned> getNeighbours(unsigned i, unsigned order) = 0;
    virtual void growAndSubdivide(Vec3& growth, float maxFaceArea, bool subdivisionEnabled, size_t stepCount) = 0;
    virtual Vec3 getPosition(unsigned i) const = 0;
    virtual Vec3 getNormal(unsigned i) const = 0;
    virtual void setDiffTensor(unsigned index, float t0, float t1, int morphIndex);
    virtual float getArea(unsigned v) const = 0;
    virtual float getTotalArea() const = 0;
    virtual void paint(int faceIndex, Vec3 p, float paintRadius) = 0;
    virtual int raycast(const Vec3& dir, const Vec3& origin, float& t0) const = 0;
    virtual void updateDiffusionCoefs() = 0;
    virtual void exportTexture(unsigned char* pixels, int width, int height) = 0;
    virtual bool hideAnisoVec(int i) const = 0;
    virtual void updateGradLines() = 0;
    virtual void updateDiffDirLines() = 0;
    virtual void projectAniVecs(const Vec3& guessVec) = 0;
    virtual void projectAniVecs(const Vec3& guessVec, int morphIndex) = 0;
    virtual void projectAniVecs() = 0;
    virtual void copyGradientToDiffusion(int morphIndex, int gradIndex) = 0;

    virtual void clearMorphogens();
    virtual void clearMorphogens(int morphIndex);
    virtual void clearDiffusion(int morphIndex);
 
    void init(int numMorphs);
    void recalculateParameters();
    void update();
    int getGrowthMode() const;
    std::vector<Cell>& getReadFromCells();
    std::vector<Cell>& getWriteToCells();
    unsigned getCellCount();
    void swap();
    void invertSelected();
    void selectBoundary();
    void selectBorder();
    void selectNeighbours();
    void setVeinMorphIndex(int morphIndex, float veinDiff, float petalDiff);
    void updateVeinValues();
    bool hasVeins() const;
    bool isPaintingDiffDir() const;
    bool isPaintingMorph() const;
    bool isSelecting() const;
    bool isPainting() const;

    // ------- Members ------------
    std::vector<Cell> cells1;
    std::vector<Cell> cells2;
    std::vector<DiffusionTensor> diffusionTensors;
    std::vector<float> L;

    bool currentCells = true;
    bool isGPUEnabled = false;
    int veinMorphIndex_ = -1;
	float veinDiffusionScale_ = 1.f;
	float petalDiffusionScale_ = 1.f;

    std::vector<bool> paintingMorph;
    std::vector<bool> paintingDiffDir;
    std::vector<bool> showingMorph;
    std::vector<bool> showingDiffDir;
    std::vector<bool> showingGrad;
    std::vector<float> clearMValue;
    std::vector<float> paintValue;

    bool selectCells = false;
    bool showSelectedCells = false;
    bool growing = false;
    bool alternateControlMode = false;

    enum class GrowthMode : int {
        AnimationGrowth = 0,
        LinearAreaGrowth = 1,
        PercentageGrowth = 2,
        MorphogenGrowth = 3
    };

    GrowthMode growthMode = GrowthMode::AnimationGrowth;

    float paintTensor = 0.f;
    float paintTensorVals[2] = { 1.f, 1.f };
    float maxM[2] = { 1.f, 1.f };

    float normCoef = 1.f;
    float backgroundThreshold_ = 0.f;
    float backgroundOffset_ = 0.f;
    float vecLength = 0.05f;
    int selectedCell = 0;
    int numMorphs_ = 0;

    Lines anisotropicDiffusionTensor;
    Lines gradientLines;
    ColorMap colorMapOutside;
    ColorMap colorMapInside;
    Textures::Texture2D background_;
    Image backgroundImage_;

    std::set<unsigned> selectedCells;
    std::unordered_map<unsigned, NewCell> cellsToUpdate;
    std::vector<std::vector<Vec3>> tangents;

    PrepatternInfo prepatternInfo0{};
    PrepatternInfo prepatternInfo1{};

    enum MeshAttributeNames
    {
        VERTEX_ATTRB, EDGE_ATTRB, FACE_ATTRB, BOUNDARY_ATTRIB, CELL1_ATTRIB, CELL2_ATTRIB, TANGENT_ATTRIB, PARAM_ATTRB, ATTRIB_COUNT
    };

    struct MeshAttributes
    {
        std::set<unsigned> indices;
        std::vector<float> attributes;

        void clear()
        {
            indices.clear();
            attributes.clear();
        }
    };
    std::map<MeshAttributeNames, MeshAttributes> dirtyAttributes;
    // ----------------------------------------------------------------------------------

protected:
    virtual void doUpdate() {};
    virtual void doInit(int numMorphs) = 0;
    virtual void doRecalculateParameters() = 0;

    DomainType domainType = DomainType::NONE;
};
