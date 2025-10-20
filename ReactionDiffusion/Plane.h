#pragma once
#include "Vec3.h"


struct Plane
{
    Plane() = default;
    Plane(Vec3 normal, Vec3 position);

    Vec3 position_;
    Vec3 normal_;
};

Vec3 projOnto(const Vec3& pos, const Vec3& normal, Vec3 point);
bool raycastPlane(const Vec3& rayPos, const Vec3& rayDir, const Vec3& position, const Vec3& normal, float* t);