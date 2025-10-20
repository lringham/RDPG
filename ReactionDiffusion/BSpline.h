#pragma once
#include "Vec3.h"
#include "Drawable.h"
#include "Curve.h"

#include <vector>
#include <cassert>


/*
 BSplines (Basis Splines)
   
   BSplines are curves constructed from a number of individual curve segments

   Have local control property. For a given u, only k basis functions are non-zero.

   A BSpline curve of degree m and n control points has n-m curve segments

   eg. a cubic curve (degree 3) with 10 control points has (10-3 = 7) segments. 

   These segments join at "knot" locations 

   In the cubic case, we have n+4 knot values and in the general case, n + m + 1.

   A uniform spacing provides a "uniform" BSpline and a non-uniform spacing results in a 
   non-uniform curve


*/

class BSpline :
    public Curve
{   
    std::vector<float> knots_;
    size_t degree_ = 2;
    bool clamped_ = true;

public:

    BSpline() = default;
    BSpline(const std::vector<Vec3>& controlPoints, size_t degree = 2, bool clamped = true);

    Vec3 sample(float u) const override;
    void setControlPoints(const std::vector<Vec3>& points) override;
};

Vec3 bSpline(float u, const std::vector<Vec3>& controlPoints, const std::vector<float>& knots, bool clamped);
float basis(size_t i, size_t degree, float u, const std::vector<float>& knots);
std::vector<float> createKnots(size_t degree, size_t numControlPoints, bool clamped);