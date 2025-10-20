#pragma once
#include <iostream>


class Vec2
{
public:
	explicit Vec2(float val = 0);
	explicit Vec2(float x, float y);

	float length() const;
	void set(float x, float y);
	void set(const Vec2&  v);
	void zero();
	void normalize();
	float dot(const Vec2& v);
	float areaBetween(const Vec2& v);
	void rotate(float angle);

	friend std::ostream& operator<<(std::ostream& os, const Vec2& v);
	friend Vec2 operator+(const Vec2& v1, const Vec2& v2);
	friend Vec2 operator+(const float& val, const Vec2& v);
	friend Vec2 operator+(const Vec2& v, const float& val);

	friend Vec2 operator-(const Vec2& v1, const Vec2& v2);
	friend Vec2 operator-(const float& val, const Vec2& v);
	friend Vec2 operator-(const Vec2& v, const float& val);

	friend Vec2 operator*(const Vec2& v1, const Vec2& v2);
	friend Vec2 operator*(const float& val, const Vec2& v);
	friend Vec2 operator*(const Vec2& v, const float& val);

	union
	{
		float x_, u_;
	};

	union
	{
		float y_, v_;
	};
};

Vec2 normalize(const Vec2& v);
Vec2 add(const Vec2& v1, const Vec2& v2);
Vec2 sub(const Vec2& v1, const Vec2& v2);
Vec2 mul(const Vec2& v, float val);
Vec2 div(const Vec2& v, float val);
float length(const Vec2& v);