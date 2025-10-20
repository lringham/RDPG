#include "Vec4.h"

#include <math.h>


Vec4::Vec4(float val) :
	x(val), y(val), z(val), w(val)
{}

Vec4::Vec4(float newX, float newY, float newZ, float newW) :
	x(newX), y(newY), z(newZ), w(newW)
{}

Vec4::Vec4(Vec3 v, float newW) :
	x(v.x), y(v.y), z(v.z), w(newW)
{}

void Vec4::set(float newX, float newY, float newZ, float newW)
{
    this->x = newX;
    this->y = newY;
    this->z = newZ;
    this->w = newW;
}

void Vec4::set(const Vec4& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	w = v.w;
}

void Vec4::zero()
{
	x = 0;
	y = 0;
	z = 0;
	w = 0;
}

Vec3 Vec4::xyz()
{
	return Vec3(x, y, z);
}

float Vec4::dot(const Vec4&  v) const
{
	return x*v.x + y*v.y + z*v.z + w*v.w;
}

void Vec4::add(float newX, float newY, float newZ, float newW)
{
	this->x += newX;
	this->y += newY;
	this->z += newZ;
	this->w += newW;
}

void Vec4::add(const Vec4& translation)
{
	this->x += translation.x;
	this->y += translation.y;
	this->z += translation.z;
	this->w += translation.w;
}

void Vec4::sub(float newX, float newY, float newZ, float newW)
{
	this->x -= newX;
	this->y -= newY;
	this->z -= newZ;
	this->w -= newW;
}

void Vec4::sub(const Vec4& translation)
{
	this->x -= translation.x;
	this->y -= translation.y;
	this->z -= translation.z;
	this->w -= translation.w;
}

void Vec4::normalize()
{
	float len = length();

	if (len == 0)
		return;

	x = x / len;
	y = y / len;
	z = z / len;
	w = w / len;
}

float Vec4::length() const
{
	return sqrt(x*x + y*y + z*z + w*w);
}

std::ostream& operator<<(std::ostream& os, const Vec4& v)
{
	os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
	return os;
}

Vec4 operator+(const Vec4& v1, const Vec4& v2)
{
	return Vec4(v1.x + v2.x, v1.y + v2.y,v1.z + v2.z, v1.w + v2.w);
}

Vec4 operator+(const float& val, const Vec4& v)
{
	return Vec4(v.x + val, v.y + val, v.z + val, v.z + val);
}

Vec4 operator+(const Vec4& v, const float& val)
{
	return Vec4(v.x + val, v.y + val, v.z + val, v.z + val);
}

Vec4 operator-(const Vec4& v1, const Vec4& v2)
{
	return Vec4(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

Vec4 operator-(const float& val, const Vec4& v)
{
	return Vec4(v.x - val, v.y - val, v.z - val, v.w - val);
}

Vec4 operator-(const Vec4& v, const float& val)
{
	return Vec4(v.x - val, v.y - val, v.z - val, v.w - val);
}

Vec4 operator*(const Vec4& v1, const Vec4& v2)
{
	return Vec4(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w);
}

Vec4 operator*(const float& val, const Vec4& v)
{
	return Vec4(v.x * val, v.y * val, v.z * val, v.w * val);
}

Vec4 operator*(const Vec4& v, const float& val)
{
	return Vec4(v.x * val, v.y * val, v.z * val, v.w * val);
}

Vec4 lerp(float s, const Vec4& v1, const Vec4& v2)
{
	return s * (v2)+(1.f - s) * v1;
}
