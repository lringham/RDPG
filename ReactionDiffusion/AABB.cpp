#include "AABB.h"
#include "Triangle.h"

#include <algorithm>
#include <vector>
#include <limits>


BVHTriangle::BVHTriangle(Vec3 p0, Vec3 p1, Vec3 p2) :
    p0_(p0), p1_(p1), p2_(p2)
{
    n0_ = normalize(cross(p1_ - p0_, p2_ - p0_));
    n1_ = n0_;
    n2_ = n0_;
}

/* From pg 78 in Fundamentals of Computer Graphics 3rd Edition

Considering barycentric coordiantes, a point in a triangle (rayPos+rayDir*t) can be expressed as a weighted sum of the vertices
(rayPos+rayDir*t) = (1-u-v)v0 + u*v1 + v*v2
We can re arrange this into a system of equations Ax = b

[             ][t]   [         ]
[-rayDir e0 e1][u] = [rayPos-v0]
[             ][v]   [         ]

Where e0 = v1 - v0; e1 = v2 - v0

Using Cramer's rule we can solve for t, u, v by subbing b into a coulmn of A to get A1, A2, A3
Then we divide by M = det(A)
det(A1)/M = t
det(A2)/M = u
det(A3)/M = v
*/
bool BVHTriangle::raycast(Ray& ray) const
{
    Vec3 e0 = p1_ - p0_;
    Vec3 e1 = p2_ - p0_;
    Vec3 rsubv0 = ray.origin_ - p0_;

    float a = -ray.dir_.x, b = -ray.dir_.y, c = -ray.dir_.z;
    float d = e0.x, e = e0.y, f = e0.z;
    float g = e1.x, h = e1.y, i = e1.z;
    float j = rsubv0.x, k = rsubv0.y, l = rsubv0.z;

    float eisubhf = e * i - h * f;
    float gfsubdi = g * f - d * i;
    float dhsubeg = d * h - e * g;

    float Minv = 1.f / (a * eisubhf + b * gfsubdi + c * dhsubeg);

    ray.t_ = (j * eisubhf + k * gfsubdi + l * dhsubeg) * Minv;
    if (ray.t_ < 0)
        return false;

    float aksubjb = a * k - j * b;
    float jcsubal = j * c - a * l;
    float blsubkc = b * l - k * c;

    float u = (i * aksubjb + h * jcsubal + g * blsubkc) * Minv;
    if (u < 0.f || u > 1.f)
        return false;

    float v = -(f * aksubjb + e * (jcsubal)+d * blsubkc) * Minv;
    if (v < 0.f || v > 1.f - u)
        return false;

    ray.normal_ = n0_ * (1.f - u - v) +
        n1_ * u +
        n2_ * v;
    ray.tangent_ = t_;
    ray.uv_ = uv0_ * (1.f - u - v) +
        uv1_ * u +
        uv2_ * v;
    ray.triIndex_ = triIndex_;

    return true;
}
//
//bool raycastTri(const Vec3& p0, const Vec3& p1, const Vec3& p2, Ray& ray, float* uRet, float* vRet)
//{
//    Vec3 e0 = p1 - p0;
//    Vec3 e1 = p2 - p0;
//    Vec3 rsubv0 = ray.origin_ - p0;
//
//    float a = -ray.dir_.x, b = -ray.dir_.y, c = -ray.dir_.z;
//    float d = e0.x, e = e0.y, f = e0.z;
//    float g = e1.x, h = e1.y, i = e1.z;
//    float j = rsubv0.x, k = rsubv0.y, l = rsubv0.z;
//
//    float eisubhf = e * i - h * f;
//    float gfsubdi = g * f - d * i;
//    float dhsubeg = d * h - e * g;
//
//    float Minv = 1.f / (a * eisubhf + b * gfsubdi + c * dhsubeg);
//
//    ray.t_ = (j * eisubhf + k * gfsubdi + l * dhsubeg) * Minv;
//    if (ray.t_ < 0)
//        return false;
//
//    float aksubjb = a * k - j * b;
//    float jcsubal = j * c - a * l;
//    float blsubkc = b * l - k * c;
//
//    float u = (i * aksubjb + h * jcsubal + g * blsubkc) * Minv;
//    if (u < 0.f || u > 1.f)
//        return false;
//
//    float v = -(f * aksubjb + e * (jcsubal)+d * blsubkc) * Minv;
//    if (v < 0.f || v > 1.f - u)
//        return false;
//
//    ray.normal_ = normalize(cross(e0, e1));
//
//    if (uRet != nullptr)
//        *uRet = u;
//
//    if (vRet != nullptr)
//        *vRet = v;
//
//    return true;
//}

Vec3 barycentre(const BVHTriangle& triangle)
{
    return (triangle.p0_ + triangle.p1_ + triangle.p2_) / 3.f;
}

Vec3 calculateTangent(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec2& uv0, const Vec2& uv1, const Vec2& uv2)
{
    Vec3 Q1 = p1 - p0;
    Vec3 Q2 = p2 - p0;

    float u1 = uv1.u_ - uv0.u_;
    float u2 = uv2.u_ - uv0.u_;
    float v1 = uv1.v_ - uv0.v_;
    float v2 = uv2.v_ - uv0.v_;

    float denom = u1 * v2 - u2 * v1;
    if (denom != 0)
    {
        float invDet = 1.f / denom;
        return invDet * (v2 * Q1 - u1 * Q2);
    }
    return Vec3(0, 0, 0);
}

AABB::AABB() :
    min_(std::numeric_limits<float>::max()), max_(std::numeric_limits<float>::min())
{}

AABB::AABB(const AABB& b0, const AABB& b1) :
    min_(std::numeric_limits<float>::max()), max_(std::numeric_limits<float>::min())
{
    min_.x = std::min(b0.min_.x, b1.min_.x);
    min_.y = std::min(b0.min_.y, b1.min_.y);
    min_.z = std::min(b0.min_.z, b1.min_.z);

    max_.x = std::max(b0.max_.x, b1.max_.x);
    max_.y = std::max(b0.max_.y, b1.max_.y);
    max_.z = std::max(b0.max_.z, b1.max_.z);
}

AABB::AABB(const std::vector<Vec3>& points) :
    min_(std::numeric_limits<float>::max()), max_(std::numeric_limits<float>::min())
{
    if (points.size() == 0)
        return;

    min_ = points[0];
    max_ = points[0];
    for (const Vec3& v : points)
    {
        min_.x = std::min(min_.x, v.x);
        min_.y = std::min(min_.y, v.y);
        min_.z = std::min(min_.z, v.z);
        max_.x = std::max(max_.x, v.x);
        max_.y = std::max(max_.y, v.y);
        max_.z = std::max(max_.z, v.z);
    }
}

AABB::AABB(const BVHTriangle& t) :
    min_(std::numeric_limits<float>::max()), max_(std::numeric_limits<float>::min())
{
    min_.x = t.p0_.x;
    min_.y = t.p0_.y;
    min_.z = t.p0_.z;
    max_.x = t.p0_.x;
    max_.y = t.p0_.y;
    max_.z = t.p0_.z;

    min_.x = std::min(min_.x, t.p1_.x);
    min_.y = std::min(min_.y, t.p1_.y);
    min_.z = std::min(min_.z, t.p1_.z);
    max_.x = std::max(max_.x, t.p1_.x);
    max_.y = std::max(max_.y, t.p1_.y);
    max_.z = std::max(max_.z, t.p1_.z);

    min_.x = std::min(min_.x, t.p2_.x);
    min_.y = std::min(min_.y, t.p2_.y);
    min_.z = std::min(min_.z, t.p2_.z);
    max_.x = std::max(max_.x, t.p2_.x);
    max_.y = std::max(max_.y, t.p2_.y);
    max_.z = std::max(max_.z, t.p2_.z);
}

AABB::AABB(const std::vector<BVHTriangle>& BVHTriangles) :
    min_(std::numeric_limits<float>::max()), max_(std::numeric_limits<float>::min())
{
    if (BVHTriangles.size() > 0)
    {
        const BVHTriangle& t = BVHTriangles[0];
        min_.x = std::min(t.p0_.x, std::min(t.p1_.x, t.p2_.x));
        min_.y = std::min(t.p0_.y, std::min(t.p1_.y, t.p2_.y));
        min_.z = std::min(t.p0_.z, std::min(t.p1_.z, t.p2_.z));
        max_.x = std::max(t.p0_.x, std::max(t.p1_.x, t.p2_.x));
        max_.y = std::max(t.p0_.y, std::max(t.p1_.y, t.p2_.y));
        max_.z = std::max(t.p0_.z, std::max(t.p1_.z, t.p2_.z));
    }

    for (const auto& t : BVHTriangles)
    {
        min_.x = std::min(min_.x, std::min(t.p0_.x, std::min(t.p1_.x, t.p2_.x)));
        min_.y = std::min(min_.y, std::min(t.p0_.y, std::min(t.p1_.y, t.p2_.y)));
        min_.z = std::min(min_.z, std::min(t.p0_.z, std::min(t.p1_.z, t.p2_.z)));

        max_.x = std::max(max_.x, std::max(t.p0_.x, std::max(t.p1_.x, t.p2_.x)));
        max_.y = std::max(max_.y, std::max(t.p0_.y, std::max(t.p1_.y, t.p2_.y)));
        max_.z = std::max(max_.z, std::max(t.p0_.z, std::max(t.p1_.z, t.p2_.z)));
    }
}

AABB::AABB(const Vec3& corner0, const Vec3& corner1) :
    min_(std::numeric_limits<float>::max()), max_(std::numeric_limits<float>::min())
{
    min_.x = std::min(corner0.x, corner1.x);
    min_.y = std::min(corner0.y, corner1.y);
    min_.z = std::min(corner0.z, corner1.z);
    max_.x = std::max(corner0.x, corner1.x);
    max_.y = std::max(corner0.y, corner1.y);
    max_.z = std::max(corner0.z, corner1.z);
}

bool AABB::raycast(Ray& ray) const
{
    float tMin = std::numeric_limits<float>::min();
    float tMax = std::numeric_limits<float>::max();

    // Solve for distance from each set of planes (slabs)
    // if the min of all the intersections is less than the  max
    // of all the intersections then the box is hit
    if (ray.dir_.x != 0.f)
    {
        float tx0 = (min_.x - ray.origin_.x) / ray.dir_.x;
        float tx1 = (max_.x - ray.origin_.x) / ray.dir_.x;
        tMin = std::max(tMin, std::min(tx0, tx1));
        tMax = std::min(tMax, std::max(tx0, tx1));
    }

    if (ray.dir_.y != 0.f)
    {
        float ty0 = (min_.y - ray.origin_.y) / ray.dir_.y;
        float ty1 = (max_.y - ray.origin_.y) / ray.dir_.y;
        tMin = std::max(tMin, std::min(ty0, ty1));
        tMax = std::min(tMax, std::max(ty0, ty1));
    }

    if (ray.dir_.z != 0.f)
    {
        float tz0 = (min_.z - ray.origin_.z) / ray.dir_.z;
        float tz1 = (max_.z - ray.origin_.z) / ray.dir_.z;
        tMin = std::max(tMin, std::min(tz0, tz1));
        tMax = std::min(tMax, std::max(tz0, tz1));
    }

    if (tMin <= tMax && tMin >= 0)
    {
        ray.normal_.set(0, 0, 1);
        ray.t_ = tMin;
        return true;
    }
    return false;
}

bool AABB::fastRaycast(Ray & ray, const Vec3 & invDir) const
{
    float tx0 = (min_.x - ray.origin_.x) * invDir.x;
    float tx1 = (max_.x - ray.origin_.x) * invDir.x;
    float tMin = std::min(tx0, tx1);
    float tMax = std::max(tx0, tx1);

    float ty0 = (min_.y - ray.origin_.y) * invDir.y;
    float ty1 = (max_.y - ray.origin_.y) * invDir.y;
    tMin = std::max(tMin, std::min(ty0, ty1));
    tMax = std::min(tMax, std::max(ty0, ty1));

    float tz0 = (min_.z - ray.origin_.z) * invDir.z;
    float tz1 = (max_.z - ray.origin_.z) * invDir.z;
    tMin = std::max(tMin, std::min(tz0, tz1));
    tMax = std::min(tMax, std::max(tz0, tz1));

    if (tMin <= tMax && tMin >= 0)
    {
        ray.t_ = tMin;
        return true;
    }
    return false;
}

void AABB::move(Vec3 translation)
{
    min_ = min_ + translation;
    max_ = max_ + translation;
}

Vec3 AABB::center() const
{
    return min_ + (min_ + max_) * .5f;
}
