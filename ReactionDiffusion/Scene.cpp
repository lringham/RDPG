#include "Scene.h"
#include "Simulation.h"
#include "SimulationDomain.h"
#include "Camera.h"


void Scene::draw(const Camera& camera, bool updateColors)
{
    // Load sim results to VBO
    if (updateColors && hasSimulation())
        simulation_->updateColors();

    // Draw everything
    for (Drawable* d : drawables_)
        d->render(camera);
}

void Scene::step()
{
    stepCheck_ = true;
}

void Scene::update()
{
    if (!hasSimulation())
        return;

    // Reload
    if (simulation_->shouldReloadSim_)
    {
        simulation_->reloadSim();
        simulation_->shouldReloadSim_ = false;
    }
    else if (simulation_->shouldReloadPDEs_)
    {
        simulation_->reloadPDEs();
        simulation_->shouldReloadPDEs_ = false;
    }

    // Simulate
    if (!simulation_->isPaused() || stepCheck_)
    {
        simulation_->simulate();

        if (stepCheck_)
            stepCheck_ = false;
    }
}

void Scene::addDrawable(Drawable* drawable)
{
    drawables_.push_back(drawable);
}

void Scene::removeDrawables()
{
    drawables_.clear();
}

void Scene::removeDrawable(Drawable* drawable)
{
    drawables_.erase(std::find(drawables_.begin(), drawables_.end(), drawable));
}

unsigned Scene::getNumDrawables() const
{
    return static_cast<unsigned>(drawables_.size());
}

bool Scene::hasSimulation() const
{
    return simulation_ != nullptr;
}

void Scene::pause(bool paused)
{
    if (!hasSimulation())
        return;

    if (paused)
    { 
        simulation_->pause(paused);
        if (simulation_->isGPUEnabled && !simulation_->ramUpToDate())
            simulation_->updateRAM();
    }
    else 
    {
        simulation_->pause(false);
    }
}

bool Scene::isPaused() const
{
    if (hasSimulation())
        return simulation_->isPaused();
    return true;
}

void Scene::setSimulation(Simulation* simulation)
{
    simulation_ = simulation;
}

