#include "ResourceLocations.h"


ResourceLocations ResourceLocations::instance_;

ResourceLocations& ResourceLocations::instance() { return instance_; }

const std::string& ResourceLocations::modelsPath() const { return modelsPath_; }
const std::string& ResourceLocations::colorMapsPath() const { return colorMapsPath_; }
const std::string& ResourceLocations::shadersPath() const { return shadersPath_; }
const std::string& ResourceLocations::exePath() const { return exePath_; }
const std::string& ResourceLocations::launchPath() const { return launchPath_; }

void ResourceLocations::modelsPath(const std::string& path) { modelsPath_ = path; }
void ResourceLocations::colorMapsPath(const std::string& path) { colorMapsPath_ = path; }
void ResourceLocations::shadersPath(const std::string& path) { shadersPath_ = path; }
void ResourceLocations::exePath(const std::string& path) { exePath_ = path; }
void ResourceLocations::launchPath(const std::string& path) { launchPath_ = path; }