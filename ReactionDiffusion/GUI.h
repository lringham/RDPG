#pragma once
#include "Graphics.h"
#include "imgui_color_gradient.h"

#include <map>
#include <string>
#include <functional>


class SimulationDomain;
class RDPG;
class ColorMap;
class GUI
{
public:
    GUI(GLFWwindow* windowPtr);
    ~GUI();

    void draw(RDPG* app);
    void confExit(RDPG* app);
    bool hasUsedEvent();
    void setColorMapOutside(SimulationDomain* domain);
    void setColorMapInside(SimulationDomain* domain);
    void save(RDPG* app);
    std::string getColorMapOutsideName();
    std::string getColorMapInsideName();
    void saveColorMap(const std::string& name, ImGradient& gradient);
    void setPatternName(const std::string& name);
    void setSimulationParams(const std::map<std::string, float>& originalParams);
    void updateColormap(ImGradient& gradient, ColorMap& colormap);
    void toggleVisible();
    void setDisablePainting(bool disable);
    bool isPaintingDisabled() const;
    void setVisible(bool visible);

    std::map<std::string, float*> rdParams_;
    float tangentComponents_[3] = { 1.f, 0.f, 0.f };
    float lineWidth_ = 1.f;
    std::map<std::string, float> tempParams_;

    int selectedLocalParams_ = 0;
    bool selectAllParams_ = true;
    bool showExitConf_ = false;
    bool exitProgram_ = false;
    float paintRadius_ = .025f;
    char patternName_[128] = "pattern";
    char patternDesc_[256] = "description";
    char screenshotName_[128] = "screenshot";
    char simulationName_[128] = "reactionDiffusion";
    char objName_[128] = "model";
    char plyName_[128] = "model";
    char configFileName_[128] = "SimConfig";
    char simNameLoad_[128] = "reactionDiffusion";
    char objNameLoad_[128] = "model";
    char textureName_[256] = "texture";
    char prepatternTarget0_[256] = "";
    char prepatternSource0_[256] = "";
    char prepatternTarget1_[256] = "";
    char prepatternSource1_[256] = "";

    std::function<void(const std::string&)> saveTextureCallback_;
    std::function<void(const std::string&)> loadCallback_;

private:
    void drawMenuBar(RDPG* app);
    void drawParametersControls(RDPG* app);
    void drawGrowthControls(RDPG* app);
    void drawPaintingControls(RDPG* app);
    void drawSelectOptions(RDPG* app);
    void drawRenderingControls(RDPG* app);
    void drawStats(RDPG* app);
    void drawInfo(RDPG* app);
    void drawControls(RDPG* app);
    void drawSaveWindow(RDPG* app);
    void drawAnimationControls(RDPG* app);

    std::string colorMapOutsideFilename_;
    std::string colorMapInsideFilename_;

    std::vector<bool> selectedParams_;
    
    std::vector<std::tuple<int, std::vector<float>>> checkpoints_;

    int curGrowthMode_ = 0;
    int curControlPointIndex_ = 0;
    bool showSaveWindow_ = false;
    bool showControlsWindow_ = false;
    bool showStatsWindow_ = false;
    bool showInfoWindow_ = false;
    bool showSelectOptions_ = false;
    bool useInsideColormap_ = false;
    bool visible_ = true;
    bool disablePaint_ = false;
    bool wireframeEnabled_ = false;

    ImGradientMark* draggingMarkOutside_ = nullptr; ImGradientMark* draggingMarkInside_ = nullptr;
    ImGradientMark* selectedMarkOutside_ = nullptr; ImGradientMark* selectedMarkInside_ = nullptr;
    ImGradient gradientInside_, gradientOutside_;
};

