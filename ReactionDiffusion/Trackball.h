#pragma once
#include "Drawable.h"
#include "Vec3.h"


class Drawable;
class Window;
class Trackball
{
public:
	Trackball() = default;
	void reset();
	void update(float x, float y, bool pressed, const Window& window);
	void setDrawable(Drawable* drawable);
	Drawable* getDrawable();
	void setOrientation(const Vec3& forward, const Vec3& up);

private:
	Drawable* drawable_ = nullptr;
	float sensitivity_ = 4.f;
	float lastX_ = 0.f, lastY_ = 0.f, lastZ_ = 0.f;
	
	Vec3 forward_ = Vec3(0.f, 0.f, 1.f);
	Vec3 up_ = Vec3(0.f, 1.f, 0.f);
	Vec3 origForward_, origUp_;
};

