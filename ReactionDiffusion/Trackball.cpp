#include "Trackball.h"
#include "Window.h"


void Trackball::update(float x, float y, bool pressed, const Window& window)
{
	// Normalize coordinates
	float aspectRatio = window.getAspectRatio(); // used to ensure y-rotation speed is the same as x
	x = (2.f * x / window.getWidth()) - 1.f;
	y = (y - window.getHeight()) / aspectRatio;
	y = (2.f * y / window.getHeight()) - 1.f;

    if (pressed)
    {
		float dx = x - lastX_;
		float dy = y - lastY_;

		Mat3 rotMat = drawable_->transform_.getRotationMat().mat3();
		rotMat = createRotXMat(3.14159265f * dy) * rotMat; // rotate around x-axis
		rotMat = createRotYMat(3.14159265f * dx) * rotMat; // rotate around y-axis
		drawable_->transform_.setOrientation(rotMat.col(0), rotMat.col(1), rotMat.col(2));
	}

	lastX_ = x;
	lastY_ = y;
}

void Trackball::setDrawable(Drawable* drawablePtr)
{
	this->drawable_ = drawablePtr;
	up_ = drawable_->transform_.getRotationMat().col(1).xyz();
	forward_ = drawable_->transform_.getRotationMat().col(2).xyz();
	origUp_ = up_;
	origForward_ = forward_;
}

Drawable* Trackball::getDrawable()
{
	return drawable_;
}

void Trackball::setOrientation(const Vec3& newForward, const Vec3& newUp)
{
	this->forward_ = normalize(newForward);
	this->up_ = normalize(newUp);
	drawable_->transform_.setOrientation(normalize(cross(newUp, newForward)), newUp, newForward);
}

void Trackball::reset()
{
	up_ = origUp_;
	forward_ = origForward_;
	drawable_->transform_.setOrientation(up_.cross(forward_), up_, forward_);
}