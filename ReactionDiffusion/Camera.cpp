#include "Camera.h"
#include "Vec4.h"
#include "Transform.h"


Camera::Camera(const Vec3& position, const Vec3& lookAt, const Vec3& up)
	: position_(position), lookAt_(lookAt), up_(up), zoomEnabled_(true)
{}

void Camera::setProjMatrix(float fovy, float aspectRatio, float zNear, float zFar)
{
	this->fovy_ = fovy;
	this->aspectRatio_ = aspectRatio;
	this->zNear_ = zNear;
	this->zFar_ = zFar;
	projection_ = createPerspectiveProj(fovy, aspectRatio, zNear, zFar, &invProjection_);
}

void Camera::setProjMatrixOrtho(float left, float right, float bottom, float top, float in, float out)
{
	projection_ = createOrthographicProj(left, right, bottom, top, in, out);
}

const float* Camera::getProjectionData() const
{
	return projection_.elements;
}

Mat4 Camera::getPojectionMat() const
{
	return projection_;
}

Vec3 Camera::getDir() const
{
	Vec3 dir = lookAt_ - position_;
	dir.normalize();
	return dir;
}

Mat4 Camera::getViewMatrix() const
{
	// Form new basis
	Vec3 dir = normalize(position_ - lookAt_);
	Vec3 right = normalize(up_.cross(dir));
	Vec3 up = normalize(dir.cross(right));

	// This is transposed because the inverse of a orthogonal matrix is its transpose.
	// We want the inverse so that opposite rotation is applied to our vertices	
	float viewMatrix[]{
		right.x,  right.y,   right.z,	 0.f,
		up.x,	  up.y,		 up.z,		 0.f,
		dir.x,    dir.y,     dir.z,      0.f,
		0.f,	  0.f,		 0.f,		 1.f
	};

	float invTranslation[]{
		1.f, 0.f, 0.f, -position_.x,
		0.f, 1.f, 0.f, -position_.y,
		0.f, 0.f, 1.f, -position_.z,
		0.f, 0.f, 0.f, 1.f
	};

	return Mat4(viewMatrix) * Mat4(invTranslation);
}

Mat4 Camera::getInvViewMatrix() const
{
	Vec3 dir = normalize(position_ - lookAt_);
	Vec3 right = normalize(up_.cross(dir));
	Vec3 up = normalize(dir.cross(right));

	float invViewMatrix[]{
		right.x,  up.x,   dir.x,   position_.x,
		right.y,  up.y,   dir.y,   position_.y,
		right.z,  up.z,   dir.z,   position_.z,
		0.f,	  0.f,		 0.f,	       1.f
	};

	return Mat4(invViewMatrix);
}

Mat4 Camera::getCameraMatrix() const
{
	Mat4 view = getViewMatrix();
	view.transpose();
	return view;
}

void Camera::move(float x, float y, float z)
{
	position_.x += x;
	position_.y += y;
	position_.z += z;
}

void Camera::moveTarget(float x, float y, float z)
{
	lookAt_.x += x;
	lookAt_.y += y;
	lookAt_.z += z;
}

Vec2 Camera::mouseToNDC(int mouseX, int mouseY, int screenWidth, int screenHeight) const
{
	return Vec2(2.f * (float)mouseX / (float)screenWidth - 1.f, 1.f - 2.f * (float)mouseY / (float)screenHeight);
}

Vec3 Camera::mouseToWorldRay(int mouseX, int mouseY, int screenWidth, int screenHeight) const
{
	// Viewport -> NDC
	float x = 2.f * (float)mouseX / (float)screenWidth - 1.f;  // map mousex to [-1, 1]
	float y = 1.f - 2.f * (float)mouseY / (float)screenHeight; // map mousey to [-1, 1]

	// Point ray forward
	Vec4 ray_clip(x, y, -1.f, 1.f);

	// Unproject x, y
	// Perform inverse projection (clip -> view)
	Vec4 ray_eye = invProjection_ * ray_clip;
	ray_eye.z = -1; // point down z (forward in camera terms)
	ray_eye.w = 0;  // not a point

	// Perform inverse view transform (view -> world) and normalize
	return normalize((getInvViewMatrix() * ray_eye).xyz());
}

float Camera::getFovy() const
{
	return fovy_;
}

float Camera::getAspectRatio() const
{
	return aspectRatio_;
}

float Camera::getZNear() const
{
	return zNear_;
}

float Camera::getZFar() const
{
	return zFar_;
}

void Camera::fitTo(const std::pair<Vec3, Vec3>& span)
{
	Vec3 minSpan = span.second; // TODO: this isn't right
	Vec3 maxSpan = span.first;

	float l = length(minSpan - maxSpan);
	position_.z = l / tan(fovy_) - maxSpan.z;
	position_.x = ((minSpan + maxSpan) * .5f).x;
	position_.y = ((minSpan + maxSpan) * .5f).y;
	position_.z *= 1.25f; // Move it back a bit
	lookAt_ = position_;
	lookAt_.z = 0;
}