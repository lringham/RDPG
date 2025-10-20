#pragma once
#include "Vec2.h"

#include <iostream>


class Mat2
{
public:
	explicit Mat2(float mainDiag = 0);
	explicit Mat2(float elements[4]);

	void identity();
	void zero();
	void transpose();
	void set(Mat2 m);
	void set(float newElements[9]);
	void setRow(int index, float x, float y);
	void setRow(int index, Vec2 row);
	void setCol(int index, float x, float y);
	void setCol(int index, Vec2 col);

	//operators
	friend std::ostream& operator<<(std::ostream& os, const Mat2& m);
	friend Mat2 operator+(const Mat2& m1, const Mat2& m2);
	friend Mat2 operator-(const Mat2& m1, const Mat2& m2);
	friend Mat2 operator*(const Mat2& m1, const Mat2& m2);
	friend Vec2 operator*(const Mat2& m, const Vec2& v);

	float elements[4];
};