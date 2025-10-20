#pragma once
#include <vector>

class Camera;
class Drawable;
class Simulation;
class Scene
{
public:
	Scene() = default;

	void update();
	void draw(const Camera& camera, bool updateColors = true);
	void pause(bool paused);
	void step();
	bool isPaused() const;
	void addDrawable(Drawable* drawable);
	unsigned getNumDrawables() const;
	void removeDrawable(Drawable* drawable);
	void removeDrawables();
	void setSimulation(Simulation* simulation);
	bool hasSimulation() const;
	
private:
	std::vector<Drawable*> drawables_;
	Simulation* simulation_ = nullptr;
	bool stepCheck_ = false;
};
