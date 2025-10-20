#include "Vec2.h"

#include <math.h>


Vec2::Vec2(float val) : 
	x_(val), y_(val)
{}

Vec2::Vec2(float x, float y) : 
	x_(x), y_(y)
{}

float Vec2::length() const
{
	return sqrt(x_*x_ + y_*y_);
}

void Vec2::set(float x, float y)
{
	x_ = x;
	y_ = y;
}

void Vec2::set(const Vec2&  v)
{
	x_ = v.x_;
	y_ = v.y_;
}

float Vec2::dot(const Vec2&  v)
{
	return x_*v.x_ + y_*v.y_;
}

float Vec2::areaBetween(const Vec2& v)
{
	return (x_*v.y_ - y_*v.x_)*.5f;
}

void Vec2::zero()
{
	x_ = 0;
	y_ = 0;
}

void Vec2::normalize()
{
	float len = length();

	if (len == 0)
		return;

	x_ = x_ / len;
	y_ = y_ / len;
}

void Vec2::rotate(float angle)
{
	float newX = cos(angle)*x_ - sin(angle)*y_;
	float newY = sin(angle)*x_ + cos(angle)*y_;
	x_ = newX;
	y_ = newY;
}

std::ostream& operator<<(std::ostream& os, const Vec2& v)
{
	os << "(" << v.x_ << ", " << v.y_ << ")";
	return os;
}

Vec2 operator+(const Vec2& v1, const Vec2& v2)
{
	return Vec2(v1.x_ + v2.x_, v1.y_ + v2.y_);
}

Vec2 operator+(const float& val, const Vec2& v)
{
	return Vec2(v.x_ + val, v.y_ + val);
}

Vec2 operator+(const Vec2& v, const float& val)
{
	return Vec2(v.x_ + val, v.y_ + val);
}

Vec2 operator-(const Vec2& v1, const Vec2& v2)
{
	return Vec2(v1.x_ - v2.x_, v1.y_ - v2.y_);
}

Vec2 operator-(const float& val, const Vec2& v)
{
	return Vec2(v.x_ - val, v.y_ - val);
}

Vec2 operator-(const Vec2& v, const float& val)
{
	return Vec2(v.x_ - val, v.y_ - val);
}

Vec2 operator*(const Vec2& v1, const Vec2& v2)
{
	return Vec2(v1.x_ * v2.x_, v1.y_ * v2.y_);
}

Vec2 operator*(const float& val, const Vec2& v)
{
	return Vec2(v.x_ * val, v.y_ * val);
}

Vec2 operator*(const Vec2& v, const float& val)
{
	return Vec2(v.x_ * val, v.y_ * val);
}

// TODO: Add non member funcs
Vec2 add(const Vec2& v1, const Vec2& v2)
{
	return Vec2(
		v1.x_ + v2.x_,
		v1.y_ + v2.y_);
}

Vec2 sub(const Vec2& v1, const Vec2& v2)
{
	return Vec2(
		v1.x_ - v2.x_,
		v1.y_ - v2.y_);
}

Vec2 mul(const Vec2& v, float val)
{
	return Vec2(
		v.x_ * val,
		v.y_ * val);
}

Vec2 div(const Vec2& v, float val)
{
	return Vec2(
		v.x_ / val,
		v.y_ / val);
}

float length(const Vec2& v)
{
	return sqrt(v.x_*v.x_ + v.y_*v.y_);
}

Vec2 normalize(const Vec2& v)
{
	float len = length(v);
	if (len > 0)
		return div(v, len);
	else
		return v;
}
