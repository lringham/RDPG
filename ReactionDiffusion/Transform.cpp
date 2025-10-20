#include "Transform.h"

Transform::Transform()
{
	reset();
}

void Transform::reset()
{
	translationMat.identity();	
	scalingMat.identity();
	rotationMat.identity();

	position.zero();
	scaling = 1.f;
}

void Transform::setCustomTransform(const Mat4& newCustomTransform)
{
	this->customTransform = newCustomTransform;
	useCustomTransform(true);
}

void Transform::useCustomTransform(bool useCustomTransform)
{
	this->customTransformSet = useCustomTransform;
}

bool Transform::usingCustomTransform()
{
	return customTransformSet;
}

Mat4 Transform::getTransformMat() const
{
	if (customTransformSet)
		return customTransform;
	else
		return translationMat * rotationMat * scalingMat;	
}

Mat4 Transform::getInvTransformMat() const
{
	Mat4 invTrans = translationMat;
	invTrans.elements[3]  *= -1.f;
	invTrans.elements[7]  *= -1.f;
	invTrans.elements[11] *= -1.f;

	Mat4 invScale = scalingMat;
	invScale.elements[0] /= 1.f;
	invScale.elements[4] /= 1.f;
	invScale.elements[8] /= 1.f;

	Mat4 invRotation = transpose(rotationMat);

	return  invScale * invRotation * invTrans;
}

Mat4 Transform::getRotationMat() const
{
	return rotationMat;
}

float Transform::getScale() const
{
	return scaling;
}

Vec3 Transform::getPosition() const
{
	return position;
}

void Transform::scale(float scale)
{
	this->scaling *= scale;
	scalingMat = createScaleMat(scaling);
}

void Transform::setPosition(const Vec3& newPosition)
{
	this->position.set(newPosition);
	translationMat = createTranslationMat(newPosition);
}

void Transform::setPosition(float x, float y, float z)
{
	position.set(x, y, z);
	translationMat = createTranslationMat(position);
}

void Transform::translate(const Vec3& translation)
{
	position.add(translation);
	translationMat = createTranslationMat(position);
}

void Transform::translate(float x, float y, float z)
{
	position.add(x, y, z);
	translationMat = createTranslationMat(position);
}

void Transform::setScale(float scale)
{
	scaling = scale;
	scalingMat = createScaleMat(scale);
}

void Transform::setOrientation(const Vec3& x, const Vec3& y, const Vec3& z)
{
	rotationMat.setCol(0, x, 0);
	rotationMat.setCol(1, y, 0);
	rotationMat.setCol(2, z, 0);
	rotationMat.setCol(3, 0, 0, 0, 1);
}

void Transform::rotate(float angle, const Vec3& axis)
{
	Vec3 x(1, 0, 0);
	Vec3 y(0, 1, 0);
	x.rotate(angle, axis);
	y.rotate(angle, axis);

	Mat4 rot;
	rot.setCol(0, x, 0);
	rot.setCol(1, y, 0);
	rot.setCol(2, cross(x, y), 0);
	rot.setCol(3, 0, 0, 0, 1);

	rotationMat = rot * rotationMat;
}

// non member functions 
Mat4 createTranslationMat(const Vec3& vec)
{
	float newElements[] = {
		1.f, 0.f, 0.f, vec.x,
		0.f, 1.f, 0.f, vec.y,
		0.f, 0.f, 1.f, vec.z,
		0.f, 0.f, 0.f, 1.f };
	return Mat4(newElements);
}

Mat3 createRotZMat(float angleDeg)
{
    float newElements[] = {
        cos(angleDeg), -sin(angleDeg), 0.f,
		sin(angleDeg), cos(angleDeg),  0.f,
        0.f,        0.f,         1.f
	};
    return Mat3(newElements);
}

Mat3 createRotYMat(float angleDeg)
{
    float newElements[] = {
        cos(angleDeg), 0.f, sin(angleDeg),
		0.f,        1.f, 0.f,
		-sin(angleDeg), 0.f, cos(angleDeg)
    };
    return Mat3(newElements);
}

Mat3 createRotXMat(float angleDeg)
{
    float newElements[] = {
		1.f, 0.f, 0.f,
		0.f, cos(angleDeg), -sin(angleDeg),
        0.f, sin(angleDeg), cos(angleDeg)
    };
    return Mat3(newElements);
}

Mat4 createTranslationMat(float x, float y, float z)
{
	float newElements[] = {
		1.f, 0.f, 0.f, x,
		0.f, 1.f, 0.f, y,
		0.f, 0.f, 1.f, z,
		0.f, 0.f, 0.f, 1.f };
	return Mat4(newElements);
}

Mat4 createScaleMat(const Vec3& vec)
{
	float newElements[] = {
		vec.x, 0.f, 0.f, 0.f,
		0.f, vec.y, 0.f, 0.f,
		0.f, 0.f, vec.z, 0.f,
		0.f, 0.f, 0.f, 1.f };
	return Mat4(newElements);
}

Mat4 createScaleMat(float val)
{
	float newElements[] = {
		val, 0.f, 0.f, 0.f,
		0.f, val, 0.f, 0.f,
		0.f, 0.f, val, 0.f,
		0.f, 0.f, 0.f, 1.f };
	return Mat4(newElements);
}

Mat4 createPerspectiveProj(float fovy, float aspectRatio, float Znear, float Zfar, Mat4* invMat)
{	
	float n = Znear;
	float f = Zfar;
	float t = tan(fovy / 2.f)*Znear;
	float b = -t;
	float r = t * aspectRatio;
	float l = b * aspectRatio;
	
	// TODO: Simplify this
	float A = 2.f*n / (r - l);
	float B = (r + l) / (r - l);
	float C = 2.f*n / (t - b);
	float D = (t + b) / (t - b);
	float E = -(f + n) / (f - n);
	float F = -2.f*f*n / (f - n);

	float newElements[] =
	{   2.f*n/(r-l),	0.f,			(r+l)/(r-l),	0.f,
		0.f,			2.f*n/(t-b),	(t+b)/(t-b),	0.f, 
		0.f,			0.f,			-(f+n)/(f-n),	-2.f*f*n/(f-n),
		0.f,			0.f,			-1.f,			0.f};

	if (invMat != nullptr)
	{
		float newElementsInv[] = {
							1.f/A,	0,		0,		B/A,
							0,		1.f/C,  0,		D/C,
							0,      0,      0,		-1.f,
							0,      0,      1.f/F,	E/F};
		*invMat = Mat4(newElementsInv);
	}
	return Mat4(newElements);
}

Mat4 createOrthographicProj(float left, float right, float bottom, float top, float in, float out)
{
	float newElements[] =
	{ 2.f / (right - left),				 0.f,								0.f,						 (-(right + left)) / (right - left),
		0.f,								 2.f / (top - bottom),				0.f,						 (-(top + bottom)) / (top - bottom),
		0.f,							     0.f,								-2.f / (out - in),			 (-(out + in)) / (out - in),
		0.f,								 0.f,								0.f,						 1.f };
	return Mat4(newElements);
}