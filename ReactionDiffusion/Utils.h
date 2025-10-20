#pragma once
#include "Vec3.h"
#include "Image.h"

#ifdef _WIN32	            
    #include <filesystem>
#endif
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <math.h>
#include <regex> 
#include <iterator> 
#include <functional> 


std::string loadTextFile(const std::string& filename);
std::vector<char> loadBinaryFile(const std::string& filename, bool* loaded = nullptr);
namespace Math
{
    static const float PI = 3.14159266f;
    static const float PI2 = 6.28318531f;
    static const float degToRadFract = PI2 / 360.f;
    static const float radToDegFract = 360.f / PI2;

    static constexpr inline float degToRad(float deg)
    {
        return (6.28318531f / 360.f) * deg;
    }

    static constexpr inline float radToDeg(float rad)
    {
        return (360.f / 6.28318531f) * rad;
    }

    static inline Vec3 sphericalToCartesian(float theta, float phi, float radius)
    {
        float sinPhi = sin(phi);
        return radius * Vec3(cos(theta) * sinPhi, cos(phi), sin(theta) * sinPhi);
    }
}

namespace Utils
{
    static const char* BAT_FILENAME = "createDLL.bat";
    static const char* ws = " \t\n\r\f\v";

#ifdef _WIN32	            
    inline bool doesFileExist(const std::string& filename)
    {
        return std::filesystem::exists(filename);
#elif __APPLE__
    inline bool doesFileExist(const std::string&)
    {
        std::ifstream file(filename.c_str());
        bool fileExists = !file.fail();
        if (fileExists) 
            file.close();
        return fileExists;
#elif __linux__
    inline bool doesFileExist(const std::string&)
    {
        std::ifstream file(filename.c_str());
        bool fileExists = !file.fail();
        if (fileExists)
            file.close();
        return fileExists;
#endif
    }

    inline void createBatFile(std::string filename = BAT_FILENAME)
    {
        // std::ifstream vsInfo("vs_info.txt");
        //if (vsInfo.is_open())
        //{
        //    std::string line = "";
        //    while (std::getline(vsInfo, line)) 
        //    {
        //        std::string heading = "installationPath: ";
        //        auto pos = line.rfind(heading);
        //        if (pos == std::string::npos)
        //        {
        //            std::cout << line.substr(pos, line.length() - heading.size()) << "\n";
        //        }
        //    }

        //    vsInfo.close();
        //}

        std::ofstream batFile(filename);
        if (batFile.is_open())
        {
            batFile << R"(
@ECHO =========================== Compiling PDEs ===========================
@ECHO off

SET simFileSource=PDES

CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
cl.exe %simFileSource%.cpp /EHa /LDd /MD /link /dll

DEL %simFileSource%.obj
DEL %simFileSource%.exp
DEL %simFileSource%.cpp
DEL %simFileSource%.lib

@ECHO on
@ECHO =========================== Compiling Finished! ======================= 
)";
            batFile.close();
        }
    }

    inline size_t findAndReplace(std::string& str, std::string toFind, std::string replaceWith)
    {
        // Find and replace all occurrences of toFind with replaceWith in str
        size_t pos = 0, offset = 0;
        while ((pos = str.find(toFind, offset)) != std::string::npos)
        {
            str.replace(pos, toFind.size(), replaceWith);
            offset = pos + replaceWith.size(); // skip replaceWith contents
        }
        return pos;
    }

    inline std::string mkdirSystemCommand(const std::string& command)
    {	
#ifdef _WIN32	    
        std::string commandStr = command;
        Utils::findAndReplace(commandStr, "/", "\\");
        return "mkdir " + commandStr;
#else
	std::string commandStr = command;        
	Utils::findAndReplace(commandStr, "\\", "/");
        return "mkdir -p " + commandStr;
#endif
    }

    inline std::string copySystemCommand(const std::string& command)
    {
#ifdef _WIN32	    
        std::string commandStr = command;
        Utils::findAndReplace(commandStr, "/", "\\");
        return "copy " + commandStr;
#else        
	std::string commandStr = command;        
	Utils::findAndReplace(commandStr, "\\", "/");
        return "cp -r " + commandStr;
#endif
    }

    // trim from end of string (right)
    inline std::string& rtrim(std::string& s, const char* t = ws)
    {
        s.erase(s.find_last_not_of(t) + 1);
        return s;
    }

    // trim from beginning of string (left)
    inline std::string& ltrim(std::string& s, const char* t = ws)
    {
        s.erase(0, s.find_first_not_of(t));
        return s;
    }

    // trim from both ends of string (left & right)
    inline std::string& trim(std::string& s, const char* t = ws)
    {
        return ltrim(rtrim(s, t), t);
    }

    inline std::string sToUpper(const std::string& s)
    {
        std::string upperS = s;
        for (unsigned i = 0; i < s.size(); ++i)
            upperS[i] = (char)toupper(s[i]);
        return upperS;
    }

    inline std::string sToLower(const std::string& s)
    {
        std::string lowerS = s;
        for (unsigned i = 0; i < s.size(); ++i)
            lowerS[i] = (char)tolower(s[i]);
        return lowerS;
    }

    inline bool sToBool(const std::string& s)
    {
        return sToLower(s) == "true";
    }

    inline std::string join(std::vector<std::string>& strArray, const std::string& joint)
    {
        std::string res;
        res += strArray[0];
        for (size_t i = 1; i < strArray.size(); ++i)
            res += joint + strArray[i];
        return res;
    }

    inline uint64_t join(uint32_t a, uint32_t b)
    {
        return ((uint64_t)a << 32) | b;
    }

    template<typename T>
    T clamp(T val, T lower, T upper)
    {
        return (val < lower ? lower : val > upper ? upper : val);
    }

    template<typename T>
    T lerp(T low, T high, T s)
    {
        return low * (1.f - s) + high * s;
    }

    template<typename T>
    T smoothStep(T v0, T v1, float t)
    {
        t = clamp(t, 0.f, 1.f);
        t = (3.0f - 2.0f * t) * t * t;
        return (1.f - t) * v0 + t * v1;
    }

    template<typename T>
    T min(T t0, T t1)
    {
        return t0 < t1 ? t0 : t1;
    }

    template<typename T>
    T max(T t0, T t1)
    {
        return t0 > t1 ? t0 : t1;
    }

    // intervalBisect searches between "low" and "high" looking for
    // a value within "eps" difference between "target" and "func(mid)"
    // where "mid" is the bisection of "low" and "high". When "eps" is reached 
    // or "maxIterations" is exceeded, the "mid" value is returned.
    template<typename T>
    T intervalBisect(std::function<T(T)> func, T target, T eps = 0.000000001f, size_t maxIterations = 20, T low = 0.f, T high = 1.f)
    {
        T mid = 0.f;
        T newTarget = target;
        while (maxIterations--)
        {
            mid = high / 2.f + low / 2.f;
            newTarget = func(mid);

            if (std::abs(newTarget - target) <= eps)
                break;
            else if (newTarget > target)
                high = mid;
            else if (newTarget < target)
                low = mid;
        }
        return mid;
    }

    inline std::string stripNulls(const std::string& str)
    {
        std::string stripped = "";
        for (char c : str)
            if (c != '\0')
                stripped += c;
        return stripped;
    }

    inline std::string replaceBetween(std::string str, std::string left, std::string right, std::string replaceWith)
    {
        std::regex re("(" + left + ")(.*)(" + right + ")");
        return std::regex_replace(str, re, left + replaceWith + right);
    }

    inline std::string getBetween(std::string str, std::string left, std::string right)
    {
        std::regex regexp(".*" + left + "(.*)" + right + ".*");
        std::smatch match;

        if (std::regex_search(str, match, regexp))
            return match[1];
        return "";
    }

    inline bool contains(std::string str, std::string substr)
    {
        return str.find(substr) != std::string::npos;
    }

    inline std::vector<std::string> split(const std::string& str, const std::string& delimiter)
    {
        std::vector<std::string> result;
        size_t prev = 0, off = str.find(delimiter);

        if (str == delimiter)
            result.push_back("");
        else if (off == std::string::npos)
            result.push_back(str);
        else
        {
            do
            {
                result.push_back(str.substr(prev, off - prev));
                prev = off + delimiter.size();
                off = str.find(delimiter, prev);
            } while (off != std::string::npos);
            result.push_back(str.substr(prev));
        }
        return result;
    }

    void saveImage(const std::string& filename, int width, int height, unsigned char* pixels, int numColorComponents = 3);
    void loadImage(const std::string& filename, int* width, int* height, unsigned char** pixels, int* numColorComponents);
    Image loadImage(const std::string& filename);
}
