#pragma once
#include "EventListener.h"


class Camera;
class CameraController 
	: public EventListener
{
public:
	CameraController() = default;
	virtual ~CameraController() = default;

	void init(Camera* camera);
	void notifyKeyEvent(int key, Action state, bool guiUsedEvent) override;
	void notifyMouseMoveEvent(double x, double y) override;
	void notifyMouseButtonEvent(int button, Action state, int mods) override;
	void notifyMouseScrollEvent(double xOffset, double yOffset) override;
	void notifyMouseEntered(bool) override;
	void update();

private:
	Camera* camera_ = nullptr;
	bool moveLeft_ = false, moveRight_ = false, moveUp_ = false, moveDown_ = false, moveIn_ = false, moveOut_ = false, leftMouseButton_ = false, rightMouseButton_ = false;
	float prevX_ = 0.f, prevY_ = 0.f;
	float movespeed_ = 0.001f;
};

