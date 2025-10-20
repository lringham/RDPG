#include "ColorMap.h"
#include "LOG.h"
#include "Utils.h"

#include <vector>
#include <fstream>


ColorMap::ColorMap(const std::string& filename, const std::string& path)
{
    init(filename, path);
}

void ColorMap::init(const std::string& filename, const std::string& path, bool trackFile)
{
    initialized_ = filename != "";
    setFilenameAndPath(initialized_ ? filename : "colormap.colors", path);
    track(trackFile);
    loadTexture();
    initialized_ = true;
}

std::string ColorMap::getName() const
{
    return filename_ + getExt();
}

std::string ColorMap::getPath() const
{
    return path_;
}

std::string ColorMap::getNameAndPath() const
{
    return path_ + getName();
}

std::string ColorMap::getExt() const
{
    return ext_;
}

void ColorMap::setFilenameAndPath(const std::string& filename, const std::string& path)
{
    colorMarks_.clear();
    data_.clear();

    // parse path, name, and exts
    path_    = path;
    auto loc  = filename.find_last_of(".");
    ext_      = Utils::sToLower(filename.substr(loc, filename.size() - loc));
    filename_ = filename.substr(0, loc);

    // strip placeholder nulls 
    path_     = path_.substr(0, path_.find_first_of('\0'));
    filename_ = filename_.substr(0, filename_.find_first_of('\0'));
    ext_      = ext_.substr(0, ext_.find_first_of('\0'));
}

void ColorMap::setFilename(const std::string& filename)
{
    filename_ = filename;
    loadTexture();
}

void ColorMap::setPath(const std::string& path)
{
    path_ = path;
    loadTexture();
}

void ColorMap::setExtToColors()
{
    ext_ = ".colors";
}

void ColorMap::setExtToMap()
{
    ext_ = ".map";
}

void ColorMap::track(bool track)
{
    tracking_ = track;
}

bool ColorMap::isTracking() const
{
    return tracking_;
}

bool ColorMap::loadTexture()
{
    std::vector<char> bytes;
    bool updatedBytes = false;

    if (!initialized_)
    {
        colorMarks_.clear();
        unsigned char turbo_srgb_bytes[256][3] = { {48,18,59},{50,21,67},{51,24,74},{52,27,81},{53,30,88},{54,33,95},{55,36,102},{56,39,109},{57,42,115},{58,45,121},{59,47,128},{60,50,134},{61,53,139},{62,56,145},{63,59,151},{63,62,156},{64,64,162},{65,67,167},{65,70,172},{66,73,177},{66,75,181},{67,78,186},{68,81,191},{68,84,195},{68,86,199},{69,89,203},{69,92,207},{69,94,211},{70,97,214},{70,100,218},{70,102,221},{70,105,224},{70,107,227},{71,110,230},{71,113,233},{71,115,235},{71,118,238},{71,120,240},{71,123,242},{70,125,244},{70,128,246},{70,130,248},{70,133,250},{70,135,251},{69,138,252},{69,140,253},{68,143,254},{67,145,254},{66,148,255},{65,150,255},{64,153,255},{62,155,254},{61,158,254},{59,160,253},{58,163,252},{56,165,251},{55,168,250},{53,171,248},{51,173,247},{49,175,245},{47,178,244},{46,180,242},{44,183,240},{42,185,238},{40,188,235},{39,190,233},{37,192,231},{35,195,228},{34,197,226},{32,199,223},{31,201,221},{30,203,218},{28,205,216},{27,208,213},{26,210,210},{26,212,208},{25,213,205},{24,215,202},{24,217,200},{24,219,197},{24,221,194},{24,222,192},{24,224,189},{25,226,187},{25,227,185},{26,228,182},{28,230,180},{29,231,178},{31,233,175},{32,234,172},{34,235,170},{37,236,167},{39,238,164},{42,239,161},{44,240,158},{47,241,155},{50,242,152},{53,243,148},{56,244,145},{60,245,142},{63,246,138},{67,247,135},{70,248,132},{74,248,128},{78,249,125},{82,250,122},{85,250,118},{89,251,115},{93,252,111},{97,252,108},{101,253,105},{105,253,102},{109,254,98},{113,254,95},{117,254,92},{121,254,89},{125,255,86},{128,255,83},{132,255,81},{136,255,78},{139,255,75},{143,255,73},{146,255,71},{150,254,68},{153,254,66},{156,254,64},{159,253,63},{161,253,61},{164,252,60},{167,252,58},{169,251,57},{172,251,56},{175,250,55},{177,249,54},{180,248,54},{183,247,53},{185,246,53},{188,245,52},{190,244,52},{193,243,52},{195,241,52},{198,240,52},{200,239,52},{203,237,52},{205,236,52},{208,234,52},{210,233,53},{212,231,53},{215,229,53},{217,228,54},{219,226,54},{221,224,55},{223,223,55},{225,221,55},{227,219,56},{229,217,56},{231,215,57},{233,213,57},{235,211,57},{236,209,58},{238,207,58},{239,205,58},{241,203,58},{242,201,58},{244,199,58},{245,197,58},{246,195,58},{247,193,58},{248,190,57},{249,188,57},{250,186,57},{251,184,56},{251,182,55},{252,179,54},{252,177,54},{253,174,53},{253,172,52},{254,169,51},{254,167,50},{254,164,49},{254,161,48},{254,158,47},{254,155,45},{254,153,44},{254,150,43},{254,147,42},{254,144,41},{253,141,39},{253,138,38},{252,135,37},{252,132,35},{251,129,34},{251,126,33},{250,123,31},{249,120,30},{249,117,29},{248,114,28},{247,111,26},{246,108,25},{245,105,24},{244,102,23},{243,99,21},{242,96,20},{241,93,19},{240,91,18},{239,88,17},{237,85,16},{236,83,15},{235,80,14},{234,78,13},{232,75,12},{231,73,12},{229,71,11},{228,69,10},{226,67,10},{225,65,9},{223,63,8},{221,61,8},{220,59,7},{218,57,7},{216,55,6},{214,53,6},{212,51,5},{210,49,5},{208,47,5},{206,45,4},{204,43,4},{202,42,4},{200,40,3},{197,38,3},{195,37,3},{193,35,2},{190,33,2},{188,32,2},{185,30,2},{183,29,2},{180,27,1},{178,26,1},{175,24,1},{172,23,1},{169,22,1},{167,20,1},{164,19,1},{161,18,1},{158,16,1},{155,15,1},{152,14,1},{149,13,1},{146,11,1},{142,10,1},{139,9,2},{136,8,2},{133,7,2},{129,6,2},{126,5,2},{122,4,3} };
        for (int i = 0; i <= 255; i += 17)
        {
            bytes.push_back(turbo_srgb_bytes[i][0]);
            bytes.push_back(turbo_srgb_bytes[i][1]);
            bytes.push_back(turbo_srgb_bytes[i][2]);

            float s = static_cast<float>(i) / 255.f;
            colorMarks_.push_back((float)turbo_srgb_bytes[i][0] / 255.f);
            colorMarks_.push_back((float)turbo_srgb_bytes[i][1] / 255.f);
            colorMarks_.push_back((float)turbo_srgb_bytes[i][2] / 255.f);
            colorMarks_.push_back(s);
        }
        updatedBytes = true;
    }
    else if (ext_ == ".map")
        bytes = loadBinaryFile(getNameAndPath(), &updatedBytes);
    else if (ext_ == ".colors")
    {
        std::ifstream file(getNameAndPath());
        if (file.is_open())
        {
            colorMarks_.clear();
            float rVal = 0.f, gVal = 0.f, bVal = 0.f, sVal = 0.f;
            while (file >> rVal >> gVal >> bVal >> sVal)
            {
                colorMarks_.push_back(rVal);
                colorMarks_.push_back(gVal);
                colorMarks_.push_back(bVal);
                colorMarks_.push_back(sVal);
            }
            file.close();

            for (int i = 0; i < 256; ++i)
            {
                float s0 = 0.f, s1 = 0.f;
                float s = float(i) / 255.f;
                int leftMark = -1;
                int rightMark = -1;
                // find mark before and after s
                for (size_t j = 3; j < colorMarks_.size(); j += 4)
                {
                    if (colorMarks_[j] <= s)
                    {
                        s0 = colorMarks_[j];
                        leftMark = static_cast<int>(j) / 4;
                    }
                    else if (colorMarks_[j] > s)
                    {
                        s1 = colorMarks_[j];
                        rightMark = static_cast<int>(j) / 4;
                        break;
                    }
                }

                float r = 0.f, g = 0.f, b = 0.f;
                if (leftMark == -1)
                {
                    rightMark *= 4;
                    r = colorMarks_[rightMark];
                    g = colorMarks_[rightMark + 1];
                    b = colorMarks_[rightMark + 2];
                }
                else if (rightMark == -1)
                {
                    leftMark *= 4;
                    r = colorMarks_[leftMark];
                    g = colorMarks_[leftMark + 1];
                    b = colorMarks_[leftMark + 2];
                }
                else
                {
                    if (s1 == s0)
                        s = s0;
                    else
                        s = (s - s0) / (s1 - s0);

                    leftMark *= 4;
                    rightMark *= 4;

                    r = (1.f - s) * colorMarks_[leftMark] + s * colorMarks_[rightMark];
                    g = (1.f - s) * colorMarks_[leftMark + 1] + s * colorMarks_[rightMark + 1];
                    b = (1.f - s) * colorMarks_[leftMark + 2] + s * colorMarks_[rightMark + 2];
                }

                bytes.push_back(static_cast<char>(r * 255.f));
                bytes.push_back(static_cast<char>(g * 255.f));
                bytes.push_back(static_cast<char>(b * 255.f));
            }
            updatedBytes = true;
        }
    }
    else
    {
        std::cout << "Unable to load colormap: " << getNameAndPath() << std::endl;
        return false;
    }

    // create texture
    if (updatedBytes)
    {
        data_.clear();
        for (char c : bytes)
            data_.push_back((unsigned char)c);

        texture_ = Textures::create1DTexture(bytes);
    }

    return updatedBytes;
}

void ColorMap::update(bool fromcolorMarks_)
{
    if (fromcolorMarks_)
    {
        data_.clear();
        for (int i = 0; i < 256; ++i)
        {
            float s0 = 0.f, s1 = 0.f;
            float s = float(i) / 255.f;
            int leftMark = -1;
            int rightMark = -1;

            // find mark before and after s
            for (size_t j = 3; j < colorMarks_.size(); j += 4)
            {
                if (colorMarks_[j] <= s)
                {
                    s0 = colorMarks_[j];
                    leftMark = static_cast<int>(j) / 4;
                }
                else if (colorMarks_[j] > s)
                {
                    s1 = colorMarks_[j];
                    rightMark = static_cast<int>(j) / 4;
                    break;
                }
            }

            float r = 0.f, g = 0.f, b = 0.f;
            if (leftMark == -1)
            {
                rightMark *= 4;
                r = colorMarks_[rightMark];
                g = colorMarks_[rightMark + 1];
                b = colorMarks_[rightMark + 2];
            }
            else if (rightMark == -1)
            {
                leftMark *= 4;
                r = colorMarks_[leftMark];
                g = colorMarks_[leftMark + 1];
                b = colorMarks_[leftMark + 2];
            }
            else
            {
                if (s1 == s0)
                    s = s0;
                else
                    s = (s - s0) / (s1 - s0);

                leftMark *= 4;
                rightMark *= 4;

                r = (1.f - s) * colorMarks_[leftMark] + s * colorMarks_[rightMark];
                g = (1.f - s) * colorMarks_[leftMark + 1] + s * colorMarks_[rightMark + 1];
                b = (1.f - s) * colorMarks_[leftMark + 2] + s * colorMarks_[rightMark + 2];
            }

            data_.push_back((unsigned char)(r * 255.f));
            data_.push_back((unsigned char)(g * 255.f));
            data_.push_back((unsigned char)(b * 255.f));
        }
    }

    texture_ = Textures::create1DTexture(data_);
}

void ColorMap::sample(float t, unsigned char& r, unsigned char& g, unsigned char& b, bool linearBlend)
{
    if (data_.size() == 0)
        return;

    //clamp from 0 to 1
    t = t < 0.f ? 0.f : t > 1.f ? 1.f : t;

    // Handle edge cases
    if (t == 1.f)
    {
        r = data_[data_.size() - 3];
        g = data_[data_.size() - 2];
        b = data_[data_.size() - 1];
        return;
    }
    else if (t == 0.f)
    {
        r = data_[0];
        g = data_[1];
        b = data_[2];
        return;
    }

    // Get samples
    unsigned numColors = static_cast<unsigned>(data_.size()) / 3;
    unsigned lastIndex = numColors - 1;
    float c0 = lastIndex * t;
    unsigned s0 = (unsigned)c0;
    if (linearBlend)
    {
        unsigned s1 = (unsigned)(c0 + 1.f);

        float v0 = float(s0) / (float(lastIndex));
        float sRange = (float(s1) / (float(lastIndex)) - v0);
        t = (t - v0) / sRange;

        r = (unsigned char)(t * data_[s1 * 3] + (1.f - t) * data_[s0 * 3]);
        g = (unsigned char)(t * data_[s1 * 3 + 1] + (1.f - t) * data_[s0 * 3 + 1]);
        b = (unsigned char)(t * data_[s1 * 3 + 2] + (1.f - t) * data_[s0 * 3 + 2]);
    }
    else
    {
        r = data_[s0];
        g = data_[s0 + 1];
        b = data_[s0 + 2];
    }
}

GLuint ColorMap::getTextureID()
{
    ASSERT(initialized_, "Colormap not initialized");

    if (!tracking_)
        return texture_.id();

    return texture_.id();
}
