#include "CameraController.h"
#include "Camera.h"


void CameraController::init(Camera* camera) 
{
	this->camera_ = camera;
}

void CameraController::notifyKeyEvent(int key, Action state, bool guiUsedEvent)
{
	bool pressed = state == Action::press || state == Action::unknown;
	switch (key)
	{
	case 'W':
		if(!guiUsedEvent)
			moveUp_ = pressed;
		else if(moveUp_ && !pressed)
			moveUp_ = pressed;
		break;
	case 'A':
		if (!guiUsedEvent)
			moveLeft_ = pressed;
		else if (moveLeft_ && !pressed)
			moveLeft_ = pressed;
		break;
	case 'S':
		if (!guiUsedEvent)
			moveDown_ = pressed;
		else if (moveDown_ && !pressed)
			moveDown_ = pressed;
		break;
	case 'D':
		if (!guiUsedEvent)
			moveRight_ = pressed;
		else if (moveRight_ && !pressed)
			moveRight_ = pressed;
		break;
	case 'R':
		if (!guiUsedEvent)
			moveIn_ = pressed;
		else if (moveIn_ && !pressed)
			moveIn_ = pressed;
		break;
	case 'F':
		if (!guiUsedEvent)
			moveOut_ = pressed;
		else if (moveOut_ && !pressed)
			moveOut_ = pressed;
		break;		
	}
}

void CameraController::notifyMouseButtonEvent(int button, Action state, int)
{
	if (button == 0/*GLFW_MOUSE_BUTTON_1*/)
		leftMouseButton_ = state == Action::press;
	if (button == 1/*GLFW_MOUSE_BUTTON_2*/)
		rightMouseButton_ = state == Action::press;
}

void CameraController::notifyMouseScrollEvent(double, double yOffset)
{
	if (camera_->zoomEnabled_)
	{
		Vec3 dir = camera_->lookAt_ - camera_->position_;
		Vec3 pNew = camera_->position_ + static_cast<float>(yOffset) * dir * .1f;
		if(dir.dot(camera_->lookAt_ - pNew) > 0)
			camera_->position_ = pNew;
	}
}

void CameraController::notifyMouseMoveEvent(double x, double y)
{
	if (camera_->zoomEnabled_)
	{
		if (rightMouseButton_)
		{
			float dx = static_cast<float>(x) - prevX_;
			float dy = static_cast<float>(y) - prevY_;
			float dist = (camera_->lookAt_ - camera_->position_).length();
			camera_->position_.x	-= (dx * movespeed_) * dist;
			camera_->lookAt_.x	-= (dx * movespeed_) * dist;
			camera_->position_.y	+= (dy * movespeed_) * dist;
			camera_->lookAt_.y	+= (dy * movespeed_) * dist;
		}
	}
	prevX_ = static_cast<float>(x);
	prevY_ = static_cast<float>(y);
}

void CameraController::update()
{
	if (moveDown_)
	{
		camera_->position_.y -= movespeed_;
		camera_->lookAt_.y -= movespeed_;
	}
	if (moveLeft_)
	{
		camera_->position_.x -= movespeed_;
		camera_->lookAt_.x -= movespeed_;
	}
	if (moveUp_)
	{
		camera_->position_.y += movespeed_;
		camera_->lookAt_.y += movespeed_;
	}
	if (moveRight_)
	{
		camera_->position_.x += movespeed_;
		camera_->lookAt_.x += movespeed_;
	}
	if (moveIn_)
	{
		camera_->position_.z -= movespeed_;
		camera_->lookAt_.z -= movespeed_;
	}
	if (moveOut_)
	{
		camera_->position_.z += movespeed_;
		camera_->lookAt_.z += movespeed_;
	}
}

void CameraController::notifyMouseEntered(bool)
{
}