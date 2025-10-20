#pragma once
#include "Vec3.h"
#include "Drawable.h"


class Triangle :
    public Drawable
{
public:
    Triangle() = default;
    Triangle(const Vec3& p0, const Vec3& p1, const Vec3& p2);

    void init(const Vec3& p0, const Vec3& p1, const Vec3& p2);
};

bool raycastTriangle(const Vec3& rayPos, const Vec3& rayDir, const Vec3& v0, const Vec3& v1, const Vec3& v2, float* t, float* u, float* v);
Vec3 circumcentre(const Vec3& p0, const Vec3& p1, const Vec3& p2, float* radius = nullptr);
Vec3 barycentre(const Vec3& p0, const Vec3& p1, const Vec3& p2);
void trilinearCoordinates(const Vec3& t0, const Vec3& t1, const Vec3& t2, const Vec3& p, float* a, float* b, float* c);
bool pointInTriangle(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p, float& u, float& v, float& w);
bool pointInTriangle(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p);
void barycentric(const Vec2& p, const Vec2& t0, const Vec2& t1, const Vec2& t2, float& a, float& b, float& c);
