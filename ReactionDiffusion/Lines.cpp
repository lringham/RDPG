#include "Lines.h"


Lines::Lines()
{
    drawMode_ = GL_LINES;
}

void Lines::addLine(Vec3 start, Vec3 end, Vec3 startColor, Vec3 endColor)
{
    positions_.emplace_back(start);
    positions_.emplace_back(end);

    indices_.emplace_back(static_cast<int>(indices_.size()));
    indices_.emplace_back(static_cast<int>(indices_.size()));

    colors_.emplace_back(startColor, 1.f);
    colors_.emplace_back(endColor, 1.f);

    normals_.emplace_back(0.f, 1.f, 0.f);
    normals_.emplace_back(0.f, 1.f, 0.f);

    textureCoords_.emplace_back(0.f, 0.f);
    textureCoords_.emplace_back(1.f, 0.f);
}

size_t Lines::numLines() const
{
    return positions_.size() / 2;
}

void Lines::finalize()
{
    initVBOs();
}

const Vec3& Lines::getStart(unsigned index)
{
    return positions_[index * 2];
}

const Vec3& Lines::getEnd(unsigned index)
{
    return positions_[index * 2 + 1];
}