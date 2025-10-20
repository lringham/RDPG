#pragma once
#include "BSplinePatch.h"

#include <vector>


class Animation 
{
    struct Keyframe 
    {
        size_t first;
        BSplinePatch second;
    };

    std::vector<Keyframe> frames_;

public:
    Animation() = default;

    void updatePositionsFromUVs(std::vector<Vec3>& positions, size_t frameNum);
    void addKeyframe(size_t frameNum, const BSplinePatch& patch);
    void addVertex(size_t index, size_t neighbourIndex1, size_t neighbourIndex2);
    void clearFrames();
    void setUVs(const std::vector<Vec3>& newUVs);
    const std::vector<Vec3>& UVs() const;
   
    std::vector<Vec3> UVs_;
};