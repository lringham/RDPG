#pragma once
#include <string>


class ResourceLocations
{
    ResourceLocations() = default;
    static ResourceLocations instance_;

    std::string modelsPath_;
    std::string colorMapsPath_;
    std::string shadersPath_ = "shaders/";
    std::string exePath_;
    std::string launchPath_;

public:
    static ResourceLocations& instance();

    const std::string& modelsPath() const;
    const std::string& colorMapsPath() const;
    const std::string& shadersPath() const;
    const std::string& exePath() const;
    const std::string& launchPath() const;

    void modelsPath(const std::string& path);
    void colorMapsPath(const std::string& path);
    void shadersPath(const std::string& path);
    void exePath(const std::string& path);
    void launchPath(const std::string& path);
};