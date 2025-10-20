#pragma once
#include "App.h"
#include "Scene.h"
#include "Window.h"
#include "Cursor.h"
#include "Camera.h"
#include "DefaultController.h"
#include "CameraController.h"
#include "SimulationLoader.h"
#include "Counter.h"
#include "CmdArgsParser.h"
#include "BSplinePatch.h"


class RDPG : 
	public App
{
    friend class GUI;

    void updateWindowTitle() const;
    void createWindow();
    void loadAnimation();
    void createAnimationPatch();
    void record() const;
    void loadEditorSettings();

    Scene scene_;
    Window window_;
    bool disableRendering_ = false;
    bool modelLoaded_ = false;
    bool screenShot_ = false;
    bool updateColors_ = false;

    SimulationDomain* domain_ = nullptr;
    Simulation* simulation_ = nullptr;
    
    Cursor cursor_;
    Trackball trackball_;
    DefaultController defaultController_;
    CameraController cameraController_;
    SimulationLoader simulationLoader_;
    Camera camera_;
    CmdArgsParser cmdArgsParser_;
    Counter renderCounter_;
    Color backgroundColor_;
    std::vector<Drawable*> patchDrawable_;
    int texSize_[2] = { 2000, 2000 };

public:
    BSplinePatch patch_;

    RDPG() = default;
    virtual ~RDPG() = default;

    virtual void init(int argc, char** argv, const std::string& name = "RDPG") override;
    virtual int run() override;
    virtual int load(const std::string& filename) override;
    virtual void pause(bool paused) override;
    virtual int shutdown() override;
};

void saveTexture(const std::string& textureName, Simulation* simulation, int texSizeX, int texSizeY);
void savePly(const std::string& plyName, Simulation* simulation);