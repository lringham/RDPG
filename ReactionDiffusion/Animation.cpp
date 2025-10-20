#include "Animation.h"

#include <algorithm>


void Animation::updatePositionsFromUVs(std::vector<Vec3>& positions, size_t frameNum)
{
    if (frames_.size() == 1)
        updatePositionsFromBSplinePatch(positions, UVs_, frames_[0].second);
    else if (frames_.size() > 1)
    {
        if (frameNum > frames_.back().first)
            frameNum = frames_.back().first;

        // Find frame before and after frameNum (frame0 and frame1)
        size_t frameIdx = 0;
        for (; frameIdx < frames_.size() - 1; ++frameNum)
            if (frames_[frameIdx + 1].first >= frameNum)
                break;

        const auto& frame0 = frames_[frameIdx];
        const auto& frame1 = frames_[frameIdx + 1];

        // Calculate percentage between frame0 and frame1
        size_t span = frame1.first - frame0.first;
        size_t dist = frameNum - frame0.first;
        float t = 0.f;
        if (span != 0)
            t = static_cast<float>(dist) / static_cast<float>(span);

        // Lerp the control points from each frame to create an inbetween patch
        BSplinePatch patch;
        for (size_t splineIdx = 0; splineIdx < frame0.second.size(); ++splineIdx)
        {
            const auto& spline0 = frame0.second[splineIdx];
            const auto& spline1 = frame1.second[splineIdx];

            std::vector<Vec3> controlPoints(spline0.controlPoints().size(), Vec3{ 0.f, 0.f, 0.f });
            for (size_t controlpt = 0; controlpt < spline0.controlPoints().size(); ++controlpt)
                controlPoints[controlpt] = lerp(
                    t,
                    spline0.getControlPoint(controlpt),
                    spline1.getControlPoint(controlpt));

            patch.push_back(BSpline(controlPoints));
        }

        // Update positions using the inbetween patch
        updatePositionsFromBSplinePatch(positions, UVs_, patch);
    }
}

void Animation::addKeyframe(size_t frameNum, const BSplinePatch& patch)
{
    frames_.push_back({ frameNum, patch });
    std::sort(std::begin(frames_), std::end(frames_), [](const Keyframe& lhs, const Keyframe& rhs) {
            return lhs.first < rhs.first;
        });
}

void Animation::addVertex(size_t index, size_t neighbourIndex0, size_t neighbourIndex1)
{
    if (index >= UVs_.size())
        UVs_.resize(index + 1);

    UVs_[index] = (UVs_[neighbourIndex0] + UVs_[neighbourIndex1]) / 2.f;

}

void Animation::clearFrames()
{
    frames_.clear();
}

void Animation::setUVs(const std::vector<Vec3>& newUVs)
{
    UVs_ = newUVs;
}

const std::vector<Vec3>& Animation::UVs() const
{
    return UVs_;
}