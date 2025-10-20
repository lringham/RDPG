#include "Triangle.h"


Triangle::Triangle(const Vec3& p0, const Vec3& p1, const Vec3& p2)
{
    init(p0, p1, p2);
}

void Triangle::init(const Vec3& p0, const Vec3& p1, const Vec3& p2)
{
    positions_.emplace_back(p0);
    positions_.emplace_back(p1);
    positions_.emplace_back(p2);

    indices_.emplace_back(0);
    indices_.emplace_back(1);
    indices_.emplace_back(2);

    textureCoords_.emplace_back(0.f, 0.f);
    textureCoords_.emplace_back(.5f, 1.f);
    textureCoords_.emplace_back(1.f, 0.f);

    colors_.emplace_back(Vec3(0, 0, 0));
    colors_.emplace_back(Vec3(0, 0, 0));
    colors_.emplace_back(Vec3(0, 0, 0));

    Vec3 n = normalize(cross(p1 - p0, p2 - p0));
    normals_.emplace_back(n);
    normals_.emplace_back(n);
    normals_.emplace_back(n);

    initVBOs();
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
bool raycastTriangle(const Vec3& rayPos, const Vec3& rayDir, const Vec3& v0, const Vec3& v1, const Vec3& v2, float* t, float* u, float* v)
{
    Vec3 e0 = v1 - v0;
    Vec3 e1 = v2 - v0;
    Vec3 rsubv0 = rayPos - v0;

    float a = -rayDir.x, b = -rayDir.y, c = -rayDir.z;
    float d = e0.x, e = e0.y, f = e0.z;
    float g = e1.x, h = e1.y, i = e1.z;
    float j = rsubv0.x, k = rsubv0.y, l = rsubv0.z;

    float eisubhf = e * i - h * f;
    float gfsubdi = g * f - d * i;
    float dhsubeg = d * h - e * g;

    float Minv = 1.f / (a * eisubhf + b * gfsubdi + c * dhsubeg);

    (*t) = (j * eisubhf + k * gfsubdi + l * dhsubeg) * Minv;
    if ((*t) < 0)
        return false;

    float aksubjb = a * k - j * b;
    float jcsubal = j * c - a * l;
    float blsubkc = b * l - k * c;

    (*u) = (i * aksubjb + h * jcsubal + g * blsubkc) * Minv;
    if ((*u) < 0.f || (*u) > 1.f)
        return false;

    (*v) = -(f * aksubjb + e * (jcsubal)+d * blsubkc) * Minv;
    if ((*v) < 0.f || (*v) > 1.f - (*u))
        return false;

    return true;
}

Vec3 circumcentre(const Vec3& p0, const Vec3& p1, const Vec3& p2, float* radius)
{
    Vec3 e0 = p1 - p0;
    Vec3 e1 = p2 - p1;
    Vec3 e2 = p0 - p2;

    float a = e0.length();
    float b = e1.length();
    float c = e2.length();
    float r = (a * b * c) / sqrt((a + b + c) * (b + c - a) * (c + a - b) * (a + b - c));
    float w = c / 2.f;
    float o = sqrt(abs(r * r - w * w));

    Vec3 centre = normalize(e1.cross(e0).cross(e2));
    Vec3 m = p2 + e2 * .5f;
    Vec3 c0 = m + centre * o;

    if (radius != nullptr)
        (*radius) = r;

    if (length(c0 - p0) - r < .0001f)
        if (length(c0 - p1) - r < .0001f)
            if (length(c0 - p2) - r < .0001f)
                return c0;
    return m - centre * o;
}

bool pointInTriangle(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p)
{
    // Compute Edges 
    Vec3 v0 = p1 - p0;
    Vec3 v1 = p2 - p0;
    Vec3 v2 = p - p0;

    // Compute dot products
    float dot00 = dot(v0, v0);
    float dot01 = dot(v0, v1);
    float dot02 = dot(v0, v2);
    float dot11 = dot(v1, v1);
    float dot12 = dot(v1, v2);

    // Compute barycentric coordinates
    float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // Check if point is in triangle
    return (u >= 0) && (v >= 0) && (u + v < 1);
}

//from http://blackpawn.com/texts/pointinpoly/
bool pointInTriangle(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p, float& u, float& v, float& w)
{
    // Compute Edges 
    Vec3 v0 = p1 - p0;
    Vec3 v1 = p2 - p0;
    Vec3 v2 = p - p0;

    // Compute dot products
    float dot00 = dot(v0, v0);
    float dot01 = dot(v0, v1);
    float dot02 = dot(v0, v2);
    float dot11 = dot(v1, v1);
    float dot12 = dot(v1, v2);

    // Compute barycentric coordinates
    float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
    u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    w = 1.f - u - v;

    // Check if point is in triangle
    return (u >= 0) && (v >= 0) && (u + v < 1);
}

Vec3 barycentre(const Vec3& p0, const Vec3& p1, const Vec3& p2)
{
    return (p0 + p1 + p2) * (1.f / 3.f);
}

void trilinearCoordinates(const Vec3& t0, const Vec3& t1, const Vec3& t2, const Vec3& p, float* a, float* b, float* c)
{
    *a = length((t0 + (p - t0).projOnto(t1 - t0)) - p);
    *b = length((t1 + (p - t1).projOnto(t2 - t1)) - p);
    *c = length((t2 + (p - t2).projOnto(t0 - t2)) - p);
}

void barycentric(const Vec2& p, const Vec2& t0, const Vec2& t1, const Vec2& t2, float& a, float& b, float& c)
{
    float a0 = (p - t0).areaBetween(t1 - t0);
    float b0 = (p - t1).areaBetween(t2 - t1);
    float c0 = (p - t2).areaBetween(t0 - t2);
    float A = (t2 - t0).areaBetween(t1 - t0);

    if (A == 0.f)
        return;

    a = a0 / A;
    b = b0 / A;
    c = c0 / A;
}