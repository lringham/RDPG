#pragma once
#include "Textures.h"

#include <string>


class ColorMap
{
public:
    ColorMap() = default;
    ColorMap(const std::string& filename, const std::string& path);

    void init(const std::string& filename, const std::string& path, bool trackFile = false);
    void track(bool track);
    bool isTracking() const;
    GLuint getTextureID();
    std::string getName() const;
    std::string getPath() const;
    std::string getNameAndPath() const;
    std::string getExt() const;

    void setFilenameAndPath(const std::string& filename, const std::string& path);
    void setFilename(const std::string& filename);
    void setPath(const std::string& path);
    void setExtToColors();
    void setExtToMap();
    void sample(float t, unsigned char& r, unsigned char& g, unsigned char& b, bool linearBlending = true);
    void update(bool fromColorMarks = false);
    bool loadTexture();

    std::vector<unsigned char> data_;
    std::vector<float> colorMarks_;

private:

    std::string filename_;
    std::string path_;
    std::string ext_;
    Textures::Texture1D texture_;
    bool initialized_ = false;
    bool tracking_ = false;
};

