#pragma once
#include "Vec3.h"
#include "Vec2.h"
#include "Mat4.h"


class Camera
{
public:
	Camera() = default;
	Camera(const Vec3& position, const Vec3& lookAt, const Vec3& up);

	void setProjMatrix(float fovy, float aspectRatio, float zNear, float zFar);
	void setProjMatrixOrtho(float left, float right, float bottom, float top, float in, float out);
	const float* getProjectionData() const;
	Mat4 getPojectionMat() const;
	Vec3 getDir() const;
	Mat4 getViewMatrix() const;
	Mat4 getInvViewMatrix() const;
	Mat4 getCameraMatrix() const;
	void move(float x, float y, float z);
	void moveTarget(float x, float y, float z);
	Vec3 mouseToWorldRay(int mouseX, int mouseY, int screenWidth, int screenHeight) const;
	Vec2 mouseToNDC(int mouseX, int mouseY, int screenWidth, int screenHeight) const;

	float getFovy() const;
	float getAspectRatio() const;
	float getZNear() const;
	float getZFar() const;
	void fitTo(const std::pair<Vec3, Vec3>& span);

	Vec3 position_{ 0.f, 0.f, 1.f };
	Vec3 lookAt_{ 0.f, 0.f, 0.f };
	Vec3 up_{ 0.f, 1.f, 0.f };
	bool zoomEnabled_ = true;

private:
	Mat4 invProjection_;
	Mat4 projection_;
	float fovy_ = 0.f;
	float aspectRatio_ = 0.f;
	float zNear_ = 0.f;
	float zFar_ = 0.f;
};

