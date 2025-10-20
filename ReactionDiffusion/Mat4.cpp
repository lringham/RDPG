#include "Mat4.h"


Mat4::Mat4(float mainDiag) : elements{ mainDiag,0,0,0,
									   0,mainDiag,0,0,
									   0,0,mainDiag,0,
									   0,0,0,mainDiag }
{}

Mat4::Mat4(float elements[16])
{
	for (int i = 0; i < 16; i++)
		this->elements[i] = elements[i];
}

Mat4::Mat4(Vec3 row1, Vec3 row2, Vec3 row3, Vec3 row4)
{
	setRow(0, row1);
	setRow(1, row2);
	setRow(2, row3);
	setRow(3, row4, 1);
}

void Mat4::identity()
{
	float newElements[] = {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f };
	for(int i = 0; i < 16; ++i)
		elements[i] = newElements[i];
}

void Mat4::zero()
{
	for (int i = 0; i < 16; i++)	
		elements[i] = 0;
}

void Mat4::set(float newElements[16])
{
	for (int i = 0; i < 16; ++i)
		elements[i] = newElements[i];
}

void Mat4::set(Mat4 m)
{
	set(m.elements);
}

void Mat4::setRow(int index, float x, float y, float z, float w)
{
	int off = 4 * index;

	elements[off]     = x;
	elements[off + 1] = y;
	elements[off + 2] = z;
	elements[off + 3] = w;
}

void Mat4::setRow(int index, Vec3 row, float w)
{
	int off = 4 * index;
	
	elements[off]	  = row.x;
	elements[off + 1] = row.y;
	elements[off + 2] = row.z;
	elements[off + 3] = w;
}

void Mat4::setCol(int index, float x, float y, float z, float w)
{
	elements[0 + index] = x;
	elements[4 + index] = y;
	elements[8 + index] = z;
	elements[12+ index] = w;
}

void Mat4::setCol(int index, Vec3 col, float w)
{
	elements[0 + index] = col.x;
	elements[4 + index] = col.y;
	elements[8 + index] = col.z;
	elements[12 + index] = w;
}

Mat3 Mat4::mat3() const
{
	Mat3 m;
	m.setRow(0, row(0).xyz());
	m.setRow(1, row(1).xyz());
	m.setRow(2, row(2).xyz());
	return m;
}

Vec4 Mat4::col(int index) const
{
	return Vec4(elements[index], elements[index + 4], elements[index + 8], elements[index + 12]);
}

Vec4 Mat4::row(int index) const
{
	return Vec4(elements[index * 4], elements[1 + index * 4], elements[2 + index * 4], elements[3 + index * 4]);
}

void Mat4::transpose()
{
	//transpose
	float transposed[16] = {
		elements[0], elements[4], elements[8], elements[12],
		elements[1], elements[5], elements[9], elements[13],
		elements[2], elements[6], elements[10], elements[14],
		elements[3], elements[7], elements[11], elements[15]
	};

	//copy elements
	for (int i = 0; i < 16; i++)
		elements[i] = transposed[i];
}

// Opertors
std::ostream& operator<<(std::ostream& os, const Mat4& m)
{
	for (int i = 0; i < 4; i++)
	{
		os << "[";
		for (int j = 0; j < 4; j++)
		{
			os << m.elements[i*4+j];
			if (j != 3) os << " ";
		}
		os << "]" << std::endl;
	}
	return os;
}

Mat4 operator+(const Mat4& m1, const Mat4& m2)
{
	Mat4 m;
	for (int i = 0; i < 16; i++)
		m.elements[i] = m1.elements[i] + m2.elements[i];
	return m;
}

Mat4 operator-(const Mat4& m1, const Mat4& m2)
{
	Mat4 m;
	for (int i = 0; i < 16; i++)
		m.elements[i] = m1.elements[i] - m2.elements[i];
	return m;
}

Vec4 operator*(const Mat4& m, const Vec4& v)
{
	return Vec4(m.elements[0]  * v.x + m.elements[1]  * v.y + m.elements[2]  * v.z + m.elements[3]  * v.w,
				m.elements[4]  * v.x + m.elements[5]  * v.y + m.elements[6]  * v.z + m.elements[7]  * v.w,
				m.elements[8]  * v.x + m.elements[9]  * v.y + m.elements[10] * v.z + m.elements[11] * v.w,
				m.elements[12] * v.x + m.elements[13] * v.y + m.elements[14] * v.z + m.elements[15] * v.w);	
}

Mat4 operator*(const Mat4& m1, const Mat4& m2)
{
	Mat4 m;
	m.elements[0] = m1.elements[0] * m2.elements[0] + m1.elements[1] * m2.elements[4] + m1.elements[2] * m2.elements[8] + m1.elements[3] * m2.elements[12];
	m.elements[1] = m1.elements[0] * m2.elements[1] + m1.elements[1] * m2.elements[5] + m1.elements[2] * m2.elements[9] + m1.elements[3] * m2.elements[13];
	m.elements[2] = m1.elements[0] * m2.elements[2] + m1.elements[1] * m2.elements[6] + m1.elements[2] * m2.elements[10] + m1.elements[3] * m2.elements[14];
	m.elements[3] = m1.elements[0] * m2.elements[3] + m1.elements[1] * m2.elements[7] + m1.elements[2] * m2.elements[11] + m1.elements[3] * m2.elements[15];

	m.elements[4] = m1.elements[4] * m2.elements[0] + m1.elements[5] * m2.elements[4] + m1.elements[6] * m2.elements[8]  + m1.elements[7] * m2.elements[12];
	m.elements[5] = m1.elements[4] * m2.elements[1] + m1.elements[5] * m2.elements[5] + m1.elements[6] * m2.elements[9]  + m1.elements[7] * m2.elements[13];
	m.elements[6] = m1.elements[4] * m2.elements[2] + m1.elements[5] * m2.elements[6] + m1.elements[6] * m2.elements[10] + m1.elements[7] * m2.elements[14];
	m.elements[7] = m1.elements[4] * m2.elements[3] + m1.elements[5] * m2.elements[7] + m1.elements[6] * m2.elements[11] + m1.elements[7] * m2.elements[15];

	m.elements[8]  = m1.elements[8] * m2.elements[0] + m1.elements[9] * m2.elements[4] + m1.elements[10] * m2.elements[8]  + m1.elements[11] * m2.elements[12];
	m.elements[9]  = m1.elements[8] * m2.elements[1] + m1.elements[9] * m2.elements[5] + m1.elements[10] * m2.elements[9]  + m1.elements[11] * m2.elements[13];
	m.elements[10] = m1.elements[8] * m2.elements[2] + m1.elements[9] * m2.elements[6] + m1.elements[10] * m2.elements[10] + m1.elements[11] * m2.elements[14];
	m.elements[11] = m1.elements[8] * m2.elements[3] + m1.elements[9] * m2.elements[7] + m1.elements[10] * m2.elements[11] + m1.elements[11] * m2.elements[15];

	m.elements[12] = m1.elements[12] * m2.elements[0] + m1.elements[13] * m2.elements[4] + m1.elements[14] * m2.elements[8]  + m1.elements[15] * m2.elements[12];
	m.elements[13] = m1.elements[12] * m2.elements[1] + m1.elements[13] * m2.elements[5] + m1.elements[14] * m2.elements[9]  + m1.elements[15] * m2.elements[13];
	m.elements[14] = m1.elements[12] * m2.elements[2] + m1.elements[13] * m2.elements[6] + m1.elements[14] * m2.elements[10] + m1.elements[15] * m2.elements[14];
	m.elements[15] = m1.elements[12] * m2.elements[3] + m1.elements[13] * m2.elements[7] + m1.elements[14] * m2.elements[11] + m1.elements[15] * m2.elements[15];
	return m;
}


// Non-member functions
Mat4 transpose(const Mat4& m)
{
	Mat4 mT;

	mT.elements[0] = m.elements[0];
	mT.elements[1] = m.elements[4];
	mT.elements[2] = m.elements[8];
	mT.elements[3] = m.elements[12];

	mT.elements[4] = m.elements[1];
	mT.elements[5] = m.elements[5];
	mT.elements[6] = m.elements[9];
	mT.elements[7] = m.elements[13];

	mT.elements[8] = m.elements[2];
	mT.elements[9] = m.elements[6];
	mT.elements[10] = m.elements[10];
	mT.elements[11] = m.elements[14];

	mT.elements[12] = m.elements[3];
	mT.elements[13] = m.elements[7];
	mT.elements[14] = m.elements[11];
	mT.elements[15] = m.elements[15];

	return mT;
}

// TODO: Add non member funcs
