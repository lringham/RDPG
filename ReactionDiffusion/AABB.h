#pragma once
#include "Vec3.h"
#include "Ray.h"

#include <vector>


class BVHTriangle
{
public:
    BVHTriangle(Vec3 p0 = Vec3(-1, 0, 0), Vec3 p1 = Vec3(0, 1, 0), Vec3 p2 = Vec3(1, 0, 0));
    bool raycast(Ray& ray) const;

    Vec3 p0_, p1_, p2_;
    Vec3 t_;
    Vec3 n0_, n1_, n2_;
    Vec2 uv0_, uv1_, uv2_;
    int triIndex_ = 0;
};

class AABB
{
public:
    AABB();
    AABB(const AABB& b0, const AABB& b1);
    AABB(const std::vector<Vec3>& points);
    AABB(const BVHTriangle& t);
    AABB(const std::vector<BVHTriangle>& triangles);
    AABB(const Vec3& corner0, const Vec3& corner1);
    bool raycast(Ray& ray) const;
    bool fastRaycast(Ray& ray, const Vec3& invDir) const;
    void move(Vec3 translation);
    Vec3 center() const;

    Vec3 min_, max_;
};

Vec3 barycentre(const BVHTriangle& triangle);
Vec3 calculateTangent(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec2& uv0_, const Vec2& uv1_, const Vec2& uv2_);
