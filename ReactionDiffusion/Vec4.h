#pragma once
#include "Vec3.h"

#include <iostream>


class Vec4
{
public:
	explicit Vec4(float val = 0);
	explicit Vec4(float x, float y, float z, float w = 0.f);
	explicit Vec4(Vec3 v, float w = 0.f);

	void zero();
	void set(float x, float y, float z, float w = 0.f);
	void set(const Vec4& v);

	float dot(const Vec4& v) const;
	void normalize();
	float length() const;
	Vec3 xyz();

	void sub(float x, float y, float z, float w);
	void sub(const Vec4& translation);
	void add(float x, float y, float z, float w);
	void add(const Vec4& translation);

	friend std::ostream& operator<<(std::ostream& os, const Vec4& v);
	friend Vec4 operator+(const Vec4& v1, const Vec4& v2);
	friend Vec4 operator+(const float& val, const Vec4& v);
	friend Vec4 operator+(const Vec4& v, const float& val);

	friend Vec4 operator-(const Vec4& v1, const Vec4& v2);
	friend Vec4 operator-(const float& val, const Vec4& v);
	friend Vec4 operator-(const Vec4& v, const float& val);

	friend Vec4 operator*(const Vec4& v1, const Vec4& v2);
	friend Vec4 operator*(const float& val, const Vec4& v);
	friend Vec4 operator*(const Vec4& v, const float& val);

	union
	{
		float x, r;
	};

	union
	{
		float y, g;
	};

	union
	{
		float z, b;
	};

	union
	{
		float w, a;
	};
};

Vec4 lerp(float s, const Vec4& v1, const Vec4& v2);