#include "Mat2.h"


Mat2::Mat2(float mainDiag) : elements{  mainDiag, 0,
										0, mainDiag}
{}

Mat2::Mat2(float elements[4])
{
	for (int i = 0; i < 4; i++)
		this->elements[i] = elements[i];
}

void Mat2::identity()
{
	float newElements[] = {
		1.f, 0.f,
		0.f, 1.f
	};
	for(int i = 0; i < 4; ++i)
		elements[i] = newElements[i];
}

void Mat2::zero()
{
	float newElements[] = {
		0.f, 0.f,
		0.f, 0.f
	};
	for(int i = 0; i < 4; ++i)
		elements[i] = newElements[i];
}

void Mat2::set(float newElements[9])
{
	for (int i = 0; i < 4; ++i)
		elements[i] = newElements[i];
}

void Mat2::set(Mat2 m)
{
	set(m.elements);
}

void Mat2::setRow(int index, float x, float y)
{
	int off = 2 * index;
	elements[off] = x;
	elements[off + 1] = y;
}

void Mat2::setRow(int index, Vec2 row)
{
	int off = 2 * index;
	elements[off] = row.x_;
	elements[off + 1] = row.y_;
}

void Mat2::setCol(int index, float x, float y)
{
	elements[0 + index] = x;
	elements[2 + index] = y;
}

void Mat2::setCol(int index, Vec2 col)
{
	elements[0 + index] = col.x_;
	elements[2 + index] = col.y_;
}


void Mat2::transpose()
{
	//transpose
	float transposed[4] = {
		elements[0], elements[2],
		elements[1], elements[3]
	};

	//copy elements
	for (int i = 0; i < 4; i++)
		elements[i] = transposed[i];
}

// Opertors
std::ostream& operator<<(std::ostream& os, const Mat2& m)
{
	for (int i = 0; i < 2; i++)
	{
		os << "[";
		for (int j = 0; j < 2; j++)
		{
			os << m.elements[i * 2 + j];
			if (j != 2) os << " ";
		}
		os << "]" << std::endl;
	}
	return os;
}

Mat2 operator+(const Mat2& m1, const Mat2& m2)
{
	Mat2 m;
	for (int i = 0; i < 4; i++)
		m.elements[i] = m1.elements[i] + m2.elements[i];
	return m;
}

Mat2 operator-(const Mat2& m1, const Mat2& m2)
{
	Mat2 m;
	for (int i = 0; i < 4; i++)
		m.elements[i] = m1.elements[i] - m2.elements[i];
	return m;
}

Vec2 operator*(const Mat2& m, const Vec2& v)
{
	Vec2 ans;

	ans.x_ = m.elements[0] * v.x_ + m.elements[1] * v.y_;
	ans.y_ = m.elements[2] * v.x_ + m.elements[3] * v.y_;

	return ans;
}

Mat2 operator*(const Mat2& m1, const Mat2& m2)
{
	Mat2 m;

	m.elements[0] = m1.elements[0] * m2.elements[0] + m1.elements[1] * m2.elements[2];
	m.elements[1] = m1.elements[0] * m2.elements[1] + m1.elements[1] * m2.elements[3];
	
	m.elements[2] = m1.elements[2] * m2.elements[0] + m1.elements[3] * m2.elements[2];
	m.elements[3] = m1.elements[2] * m2.elements[1] + m1.elements[3] * m2.elements[3];

	return m;
}
