#pragma once
#include "SimulationDomain.h"
#include "ColorMap.h"

#include <vector>
#include <set>


class Grid
    : public SimulationDomain
{
public:
    struct Tensor
    {
        float dxx = 1;
        float dxy_yx = 1;
        float dyy = 1;
    };

    Grid(int xRes, int yRes, float cellSize);
    virtual ~Grid() = default;

    void insertRow(int row);
    void insertColumn(int column);
    int getXRes() const;
    int getYRes() const;
    float getCellSize() const;

    bool isBoundary(unsigned i) override;
    std::vector<unsigned> getNeighbours(unsigned i, unsigned order) override;
    std::vector<unsigned> getNeighboursByRadius(unsigned i, float radius);
    void growAndSubdivide(Vec3& growth, float maxFaceArea, bool subdivisionEnabled, size_t stepCount) override;
    void doUpdate() override;
    float getArea(unsigned i) const override;
    float getTotalArea() const override;
    Vec3 getPosition(unsigned i) const override;
    Vec3 getNormal(unsigned i) const override;
    virtual void paint(int faceIndex, Vec3 p, float paintRadius) override;
    virtual int raycast(const Vec3& dir, const Vec3& origin, float& t0) const override;
    void updateDiffusionCoefs() override;
    void updateDiffusionCoef(unsigned i, int morph);
    void exportTexture(unsigned char* pixels, int width, int height) override;
    bool hideAnisoVec(int i) const override;
    void updateGradLines() override;
    void updateDiffDirLines() override;

    void projectAniVecs(const Vec3& guessVec) override;
    void projectAniVecs(const Vec3& guessVec, int morphIndex) override;
    void projectAniVecs() override;
    void copyGradientToDiffusion(int morphIndex, int gradIndex) override;

    Textures::Texture2D morphTexture_;
    std::vector<std::vector<Tensor>> anisoCoefs_;

protected:
    void doInit(int numMorphs) override;
    void doRecalculateParameters() override;
    void laplacian(unsigned index, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap) override;
	void laplacianCLE(unsigned i, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap, std::vector<float>::iterator lapNoise) override;
	void generateRandomNumbers(unsigned i) override;
    void gradient(unsigned i, int morphIndex, std::vector<Cell>& readFromCells, Vec3& grad) override;
    std::vector<unsigned> getCellIndices(const Vec3& dir, const Vec3& origin, float radius) const;
    std::set<unsigned> getBoundaryCellsIndices() override;
    void calculateNeighbours();
    void indexToVec2(int index, int& x, int& y) const;
    int vec2ToIndex(int x, int y) const;

    int xRes_ = 0, yRes_ = 0;
    float width_ = 0, height_ = 0, cellSize_ = 0, areaScale_ = 0;

    std::vector<unsigned> neighbours_;
    Vec3 prevP_;
};

