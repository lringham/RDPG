#include "BSplinePatch.h"
#include "Utils.h"

#include <functional>


std::vector<Vec3> calculateUVs(const std::vector<Vec3>& positions, Curve& xCurve, Curve& yCurve)
{
    std::vector<Vec3> UVs;
    auto xLookup = [&](float u) { return xCurve.sample(u).x; }; // Takes a normalized value and returns a Vec3 on a curve 
    auto yLookup = [&](float u) { return yCurve.sample(u).y; };

    for (Vec3 pos : positions)
    {
        UVs.push_back(Vec3{
            Utils::intervalBisect<float>(xLookup, pos.x),
            Utils::intervalBisect<float>(yLookup, pos.y),
            pos.z });
    }

    return UVs;
}

void updatePositionsFromBSplinePatch(std::vector<Vec3>& positions, const std::vector<Vec3>& UVs, const std::vector<BSpline>& splines)
{
    for (size_t i = 0; i < positions.size(); i++)
    {
        Vec3& pos = positions[i];
        const Vec3& uv = UVs[i];

        std::vector<Vec3> newCtrlPts;
        for (const BSpline& spline : splines)
            newCtrlPts.push_back(spline.sample(uv.y));

        float z = pos.z;
        BSpline spline(newCtrlPts);
        pos = spline.sample(uv.x);
        pos.z = z;
    }
}
