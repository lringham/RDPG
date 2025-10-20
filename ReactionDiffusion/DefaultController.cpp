#include "DefaultController.h"
#include "Vec3.h"
#include "Plane.h"
#include "GPUSimulation.h"
#include "Grid.h"


void DefaultController::init(Drawable* cursor, SimulationDomain* drawable, Simulation* sim, Window* window, Trackball* trackball, Scene* scene, Camera* camera)
{
    cursor_ = cursor;
    camera_ = camera;
    sim_ = sim;
    drawable_ = drawable;
    window_ = window;
    trackball_ = trackball;
    scene_ = scene;
}

void DefaultController::assign(SimulationDomain* drawable)
{
    drawable_ = drawable;
}

bool DefaultController::hasPainted() const
{
    return painted_;
}

void DefaultController::setDisableRenderingCallback(std::function<void(void)>& func)
{
    disableRenderingCallback_ = func;
}

void DefaultController::setPauseCallback(std::function<void(bool)>& func)
{
    pauseCallback_ = func;
}

void DefaultController::notifyKeyEvent(int key, Action state, bool guiUsedEvent)
{
    bool pressed = state == Action::press || state == Action::unknown;
    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        std::fill(drawable_->paintingDiffDir.begin(), drawable_->paintingDiffDir.end(), false);
        std::fill(drawable_->paintingMorph.begin(), drawable_->paintingMorph.end(), false);
        drawable_->selectCells = false;
        drawable_->childern_[0]->visible_ = false;
        drawable_->shader_->setSelectingCells(false);
        cursor_->visible_ = false;
        break;
    case '1':
        if (state == Action::press && !guiUsedEvent)
        {
            trackball_->reset();
            camera_->fitTo(calculateBounds(*drawable_));
        }
        break;
    case 'L':
        if (!guiUsedEvent && state == Action::release)
            sim_->shouldReloadSim(true);
        break;
    case 'Q':
        if (state == Action::press && !guiUsedEvent)
            window_->graphics_.toggleWireframe();
        break;
    case ' ':
        if (state == Action::press && !guiUsedEvent) {
            if (scene_ != nullptr && scene_->hasSimulation() && scene_->isPaused())
            {
                if (sim_->isGPUEnabled)
                {
                    sim_->domain->selectCells = false;
                    std::fill(sim_->domain->paintingDiffDir.begin(), sim_->domain->paintingDiffDir.end(), false);
                    for (auto& m : sim_->morphIndexMap)
                        sim_->domain->paintingMorph[m.second] = false;
                }
                pauseCallback_(false);
            }
            else
                pauseCallback_(true);
        }
        break;
    case 341 /*left cntl*/:
        drawable_->alternateControlMode = pressed;
        break;
    case GLFW_KEY_F1:
        if (state == Action::press)
            window_->getGUI()->toggleVisible();
        break;
    case GLFW_KEY_F2:
        if (state == Action::press && disableRenderingCallback_)
            disableRenderingCallback_();
        break;
    case 340 /*shift*/:
        if (state == Action::press)
            window_->getGUI()->setDisablePainting(true);
        else if (state == Action::release)
            window_->getGUI()->setDisablePainting(false);
        break;
    case 'T':
        if (state == Action::press) 
        {
            animationInteraction_ = !animationInteraction_;
            LOG(animationInteraction_);
        }
        break;
    }
}

void DefaultController::notifyMouseButtonEvent(int button, Action state, int)
{
    if (button == GLFW_MOUSE_BUTTON_1)
        leftMouseButton_ = state == Action::press;
    else if (button == GLFW_MOUSE_BUTTON_2)
        rightMouseButton_ = state == Action::press || state == Action::unknown;

    if (sim_->domain->isPainting() && button == GLFW_MOUSE_BUTTON_2 && state == Action::release)
    { 
        if (sim_->isGPUEnabled && painted_)
        {
            sim_->updateGPU();
            painted_ = false;
        }
    }
}

void DefaultController::notifyMouseScrollEvent(double, double yOffset)
{
    if (!window_->getGUI()->isPaintingDisabled() && drawable_->isPainting())
    {
        if (yOffset > 0)
            window_->getGUI()->paintRadius_ += .05f * static_cast<float>(yOffset);
        else if (yOffset < 0)
            window_->getGUI()->paintRadius_ += .05f * static_cast<float>(yOffset);

        if (window_->getGUI()->paintRadius_ <= 0)
            window_->getGUI()->paintRadius_ = .005f;

        cursor_->transform_.setScale(window_->getGUI()->paintRadius_);
    }
}

void DefaultController::notifyMouseMoveEvent(double x, double y)
{
    if (trackball_ != nullptr)
        trackball_->update(static_cast<float>(x), static_cast<float>(y), leftMouseButton_, *window_);

    SimulationDomain* domain = dynamic_cast<SimulationDomain*>(drawable_);
    if (domain && cursor_->visible_)
    {
        Vec3 camDir = camera_->mouseToWorldRay((int)x, (int)y, window_->getWidth(), window_->getHeight());
        Vec3 camPos = camera_->position_;
        float t = 0.f;

        if (animationInteraction_)
        {
            //HalfEdgeMesh* hem = dynamic_cast<HalfEdgeMesh*>(drawable_);
            // TODO: not implemented
        }
        else
        {
            int v_i = domain->raycast(camDir, camPos, t);
            if (v_i != -1)
            {
                Vec3 pos = camPos + camDir * t;
                cursor_->transform_.setPosition(pos);

                // Paint
                if (rightMouseButton_)
                {
                    domain->paint(v_i, pos, window_->getGUI()->paintRadius_);
                    sim_->updateColorsFromRam = true;
                    sim_->updateColors();
                    painted_ = true;
                }
            }
            else if (raycastPlane(camPos, camDir, drawable_->transform_.getPosition(), Vec3(0, 0, 1), &t))
            {
                Vec3 pos = camPos + camDir * t;
                cursor_->transform_.setPosition(pos);
            }
        }
    }
}

void DefaultController::notifyMouseEntered(bool)
{
    leftMouseButton_ = false;
    rightMouseButton_ = false;
}