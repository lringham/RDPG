#pragma once
#include "Vec3.h"
#include "BSpline.h"

#include <vector>


using BSplinePatch = std::vector<BSpline>;

void updatePositionsFromBSplinePatch(std::vector<Vec3>& positions, const std::vector<Vec3>& UVs, const std::vector<BSpline>& splines);
std::vector<Vec3> calculateUVs(const std::vector<Vec3>& positions, Curve& xCurve, Curve& yCurve);