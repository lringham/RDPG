#pragma once
#include "Vec3.h"
#include "Vec4.h"
#include "Mat3.h"


class Mat4
{
public:
	explicit Mat4(float mainDiag = 0);
	explicit Mat4(float elements[16]);
	explicit Mat4(Vec3 row1, Vec3 row2, Vec3 row3, Vec3 row4);

	void identity();
	void zero();
	void transpose();
	void set(Mat4 m);
	void set(float newElements[16]);
	void setRow(int index, float x, float y, float z, float w);
	void setRow(int index, Vec3 row, float w = 0);
	void setCol(int index, float x, float y, float z, float w);
	void setCol(int index, Vec3 col, float w = 0);
	Mat3 mat3() const;

	Vec4 col(int index) const;
	Vec4 row(int index) const;

	//operators
	friend std::ostream& operator<<(std::ostream& os, const Mat4& m);
	friend Mat4 operator+(const Mat4& m1, const Mat4& m2);
	friend Mat4 operator-(const Mat4& m1, const Mat4& m2);
	friend Mat4 operator*(const Mat4& m1, const Mat4& m2);	
	friend Vec4 operator*(const Mat4& m, const Vec4& v);

	//fields 
	float elements[16];
};

Mat4 transpose(const Mat4& m);
