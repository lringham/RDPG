#include "GUI.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "EventHandler.h"
#include "Utils.h"
#include "Grid.h"
#include "ResourceLocations.h"
#include "GPUSimulation.h"
#include "RDPG.h"
#include "Window.h"
#include "Camera.h"
#include "Trackball.h"
#include "Scene.h"

#include <imgui.h>
#include <iostream>
#include <algorithm>
//#include <nfd.h>


GUI::GUI(GLFWwindow* windowPtr)
{
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(windowPtr, false); // Installing callbacks seems to break the gui. Backspace and scroll wont work unless windows are undocked
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    EventHandler::setGUI(this);

    colorMapOutsideFilename_.resize(128);
    colorMapInsideFilename_.resize(128);

   
}

GUI::~GUI()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GUI::draw(RDPG* app)
{
    Simulation* simulation = app->simulation_;

    // Don't render GUI as a wireframe if enabled
    wireframeEnabled_ = app->window_.graphics_.wireframeEnabled();
    if (wireframeEnabled_)
        app->window_.graphics_.toggleWireframe();

    if (!visible_)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.WantCaptureKeyboard = false;
        io.WantCaptureMouse = false;
        return;
    }

    

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        drawMenuBar(app);

        if (simulation)
        {
            // Start frame
            ImGuiWindowFlags window_flags = 0;
            ImGui::SetNextWindowSize(ImVec2(310, 690), ImGuiCond_FirstUseEver);
            ImGui::Begin("Settings", nullptr, window_flags);

            // Start / Pause
            bool paused = simulation->isPaused();
            if (paused)
            {
                ImVec4 color(1, 0, 0, 1);
                ImGui::PushStyleColor(ImGuiCol_Button, color);
                if (ImGui::Button("START", ImVec2(ImGui::GetContentRegionAvail().x, 25)))
                    paused = false;
                ImGui::PopStyleColor();
            }
            else
            {
                if (ImGui::Button("PAUSE", ImVec2(ImGui::GetContentRegionAvail().x, 25)))
                    paused = true;
            }

            if (simulation->isPaused() && !paused)
            {
                std::fill(app->domain_->paintingDiffDir.begin(), app->domain_->paintingDiffDir.end(), false);
                std::fill(app->domain_->paintingMorph.begin(), app->domain_->paintingMorph.end(), false);
                app->domain_->selectCells = false;
                app->domain_->shader_->setSelectingCells(false);
                app->cursor_.visible_ = false;
                app->pause(false);
            }
            else if (!simulation->isPaused() && paused)
                app->pause(true);

            // 
            if (ImGui::CollapsingHeader("Parameters"))
                drawParametersControls(app);
            if (ImGui::CollapsingHeader("Growth"))
                drawGrowthControls(app);
            if (ImGui::CollapsingHeader("Painting"))
                drawPaintingControls(app);
            if (ImGui::CollapsingHeader("Rendering"))
                drawRenderingControls(app);
        
            ImGui::End();
        }
    }

    if (simulation)
    {
        if (showSaveWindow_)
            drawSaveWindow(app);
        if (app->domain_ && app->domain_->growing)
            drawAnimationControls(app);
        if (showControlsWindow_)
            drawControls(app);
        if (showStatsWindow_)
            drawStats(app);
        if (showSelectOptions_)
            drawSelectOptions(app);
        if (showInfoWindow_)
            drawInfo(app);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    { 
        auto* context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(context);
    }

    // Restore wireframe setting if enabled
    if (wireframeEnabled_)
        app->window_.graphics_.toggleWireframe();
}

void GUI::drawMenuBar(RDPG* app)
{
    // Menu bar
    ImGui::BeginMainMenuBar();
    {
        if (ImGui::BeginMenu("File"))
        {
            //if (ImGui::MenuItem("Open..", ""))
            //{
            //    nfdchar_t* outPath = nullptr;
            //    nfdresult_t result = NFD_OpenDialog(nullptr, nullptr, &outPath);
            //    if (result == NFD_OKAY) 
            //    {
            //        loadCallback_(std::string(outPath));
            //    }
            //    else if (result == NFD_CANCEL) 
            //    {
            //        LOG("Open file cancelled");
            //    }
            //    else {
            //        LOG(NFD_GetError());
            //    }
            //}
            if (ImGui::MenuItem("Save..", ""))
                showSaveWindow_ = !showSaveWindow_;
            //if (ImGui::MenuItem("Reload", ""))
            //	;
            if (ImGui::MenuItem("Exit", ""))
                showExitConf_ = true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options"))
        {
            if (ImGui::MenuItem("Controls", ""))
                showControlsWindow_ = !showControlsWindow_;
            if (ImGui::MenuItem("Stats", ""))
                showStatsWindow_ = !showStatsWindow_;
            if (ImGui::MenuItem("Info", ""))
                showInfoWindow_ = !showInfoWindow_;
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();

    confExit(app);
}

void GUI::confExit(RDPG* app)
{
    if (showExitConf_)
    {        
        ImGui::SetNextWindowSize({ 150, 55 });

        // Centre popup
        int x = 0, y = 0;
        app->window_.getPos(&x, &y);
        ImGui::SetNextWindowPos({ x + app->window_.getWidth() / 2.f - 75, y + app->window_.getHeight() / 2.f - 50 });

        ImGui::Begin("Exit program?", &showExitConf_, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        ImGui::SameLine(ImGui::GetWindowWidth() / 2 - 50);
        if (ImGui::Button("YES", { 50, 17 }))
        {
            app->window_.setShouldExit(true);
            exitProgram_ = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("NO", { 50, 17 }) || !showExitConf_)
        {
            app->window_.setShouldExit(false);
            showExitConf_ = false;
            exitProgram_ = false;
        }
        ImGui::End();
    }
}

bool GUI::hasUsedEvent()
{
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard || io.WantCaptureMouse;
}

void GUI::setColorMapOutside(SimulationDomain* domain)
{
    const ColorMap& colormap = domain->colorMapOutside;
    colorMapOutsideFilename_ = colormap.getNameAndPath();
    colorMapOutsideFilename_.resize(128);

    gradientOutside_.getMarks().clear();
    if (colormap.colorMarks_.size() > 0)
    {
        for (size_t i = 0; i < colormap.colorMarks_.size(); i += 4)
            gradientOutside_.addMark(
                colormap.colorMarks_[i + 3],
                ImColor(
                    colormap.colorMarks_[i],
                    colormap.colorMarks_[i + 1],
                    colormap.colorMarks_[i + 2]));
    }
    else
    {
        unsigned char r, g, b;
        for (size_t i = 0; i <= 10; i += 2)
        {
            float s = static_cast<float>(i) / 10.f;
            domain->colorMapOutside.sample(s, r, g, b);
            gradientOutside_.addMark(s, ImColor(r, g, b));
        }
    }

}

void GUI::setColorMapInside(SimulationDomain* domain)
{
    const ColorMap& colormap = domain->colorMapInside;
    this->colorMapInsideFilename_ = colormap.getNameAndPath();
    this->colorMapInsideFilename_.resize(128);

    useInsideColormap_ = colorMapInsideFilename_ != colorMapOutsideFilename_;

    gradientInside_.getMarks().clear();
    if (colormap.colorMarks_.size() > 0)
    {
        for (size_t i = 0; i < colormap.colorMarks_.size(); i += 4)
            gradientInside_.addMark(
                colormap.colorMarks_[i + 3],
                ImColor(
                    colormap.colorMarks_[i],
                    colormap.colorMarks_[i + 1],
                    colormap.colorMarks_[i + 2]));
    }
    else
    {
        unsigned char r, g, b;
        for (int i = 0; i <= 10; i += 2)
        {
            domain->colorMapInside.sample(i / 10.f, r, g, b);
            gradientInside_.addMark(i / 10.f, ImColor(r, g, b));
        }
    }
}

void GUI::setSimulationParams(const std::map<std::string, float>& originalParams)
{
    tempParams_ = originalParams;
    selectedParams_.resize(tempParams_.size(), true);
}

void GUI::updateColormap(ImGradient& gradient, ColorMap& colormap)
{
    colormap.colorMarks_.clear();
    colormap.setExtToColors();
    for (const auto& mark : gradient.getMarks())
    {
        colormap.colorMarks_.push_back(mark->color[0]);
        colormap.colorMarks_.push_back(mark->color[1]);
        colormap.colorMarks_.push_back(mark->color[2]);
        colormap.colorMarks_.push_back(mark->position);
    }
    colormap.update(true);
}

void GUI::setDisablePainting(bool disable)
{
    disablePaint_ = disable;
}

bool GUI::isPaintingDisabled() const
{
    return disablePaint_;
}

void GUI::setVisible(bool visible)
{
    visible_ = visible;
}

void GUI::toggleVisible()
{
    visible_ = !visible_;
}

std::string GUI::getColorMapOutsideName()
{
    return colorMapOutsideFilename_;
}

std::string GUI::getColorMapInsideName()
{
    return colorMapInsideFilename_;
}

void GUI::save(RDPG* app)
{
    LOG("Saving....");
    Simulation* simulation = app->simulation_;
    SimulationDomain* domain = app->domain_;
    std::string patternNameStr = std::string(patternName_);

    std::string shaderPath = ResourceLocations::instance().shadersPath();
    for (size_t loc = 0; (loc = shaderPath.find('/', loc)) != std::string::npos;)
        shaderPath[loc] = '\\';

    // Create folder
    std::string mkdirCommandStr = Utils::mkdirSystemCommand("\"" + patternNameStr + "\"");
    system(mkdirCommandStr.data());

    mkdirCommandStr = Utils::mkdirSystemCommand("\"" + patternNameStr + "/shaders\"");
    system(mkdirCommandStr.data());

    std::string command = Utils::copySystemCommand("\"" + shaderPath + "\" \"" + patternNameStr + 
#ifdef _WIN32
        "/shaders" +
#endif
        "/\"");
    system(command.data());

    // make a pattern.rd
    std::string simName = patternNameStr + ".rd";
    simulation->saveConcentrations(patternNameStr + "/", simName);

    // make a obj file
    std::string objNameStr = patternNameStr + ".ply";
    if (app->domain_->isDomainType(SimulationDomain::DomainType::MESH))
    {
        HalfEdgeMesh* meshDomain = dynamic_cast<HalfEdgeMesh*>(domain);
        meshDomain->saveToPly(patternNameStr + "/" + objNameStr, domain->hasVeins());
    }

    bool colorMapsSameName = domain->colorMapInside.getName() == domain->colorMapOutside.getName();

    // copy color maps
    if (domain->colorMapOutside.getExt() == ".colors")
    {
        std::string colorMapName = patternNameStr + "/" + (useInsideColormap_ && colorMapsSameName ? "outside" : "") + domain->colorMapOutside.getName();
        saveColorMap(colorMapName, gradientOutside_);
    }
    else if (domain->colorMapOutside.getExt() == ".map")
    {
        std::string colorMap = domain->colorMapOutside.getPath() + domain->colorMapOutside.getName();
        for (size_t loc = 0; (loc = colorMap.find('/', loc)) != std::string::npos;)
            colorMap[loc] = '\\';
        //string copyColormapCmd = "copy \"" + Utils::stripNulls(colorMap) + "\" \"" + patternNameStr + "\"";
        std::string copyColormapCmd = Utils::copySystemCommand("\"" + Utils::stripNulls(colorMap) + "\" \"" + patternNameStr + "\"");
        system(copyColormapCmd.data());
    }

    if (domain->colorMapInside.getExt() == ".colors")
    {
        if (useInsideColormap_)
        {
            std::string colorMapName = patternNameStr + "/" + (colorMapsSameName ? "inside" : "") + domain->colorMapInside.getName();
            saveColorMap(colorMapName, gradientInside_);
        }
    }
    else
    {
        std::string colorMap = domain->colorMapInside.getPath() + domain->colorMapInside.getName();
        for (size_t loc = 0; (loc = colorMap.find('/', loc)) != std::string::npos;)
            colorMap[loc] = '\\';
        //string copyColormapCmd = "copy \"" + Utils::stripNulls(colorMap) + "\" \"" + patternNameStr + "\"";
        std::string copyColormapCmd = Utils::copySystemCommand("\"" + Utils::stripNulls(colorMap) + "\" \"" + patternNameStr + "\"");
        system(copyColormapCmd.data());
    }
    
    {        
        std::string copyColormapCmd = Utils::copySystemCommand("\"" + ResourceLocations::instance().exePath() + "\" \"" + patternNameStr + "\"");
        system(copyColormapCmd.data());
    }

    //system(("copy \"imgui.ini\" \"" + patternNameStr + "\"").data());
    std::string copyColormapCmd = Utils::copySystemCommand("\"imgui.ini\" \"" + patternNameStr + "\"");
    system(copyColormapCmd.data());

    // make a SimConfig.txt
    std::ofstream configFile;
    configFile.open(patternNameStr + "/SimConfig.txt", std::ofstream::out);
    if (configFile.is_open())
    {
        configFile << "# SimStep = " << simulation->stepCount << "\n";
        std::string patternDescStr = patternDesc_;
        size_t prevLoc = 0;
        for (size_t loc = 0; (loc = patternDescStr.find('\n', loc)) != std::string::npos;)
        {
            std::cout << prevLoc << " " << loc << " " << patternDescStr.substr(prevLoc, loc - prevLoc);
            configFile << "# " << patternDescStr.substr(prevLoc, loc - prevLoc) << "\n";
            prevLoc = ++loc;
        }
        configFile << "# " << patternDescStr.substr(prevLoc, patternDescStr.size() - prevLoc) << "\n";

        configFile << "\n### file paths ###\n";
        configFile << "ModelsPath: ./\n";
        configFile << "ColorMapsPath: ./\n";
        configFile << "ShadersPath: ./shaders/\n";

        // Write camera and model orientation info
        configFile << "\n### Camera position and lookAt ###\n";
        configFile << "camera: ";
        {
            auto& camera = app->camera_;
            configFile << camera.position_.x << " " << camera.position_.y << " " << camera.position_.z << " ";
            configFile << camera.lookAt_.x   << " " << camera.lookAt_.y   << " " << camera.lookAt_.z << "\n";
        }

        configFile << "\n### Model orientation X, Y, Z ###\n";
        configFile << "model: ";
        {
            Vec3 mX = domain->transform_.getRotationMat().col(0).xyz();
            Vec3 mY = domain->transform_.getRotationMat().col(1).xyz();
            Vec3 mZ = domain->transform_.getRotationMat().col(2).xyz();

            configFile << mX.x << " " << mX.y << " " << mX.z << " ";
            configFile << mY.x << " " << mY.y << " " << mY.z << " ";
            configFile << mZ.x << " " << mZ.y << " " << mZ.z << "\n";
        }

        // Background texture 
        if (app->domain_->backgroundImage_.filename_ !=  "")
            configFile << "backgroundTexture: " << app->domain_->backgroundImage_.filename_ << "\n";

        // Write growth params
        configFile << "\n### Growth parameters ###\n";
        configFile << "maxFaceArea: " << simulation->maxFaceArea << "\n";
        configFile << "growing: " << (domain->growing ? "true" : "false") << "\n";
        configFile << "growthX: " << simulation->growth.x << "\n";
        configFile << "growthY: " << simulation->growth.y << "\n";
        configFile << "growthZ: " << simulation->growth.z << "\n";
        configFile << "growthTickLimit: " << simulation->getGrowthTickLimit() << "\n";

        // Write morphogens used		
        configFile << "\n### Morphogens and domain info ###\n";
        if (simulation->name == "CPU" || simulation->name == "GPU")
        {
            std::vector<std::string> morphNames(simulation->morphIndexMap.size());
            for (auto& m : simulation->morphIndexMap)
                morphNames[m.second] = m.first;
            configFile << "morphogens: ";
            for (size_t i = 0; i < morphNames.size() - 1; ++i)
                configFile << morphNames[i] + ", ";
            configFile << *morphNames.rbegin() + "\n";

            if(domain->hasVeins())
                configFile << "veinMorph: " << morphNames[domain->veinMorphIndex_] + "\n";
        }

        // Write global parameters
        if (domain->isDomainType(SimulationDomain::DomainType::MESH))
        {
            configFile << "domain: " << objNameStr << "\n";
        }
        else if (domain->isDomainType(SimulationDomain::DomainType::GRID))
        {
            Grid* gridDomain = static_cast<Grid*>(domain);
            configFile << "domain: grid \n";
            configFile << "xRes: " << gridDomain->getXRes() << "\n";
            configFile << "yRes: " << gridDomain->getYRes() << "\n";
            configFile << "cellSize: " << gridDomain->getCellSize() << "\n";
        }

        if (simulation->name != "CPU" && simulation->name != "GPU")
            configFile << "rdModel: " << simulation->name << "\n";

        if (useInsideColormap_)
        {
            if (colorMapsSameName)
            {
                configFile << "colorMapOutside: " << "outside" + Utils::stripNulls(domain->colorMapOutside.getName()) << "\n";
                configFile << "colorMapInside: " << "inside" + Utils::stripNulls(domain->colorMapInside.getName()) << "\n";
            }
            else
            {
                configFile << "colorMapOutside: " << Utils::stripNulls(domain->colorMapOutside.getName()) << "\n";
                configFile << "colorMapInside: " << Utils::stripNulls(domain->colorMapInside.getName()) << "\n";
            }
        }
        else
            configFile << "colorMap: " << Utils::stripNulls(domain->colorMapOutside.getName()) << "\n";

        configFile << "simFile: " << simName << "\n";
        if (domain->background_.id() != 0)
            configFile << "backgroundTexture: " << app->domain_->backgroundImage_.filename_ << "\n";

        configFile << "\n### Parameters, PDEs, and Initial Conditions ###\n";

        // Write params
        configFile << "params:\n{\n";
        for (auto& param : simulation->paramsMap)
        {
            configFile << "\t" << indicesToString(param.indices_) << "\n";
            for (auto& p : param.params_)
            {
                if (p.first != "growthX" && p.first != "growthY" && p.first != "growthZ" && p.first != "maxFaceArea")
                    configFile << "\t" << p.first << ": " << p.second << "\n";
            }
        }
        configFile << "}\n";

        configFile << simulation->rawInitConds;
        configFile << simulation->rawBCs;

        if (simulation->name == "CPU" || simulation->name == "GPU")
            configFile << simulation->rawRDModel;

        configFile.close();

        Utils::createBatFile(patternNameStr + "/" + Utils::BAT_FILENAME);

        // editor settings
        simulation->saveEditorSettings(patternNameStr + "/");
        std::ofstream editorSettingsFile(patternNameStr + "/EditorSettings.txt", std::ios::app);
        if (editorSettingsFile.is_open())
        {
            auto bgColor = app->backgroundColor_;
            editorSettingsFile << "background: "
                << static_cast<int>(bgColor.r) << " "
                << static_cast<int>(bgColor.g) << " "
                << static_cast<int>(bgColor.b) << "\n";

            editorSettingsFile << "wireframe: " << wireframeEnabled_ << "\n";
            editorSettingsFile.close();
        }
    }
}

void GUI::drawParametersControls(RDPG* app)
{
    SimulationDomain* domain = app->domain_;
    Simulation* simulation = app->simulation_;

    // Draw select all params controls
    {
        bool tempSelectAllParams = selectAllParams_;
        ImGui::Checkbox("##selectAllParams", &selectAllParams_);
        for (size_t i = 0; i < selectedParams_.size(); ++i)
            if (tempSelectAllParams != selectAllParams_)
                selectedParams_[i] = selectAllParams_;
    }
    // Draw params selection selector
    {
        ImGui::SameLine();
        int oldSelectedLocalParams = selectedLocalParams_;
        ImGui::InputInt("Params", &selectedLocalParams_, 1, 1);
        selectedLocalParams_ = Utils::clamp(selectedLocalParams_, 0, int(simulation->paramsMap.size() - 1));
        if (oldSelectedLocalParams != selectedLocalParams_)
        {
            simulation->updateColorsFromRam = simulation->isPaused();
            for (auto& newP : simulation->paramsMap[selectedLocalParams_].params_)
                tempParams_[newP.first] = newP.second;
            domain->selectedCells.clear();
            domain->selectedCells = simulation->paramsMap[selectedLocalParams_].indices_;
        }
        ImGui::Separator();
    }
    // Draw param text boxes and their selection boxes
    {
        int i = 0;
        bool allParamsSelected = true;
        for (auto& param : tempParams_)
        {
            bool isSelected = selectedParams_[i];
            ImGui::Checkbox(("##params" + param.first).c_str(), &isSelected);
            selectedParams_[i++] = isSelected;
            ImGui::SameLine();
            ImGui::DragFloat((param.first + "##params").c_str(), &param.second, .005f, 0.f, 2000, "%.9f");
            if (!isSelected)
            {
                selectAllParams_ = false;
                allParamsSelected = false;
            }
        }
        selectAllParams_ = allParamsSelected;
    }

    ImGui::NewLine();
    ImGui::SameLine(35);
    if (ImGui::Button("update##Params"))
    {
        int i = 0;
        Parameters changedParams;
        for (auto& param : tempParams_)
        {
            if (selectedParams_[i++])
                changedParams[param.first] = param.second;
        }

        if (simulation->updateParams(changedParams))
            selectedLocalParams_ = static_cast<int>(simulation->paramsMap.size()) - 1;

        if (simulation->isGPUEnabled)
            simulation->gpuUpToDate(false);
    }
}

void GUI::drawGrowthControls(RDPG* app)
{
    SimulationDomain* domain = app->domain_;
    Simulation* simulation = app->simulation_;
    ImGui::Checkbox("Enabled", &domain->growing);

    if (domain->growing)
    {
        ImGui::Checkbox("Subdivision##grow", &simulation->subdivisionEnabled_);

        curGrowthMode_ = (int)domain->growthMode;
        const char* items[]{ "Animation", "Linear", "Percentage", "Morphogen" };        
        if (ImGui::Combo("mode##growth", &curGrowthMode_, items, IM_ARRAYSIZE(items)))
        {
            domain->growthMode = (SimulationDomain::GrowthMode) curGrowthMode_;
        }

        if (domain->growthMode == SimulationDomain::GrowthMode::PercentageGrowth)
        {
            float growthRate[3]{ simulation->growth.x, simulation->growth.y, simulation->growth.z };
            if (domain->isDomainType(SimulationDomain::DomainType::MESH))
                ImGui::InputFloat3("% growth", growthRate, "%.6f");
            if (domain->isDomainType(SimulationDomain::DomainType::GRID))
                ImGui::InputFloat2("growth chance", growthRate, "%.6f");
            simulation->growth.x = growthRate[0];
            simulation->growth.y = growthRate[1];
            simulation->growth.z = growthRate[2];
        }
        else if (domain->growthMode == SimulationDomain::GrowthMode::LinearAreaGrowth)
            ImGui::InputFloat("area per grow", &simulation->growth.x);
        else if (domain->growthMode == SimulationDomain::GrowthMode::MorphogenGrowth)
        {
            float growthRate[3]{ simulation->growth.x, simulation->growth.y };

            if (domain->isDomainType(SimulationDomain::DomainType::MESH))
                ImGui::InputFloat2("growth rate", growthRate, "%.6f");
            if (domain->isDomainType(SimulationDomain::DomainType::GRID))
                ImGui::InputFloat2("growth chance", growthRate, "%.6f");

            simulation->growth.x = growthRate[0];
            simulation->growth.y = growthRate[1];
        }

        int growPerSim = (int)simulation->getGrowthTickLimit();
        if (ImGui::InputInt("sims/grow", &growPerSim))
            simulation->setGrowthTickLimit((unsigned long long) growPerSim);

        if (simulation->subdivisionEnabled_)
        {
            if (domain->isDomainType(SimulationDomain::DomainType::MESH))
                ImGui::InputFloat("max area", &simulation->maxFaceArea, .01f, .1f, "%.6f");
        }
    }

    ImGui::NewLine();
}

void GUI::drawPaintingControls(RDPG* app)
{
    SimulationDomain* domain = app->domain_;
    Simulation* simulation = app->simulation_;
    if (ImGui::TreeNode("Prepatterns"))
    {
        ImGui::Checkbox("Use prepattern t0##t0", &domain->prepatternInfo0.enabled);
        if (domain->prepatternInfo0.enabled)
        {
            if (ImGui::BeginCombo("source morph##t0", prepatternSource0_))
            {
                for (auto& pair : simulation->morphIndexMap)
                {
                    bool isSelected = (std::string(prepatternSource0_) == pair.first);
                    if (ImGui::Selectable(pair.first.c_str(), isSelected))
                    {
                        int i = 0;
                        for (auto c : pair.first)
                            prepatternSource0_[i++] = c;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();

                for (auto& pair : simulation->morphIndexMap)
                    if (std::string(prepatternSource0_) == pair.first)
                        domain->prepatternInfo0.sourceIndex = pair.second;
            }

            if (ImGui::BeginCombo("target morph##t0", prepatternTarget0_))
            {
                for (auto& pair : simulation->morphIndexMap)
                {
                    bool isSelected = (std::string(prepatternTarget0_) == pair.first);
                    if (ImGui::Selectable(pair.first.c_str(), isSelected))
                    {
                        int i = 0;
                        for (auto c : pair.first)
                            prepatternTarget0_[i++] = c;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();

                for (auto& pair : simulation->morphIndexMap)
                    if (std::string(prepatternTarget0_) == pair.first)
                        domain->prepatternInfo0.targetIndex = pair.second;
            }

            ImGui::DragFloat("lowVal##t0", &domain->prepatternInfo0.lowVal);
            ImGui::DragFloat("highVal##t0", &domain->prepatternInfo0.highVal);
            ImGui::DragFloat("exponent##t0", &domain->prepatternInfo0.exponent);
            ImGui::Checkbox("squared##t0", &domain->prepatternInfo0.squared);
        }

        ImGui::Checkbox("Use prepattern t1##t1", &domain->prepatternInfo1.enabled);
        if (domain->prepatternInfo1.enabled)
        {
            if (ImGui::BeginCombo("source morph##t1", prepatternSource1_))
            {
                for (auto& pair : simulation->morphIndexMap)
                {
                    bool isSelected = (std::string(prepatternSource1_) == pair.first);
                    if (ImGui::Selectable(pair.first.c_str(), isSelected))
                    {
                        int i = 0;
                        for (auto c : pair.first)
                            prepatternSource1_[i++] = c;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();

                for (auto& pair : simulation->morphIndexMap)
                    if (std::string(prepatternSource1_) == pair.first)
                        domain->prepatternInfo1.sourceIndex = pair.second;
            }

            if (ImGui::BeginCombo("target morph##t1", prepatternTarget1_))
            {
                for (auto& pair : simulation->morphIndexMap)
                {
                    bool isSelected = (std::string(prepatternTarget1_) == pair.first);
                    if (ImGui::Selectable(pair.first.c_str(), isSelected))
                    {
                        int i = 0;
                        for (auto c : pair.first)
                            prepatternTarget1_[i++] = c;
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();

                for (auto& pair : simulation->morphIndexMap)
                    if (std::string(prepatternTarget1_) == pair.first)
                        domain->prepatternInfo1.targetIndex = pair.second;
            }

            ImGui::DragFloat("lowVal##t1", &domain->prepatternInfo1.lowVal);
            ImGui::DragFloat("highVal##t1", &domain->prepatternInfo1.highVal);
            ImGui::DragFloat("exponent##t1", &domain->prepatternInfo1.exponent);
            ImGui::Checkbox("squared##t1", &domain->prepatternInfo1.squared);
        }

        if (domain->prepatternInfo0.enabled || domain->prepatternInfo1.enabled)
        {
            if (ImGui::Button("Update Diffusion Coefs"))
            {
                simulation->updateDiffusionCoefs();
                LOG("Updating DiffCoefs");
            }
        }
        ImGui::TreePop();
    }

    ImGui::Separator();

    // Select paint mode
    bool sc = domain->selectCells;
    if (ImGui::Checkbox("Select Cells", &domain->selectCells))
    {
        if (!sc) // on check
        {
            std::fill(begin(domain->paintingMorph), end(domain->paintingMorph), false);
            domain->shader_->setSelectingCells(true);
            simulation->updateColorsFromRam = true;
            domain->showSelectedCells = true;
            std::fill(domain->paintingDiffDir.begin(), domain->paintingDiffDir.end(), false);
            std::fill(domain->showingDiffDir.begin(), domain->showingDiffDir.end(), false);
            domain->childern_[0]->visible_ = false;
        }
        else // on uncheck
        {
            domain->shader_->setSelectingCells(false);
            domain->showSelectedCells = false;
        }
    }

    ImGui::SameLine(ImGui::GetWindowWidth() - 80);
    if (ImGui::Button("options##", ImVec2(60, 18)))
        showSelectOptions_ = !showSelectOptions_;

    int selectedMorphIndex = -1;
    bool pmorph = false;
    for (auto& m : simulation->morphIndexMap)
    {
        bool paintM = domain->paintingMorph[m.second];
        ImGui::Checkbox((m.first + "##painting").c_str(), &paintM);
        pmorph = pmorph || paintM;

        if (!domain->paintingMorph[m.second] && paintM) // on check
        {
            selectedMorphIndex = m.second;
            simulation->updateColorsFromRam = true;
            simulation->outMorphIndex = m.second;
            domain->showingMorph[m.second] = true;

            domain->shader_->setSelectingCells(false);
            domain->showSelectedCells = false;
            domain->selectCells = false;
            std::fill(domain->paintingDiffDir.begin(), domain->paintingDiffDir.end(), false);
            std::fill(domain->showingDiffDir.begin(), domain->showingDiffDir.end(), false);
            domain->childern_[0]->visible_ = false;
        }
        domain->paintingMorph[m.second] = paintM;
    }

    for (int i = 0; i < simulation->MORPH_COUNT && selectedMorphIndex != -1; ++i)
    {
        if (i != selectedMorphIndex)
        {
            domain->paintingMorph[i] = false;
            domain->showingMorph[i] = false;
        }
    }

    bool pv = false;
    for (auto& m : simulation->morphIndexMap)
    {
        bool paintV = domain->paintingDiffDir[m.second];
        ImGui::Checkbox((m.first + " Diff Vectors##painting").c_str(), &paintV);
        pv = pv || paintV;

        if (domain->paintingDiffDir[m.second] != paintV)
        {
            selectedMorphIndex = m.second;
            domain->showSelectedCells = false;
            domain->selectCells = false;
            domain->shader_->setSelectingCells(false);
            std::fill(domain->paintingMorph.begin(), domain->paintingMorph.end(), false);
            std::fill(domain->paintingDiffDir.begin(), domain->paintingDiffDir.end(), false);
            std::fill(domain->showingDiffDir.begin(), domain->showingDiffDir.end(), false);
            if (paintV)
            {
                domain->paintingDiffDir[m.second] = true;
                domain->showingDiffDir[m.second] = true;
            }
            else
                domain->childern_[0]->visible_ = false;
        }
    }

    // Get active paint 
    int activePaint = -1;
    for (int i = 0; i < simulation->MORPH_COUNT; ++i)
    {
        if (domain->paintingMorph[i])
        {
            activePaint = i;
            break;
        }
    }
    
    if (activePaint >= 0)
    {       
        if (ImGui::Button("checkpoint"))
        {
            std::vector<float> checkpoint_;
            const auto cells = domain->getReadFromCells();
            checkpoint_.resize(domain->getCellCount());
            for (size_t i = 0; i < domain->getCellCount(); ++i)                
                checkpoint_[i] = cells.at(i)[activePaint];
            checkpoints_.push_back(std::make_tuple(activePaint, checkpoint_));
        }

        if (checkpoints_.size() > 0) 
        {
            if (ImGui::Button("restore"))
            {
                auto& [checkpointPaint, checkpoint] = checkpoints_.back();
                auto& cells0 = domain->getReadFromCells();
                auto& cells1 = domain->getWriteToCells();
                for (size_t i = 0; i < domain->getCellCount(); ++i)
                { 
                    cells0[i][checkpointPaint] = checkpoint[i];
                    cells1[i][checkpointPaint] = checkpoint[i];
                    if (simulation->isGPUEnabled) 
                    {
                        domain->dirtyAttributes[domain->CELL1_ATTRIB].indices.insert((unsigned) i);
                        domain->dirtyAttributes[domain->CELL2_ATTRIB].indices.insert((unsigned) i);
                    }
                }
                checkpoints_.pop_back();
                if (simulation->isGPUEnabled)
                {
                    simulation->gpuUpToDate(false);
                    simulation->updateColorsFromRam = true;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("clear")) 
                checkpoints_.clear();
        }
    }

    if (ImGui::DragFloat("Radius", &paintRadius_, .005f, 0.f, 100.f))
        app->cursor_.transform_.setScale(paintRadius_);

    if (domain->isPaintingDiffDir())
    {
        domain->childern_[0]->visible_ = true;
        if (!pv)
        {
            std::fill(begin(domain->paintingMorph), end(domain->paintingMorph), false);
            domain->selectCells = false;
        }
    }

    if ((domain->isPaintingDiffDir() || domain->selectCells || pmorph) && !disablePaint_)
    {
        app->cursor_.visible_ = true;
        app->camera_.zoomEnabled_ = false;
        if (simulation->isGPUEnabled)
        {
            app->scene_.pause(true);
            if (!simulation->ramUpToDate())
                simulation->updateRAM();
        }
    }
    else
    {
        app->cursor_.visible_ = false;
        app->camera_.zoomEnabled_ = true;
    }

    for (auto m : simulation->morphIndexMap)
    {
        if (pmorph && domain->paintingMorph[m.second])
        {
            float paintVal = domain->clearMValue[m.second];
            ImGui::Separator();
            ImGui::Text("Value");
            ImGui::DragFloat(("##Value" + m.first + "set").c_str(), &paintVal, .01f, -10.f, 10.f);
            ImGui::SameLine();
            if (ImGui::Button(("set##" + m.first).c_str()))
            {
                domain->clearMorphogens(m.second);
                simulation->updateColorsFromRam = true;
                simulation->gpuUpToDate(false);
            }
            domain->clearMValue[m.second] = paintVal;
        }
    }

    if (pv)
    {
        ImGui::Separator();
        ImGui::Text("Eigen Values");
        ImGui::InputFloat2("##eigenValues", domain->paintTensorVals, "%.6f"); ImGui::SameLine();
        bool clearEigens = ImGui::Button("set##DiffusionEigenVals");

        ImGui::Text("Direction");
        ImGui::InputFloat3("##tangentComponents", tangentComponents_); ImGui::SameLine();
        bool clearVects = ImGui::Button("set##DiffusionComponents");

        if (clearEigens || clearVects)
        {
            int paintingDiffIndex = 0;
            for (unsigned i = 0; i < domain->paintingDiffDir.size(); ++i)
                if (domain->paintingDiffDir[i])
                    paintingDiffIndex = i;

            if (clearEigens)
            {
                if (domain->paintingDiffDir[paintingDiffIndex])
                    domain->clearDiffusion(paintingDiffIndex);
            }
            if (clearVects)
            {
                domain->projectAniVecs(Vec3(tangentComponents_[0], tangentComponents_[1], tangentComponents_[2]), paintingDiffIndex);
            }

            domain->updateDiffusionCoefs();
            simulation->gpuUpToDate(false);
        }
    }

    ImGui::NewLine();
}

void GUI::drawSelectOptions(RDPG* app)
{
    SimulationDomain* domain = app->domain_;
    Simulation* simulation = app->simulation_;
    ImGui::Begin("Select Options", &showSelectOptions_, ImGuiWindowFlags_AlwaysAutoResize);
    auto buttonSize = ImVec2{ 130, 25 };
    if (ImGui::Button("all##selectAll", buttonSize))
    {
        domain->selectedCells.clear();
        domain->invertSelected();
        simulation->updateColorsFromRam = true;
    }

    if (ImGui::Button("holes##selectBorder", buttonSize))
    {
        domain->selectBorder();
        simulation->updateColorsFromRam = true;
    }

    if (ImGui::Button("invert##toggleInvert", buttonSize))
    {
        domain->invertSelected();
        simulation->updateColorsFromRam = true;
    }

    if (ImGui::Button("clear##clearSelected", buttonSize))
    {
        domain->selectedCells.clear();
        simulation->updateColorsFromRam = true;
    }
    
    if (ImGui::Button("boundary##selectBoundary", buttonSize))
    {
        domain->selectBoundary();
        simulation->updateColorsFromRam = true;
    }

    if (ImGui::Button("select neighbours##selectNeighbours", buttonSize))
    {
        domain->selectNeighbours();
        simulation->updateColorsFromRam = true;
    }

    ImGui::End();
}

void GUI::drawRenderingControls(RDPG* app)
{
    SimulationDomain* domain = app->domain_;
    Simulation* simulation = app->simulation_;
    bool ssc = domain->showSelectedCells;

    // Color map editing
    if (ImGui::TreeNode("Colormap"))
    {
        // Outside colormap
        ImGui::Text(useInsideColormap_ ? "Outside" : "Outside/Inside");
        if (ImGui::GradientEditor(&gradientOutside_, draggingMarkOutside_, selectedMarkOutside_))
        {
            updateColormap(gradientOutside_, domain->colorMapOutside);
            if (!useInsideColormap_)
                updateColormap(gradientOutside_, domain->colorMapInside);
        }
        ImGui::InputText("##colorMapNameOutside", &colorMapOutsideFilename_[0], 128);
        ImGui::SameLine();
        if (ImGui::Button("update##colormapOutside"))
        {
            for (size_t loc = 0; (loc = colorMapOutsideFilename_.find('/', loc)) != std::string::npos;)
                colorMapOutsideFilename_[loc] = '\\';

            size_t lastBackSlashLoc = colorMapOutsideFilename_.find_last_of('\\');
            domain->colorMapOutside.setFilenameAndPath(
                colorMapOutsideFilename_.substr(lastBackSlashLoc + 1, colorMapOutsideFilename_.size() - lastBackSlashLoc),
                colorMapOutsideFilename_.substr(0, lastBackSlashLoc + 1));

            if (domain->colorMapOutside.loadTexture())
                setColorMapOutside(domain);
        }

        // Inside colormap --------------------------
        if (useInsideColormap_)
        {
            ImGui::NewLine();
            ImGui::Text("Inside");
            if (ImGui::GradientEditor(&gradientInside_, draggingMarkInside_, selectedMarkInside_))
                updateColormap(gradientInside_, domain->colorMapInside);
            ImGui::InputText("##colorMapNameInside", &colorMapInsideFilename_[0], 128);
            ImGui::SameLine();
            if (ImGui::Button("update##colormapInside"))
            {
                for (size_t loc = 0; (loc = colorMapInsideFilename_.find('/', loc)) != std::string::npos;)
                    colorMapInsideFilename_[loc] = '\\';

                size_t lastBackSlashLoc = colorMapInsideFilename_.find_last_of('\\');
                domain->colorMapInside.setFilenameAndPath(
                    colorMapInsideFilename_.substr(lastBackSlashLoc + 1, colorMapInsideFilename_.size() - lastBackSlashLoc),
                    colorMapInsideFilename_.substr(0, lastBackSlashLoc + 1));

                if (domain->colorMapInside.loadTexture())
                    setColorMapInside(domain);
            }
        }

        if (ImGui::Checkbox("Use inside colormap", &useInsideColormap_)) // unchecked
        {
            if (useInsideColormap_)
                updateColormap(gradientInside_, domain->colorMapInside);
            else
                updateColormap(gradientOutside_, domain->colorMapInside);
        }
        ImGui::TreePop();
    }
    ImGui::Separator();

    // Other graphics options
    bool cullface = app->window_.graphics_.cullfaceEnabled();
    ImGui::Checkbox("Hide faces", &cullface);
    if (cullface != app->window_.graphics_.cullfaceEnabled())
        app->window_.graphics_.toggleCullFace();

    if (cullface)
    {
        ImGui::SameLine(ImGui::GetWindowWidth() - 70);
        bool toggleFrontFace = false;
        if (app->window_.graphics_.isFrontFaceCCW())
            toggleFrontFace = ImGui::Button("CCW", ImVec2(50, 18));
        else
            toggleFrontFace = ImGui::Button("CW", ImVec2(50, 18));
        if (toggleFrontFace)
            app->window_.graphics_.toggleFrontFace();
    }

    ImGui::Checkbox("Wireframe (Q)", &wireframeEnabled_);

    ImGui::Checkbox("Show selected", &domain->showSelectedCells);
    if (domain->showSelectedCells)
    {
        simulation->outMorphIndex = -1;

        if (!ssc)
        {
            simulation->updateColorsFromRam = simulation->isGPUEnabled;
            domain->shader_->setSelectingCells(true);
            std::fill(begin(domain->paintingMorph), end(domain->paintingMorph), false);
        }
    }
    else if (ssc)
    {
        domain->shader_->setSelectingCells(false);
        domain->selectCells = false;
    }

    // Draw controls for showing morphogens
    int selectedMorphIndex = -1;
    for (auto& m : simulation->morphIndexMap)
    {
        bool showM = domain->showingMorph[m.second];
        ImGui::Checkbox(("Show " + m.first).c_str(), &showM);

        if (!domain->showingMorph[m.second] && showM) // on check
        {
            selectedMorphIndex = m.second;
            domain->showSelectedCells = false;
            domain->selectCells = false;
            domain->shader_->setSelectingCells(false);
            domain->showingMorph[m.second] = true;
            simulation->outMorphIndex = m.second;
            simulation->updateColorsFromRam = simulation->isPaused();
        }
    }

    for (int i = 0; i < simulation->MORPH_COUNT && selectedMorphIndex != -1; ++i)
        if (i != selectedMorphIndex)
        {
            domain->showingMorph[i] = false;
            domain->paintingMorph[i] = false;
        }

    // Draw controls for showing gradient
    int selectedGradIndex = -1;
    for (auto& m : simulation->morphIndexMap)
    {
        bool showGrad = domain->showingGrad[m.second];
        ImGui::Checkbox(("Show " + m.first + " gradient vectors").c_str(), &showGrad);
        if (showGrad != domain->showingGrad[m.second])
            selectedGradIndex = m.second;
        domain->showingGrad[m.second] = showGrad;
    }
    for (int i = 0; i < simulation->MORPH_COUNT && selectedGradIndex != -1; ++i)
        if (i != selectedGradIndex)
            domain->showingGrad[i] = false;

    domain->gradientLines.visible_ = false;
    for (bool showGrad : domain->showingGrad)
        domain->gradientLines.visible_ = showGrad || domain->gradientLines.visible_;

    // Show diffusion vectors
    domain->childern_[0]->visible_ = false;
    for (auto& m : simulation->morphIndexMap)
    {
        bool showTensor = domain->showingDiffDir[m.second];
        ImGui::Checkbox(("Show " + m.first + " diffusion vectors").c_str(), &showTensor);

        if (showTensor != domain->showingDiffDir[m.second]) // state changed
        {
            if (showTensor)
            {
                std::fill(domain->showingDiffDir.begin(), domain->showingDiffDir.end(), false);
                std::fill(domain->paintingDiffDir.begin(), domain->paintingDiffDir.end(), false);
                domain->showingDiffDir[m.second] = true;
            }
            else
            {
                domain->showingDiffDir[m.second] = false;
                domain->paintingDiffDir[m.second] = false;
            }
        }
        domain->childern_[0]->visible_ = showTensor || domain->childern_[0]->visible_;
    }
    if (!domain->childern_[0]->visible_)
        std::fill(domain->paintingDiffDir.begin(), domain->paintingDiffDir.end(), false);

    // Show gradient vectors
    if (ImGui::Button("Use gradient for diffusion"))
    {
        int morphIndex = 0, gradIndex = 0;
        for (int i = 0; i < domain->numMorphs_; ++i)
        {
            if (domain->showingDiffDir[i])
                morphIndex = i;
            if (domain->showingGrad[i])
                gradIndex = i;
        }
        domain->copyGradientToDiffusion(morphIndex, gradIndex);     
        simulation->gpuUpToDate(false);
    }

    ImGui::NewLine();

    if (ImGui::Button("Reset Orientation (1)"))
    {
        app->trackball_.reset();
        app->camera_.fitTo(calculateBounds(*domain));
    }

    ImGui::DragFloat("Normalization", &domain->normCoef, .01f, 0.f, 100.f);
    ImGui::DragFloat("Background Span", &domain->backgroundThreshold_, .01f, 0.f, 10.f);
    ImGui::DragFloat("Background Offset", &domain->backgroundOffset_, .01f, 0.f, 10.f);
    ImGui::DragFloat("Line width", &lineWidth_, .1f, 1.f, 10.f);
    glLineWidth(lineWidth_);
    {
        ImGui::DragFloat("Vector length", &domain->vecLength, .001f, 0.f, 100.f);
    }
    ImGui::NewLine();

    if (!domain->childern_[0]->visible_)
    {
        std::fill(domain->paintingDiffDir.begin(), domain->paintingDiffDir.end(), false);
    }

    int freq = (int)app->renderCounter_.getDuration() + 1;
    ImGui::Text("Render Frequency (Sims/Frame)");
    ImGui::InputInt("##renderFreq", &freq);
    
    if (freq != 0)
        freq--;

    if ((int)app->renderCounter_.getDuration() != freq)
        app->renderCounter_.setDuration((long long unsigned int)freq);


    float backgroundColors[3] = {
        app->backgroundColor_.r / 255.f,
        app->backgroundColor_.g / 255.f,
        app->backgroundColor_.b / 255.f
    };

    if (ImGui::ColorEdit3("Background", backgroundColors))
    {
        app->backgroundColor_.r = static_cast<unsigned char>(Utils::clamp(backgroundColors[0] * 255.f, 0.f, 255.f));
        app->backgroundColor_.g = static_cast<unsigned char>(Utils::clamp(backgroundColors[1] * 255.f, 0.f, 255.f));
        app->backgroundColor_.b = static_cast<unsigned char>(Utils::clamp(backgroundColors[2] * 255.f, 0.f, 255.f));
        app->window_.setBackgroundColor(app->backgroundColor_);
    }

    ImGui::NewLine();
}

void GUI::drawStats(RDPG* app)
{
    SimulationDomain* domain = app->domain_;
    Simulation* simulation = app->simulation_;
    ImGui::Begin("Stats", &showStatsWindow_, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Avg %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::Separator();
    ImGui::Text(("Sim Steps:       " + std::to_string(simulation->stepCount)).c_str());
    ImGui::Text(("Cell count:      " + std::to_string(domain->getCellCount())).c_str());

    ImGui::Separator();
    if (ImGui::TreeNode("Geometry"))
    {
        ImGui::Text(("Vertices count:  " + std::to_string(domain->positions_.size())).c_str());
        ImGui::Text(("Normals count:   " + std::to_string(domain->normals_.size())).c_str());
        ImGui::Text(("Colors count:    " + std::to_string(domain->colors_.size())).c_str());
        ImGui::Text(("Tangents count:  " + std::to_string(domain->tangents[0].size())).c_str());
        ImGui::Text(("Indices count:   " + std::to_string(domain->indices_.size())).c_str());

        if (domain->isDomainType(SimulationDomain::DomainType::MESH))
        {
            HalfEdgeMesh* meshDomain = dynamic_cast<HalfEdgeMesh*>(domain);
            ImGui::Text(("Edges:           " + std::to_string(meshDomain->edges.size() / 2)).c_str());
            ImGui::Text(("Faces:           " + std::to_string(meshDomain->faces.size())).c_str());
        }
        float totalArea = domain->getTotalArea();        
        ImGui::Text(("Total Area:      " + std::to_string(totalArea)).c_str());
        ImGui::Text(("Sqrt Total Area: " + std::to_string(std::sqrt(totalArea))).c_str());
        
        if (domain->isDomainType(SimulationDomain::DomainType::MESH))
        {
            HalfEdgeMesh* meshDomain = dynamic_cast<HalfEdgeMesh*>(domain);
            ImGui::Text(("Avg Area:        " + std::to_string(totalArea / meshDomain->faces.size())).c_str());
        }
        else if (domain->isDomainType(SimulationDomain::DomainType::GRID))
        {
            Grid* gridDomain = static_cast<Grid*>(domain);
            ImGui::Text(("Avg Area:        " + std::to_string(gridDomain->getCellSize() * gridDomain->getCellSize())).c_str());
        }

        ImGui::TreePop();
    }

    ImGui::End();
}

void GUI::drawInfo(RDPG* app)
{
    SimulationDomain* domain = app->domain_;
    Simulation* simulation = app->simulation_;
    ImGui::Begin("Info", &showInfoWindow_);

    // Draw vertex selected controls
    int v = domain->selectedCell;
    int cellCount = static_cast<int>(domain->getCellCount());
    ImGui::Text("Selected Vertex"); ImGui::SameLine();
    ImGui::InputInt("##SelectedVertexInput", &v);
    if (v < 0)
        v = 0;
    else if (v >= cellCount)
        v = cellCount - 1;
    domain->selectedCell = v;

    ImGui::Separator();

    for (auto morph : simulation->morphIndexMap)
    {
        ImGui::Text((morph.first + " vals: " + std::to_string(domain->getReadFromCells()[v].vals[morph.second])).c_str());
        ImGui::Text((morph.first + " v: " + domain->tangents[morph.second][v].toString()).c_str());
        ImGui::Separator();
    }

    if (simulation->rawInitConds != "")
    {
        ImGui::Text((simulation->rawInitConds).c_str());
        ImGui::Separator();
    }

    if (simulation->rawBCs != "")
    {
        ImGui::Text((simulation->rawBCs).c_str());
        ImGui::Separator();
    }
    
    ImGui::Text((simulation->rawRDModel).c_str());
    ImGui::End();
}

void GUI::drawControls(RDPG* app)
{
    SimulationDomain* domain = app->domain_;
    Simulation* simulation = app->simulation_;
    ImGui::Begin("Controls", &showControlsWindow_);

    ImGui::Text("Simulation");
    ImGui::Separator();

    // Screenshot
    ImGui::InputText(".png", screenshotName_, 128);
    ImGui::SameLine();
    if (ImGui::Button("save##png"))
        app->window_.screenshot((std::string(screenshotName_) + ".png").c_str());

    // rd file
    ImGui::InputText(".rd ", simulationName_, 128);
    ImGui::SameLine();
    if (ImGui::Button("save##rd")) 
        simulation->saveConcentrations("", std::string(simulationName_) + ".rd");

    // obj file
    if (domain->isDomainType(SimulationDomain::DomainType::MESH))
    {
        HalfEdgeMesh* meshDomain = dynamic_cast<HalfEdgeMesh*>(domain);
        ImGui::InputText(".obj", objName_, 128);
        ImGui::SameLine();
        if (ImGui::Button("save##obj"))
            meshDomain->saveToObj((std::string(objName_) + ".obj").c_str());

        ImGui::InputText(".ply", plyName_, 128);
        ImGui::SameLine();
        if (ImGui::Button("save##ply"))
            meshDomain->saveToPly((std::string(plyName_) + ".ply").c_str());
    }

    if (ImGui::Button("Reload PDEs"))
    {
        LOG("Reloading PDEs...");
        simulation->shouldReloadPDEs(true);
    }

    ImGui::NewLine();
    ImGui::Text("Pause/Exit conditions");
    ImGui::Separator();
    ImGui::InputInt("##PauseStep", &simulation->pauseStepCount);
    ImGui::SameLine();
    if (ImGui::Checkbox("Pause step", &simulation->pauseAt))
    {
        if (simulation->stepCount >= simulation->pauseStepCount)
            app->pause(true);
    }

    ImGui::InputInt("##ExitStep", &simulation->exitStepCount);
    ImGui::SameLine();
    ImGui::Checkbox("Exit step", &simulation->exitAt);
    ImGui::Checkbox("Save on exit##", &simulation->createModelOnExit);

    // Save model on exit
    if (simulation->createModelOnExit)
    {
        ImGui::InputText("Pattern Name", patternName_, 128);
        ImGui::InputTextMultiline("Pattern Description", patternDesc_, 256);
    }
    ImGui::NewLine();

    // Save a video
    ImGui::Text("Recording");
    ImGui::Separator();
    ImGui::InputText("Output Path", simulation->videoPath, 256);
    ImGui::Checkbox("Output textures", &simulation->outputTextures);
    ImGui::Checkbox("Output screenshots", &simulation->outputScreens);
    ImGui::Checkbox("Output plys", &simulation->outputPlys);
    ImGui::NewLine();

    // Save a texture
    ImGui::Text("Texture Output");
    ImGui::Separator();
    ImGui::InputText(".png##texture", textureName_, 256);
    ImGui::SameLine();
    if (ImGui::Button("save##texture"))
    {
        saveTextureCallback_(
            Utils::stripNulls(std::string(textureName_ + std::string(".png"))));
    }
    ImGui::InputInt2("Texture Size", app->texSize_);
    ImGui::Checkbox("Save texture on exit", &simulation->createTextureOnExit);
    ImGui::NewLine();
    ImGui::NewLine();

    // Start and step controls
    ImGui::SameLine(ImGui::GetWindowWidth() / 2 - 172);
    if (ImGui::Button("STEP##sim", ImVec2(170, 25)))
        app->scene_.step();

    ImGui::SameLine(ImGui::GetWindowWidth() / 2 + 2);
    if (simulation->isPaused())
    {
        if (ImGui::Button("START", ImVec2(170, 25)))
        {
            std::fill(domain->paintingDiffDir.begin(), domain->paintingDiffDir.end(), false);
            std::fill(domain->paintingMorph.begin(), domain->paintingMorph.end(), false);
            domain->selectCells = false;
            domain->shader_->setSelectingCells(false);
            app->cursor_.visible_ = false;
            app->pause(false);
        }
    }
    else if (ImGui::Button("PAUSE", ImVec2(170, 25)))
        app->pause(true);

    ImGui::End();
}

void GUI::drawSaveWindow(RDPG* app)
{
    //ImGui::SetNextWindowPos(ImVec2(366, 227), ImGuiCond_FirstUseEver);

    //{
    //    nfdchar_t* outPath = nullptr;
    //    nfdresult_t result = NFD_SaveDialog("", nullptr, &outPath);
    //    if (result == NFD_OKAY)
    //    {
    //        LOG(std::string(outPath));
    //        //loadCallback_(std::string(outPath));
    //    }
    //    else if (result == NFD_CANCEL)
    //    {
    //        LOG("Save file cancelled");
    //    }
    //    else {
    //        LOG(NFD_GetError());
    //    }
    //}

    ImGui::Begin("Save", &showSaveWindow_); //, ImVec2(371, 229)
    ImGui::Text("Pattern Name");
    ImGui::InputText("##Pattern Name", patternName_, 128);

    ImGui::Text("Pattern Description");
    ImGui::InputTextMultiline("##Pattern Description", patternDesc_, 256);
    ImGui::NewLine();
    ImGui::SameLine(ImGui::GetWindowWidth() / 2 - 55);

    if (ImGui::Button("SAVE##pattern", ImVec2(110, 25)))
    {
        save(app);
        showSaveWindow_ = false;
    }
    ImGui::End();
}

void GUI::drawAnimationControls(RDPG* app)
{
    if (!app->domain_->isDomainType(SimulationDomain::DomainType::MESH))
        return;

    HalfEdgeMesh* domain = dynamic_cast<HalfEdgeMesh*>(app->domain_);
    ASSERT(domain, "Cast to half edge mesh failed");

    if (domain && domain->growthMode == SimulationDomain::GrowthMode::AnimationGrowth)
    {
        ImGui::Begin("Animation ctrls");
        ImGui::InputInt("Cur control point", &curControlPointIndex_);

        const int maxCtrlPts = static_cast<int>(app->patch_.size() * app->patch_[0].controlPoints().size());
        curControlPointIndex_ = curControlPointIndex_ < 0 ? 0 : curControlPointIndex_ >= maxCtrlPts ? maxCtrlPts - 1 : curControlPointIndex_;
        size_t splineIndex = curControlPointIndex_ / app->patch_.size();
        size_t curPtIndex = curControlPointIndex_ % app->patch_[splineIndex].controlPoints().size();

        Vec3 ctrlPoint = app->patch_[splineIndex].getControlPoint(curPtIndex);
        float pos[3]{ ctrlPoint.x, ctrlPoint.y, ctrlPoint.z };
        if (ImGui::InputFloat3("pos", pos, "%.6f"))
        {
            ctrlPoint.set(pos[0], pos[1], pos[2]);
            app->patch_[splineIndex].setControlPoint(curPtIndex, ctrlPoint);
            app->patch_[splineIndex].initDrawable(*app->patchDrawable_[splineIndex]);
            updatePositionsFromBSplinePatch(domain->positions_, domain->animation_.UVs(), app->patch_);
            domain->updatePositionVBO();
        }

        static int keyNum = 0;
        ImGui::InputInt("keyframe num", &keyNum);
        if (ImGui::Button("record keyframe"))
        {
            LOG("keyframe output");
            std::cout << keyNum << "\n";
            for (auto& s : app->patch_)
            {
                for (auto& p : s.controlPoints())
                {
                    std::cout << p << " ";
                }
                std::cout << "\n";
            }
            domain->animation_.addKeyframe(keyNum, app->patch_);
        }

        if (ImGui::Button("update"))
        {
            updatePositionsFromBSplinePatch(domain->positions_, domain->animation_.UVs(), app->patch_);
            domain->updatePositionVBO();
        }

        ImGui::End();
    }
}

void GUI::setPatternName(const std::string& name)
{
    for (int i = 0; i < 128; i++)
    {
        patternName_[i] = name[i];
        if (name[i] == '\0')
            return;
    }
}

void GUI::saveColorMap(const std::string& name, ImGradient& gradient)
{
    std::ofstream colorFile(name);
    if (colorFile.is_open())
    {
        auto marks = gradient.getMarks();
        for (auto mark : marks)
        {
            colorFile << mark->color[0] << ' ';
            colorFile << mark->color[1] << ' ';
            colorFile << mark->color[2] << ' ';
            colorFile << mark->position << '\n';
        }
        colorFile.close();
    }
    else
        std::cout << "Cannot save colormap: " << name << "\n";
}
