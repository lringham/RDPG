#include "SimulationDomain.h"
#include "DefaultShader.h"

#include <algorithm>


void SimulationDomain::recalculateParameters()
{
    doRecalculateParameters();

    // Update tensor visualization
    projectAniVecs();

    if (anisotropicDiffusionTensor.shader_ == nullptr)
    {
        anisotropicDiffusionTensor.visible_ = false;
        anisotropicDiffusionTensor.shader_ = std::make_unique<DefaultShader>();
        anisotropicDiffusionTensor.shader_->setDisableColormap(true);
        anisotropicDiffusionTensor.shader_->setDisableShading(true);
    }

    if (gradientLines.shader_ == nullptr)
    {
        gradientLines.finalize();
        gradientLines.visible_ = false;
        gradientLines.shader_ = std::make_unique<DefaultShader>();
        gradientLines.shader_->setDisableColormap(true);
        gradientLines.shader_->setDisableShading(true);
    }

    if (std::find(begin(childern_), end(childern_), &anisotropicDiffusionTensor) == end(childern_))
        childern_.push_back(&anisotropicDiffusionTensor);
    if (std::find(begin(childern_), end(childern_), &gradientLines) == end(childern_))
        childern_.push_back(&gradientLines);
}

void SimulationDomain::init(int numMorphs)
{
#if DIFFUSION_EQNS==0
	LOG("Using Andreux et al.'s (2014) diffusion equations");
#elif DIFFUSION_EQNS==1
	LOG("Using Ringham et al.'s (2021) diffusion equations");
#else
	LOG("Diffusion equations not specified!");
#endif

    doInit(numMorphs);

    if (anisotropicDiffusionTensor.shader_ == nullptr)
    {
        anisotropicDiffusionTensor.visible_ = false;
        anisotropicDiffusionTensor.shader_ = std::make_unique<DefaultShader>();
        anisotropicDiffusionTensor.shader_->setDisableColormap(true);
        anisotropicDiffusionTensor.shader_->setDisableShading(true);
    }

    if (gradientLines.shader_ == nullptr)
    {
        gradientLines.finalize();
        gradientLines.visible_ = false;
        gradientLines.shader_ = std::make_unique<DefaultShader>();
        gradientLines.shader_->setDisableColormap(true);
        gradientLines.shader_->setDisableShading(true);
    }

    if (std::find(begin(childern_), end(childern_), &anisotropicDiffusionTensor) == end(childern_))
        childern_.push_back(&anisotropicDiffusionTensor);
    if (std::find(begin(childern_), end(childern_), &gradientLines) == end(childern_))
        childern_.push_back(&gradientLines);

    recalculateParameters();
}

unsigned SimulationDomain::getCellCount()
{
    return static_cast<unsigned>(cells1.size());
}

void SimulationDomain::swap()
{
    currentCells = !currentCells;
}

void SimulationDomain::clearMorphogens()
{
    for (int i = 0; i < numMorphs_; ++i)
        clearMorphogens(i);
}

void SimulationDomain::clearMorphogens(int morphIndex)
{
    if (selectedCells.size() > 0)
    {
        for (unsigned i : selectedCells)
        {
            cells1[i][morphIndex] = clearMValue[morphIndex];
            cells2[i][morphIndex] = cells1[i][morphIndex];

            if (isGPUEnabled)
            {
                dirtyAttributes[CELL1_ATTRIB].indices.insert(i);
                dirtyAttributes[CELL2_ATTRIB].indices.insert(i);
            }
        }
    }
    else
    {
        for (unsigned i = 0; i < cells1.size(); ++i)
        {
            cells1[i][morphIndex] = clearMValue[morphIndex];
            cells2[i][morphIndex] = cells1[i][morphIndex];

            if (isGPUEnabled)
            {
                dirtyAttributes[CELL1_ATTRIB].indices.insert(i);
                dirtyAttributes[CELL2_ATTRIB].indices.insert(i);
            }
        }
    }
}

int SimulationDomain::getGrowthMode() const
{
    return (int) growthMode;
}

void SimulationDomain::clearDiffusion(int morphIndex)
{
    if (selectedCells.size() > 0)
    {
        for (unsigned i : selectedCells)
        {
            diffusionTensors[morphIndex].t0_[i] = paintTensorVals[0];
            diffusionTensors[morphIndex].t1_[i] = paintTensorVals[1];

            if (isGPUEnabled)
                dirtyAttributes[TANGENT_ATTRIB].indices.insert(i);
        }
    }
    else
    {
        for (unsigned i = 0; i < cells1.size(); ++i)
        {
            diffusionTensors[morphIndex].t0_[i] = paintTensorVals[0];
            diffusionTensors[morphIndex].t1_[i] = paintTensorVals[1];

            if (isGPUEnabled)
                dirtyAttributes[TANGENT_ATTRIB].indices.insert(i);
        }
    }
}

void SimulationDomain::invertSelected()
{
    std::set<unsigned> newCells;
    for (unsigned i = 0; i < cells1.size(); ++i)
        if (selectedCells.find(i) == selectedCells.end())
            newCells.insert(i);
    selectedCells = newCells;
}

void SimulationDomain::selectBorder()
{
    selectedCells = getBoundaryCellsIndices();
}

void SimulationDomain::selectBoundary()
{
    std::set<unsigned int> removeThese;

    // Look at all the cells neighbouring cell i and 
    // determine if it is on the boundary. Boundary cells
    // will have a neighbour that is not selected.
    for (auto i : selectedCells)
    {
        bool onBoundary = false;
        auto n = getNeighbours(i, 1);
        for (unsigned c : n)
        {
            if (isBoundary(i) || selectedCells.count(c) == 0)
            {
                onBoundary = true;
                break;
            }
        }

        if (!onBoundary)
            removeThese.insert(i);
    }

    for (auto c : removeThese)
        selectedCells.erase(c);
}

void SimulationDomain::selectNeighbours()
{
    std::set<unsigned int> newCells;

    for (auto i : selectedCells)
    {
        auto n = getNeighbours(i, 1);
        for (unsigned c : n)
            newCells.insert(c);
    }

    for (auto c : newCells)
        selectedCells.insert(c);
}

void SimulationDomain::setVeinMorphIndex(int morphIndex, float veinDiff, float petalDiff)
{
    veinMorphIndex_ = morphIndex;
	veinDiffusionScale_ = veinDiff;
	petalDiffusionScale_ = petalDiff;
}

void SimulationDomain::updateVeinValues()
{
    if (veinMorphIndex_ >= 0)
    {
        size_t i = 0;
        for (auto& c : colors_)
        {
            cells1[i][veinMorphIndex_] = c.r;
            cells2[i][veinMorphIndex_] = c.r;
            i++;
        }
    }
}

bool SimulationDomain::hasVeins() const
{
    return veinMorphIndex_ >= 0;
}

void SimulationDomain::setDiffTensor(unsigned index, float t0, float t1, int morphIndex)
{
    diffusionTensors[morphIndex].t0_[index] = t0;
    diffusionTensors[morphIndex].t1_[index] = t1;
}

std::vector<SimulationDomain::Cell>& SimulationDomain::getWriteToCells()
{
    if (currentCells)
        return cells1;
    else
        return cells2;
}

std::vector<SimulationDomain::Cell>& SimulationDomain::getReadFromCells()
{
    if (currentCells)
        return cells2;
    else
        return cells1;
}

void SimulationDomain::update()
{
    std::vector<Cell>& writeToCells = getWriteToCells();

    // update colors
    if (showSelectedCells)
    {
        for (int i = 0; i < numMorphs_; ++i)
            if (showingMorph[i])
            {
                for (unsigned j = 0; j < writeToCells.size(); ++j)
                    colors_[j].set(writeToCells[j][i], .5f, 0);

                for (unsigned j : selectedCells)
                    colors_[j].set(writeToCells[j][i], 1, 0);
            }
    }
    else
    {
        for (int i = 0; i < numMorphs_; ++i)
            if (showingMorph[i])
            {
                for (unsigned j = 0; j < writeToCells.size(); ++j)
                {
                    colors_[j].r = writeToCells[j][i];
                }
                break;
            }
    }

    shader_->setTexture("colorMapInside", colorMapInside.getTextureID());
    shader_->setTexture("colorMapOutside", colorMapOutside.getTextureID());
    shader_->setTexture("background", background_.id());
    shader_->setUniform1f("normCoef", normCoef);
    shader_->setUniform1f("backgroundThreshold", backgroundThreshold_);
    shader_->setUniform1f("backgroundOffset", backgroundOffset_);
    shader_->setUniform1i("useBackgroundTexture", background_.id() != 0);
    
    doUpdate();
}

bool SimulationDomain::isPaintingDiffDir() const
{
    for (size_t i = 0; i < paintingDiffDir.size(); ++i)
        if (paintingDiffDir[i])
            return true;
    return false;
}

bool SimulationDomain::isPaintingMorph() const
{
    for (size_t i = 0; i < paintingMorph.size(); ++i)
        if (paintingMorph[i])
            return true;
    return false;
}

bool SimulationDomain::isSelecting() const
{
    return selectCells;
}

bool SimulationDomain::isPainting() const
{
    return isPaintingDiffDir() || isPaintingMorph() || isSelecting();
}