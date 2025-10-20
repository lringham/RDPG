#include "Grid.h"
#include "GridShader.h"
#include "ResourceLocations.h"
#include "Plane.h"

#include <random>
#include <algorithm>


Grid::Grid(int xRes, int yRes, float cellSize) :
    xRes_(xRes), yRes_(yRes), cellSize_(cellSize)
{
    domainType = SimulationDomain::DomainType::GRID;
    xRes = xRes <= 0 ? 1 : xRes;
    yRes = yRes <= 0 ? 1 : yRes;
    width_ = cellSize * xRes;
    height_ = cellSize * yRes;
    areaScale_ = cellSize * cellSize;
}

void Grid::doInit(int numMorphs)
{
    if (shader_ == nullptr)
        shader_ = std::make_unique<GridShader>();

    positions_.push_back(Vec3(width_ * -.5f, height_ * -.5f, 0.f));
    normals_.push_back(Vec3(0.f, 0.f, 1.f));
    textureCoords_.push_back(Vec2(0.f, 0.f));

    positions_.push_back(Vec3(width_ * .5f, height_ * -.5f, 0.f));
    normals_.push_back(Vec3(0.f, 0.f, 1.f));
    textureCoords_.push_back(Vec2(0.f, 1.f)); // flipped because of the way opengl loads textures

    positions_.push_back(Vec3(width_ * -.5f, height_ * .5f, 0.f));
    normals_.push_back(Vec3(0.f, 0.f, 1.f));
    textureCoords_.push_back(Vec2(1.f, 0.f)); // flipped because of the way opengl loads textures

    positions_.push_back(Vec3(width_ * .5f, height_ * .5f, 0.f));
    normals_.push_back(Vec3(0.f, 0.f, 1.f));
    textureCoords_.push_back(Vec2(1.f, 1.f));

    indices_.push_back(0);
    indices_.push_back(1);
    indices_.push_back(2);
    indices_.push_back(1);
    indices_.push_back(3);
    indices_.push_back(2);

    //=========================================
    numMorphs_ = numMorphs;
    paintingMorph.resize(numMorphs, false);
    paintingDiffDir.resize(numMorphs, false);
    showingMorph.resize(numMorphs, false);
    showingDiffDir.resize(numMorphs, false);
    showingGrad.resize(numMorphs, false);
    clearMValue.resize(numMorphs, 0.1f);
    paintValue.resize(numMorphs, 0.f);
    L.resize(numMorphs * xRes_ * yRes_, 0.f);
    colors_.resize(xRes_ * yRes_, Vec4(1.f, 1.f, 1.f, 1.f));

    paintValue[0] = .1f;
    showingMorph[0] = true;

    cells1.resize(xRes_ * yRes_);
    for (auto& c : cells1)
    {
        c.vals.resize(numMorphs);
        c.isConstVal.resize(numMorphs, false);
    }

    cells2.resize(xRes_ * yRes_);
    for (auto& c : cells2)
    {
        c.vals.resize(numMorphs);
        c.isConstVal.resize(numMorphs, false);
    }

    tangents.resize(numMorphs);
    for (int i = 0; i < numMorphs; ++i)
        for (int x = 0; x < xRes_; ++x)
            for (int y = 0; y < yRes_; ++y)
                tangents[i].emplace_back(1.f, 0.f, 0.f);

    anisoCoefs_.resize(numMorphs);
    for (int i = 0; i < numMorphs; ++i)
        anisoCoefs_[i].resize(cells1.size());

    diffusionTensors.resize(numMorphs);
    for (int i = 0; i < numMorphs; ++i)
        diffusionTensors[i].resize(cells1.size(), 1.f);

    updateDiffusionCoefs();

    initVBOs();

    for (unsigned i = 0; i < getCellCount(); ++i)
    {
        Vec3 pos = getPosition(i);
        Vec3 nor = getNormal(i);
        gradientLines.addLine(pos, pos + nor, Vec3(0.f, 0.f, 0.f), Vec3(1.f, 0.f, 0.f));
    }

    // Update tensor visualization
    anisotropicDiffusionTensor.clearBuffers();
    anisotropicDiffusionTensor.destroyVBOs();
    for (unsigned i = 0; i < getCellCount(); ++i)
    {
        anisotropicDiffusionTensor.addLine(
            Vec3(0, 0, 0), Vec3(0, 0, 0),
            Vec3(0, 0, 0), Vec3(0, 1, 0));
    }
    anisotropicDiffusionTensor.finalize();

    calculateNeighbours();

    morphTexture_ = Textures::create2DTexture(colors_, yRes_, xRes_, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_RGBA, GL_RGBA32F, GL_FLOAT);
    shader_->setTexture("morphogensTexture", morphTexture_.id());
}

void Grid::calculateNeighbours()
{
    if (isGPUEnabled)
        return;

    neighbours_.clear();
    int x, y;
    const unsigned NUM_CELLS = getCellCount();
    for (unsigned i = 0; i < NUM_CELLS; ++i)
    {
        indexToVec2(i, x, y);
        int left = x - 1 >= 0;
        int right = x + 1 != xRes_;
        int up = y + 1 != yRes_;
        int down = y - 1 >= 0;

        int i_x1y0, i_x1y2;
        i_x1y0 = i - down;
        i_x1y2 = i + up;

        int i_x0y0, i_x0y1, i_x0y2;
        i_x0y0 = i_x1y0 - yRes_ * left;
        i_x0y1 = i - yRes_ * left;
        i_x0y2 = i_x1y2 - yRes_ * left;

        int i_x2y0, i_x2y1, i_x2y2;
        i_x2y0 = i_x1y0 + right * yRes_;
        i_x2y1 = i + right * yRes_;
        i_x2y2 = i_x1y2 + right * yRes_;


        neighbours_.push_back(i_x0y0);
        neighbours_.push_back(i_x0y1);
        neighbours_.push_back(i_x0y2);

        neighbours_.push_back(i_x1y0);
        neighbours_.push_back(i_x1y2);

        neighbours_.push_back(i_x2y0);
        neighbours_.push_back(i_x2y1);
        neighbours_.push_back(i_x2y2);
    }

}

void Grid::indexToVec2(int index, int& x, int& y) const
{
    x = index / yRes_;
    y = index % yRes_;
}

int Grid::vec2ToIndex(int x, int y) const
{
    return y + x * yRes_;
}

void Grid::doRecalculateParameters()
{
    updateDiffusionCoefs();
}

void Grid::insertRow(int rowIndex)
{
    // update rect size
    height_ += cellSize_;
    positions_.clear();
    positions_.push_back(Vec3(width_ * -.5f, height_ * -.5f, 0.f));
    positions_.push_back(Vec3(width_ * .5f, height_ * -.5f, 0.f));
    positions_.push_back(Vec3(width_ * -.5f, height_ * .5f, 0.f));
    positions_.push_back(Vec3(width_ * .5f, height_ * .5f, 0.f));
    updatePositionVBO();

    // create a row
    std::vector<Cell> row(xRes_);
    for (auto& r : row)
    {
        r.vals.resize(numMorphs_);
        //r.t0.resize(numMorphs_); // FIX ME
        //r.t1.resize(numMorphs_);
        r.isConstVal.resize(numMorphs_);
    }

    // copy adjacent row
    std::vector<int> affectedRowIndices;
    std::vector<Cell>& cells = getReadFromCells();
    for (int i = 0; i < xRes_; ++i)
    {
        auto offset = i * yRes_ + rowIndex;
        affectedRowIndices.push_back(offset + int(affectedRowIndices.size()));

        for (int j = 0; j < numMorphs_; ++j)
        {
            row[i].vals[j] = cells[offset].vals[j];
            //row[i].t0[j] = cells[offset].t0[j]; // FIX ME
            //row[i].t1[j] = cells[offset].t1[j];
            row[i].isConstVal[j] = cells[offset].isConstVal[j];
        }
    }

    // add row to current cells
    yRes_++;
    unsigned loc = rowIndex;
    for (int c = 0; c < xRes_; ++c)
    {
        cells1.insert(begin(cells1) + loc, begin(row) + c, begin(row) + c + 1);
        cells2.insert(begin(cells2) + loc, begin(row) + c, begin(row) + c + 1);

        for (int j = 0; j < numMorphs_; ++j)
        {
            tangents[j].insert(
                begin(tangents[j]) + loc,
                1,
                tangents[j][loc]
            );
        }

        colors_.push_back(Vec4(1.f, 0.f, 0.f, 1.f));

        Vec3 pos = getPosition(loc + 1);
        gradientLines.addLine(
            pos,
            pos + tangents[0][loc] * vecLength,
            Vec3(0, 0, 0), Vec3(1, 0, 0));

        anisotropicDiffusionTensor.addLine(
            pos,
            pos + tangents[0][loc] * vecLength,
            Vec3(0, 0, 0), Vec3(0, 1, 0));

        loc += yRes_;
    }
    {
        int offset = 0;
        for (int x = xRes_ - 1; x >= 0; --x)
        {
            for (int y = yRes_ - 1; y >= 0; --y)
            {
                int i = vec2ToIndex(x, y);
                if (std::find(begin(affectedRowIndices), end(affectedRowIndices), i) != end(affectedRowIndices))
                {
                    offset++;
                    if (offset != xRes_)
                    {
                        NewCell nc;
                        nc.neighbours.push_back(i - xRes_ + offset);
                        cellsToUpdate[i] = nc;
                    }
                }

                else if (offset != xRes_)
                {
                    NewCell nc;                    
                    nc.neighbours.push_back(i - xRes_ + offset);
                    cellsToUpdate[i] = nc;
                }
            }
        }
    }

    L.resize(numMorphs_ * xRes_ * yRes_, 0.f);

    anisoCoefs_.resize(numMorphs_);
    for (int i = 0; i < numMorphs_; ++i)
        anisoCoefs_[i].resize(cells1.size());

    // Reinit OpenGL arrays
    gradientLines.destroyVBOs();
    gradientLines.initVBOs();
    anisotropicDiffusionTensor.destroyVBOs();
    anisotropicDiffusionTensor.initVBOs();
    destroyVBOs();
    initVBOs();

    // update diffusion coefs
    updateDiffusionCoefs();
    updateDiffDirLines();
}

void Grid::insertColumn(int columnIndex)
{
    // update dimensions and grid render size
    width_ += cellSize_;
    positions_.clear();
    positions_.push_back(Vec3(width_ * -.5f, height_ * -.5f, 0.f));
    positions_.push_back(Vec3(width_ * .5f, height_ * -.5f, 0.f));
    positions_.push_back(Vec3(width_ * -.5f, height_ * .5f, 0.f));
    positions_.push_back(Vec3(width_ * .5f, height_ * .5f, 0.f));
    updatePositionVBO();
    std::vector<Cell>& cells = getReadFromCells();

    // create column
    std::vector<Cell> column(yRes_);
    for (auto& c : column)
    {
        c.vals.resize(numMorphs_);
        //c.t0.resize(numMorphs_); // FIX ME
        //c.t1.resize(numMorphs_);
        c.isConstVal.resize(numMorphs_);
    }

    // copy column
    xRes_++;
    std::vector<int> affectedColIndices;
    for (int i = 0; i < yRes_; ++i)
    {
        auto offset = columnIndex * yRes_ + i;
        affectedColIndices.push_back(offset);

        for (int j = 0; j < numMorphs_; ++j)
        {
            tangents[j].insert(
                begin(tangents[j]) + offset,
                1,
                tangents[j][offset]
            );
        }

        Vec3 pos = getPosition(xRes_ * yRes_ - i - 1);
        gradientLines.addLine(
            pos,
            pos + tangents[0][offset] * vecLength,
            Vec3(0, 0, 0), Vec3(1, 0, 0));

        anisotropicDiffusionTensor.addLine(
            pos,
            pos + tangents[0][offset] * vecLength,
            Vec3(0, 0, 0), Vec3(0, 1, 0));

        for (int j = 0; j < numMorphs_; ++j)
        {
            column[i].vals[j] = cells[offset].vals[j];
            //column[i].t0[j] = cells[offset].t0[j]; // FIX ME
            //column[i].t1[j] = cells[offset].t1[j];
            column[i].isConstVal[j] = cells[offset].isConstVal[j];
        }
    }

    // add row to current cells
    cells1.insert(cells1.begin() + columnIndex * yRes_, column.begin(), column.end());
    cells2.insert(cells2.begin() + columnIndex * yRes_, column.begin(), column.end());

    {
        bool updating = true;
        for (int x = xRes_ - 1; x >= 0 && updating; --x)
        {
            for (int y = yRes_ - 1; y >= 0 && updating; --y)
            {
                int i = vec2ToIndex(x, y);
                updating = std::find(begin(affectedColIndices), end(affectedColIndices), i) == end(affectedColIndices);

                if (updating)
                {
                    NewCell nc;
                    nc.neighbours.push_back(i - yRes_);
                    cellsToUpdate[i] = nc;
                }
            }
        }
    }

    L.resize(numMorphs_ * xRes_ * yRes_, 0.f);
    colors_.resize(getCellCount(), Vec4(1, 1, 1, 1));
    anisoCoefs_.resize(numMorphs_);
    for (int i = 0; i < numMorphs_; ++i)
        anisoCoefs_[i].resize(cells1.size());

    // Reinit OpenGL arrays
    gradientLines.destroyVBOs();
    gradientLines.initVBOs();
    anisotropicDiffusionTensor.destroyVBOs();
    anisotropicDiffusionTensor.initVBOs();
    destroyVBOs();
    initVBOs();

    // update diffusion coefs
    updateDiffusionCoefs();
    updateDiffDirLines();
}

int Grid::getXRes() const
{
    return xRes_;
}

int Grid::getYRes() const
{
    return yRes_;
}

float Grid::getCellSize() const
{
    return cellSize_;
}

void Grid::laplacian(unsigned index, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap)
{
    for (size_t i = 0; i < numMorphs_; ++i)
        *(lap + i) = 0;

    Tensor t{};
    float m = 0;
    int i = index * 8;

    float invAreaScale = 1.f / areaScale_;
    for (int morph = 0; morph < numMorphs_; ++morph)
    {
        t = anisoCoefs_[morph][index];
        m = -2.f * readFromCells[index][morph] * (t.dxx + t.dyy);

        // left column
        m -= readFromCells[neighbours_[i]][morph] * t.dxy_yx;
        m += readFromCells[neighbours_[i + 1]][morph] * t.dxx;
        m += readFromCells[neighbours_[i + 2]][morph] * t.dxy_yx;

        // middle column
        m += readFromCells[neighbours_[i + 3]][morph] * t.dyy;
        m += readFromCells[neighbours_[i + 4]][morph] * t.dyy;

        // right column
        m += readFromCells[neighbours_[i + 5]][morph] * t.dxy_yx;
        m += readFromCells[neighbours_[i + 6]][morph] * t.dxx;
        m -= readFromCells[neighbours_[i + 7]][morph] * t.dxy_yx;

        *(lap + morph) = m * invAreaScale;
    }
}

void Grid::laplacianCLE(unsigned index, const std::vector<Cell>& readFromCells, std::vector<float>::iterator lap, std::vector<float>::iterator lapNoise)
{
	// TODO: finish implementation!!! Get code from old branch!
	for (size_t i = 0; i < numMorphs_; ++i) {
		*(lap + i * getCellCount()) = 0;
		*(lapNoise + i * getCellCount()) = 0;
	}

	Tensor t{};
	float m = 0.f;
	int i = index * 8;

	float invAreaScale = 1.f / areaScale_;
	for (int morph = 0; morph < numMorphs_; ++morph)
	{
		t = anisoCoefs_[morph][index];
		m = -2.f * readFromCells[index][morph] * (t.dxx + t.dyy);

		// left column
		m -= readFromCells[neighbours_[i]][morph] * t.dxy_yx;
		m += readFromCells[neighbours_[i + 1]][morph] * t.dxx;
		m += readFromCells[neighbours_[i + 2]][morph] * t.dxy_yx;

		// middle column
		m += readFromCells[neighbours_[i + 3]][morph] * t.dyy;
		m += readFromCells[neighbours_[i + 4]][morph] * t.dyy;

		// right column
		m += readFromCells[neighbours_[i + 5]][morph] * t.dxy_yx;
		m += readFromCells[neighbours_[i + 6]][morph] * t.dxx;
		m -= readFromCells[neighbours_[i + 7]][morph] * t.dxy_yx;

		*(lap + morph * getCellCount()) = m * invAreaScale;
	}
}

void Grid::generateRandomNumbers(unsigned)
{
	// TODO: implement
}

void Grid::gradient(unsigned i11, int morphIndex, std::vector<Cell>& readFromCells, Vec3& grad)
{
    grad.set(0, 0, 0);
    int x, y;
    indexToVec2(i11, x, y);

    int xCoords[2]{ (x - 1 < 0 ? x : x - 1), (x + 1 == xRes_ ? x : x + 1) };
    int yCoords[2]{ (y - 1 < 0 ? y : y - 1), (y + 1 == yRes_ ? y : y + 1) };


    int i00 = vec2ToIndex(xCoords[0], yCoords[0]);
    int i01 = vec2ToIndex(xCoords[0], y);
    int i02 = vec2ToIndex(xCoords[0], yCoords[1]);

    int i10 = vec2ToIndex(x, yCoords[0]);
    int i12 = vec2ToIndex(x, yCoords[1]);

    int i20 = vec2ToIndex(xCoords[1], yCoords[0]);
    int i21 = vec2ToIndex(xCoords[1], y);
    int i22 = vec2ToIndex(xCoords[1], yCoords[1]);

    grad.x -= readFromCells[i00][morphIndex];
    grad.x -= readFromCells[i01][morphIndex] * 2.f;
    grad.x -= readFromCells[i02][morphIndex];

    grad.x += readFromCells[i20][morphIndex];
    grad.x += readFromCells[i21][morphIndex] * 2.f;
    grad.x += readFromCells[i22][morphIndex];

    grad.y -= readFromCells[i00][morphIndex];
    grad.y -= readFromCells[i10][morphIndex] * 2.f;
    grad.y -= readFromCells[i20][morphIndex];

    grad.y += readFromCells[i02][morphIndex];
    grad.y += readFromCells[i12][morphIndex] * 2.f;
    grad.y += readFromCells[i22][morphIndex];

    if (showingGrad[morphIndex])
    {
        Vec3 pos = getPosition(i11);
        gradientLines.positions_[i11 * 2] = pos;
        gradientLines.positions_[i11 * 2 + 1] = pos + normalize(grad) * vecLength;
    }
}

bool Grid::isBoundary(unsigned i)
{
    int x, y;
    indexToVec2(i, x, y);
    return x == 0 || x == xRes_ - 1 || y == 0 || y == yRes_ - 1;
}

std::vector<unsigned> Grid::getNeighbours(unsigned i, unsigned order)
{
    int order_s = (int)order;
    int x, y;
    indexToVec2(i, x, y);
    std::vector<unsigned> foundNeighbours;

    int x0 = x - order_s;
    if (x0 < 0)
        x0 = 0;

    for (; x0 < x + order_s; ++x0)
    {
        int y0 = y - order_s;
        if (y0 < 0)
            y0 = 0;
        for (; y0 < y + order_s; ++y0)
        {
            if (x0 < xRes_ && y0 < yRes_ && (x0 != x || y0 != y))
                foundNeighbours.push_back(vec2ToIndex(x0, y0));
        }
    }

    return foundNeighbours;
}

std::vector<unsigned> Grid::getNeighboursByRadius(unsigned i, float radius)
{
    int x_i, y_i;
    std::vector<unsigned> foundNeighbours;
    indexToVec2(i, x_i, y_i);
    radius /= cellSize_;

    int x0 = x_i - static_cast<int>(radius);
    int y0 = y_i - static_cast<int>(radius);
    x0 = x0 < 0 ? 0 : x0;
    y0 = y0 < 0 ? 0 : y0;

    int x1 = x_i + static_cast<int>(radius);
    int y1 = y_i + static_cast<int>(radius);
    x1 = x1 >= xRes_ ? xRes_ - 1 : x1;
    y1 = y1 >= yRes_ ? yRes_ - 1 : y1;

    for (int x = x0; x <= x1; ++x)
    {
        for (int y = y0; y <= y1; ++y)
        {
            if (length(Vec2(float(x - x_i), float(y - y_i))) <= radius)
                foundNeighbours.push_back(vec2ToIndex(x, y));
        }
    }

    return foundNeighbours;
}

void Grid::growAndSubdivide(Vec3& growth, float, bool, size_t)
{
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> wDis(0, xRes_ - 1);
    std::uniform_int_distribution<> hDis(0, yRes_ - 1);
    std::uniform_real_distribution<float> dist(0.f, std::nextafter(1.f, std::numeric_limits<float>::max()));

    bool growHappened = false;

    if (growth.x >= dist(rd))
    {
        insertColumn(wDis(rd));
        growHappened = true;
    }

    if (growth.y >= dist(rd))
    {
        insertRow(hDis(rd));
        growHappened = true;
    }

    if (growHappened)
        calculateNeighbours();
}

std::vector<unsigned> Grid::getCellIndices(const Vec3&, const Vec3& origin, float radius) const
{
    std::vector<unsigned> foundIndices;
    Vec3 loc = Vec3(origin.x, origin.y, 0);

    for (int x = 0; x < xRes_; ++x)
    {
        for (int y = 0; y < yRes_; ++y)
        {
            Vec3 p = Vec3((float)x, (float)y, 0.f) - Vec3((float)xRes_ * .5f, (float)yRes_ * .5f, 0.f);
            if (length(p - loc) <= radius)
                foundIndices.push_back(vec2ToIndex(x, y));
        }
    }
    return foundIndices;
}

void Grid::doUpdate()
{
    morphTexture_ = Textures::create2DTexture(colors_, yRes_, xRes_, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_RGBA, GL_RGBA32F, GL_FLOAT);
    shader_->setTexture("morphogensTexture", morphTexture_.id());
}

float Grid::getArea(unsigned) const
{
    return areaScale_;
}

float Grid::getTotalArea() const
{
    return width_ * height_;
}

Vec3 Grid::getPosition(unsigned i) const
{
    int x = 0, y = 0;
    indexToVec2(i, x, y);

    float dx = (float)width_ / (float)xRes_;
    float dy = (float)height_ / (float)yRes_;
    float xMag = dx * (float)x + dx * .5f;
    float yMag = dy * (float)y + dy * .5f;

    return positions_[0] + normalize(positions_[1] - positions_[0]) * xMag + normalize(positions_[2] - positions_[0]) * yMag;
}

Vec3 Grid::getNormal(unsigned) const
{
    return Vec3(0, 0, 1);
}

void Grid::paint(int cellIndex, Vec3 p, float paintRadius)
{
    auto n = getNeighboursByRadius(cellIndex, paintRadius);

    int morphPaintIndex = -1;
    for (int i = 0; i < numMorphs_; ++i)
        if (paintingDiffDir[i])
            morphPaintIndex = i;

    if (morphPaintIndex >= 0)
    {
        for (auto i : n)
        {
            float s = length(p - getPosition(i)) / paintRadius;
            Vec3 N = getNormal(i);
            Vec3 T = normalize(p - prevP_);
            T = T.cross(N);
            T = N.cross(T);
            if (p.length() != 0 && T.length() != 0)
            {
                tangents[morphPaintIndex][i] = normalize(s * tangents[morphPaintIndex][i] + (1.f - s) * T);
                setDiffTensor(i, paintTensorVals[0], paintTensorVals[1], morphPaintIndex);
                updateDiffusionCoef(i, morphPaintIndex);
            }

            if (isGPUEnabled)
            {
                dirtyAttributes[TANGENT_ATTRIB].indices.insert(i);
                dirtyAttributes[VERTEX_ATTRB].indices.insert(i);
            }
        }
    }
    else if (selectCells)
    {
        if (alternateControlMode)
            for (auto i : n)
                selectedCells.erase(i);
        else
            for (auto i : n)
                selectedCells.insert(i);
    }
    else
    {
        for (int j = 0; j < numMorphs_; ++j)
        {
            if (alternateControlMode)
            {
                for (auto i : n)
                    if (paintingMorph[j])
                    {
                        float val = clearMValue[j];
                        cells1[i][j] = val;
                        cells2[i][j] = val;
                    }
            }
            else
            {
                for (auto i : n)
                    if (paintingMorph[j])
                    {
                        float val = clearMValue[j];
                        cells1[i][j] += val;
                        cells2[i][j] += val;

                        if (cells1[i][j] < 0.f)
                            cells1[i][j] = 0.f;

                        if (cells2[i][j] < 0.f)
                            cells2[i][j] = 0.f;
                    }
            }
        }

        for (auto i : n)
        {
            if (isGPUEnabled)
            {
                dirtyAttributes[CELL1_ATTRIB].indices.insert(i);
                dirtyAttributes[CELL2_ATTRIB].indices.insert(i);
            }
        }
    }
    selectedCell = cellIndex;
    prevP_ = p;
}

int Grid::raycast(const Vec3& dir, const Vec3& origin, float& t0) const
{
    t0 = std::numeric_limits<float>::max();
    Vec3 origModel = (transform_.getInvTransformMat() * Vec4(origin)).xyz();
    Vec3 dirModel = (transform_.getInvTransformMat() * Vec4(dir)).xyz();

    bool hit = raycastPlane(origModel, dirModel, positions_[0], normals_[0], &t0);

    if (hit)
    {
        Vec3 hitPos = origModel + dirModel * t0;
        hitPos.x += width_ * .5f;
        hitPos.y += height_ * .5f;

        if (hitPos.x >= 0 && hitPos.x <= width_ && hitPos.y >= 0 && hitPos.y <= height_)
        {
            int x = int(hitPos.x / cellSize_);
            int y = int(hitPos.y / cellSize_);

            return vec2ToIndex(x, y);
        }
    }
    return -1;
}

std::set<unsigned> Grid::getBoundaryCellsIndices()
{
    std::set<unsigned> foundIndices;

    for (int x = 0; x < xRes_; ++x)
    {
        foundIndices.insert(vec2ToIndex(int(x), 0));
        foundIndices.insert(vec2ToIndex(int(x), int(height_) - 1));
    }

    for (int y = 0; y < yRes_; ++y)
    {
        foundIndices.insert(vec2ToIndex(0, y));
        foundIndices.insert(vec2ToIndex(int(height_) - 1, y));
    }

    return foundIndices;
}

void Grid::updateDiffusionCoefs()
{
    for (unsigned i = 0; i < getCellCount(); ++i)
        for (int morph = 0; morph < numMorphs_; ++morph)
            updateDiffusionCoef(i, morph);
}

void Grid::updateDiffusionCoef(unsigned i, int morph)
{
    float sigmaLow = diffusionTensors[morph].t0_[i];
    float sigmaLow2 = sigmaLow * sigmaLow;
    float sigmaHigh = diffusionTensors[morph].t1_[i];
    float sigmaHigh2 = sigmaHigh * sigmaHigh;

    float angle = atan2f(tangents[morph][i].y, tangents[morph][i].x);

    float cosTheta = cos(angle);
    float cosTheta2 = cosTheta * cosTheta;
    float sinTheta = sin(angle);
    float sinTheta2 = sinTheta * sinTheta;

    anisoCoefs_[morph][i].dxx = (sigmaLow2 * cosTheta2 + sigmaHigh2 * sinTheta2);
    anisoCoefs_[morph][i].dxy_yx = (sigmaHigh2 - sigmaLow2) * cosTheta * sinTheta * 0.5f;
    anisoCoefs_[morph][i].dyy = (sigmaHigh2 * cosTheta2 + sigmaLow2 * sinTheta2);
}

bool Grid::hideAnisoVec(int) const
{
    return false;
}

void Grid::updateGradLines()
{
    for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
    {
        if (showingGrad[morphIndex])
        {
            for (size_t i = 0; i < positions_.size(); ++i)
            {
                Vec3 start = positions_[i];
                Vec3 grad;
                gradient(static_cast<unsigned>(i), morphIndex, getReadFromCells(), grad);
                gradientLines.positions_[i * 2] = 0.f * normals_[i] + start;
                gradientLines.positions_[i * 2 + 1] = 0.f * normals_[i] + start + normalize(grad) * vecLength;
            }
        }
    }
}

void Grid::updateDiffDirLines()
{
    for (int morphIndex = 0; morphIndex < numMorphs_; ++morphIndex)
    {
        if (showingDiffDir[morphIndex])
        {
            for (size_t i = 0; i < positions_.size(); ++i)
            {
                Vec3 start = positions_[i];
                anisotropicDiffusionTensor.positions_[i * 2] = 0.f * normals_[i] + start;
                anisotropicDiffusionTensor.positions_[i * 2 + 1] = 0.f * normals_[i] + start + tangents[morphIndex][i] * vecLength;
            }
        }
    }
}

void Grid::projectAniVecs(const Vec3& guessVec)
{
    if (selectedCells.size() > 0)
    {
        for (int j = 0; j < numMorphs_; ++j)
        {
            for (unsigned i : selectedCells)
            {
                Vec3 N = getNormal(i);
                Vec3 T = cross(N, normalize(guessVec));
                tangents[j][i] = normalize(cross(T, N));

                if (isGPUEnabled)
                    dirtyAttributes[TANGENT_ATTRIB].indices.insert(i);
            }
        }
    }
    else
    {
        for (int j = 0; j < numMorphs_; ++j)
        {
            for (unsigned i = 0; i < getCellCount(); ++i)
            {
                Vec3 N = getNormal(i);
                Vec3 T = cross(N, normalize(guessVec));
                tangents[j][i] = normalize(cross(T, N));

                if (isGPUEnabled)
                    dirtyAttributes[TANGENT_ATTRIB].indices.insert(i);
            }
        }
    }
}

void Grid::projectAniVecs(const Vec3& guessVec, int morphIndex)
{
    if (selectedCells.size() > 0)
    {
        for (unsigned i : selectedCells)
        {
            Vec3 N = getNormal(i);
            Vec3 T = cross(N, normalize(guessVec));
            tangents[morphIndex][i] = normalize(cross(T, N));

            if (isGPUEnabled)
                dirtyAttributes[TANGENT_ATTRIB].indices.insert(i);
        }
    }
    else
    {
        for (unsigned i = 0; i < getCellCount(); ++i)
        {
            Vec3 N = getNormal(i);
            Vec3 T = cross(N, normalize(guessVec));
            tangents[morphIndex][i] = normalize(cross(T, N));

            if (isGPUEnabled)
                dirtyAttributes[TANGENT_ATTRIB].indices.insert(i);
        }
    }
}

void Grid::projectAniVecs()
{
    if (selectedCells.size() > 0)
    {
        for (int j = 0; j < numMorphs_; ++j)
        {
            for (unsigned i : selectedCells)
            {
                Vec3 N = getNormal(i);
                Vec3 T = cross(N, normalize(tangents[j][i]));
                tangents[j][i] = normalize(cross(T, N));
            }
        }
    }
    else
    {
        for (int j = 0; j < numMorphs_; ++j)
        {
            for (unsigned i = 0; i < getCellCount(); ++i)
            {
                Vec3 N = getNormal(i);
                Vec3 T = cross(N, normalize(tangents[j][i]));
                tangents[j][i] = normalize(cross(T, N));
            }
        }
    }
}

void Grid::copyGradientToDiffusion(int /*morphIndex*/, int /*gradIndex*/)
{
    // TODO
}

void Grid::exportTexture(unsigned char* pixels, int exportWidth, int exportHeight)
{
    auto lerp = [](float s, float a, float b)
    {
        return s * b + (1.f - s) * a;
    };

    int i = 0;
    unsigned char r, g, b;
    for (int y = exportHeight - 1; y >= 0; --y)
    {
        for (int x = 0; x < exportWidth; ++x)
        {
            float u = float(x) / float(exportWidth - 1);
            float v = float(y) / float(exportHeight - 1);

            int x0 = int(u * (xRes_ - 1));
            int x1 = x0 < xRes_ - 1 ? x0 + 1 : x0;
            int y0 = int(v * (yRes_ - 1));
            int y1 = y0 < yRes_ - 1 ? y0 + 1 : y0;

            float u0 = (float(x0) / float(xRes_ - 1));
            float u1 = (float(x1) / float(xRes_ - 1));
            float v0 = (float(y0) / float(yRes_ - 1));
            float v1 = (float(y1) / float(yRes_ - 1));

            u = u1 != u0 ? (u - u0) / (u1 - u0) : 0.f;
            v = v1 != v0 ? (v - v0) / (v1 - v0) : 0.f;

            float s = lerp(v,
                lerp(u, colors_[vec2ToIndex(x0, y0)].r, colors_[vec2ToIndex(x1, y0)].r),
                lerp(u, colors_[vec2ToIndex(x0, y1)].r, colors_[vec2ToIndex(x1, y1)].r));

            colorMapOutside.sample(
                s / normCoef,
                r, g, b
            );

            pixels[i++] = r;
            pixels[i++] = g;
            pixels[i++] = b;
        }
    }
}
