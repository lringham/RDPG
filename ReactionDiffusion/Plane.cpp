#include "Plane.h"
#include "LOG.h"


Plane::Plane(Vec3 normal, Vec3 position) :
    normal_(normal), position_(position)
{}

bool raycastPlane(const Vec3& rayPos, const Vec3& rayDir, const Vec3& position, const Vec3& normal, float* t)
{
    float rayDirDotNormal = rayDir.dot(normal);
    if (rayDirDotNormal == 0)
        return false;
    (*t) = (position - rayPos).dot(normal) / rayDirDotNormal;
    return true;
}

Vec3 projOnto(const Vec3& planeP, const Vec3& normal, Vec3 point)
{
    ASSERT(length(normal) - 1.f < .00001, "Normal not normalized.");

    Vec3 pointVec = point - planeP;
    Vec3 nDotP = normal * (pointVec.dot(normal)) + planeP;
    Vec3 vNtoP = normalize(point - nDotP);

    return vNtoP * (pointVec.dot(vNtoP)) + planeP;;
}