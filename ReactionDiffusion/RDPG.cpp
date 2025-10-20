#include "RDPG.h"
#include "Utils.h"

void RDPG::init(int argc, char** argv, const std::string& appName)
{
    if (initialized_)
        return;

    appName_ = appName;
    backgroundColor_.set(53, 53, 53);

    // Load arguments
    if (!cmdArgsParser_.parseArgs(argc, argv))
    {
        errorCode_ = -1;
        PAUSE();
        return;
    }

    createWindow();

    if (cmdArgsParser_.hasOption("SavePath"))
        window_.getGUI()->setPatternName(cmdArgsParser_.getOption("SavePath"));
    
    screenShot_ = cmdArgsParser_.flagSet("ScreenShot");

    bool disableRendering = cmdArgsParser_.flagSet("DisableRendering");

    cursor_.init();
    window_.setBackgroundColor(backgroundColor_);
    window_.graphics_.toggleDepthTest();
    window_.graphics_.clear();
    window_.swapBuffers();
    window_.getGUI()->setVisible(!cmdArgsParser_.flagSet("HideGUI"));

    {
        auto disableRenderCallback = std::function<void(void)>([&disableRendering]() {
                disableRendering = !disableRendering;
            });
        auto pauseCallback = std::function<void(bool)>([&](bool paused) {
                pause(paused);
            });
        defaultController_.setDisableRenderingCallback(disableRenderCallback);
        defaultController_.setPauseCallback(pauseCallback);
    }

    cameraController_.init(&camera_);
    
    std::string configFilename = cmdArgsParser_.getOption("ConfigFile");
    if (configFilename == "" && Utils::doesFileExist("./SimConfig.txt"))
        configFilename = "SimConfig.txt";

    if (Utils::doesFileExist(configFilename))
    {
        load(configFilename);
        createAnimationPatch();
    }

    loadEditorSettings();

    if (!cmdArgsParser_.flagSet("Run"))
        pause(true);

    initialized_ = true;
}

int RDPG::run()
{
    if (!initialized_ || errorCode_ != 0)
    {
        if (!initialized_)
        {
            errorCode_ = -1;
            LOG("Cannot run! App not initialized.");
        }
        return shutdown();
    }
    
    // Main loop
    while (!window_.shouldClose())
    {
        // Simulate 
        scene_.update(); 

        // Set patch spline visiblity
        for (auto& spline : patchDrawable_)
            spline->visible_ = simulation_->isPaused() && domain_->growing && domain_->growthMode == SimulationDomain::GrowthMode::AnimationGrowth;

        // Check if should pause or exit at a step 
        if (simulation_->pauseAt && simulation_->stepCount == simulation_->pauseStepCount)
            pause(true);
        
        // Sim is finished so exit
        if (modelLoaded_ && simulation_->exitAt && simulation_->stepCount == simulation_->exitStepCount)
        {
            window_.setShouldExit(true);
            window_.getGUI()->exitProgram_ = true;
        }

        // Render
        if (!disableRendering_)
        {
            // update the camera
            cameraController_.update();

            // Draw the scene
            window_.graphics_.clear();
            bool updateColors = renderCounter_.countElapsedAndReset() || defaultController_.hasPainted();
            scene_.draw(camera_, updateColors);
  

            // Record video frames / textures / meshes
            record();

            // Draw the cursor
            if (cursor_.visible_)
            {
                window_.graphics_.clearDepthBuff(); // Always render cursor over the model			
                cursor_.render(camera_);
            }

            // Draw the gui
            window_.getGUI()->draw(this);
        }

        // Display results
        window_.swapBuffers();
        
        // Update events 
        window_.waitEvents(!scene_.isPaused());
    }
    
    // Perform final actions
    if (screenShot_) { 
        window_.graphics_.clear();
        scene_.draw(camera_);
        window_.screenshot("screenshot.png"); 
    }

    if (simulation_->createTextureOnExit)
        saveTexture("", simulation_, texSize_[0], texSize_[1]);
    
    if (simulation_->createModelOnExit)
        window_.getGUI()->save(this);

    return shutdown();
}

int RDPG::load(const std::string& filename) 
{   
    LOG(std::filesystem::current_path());

    auto p = std::filesystem::canonical(filename);
    p.remove_filename();
    LOG(p);
    std::filesystem::current_path(p);
    LOG(filename);
    LOG(std::filesystem::current_path());
    
    // Setup the camera
    camera_.position_.set(0.f, 0.f, 26.5f);
    camera_.lookAt_.set(0.f, 0.f, -1.f);
    camera_.up_.set(0.f, 1.f, 0.f);
    camera_.setProjMatrix(
        Math::degToRad(45.f)	/*fovy*/,
        window_.getAspectRatio(),
        .1f						/*nearZ*/,
        50000.f);				/*farZ*/

    // Load the simulation
    Simulation* newSimulation = nullptr;
    SimulationDomain* newDomain = nullptr;
    if (!simulationLoader_.loadSim(filename, newDomain, newSimulation, camera_, cmdArgsParser_))
    {
        LOG("Failed to load simulation");
        errorCode_ = -1;
        PAUSE();
        return -1;
    }

    // todo Should be done after these are deassociated with other instances
    delete domain_; 
    domain_ = nullptr;
    
    delete simulation_; 
    simulation_ = nullptr;

    simulation_ = newSimulation;
    domain_ = newDomain;

    scene_.removeDrawables();

    window_.getGUI()->saveTextureCallback_ = [&](const std::string& textureName) {
        saveTexture(textureName, simulation_, texSize_[0], texSize_[1]);
    };
    window_.getGUI()->loadCallback_ = [&](const std::string& textureName) {
        load(textureName);
    };

    if (cmdArgsParser_.flagSet("SaveOnExit"))
        simulation_->createModelOnExit = true;
    
    if (cmdArgsParser_.hasOption("Steps"))
    {
        simulation_->exitAt = true;
        simulation_->exitStepCount = std::atoi(cmdArgsParser_.getOption("Steps").c_str());
    }

    // populate scene
    scene_.setSimulation(simulation_);
    scene_.addDrawable(domain_);
    if (cmdArgsParser_.hasOption("FrameOutputFreq"))
    {
        auto freq = std::atoi(cmdArgsParser_.getOption("FrameOutputFreq").c_str());
        renderCounter_.setDuration(freq);
        std::string screenshotPath = std::string("");
        for (size_t i = 0; i < screenshotPath.size(); ++i)
            simulation_->videoPath[i] = screenshotPath[i];

        std::string mkdirStr = Utils::mkdirSystemCommand(screenshotPath);
        system(mkdirStr.c_str());
        simulation_->outputScreens = true;
    }

    window_.getGUI()->setColorMapOutside(domain_);
    window_.getGUI()->setColorMapInside(domain_);

    if (domain_->prepatternInfo0.enabled)
    {
        for (auto pair : simulation_->morphIndexMap)
        {
            if (pair.second == domain_->prepatternInfo0.targetIndex)
            {
                size_t i = 0;
                for (auto c : pair.first)
                    window_.getGUI()->prepatternTarget0_[i++] = c;
            }
            if (pair.second == domain_->prepatternInfo0.sourceIndex)
            {
                size_t i = 0;
                for (auto c : pair.first)
                    window_.getGUI()->prepatternSource0_[i++] = c;
            }
        }
    }

    if (domain_->prepatternInfo1.enabled)
    {
        for (auto pair : simulation_->morphIndexMap)
        {
            if (pair.second == domain_->prepatternInfo1.targetIndex)
            {
                size_t i = 0;
                for (auto c : pair.first)
                    window_.getGUI()->prepatternTarget1_[i++] = c;
            }
            if (pair.second == domain_->prepatternInfo1.sourceIndex)
            {
                size_t i = 0;
                for (auto c : pair.first)
                    window_.getGUI()->prepatternSource1_[i++] = c;
            }
        }
    }

    // Controls
    trackball_.setDrawable(domain_);
    defaultController_.init(&cursor_, domain_, simulation_, &window_, &trackball_, &scene_, &camera_);
    window_.getGUI()->setSimulationParams(simulation_->originalParams);

    loadAnimation();

    modelLoaded_ = true;
    return 0;
}

void RDPG::pause(bool paused)
{
    scene_.pause(paused);
    updateWindowTitle();
}

int RDPG::shutdown()
{
    delete simulation_;
    delete domain_;

    initialized_ = false;
    return errorCode_;
}

void RDPG::record() const 
{
    if (modelLoaded_ && simulation_->recording() && !simulation_->isPaused() && simulation_->stepCount % (renderCounter_.getDuration() + 1) == 0)
    {
        std::string stepCountStr = "/" + std::to_string(simulation_->stepCount);
        LOG("screenshot taken: " + std::string(simulation_->videoPath) + stepCountStr);
        if (simulation_->outputScreens)
            window_.screenshot(std::string(simulation_->videoPath) + stepCountStr + ".png");
        if (simulation_->outputTextures)
            saveTexture(std::string(simulation_->videoPath) + stepCountStr + ".png", simulation_, texSize_[0], texSize_[1]);
        if (simulation_->outputPlys)
            savePly(std::string(simulation_->videoPath) + stepCountStr, simulation_);
    }
}

void RDPG::loadEditorSettings()
{
    std::string header = "";
    std::string line(512, '\0');
    std::ifstream editorSettingsFile("EditorSettings.txt");
    int r = 0, g = 0, b = 0;
    bool wireframeEnabled = false;
    if (editorSettingsFile.is_open())
    {
        int visibleMorphIndex = 0;
        while (editorSettingsFile)
        {
            editorSettingsFile >> header;
            header = Utils::sToLower(header);
            if (header == "wireframe:")
            {
                editorSettingsFile >> wireframeEnabled;
                if (wireframeEnabled)
                    window_.graphics_.toggleWireframe();
            }
            else if (header == "paintradius:")
            {
                float paintRadius = 1.f;
                editorSettingsFile >> paintRadius;
                window_.getGUI()->paintRadius_ = paintRadius;
                cursor_.transform_.setScale(paintRadius);
            }
            else if (header == "backgroundthreshold:")
            {
                editorSettingsFile >> domain_->backgroundThreshold_;
            }
            else if (header == "backgroundoffset:")
            {
                editorSettingsFile >> domain_->backgroundOffset_;
            }
            else if (header == "normcoef:")
                editorSettingsFile >> domain_->normCoef;
            else if (header == "visiblemorph:")
            { 
                editorSettingsFile >> visibleMorphIndex;
                std::fill(std::begin(domain_->showingMorph), std::end(domain_->showingMorph), false);
                domain_->showingMorph[visibleMorphIndex] = true;
                simulation_->outMorphIndex = visibleMorphIndex;
            }
            else if (header == "background:")
            {
                editorSettingsFile >> r;
                editorSettingsFile >> g;
                editorSettingsFile >> b;
                backgroundColor_.set((unsigned char) r,
                             (unsigned char) g, 
                             (unsigned char) b);
                window_.setBackgroundColor(backgroundColor_);
            }
            header = "";
        }
        editorSettingsFile.close();
    }
}

void RDPG::createWindow()
{
    int winWidth = std::atoi(cmdArgsParser_.getOption("winWidth").data());
    int winHeight = std::atoi(cmdArgsParser_.getOption("winHeight").data());
    bool fullscreen = cmdArgsParser_.flagSet("Fullscreen");
    window_.init(appName_, winWidth, winHeight, fullscreen);
    window_.resizeCallback_ = [&](int width, int height) {
        float ratio = (float)width / (float)height;
        camera_.setProjMatrix(camera_.getFovy(), ratio, camera_.getZNear(), camera_.getZFar());
    };
}

void RDPG::updateWindowTitle() const
{
    std::string windowTitle = appName_ + " | ";
    windowTitle += scene_.isPaused() ? "Paused | " : "Running";
    if (simulation_)
        windowTitle += simulation_->isGPUEnabled ? " | GPU" : " | CPU";
    window_.setTitle(windowTitle);
}

void RDPG::loadAnimation()
{
    if (!Utils::doesFileExist("animation.txt"))
        return;
    
    HalfEdgeMesh* domain = dynamic_cast<HalfEdgeMesh*>(domain_);
    if (domain == nullptr || !domain_->isDomainType(SimulationDomain::DomainType::MESH))
    {
        LOG("Animations not support on non mesh domains!");
        return;
    }
    
    LOG("Loading animation");
            
    domain->animation_.clearFrames();

    std::ifstream animationFile("animation.txt");
    if (animationFile.is_open())
    {
        std::string line;
        std::getline(animationFile, line);
        do
        {
            BSplinePatch patch;
            int frameNum = std::atol(line.c_str());

            while (std::getline(animationFile, line))
            {
                if (std::count(std::begin(line), std::end(line), '(') == 0)
                    break; // hit a frame number probably

                std::vector<Vec3> controlPoints;
                auto coordinates = Utils::split(line, ") (");
                for (auto coordinate : coordinates)
                {
                    auto components = Utils::split(coordinate, ", ");
                    for (auto& component : components)
                    {
                        Utils::findAndReplace(component, "(", " ");
                        Utils::findAndReplace(component, ")", " ");
                    }

                    Vec3 controlPoint;
                    controlPoint.x = static_cast<float>(std::atof(components[0].c_str()));
                    controlPoint.y = static_cast<float>(std::atof(components[1].c_str()));
                    controlPoint.z = static_cast<float>(std::atof(components[2].c_str()));
                    controlPoints.push_back(controlPoint);
                }
                patch.push_back(BSpline(controlPoints));
            }
            domain->animation_.addKeyframe(frameNum, patch);
        } while (!animationFile.eof());

        animationFile.close();
    }
}

void RDPG::createAnimationPatch()
{
    if (!domain_->isDomainType(SimulationDomain::DomainType::MESH))
    {
        LOG("Animations not support on non mesh domains!");
        return;
    }
    HalfEdgeMesh* domain = dynamic_cast<HalfEdgeMesh*>(domain_);
    ASSERT(domain, "Cast to half edge mesh failed when loading animation");

    auto AABB = calculateBounds(*domain_);
    Vec3 start = domain_->transform_.getPosition() + AABB.first;
    Vec3 dims = AABB.second - AABB.first;
    start.z += dims.z / 2.f;

    std::vector<Vec3> xPoints;
    std::vector<Vec3> yPoints;
    const size_t numSplines = 6;
    const size_t numCtrlPts = 6;
    dims.x /= (numSplines - 1.f);
    dims.y /= (numSplines - 1.f);

    for (int i = 0; i < numSplines; ++i) 
    {
        // Create the spline
        std::vector<Vec3> points;
        for (size_t j = 0; j < numCtrlPts; j++)
            points.push_back(start + Vec3(i * dims.x, j * dims.y, 0.f));
        BSpline spline(points);
        patch_.push_back(spline);
        patchDrawable_.push_back(new Drawable());
        spline.initDrawable(*patchDrawable_.back());

        // Save points for calculating UVs later
        xPoints.push_back(points[0]);
        if (i == 0)
            yPoints = points;
    }

    for(Drawable* d : patchDrawable_)
        scene_.addDrawable(d);

    BSpline xCurve(xPoints);
    BSpline yCurve(yPoints);
    domain->animation_.setUVs(calculateUVs(domain_->positions_, xCurve, yCurve));
}

void saveTexture(const std::string& textureName, Simulation* simulation, int texSizeX, int texSizeY)
{
    if (simulation->isGPUEnabled && !simulation->ramUpToDate())
        simulation->updateRAM();

    std::vector<unsigned char> pixels(texSizeX * texSizeY * 3);
    simulation->domain->exportTexture(pixels.data(), texSizeX, texSizeY);
    Utils::saveImage(textureName.c_str(), texSizeX, texSizeY, pixels.data());
}

void savePly(const std::string& plyName, Simulation* simulation)
{
    if (simulation->isGPUEnabled && !simulation->ramUpToDate())
        simulation->updateRAM();

    HalfEdgeMesh* hem = dynamic_cast<HalfEdgeMesh*>(simulation->domain);
    if (hem)
        hem->saveToPly((plyName + ".ply").c_str());
    else
        LOG("Domain is not a mesh, cannot save ply");
}