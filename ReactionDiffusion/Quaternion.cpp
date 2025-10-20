#include "Quaternion.h"

#include <math.h>

Quaternion::Quaternion() :
	x(0), y(0), z(0), w(1)
{}

Quaternion::Quaternion(float x, float y, float z, float w) : 
	x(x), y(y), z(z), w(w)
{}

Quaternion::Quaternion(const Vec3& v, float w) : 
	x(v.x), y(v.y), z(v.z), w(w)
{}

void Quaternion::identity()
{
	x = 0.f;
	y = 0.f;
	z = 0.f;
	w = 1.f;
}

float Quaternion::magnitude()
{
	return sqrt(x*x + y*y + z*z + w*w);
}

Quaternion Quaternion::normal()
{
	float mag = magnitude();
	return Quaternion(
		x/mag,
		y/mag,
		z/mag,
		w/mag);
}

Quaternion Quaternion::conjugate()
{
	return Quaternion(-x, -y, -z, w);
}

void Quaternion::add(const Quaternion& q)
{
	x += q.x;
	y += q.y;
	z += q.z;
	w += q.w;
}

void Quaternion::sub(const Quaternion& q)
{
	x -= q.x;
	y -= q.y;
	z -= q.z;
	w -= q.w;
}

Quaternion Quaternion::mul(const Vec3& vec)
{	
	return Quaternion(
		w  * vec.x + y * vec.z - z * vec.y,
		w  * vec.y + z * vec.x - x * vec.z,
		w  * vec.z + x * vec.y - y * vec.x,
		-x * vec.x - y * vec.y - z * vec.z);
}
//Not commutative
Quaternion Quaternion::mul(const Quaternion& quat)
{
	return Quaternion(
		w*quat.x + x*quat.w + y*quat.z - z*quat.y,
		w*quat.y - x*quat.z + y*quat.w + z*quat.x,
		w*quat.z + x*quat.y - y*quat.x + z*quat.w,
		w*quat.w - x*quat.x - y*quat.y - z*quat.z);
}

Mat4 Quaternion::toMat4()
{
	// Source: http://www.gamasutra.com/view/feature/131686/rotating_objects_using_quaternions.php?page=2
	float x2 = x + x; 
	float y2 = y + y;
	float z2 = z + z;
	float xx = x * x2; 
	float xy = x * y2; 
	float xz = x * z2;
	float yy = y * y2; 
	float yz = y * z2; 
	float zz = z * z2;
	float wx = w * x2; 
	float wy = w * y2; 
	float wz = w * z2;

	Mat4 mat;
	mat.elements[0]  = 1.f - (yy + zz);
	mat.elements[4]  = xy - wz;
	mat.elements[8]  = xz + wy; 
	mat.elements[12] = 0.f;

	mat.elements[1]  = xy + wz; 
	mat.elements[5]  = 1.f - (xx + zz);
	mat.elements[9]  = yz - wx;
	mat.elements[13] = 0.f;

	mat.elements[2]  = xz - wy;
	mat.elements[6]  = yz + wx;
	mat.elements[10] = 1.f - (xx + yy); 
	mat.elements[14] = 0.f;

	mat.elements[3]  = 0.f; 
	mat.elements[7]  = 0.f;
	mat.elements[11] = 0.f;
	mat.elements[15] = 1.f;

	return mat;
}

Mat3 Quaternion::toMat3()
{
	float x2 = x + x;
	float y2 = y + y;
	float z2 = z + z;
	float xx = x * x2;
	float xy = x * y2;
	float xz = x * z2;
	float yy = y * y2;
	float yz = y * z2;
	float zz = z * z2;
	float wx = w * x2;
	float wy = w * y2;
	float wz = w * z2;

	Mat3 mat;
	mat.setRow(0, 1.f - (yy + zz),	xy - wz,			xz + wy);
	mat.setRow(1, xy + wz,			1.f - (xx + zz),	yz - wx);
	mat.setRow(2, xz - wy,			yz + wx,			1.f - (xx + yy));

	return mat;
}

std::ostream& operator<<(std::ostream& os, const Quaternion& q)
{
	os << "(" << q.x << ", " << q.y << ", " << q.z << " " << q.w << ")";
	return os;
}

Quaternion operator+(const Quaternion& q1, const Quaternion& q2)
{
	return Quaternion(q1.x + q2.x, q1.y + q2.y, q1.z + q2.z, q1.w + q2.w);
}

Quaternion operator-(const Quaternion& q1, const Quaternion& q2)
{
	return Quaternion(q1.x - q2.x, q1.y - q2.y, q1.z - q2.z, q1.w - q2.w);
}

Quaternion operator*(const Quaternion& q1, const Quaternion& q2)
{
	return Quaternion(
		q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y,
		q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x,
		q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w,
		q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z);
}

Mat3 RotationMat3(float angleInRad, const Vec3& axis)
{
	float halfAngle = angleInRad * .5f;
	Quaternion q(sin(halfAngle) * axis, cos(halfAngle));
	return q.toMat3();
}