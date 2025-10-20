#include "Vec3.h"
#include "Mat3.h"
#include "Utils.h"


Vec3::Vec3(float val) : 
    x(val), y(val), z(val)
{}

Vec3::Vec3(float newX, float newY, float newZ) :
    x(newX), y(newY), z(newZ)
{}

void Vec3::set(float newX, float newY, float newZ)
{
    this->x = newX;
    this->y = newY;
    this->z = newZ;
}

void Vec3::set(const Vec3& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
}

void Vec3::zero()
{
    x = 0;
    y = 0;
    z = 0;
}

Vec3 Vec3::cross(const Vec3& v) const
{
    return Vec3(
        y * v.z - z * v.y,
        z * v.x - x * v.z,
        x * v.y - y * v.x);
}

float Vec3::dot(const Vec3& v) const
{
    return x * v.x + y * v.y + z * v.z;
}

Vec3 Vec3::projOnto(const Vec3& v)
{
    Vec3 unitV(::normalize(v));
    return unitV * dot(unitV);
}

void Vec3::rotate(float angle, Vec3 axis)
{
    Vec3 v = *this;
    *this = v * cos(angle) + ::cross(axis, v) * sin(angle) + axis * (::dot(axis, v) * (1.f - cos(angle)));
}

float Vec3::areaBetween(const Vec3& v)
{
    return cross(v).length() / 2.f;
}

float Vec3::acuteAngleBetween(const Vec3& v)
{
    return acos(::dot(::normalize((*this)), ::normalize(v)));
}

void Vec3::add(float newX, float newY, float newZ)
{
    this->x += newX;
    this->y += newY;
    this->z += newZ;
}

void Vec3::add(const Vec3& translation)
{
    this->x += translation.x;
    this->y += translation.y;
    this->z += translation.z;
}

void Vec3::sub(float newX, float newY, float newZ)
{
    this->x -= newX;
    this->y -= newY;
    this->z -= newZ;
}

void Vec3::sub(const Vec3& translation)
{
    this->x -= translation.x;
    this->y -= translation.y;
    this->z -= translation.z;
}

void Vec3::normalize()
{
    float len = length();

    if (len == 0)
        return;

    x = x / len;
    y = y / len;
    z = z / len;
}

float Vec3::length() const
{
    return sqrt(x * x + y * y + z * z);
}

std::string Vec3::toString() const
{
    return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
}

std::ostream& operator<<(std::ostream& os, const Vec3& v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

Vec3 operator+(const Vec3& v1, const Vec3& v2)
{
    return Vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

Vec3 operator+(const float& val, const Vec3& v)
{
    return Vec3(v.x + val, v.y + val, v.z + val);
}

Vec3 operator+(const Vec3& v, const float& val)
{
    return Vec3(v.x + val, v.y + val, v.z + val);
}

Vec3 operator-(const Vec3& v1, const Vec3& v2)
{
    return Vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

Vec3 operator-(const float& val, const Vec3& v)
{
    return Vec3(v.x - val, v.y - val, v.z - val);
}

Vec3 operator-(const Vec3& v, const float& val)
{
    return Vec3(v.x - val, v.y - val, v.z - val);
}

Vec3 operator*(const Vec3& v1, const Vec3& v2)
{
    return Vec3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}

Vec3 operator*(const float& val, const Vec3& v)
{
    return Vec3(v.x * val, v.y * val, v.z * val);
}

Vec3 operator*(const Vec3& v, const float& val)
{
    return Vec3(v.x * val, v.y * val, v.z * val);
}

Vec3 operator/(const Vec3& v1, const Vec3& v2)
{
    return Vec3(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
}

Vec3 operator/(const float& val, const Vec3& v)
{
    return Vec3(v.x / val, v.y / val, v.z / val);
}

Vec3 operator/(const Vec3& v, const float& val)
{
    return Vec3(v.x / val, v.y / val, v.z / val);
}

// Non member functions
Vec3 add(const Vec3& v1, const Vec3& v2)
{
    return Vec3(
        v1.x + v2.x,
        v1.y + v2.y,
        v1.z + v2.z);
}

Vec3 sub(const Vec3& v1, const Vec3& v2)
{
    return Vec3(
        v1.x - v2.x,
        v1.y - v2.y,
        v1.z - v2.z);
}

Vec3 mul(const Vec3& v, float val)
{
    return Vec3(
        v.x * val,
        v.y * val,
        v.z * val);
}

Vec3 div(const Vec3& v, float val)
{
    return Vec3(
        v.x / val,
        v.y / val,
        v.z / val);
}

float length(const Vec3& v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float dot(const Vec3& v1, const Vec3& v2)
{
    return v1.x * v2.x +
        v1.y * v2.y +
        v1.z * v2.z;
}

Vec3 cross(const Vec3& v1, const Vec3& v2)
{
    return Vec3(
        v1.y * v2.z - v1.z * v2.y,
        v1.z * v2.x - v1.x * v2.z,
        v1.x * v2.y - v1.y * v2.x);
}

Vec3 normalize(const Vec3& v)
{
    float len = length(v);
    if (len > 0)
        return div(v, len);
    else
        return v;
}

Vec3 projOnto(const Vec3& v1, const Vec3& v2)
{
    Vec3 unitV2 = normalize(v2);
    return unitV2.dot(v1) * unitV2;
}

float areaBetween(const Vec3& v1, const Vec3& v2)
{
    return length(cross(v1, v2)) / 2.f;
}

float acuteAngleBetween(const Vec3& v1, const Vec3& v2)
{
    return acos(dot(normalize(v1), normalize(v2)));
}

Vec3 reflect(const Vec3& I, const Vec3& N)
{
    return I - 2.f * N.dot(I) * N;
}

Vec3 rotate(const Vec3& v, float angle, const Vec3& axis)
{
    return v * cos(angle) + ::cross(axis, v) * sin(angle) + axis * (::dot(axis, v) * (1.f - cos(angle)));
}

Vec3 lerp(float s, const Vec3& v1, const Vec3& v2)
{
    return s * (v2)+(1.f - s) * v1;
}

Vec3 nlerp(float s, const Vec3& v1, const Vec3& v2)
{
    return normalize(s * (v2)+(1.f - s) * v1);
}

Vec3 slerp(float s, const Vec3& v1, const Vec3& v2)
{
    float theta = acuteAngleBetween(v1, v2);
    float sinTheta = sin(theta);
    return sin((1.f - s) * theta) / sinTheta * v1 + sin(s * theta) / sinTheta * v2;
}