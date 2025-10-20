#pragma once
#include "Quaternion.h"
#include "Vec3.h"
#include "Mat4.h"
#include "Mat3.h"
#include "Transform.h"

#include <math.h>


class Transform
{
public:
	explicit Transform();

	Mat4 getTransformMat() const; 
	Mat4 getInvTransformMat() const;
	Mat4 getRotationMat() const;
	void reset();

	float getScale() const;
	Vec3 getPosition() const;
	void setPosition(const Vec3& position);
	void setPosition(float x, float y, float z);
	void translate(const Vec3& translation);
	void translate(float x, float y, float z);
	void setScale(float scale);
	void scale(float scale);
	void setOrientation(const Vec3& x, const Vec3& y, const Vec3& z);
	void rotate(float angle, const Vec3& axis);

	void setCustomTransform(const Mat4& customTransform);
	void useCustomTransform(bool useCustomTransform);
	bool usingCustomTransform();

	Mat4 customTransform;

private:
	Mat4 scalingMat;
	Mat4 translationMat;
	Mat4 rotationMat;				
	Vec3 position;
	float scaling = 1.f;
	bool customTransformSet = false;
};

// Method stubs
Mat3 createRotXMat(float angleDeg);
Mat3 createRotYMat(float angleDeg);
Mat3 createRotZMat(float angleDeg);
Mat4 createTranslationMat(const Vec3& vec);
Mat4 createTranslationMat(float x, float y, float z);
Mat4 createScaleMat(const Vec3& vec);
Mat4 createScaleMat(float val);
Mat4 createPerspectiveProj(float fovy, float aspectRatio, float Znear, float Zfar, Mat4* invMat = nullptr);
Mat4 createOrthographicProj(float left, float right, float bottom, float top, float in, float out);