#pragma once
#include "EventListener.h"
#include "Simulation.h"
#include "SimulationDomain.h"
#include "Mat4.h"
#include "Window.h"
#include "Trackball.h"
#include "Scene.h"
#include "HalfEdgeMesh.h"
#include "Camera.h"

#include <functional>


class DefaultController : 
    public EventListener
{
public:
    DefaultController() = default;
    virtual ~DefaultController() = default;

    void init(Drawable* cursor, SimulationDomain* drawable, Simulation* sim, Window* window, Trackball* trackball = nullptr, Scene* scene = nullptr, Camera* camera = nullptr);
    void notifyKeyEvent(int key, Action state, bool guiUsedEvent) override;
    void notifyMouseMoveEvent(double x, double y) override;
    void notifyMouseButtonEvent(int button, Action state, int mods) override;
    void notifyMouseScrollEvent(double xOffset, double yOffset) override;
    void notifyMouseEntered(bool hasEntered) override;
    void assign(SimulationDomain* drawable);
    bool hasPainted() const;

    void setDisableRenderingCallback(std::function<void(void)>& func);
    void setPauseCallback(std::function<void(bool)>& func);

private:
    bool leftMouseButton_ = false, rightMouseButton_ = false, shift_ = false;
    bool painted_ = false;
    bool animationInteraction_ = false;
    Drawable* cursor_ = nullptr;
    Camera* camera_ = nullptr;
    Simulation* sim_ = nullptr;
    SimulationDomain* drawable_ = nullptr;
    Window* window_ = nullptr;
    Trackball* trackball_ = nullptr;
    Scene* scene_ = nullptr;
    std::function<void()> disableRenderingCallback_;
    std::function<void(bool)> pauseCallback_;
    unsigned selectedDrawableIndex_ = 0;
};
