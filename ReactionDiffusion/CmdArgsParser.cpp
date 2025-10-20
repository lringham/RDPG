#include "CmdArgsParser.h"
#include "ResourceLocations.h"
#include "LOG.h"
#include "Utils.h"

#include <iostream>
#include <filesystem>


CmdArgsParser::CmdArgsParser() 
{
	addDefaultArgs();
}

CmdArgsParser::CmdArgsParser(int argc, char** argv)
{
	addDefaultArgs();
    parseArgs(argc, argv);
}

bool CmdArgsParser::parseArgs(int argc, char** argv)
{
    ASSERT(!initialized_, "Already initialized!");

    ResourceLocations::instance().exePath(std::string(argv[0]));
    ResourceLocations::instance().launchPath(std::filesystem::current_path().u8string());

    size_t pos = 0;
    options_["configfile"] = "";
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((pos = arg.find("=")) != std::string::npos) // Option has a value
        {
            std::string heading = Utils::sToLower(arg.substr(0, pos));
            std::string value = arg.substr(pos + 1);
            if (validateArg(heading))
                options_[heading] = value;
        }
        else if (validateArg(arg)) // Option is a flag
            flags_.push_back(Utils::sToLower(arg));
        else // Invalid arg
        {
            std::cout << "Invalid command line parameter: " << arg << "\n";
            valid_ = false;
            usage();
            return valid_;
        }
    }
    initialized_ = true;
    return valid_;
}

void CmdArgsParser::addDefaultArgs()
{
    addArg("ColorMapsPath", "", "./");
    addArg("ConfigFile", "", "./");
    addArg("ModelsPath", "", "./");
    addArg("SavePath", "", "./");
    addArg("ShadersPath", "path to where shaders are stored", "./");
    addArg("ColorMap", "", "color map file which translates concentrations to colors");
    addArg("SimFile", "starting morphogen concentration from file", "");
    addArg("rdModel", "reaction-diffusion PDEs", "");
    addArg("SaveOnExit", "save model on exit", "false");
    addArg("Steps", "sim Steps until exit", "0");
    addArg("Run", "start the sim running", "false");
    addArg("Fullscreen", "start in fullscreen", "false");
    addArg("winWidth", "Window width", "1080");
    addArg("winHeight", "Window height", "720");
    addArg("FrameOutputFreq", "screenshot frequency", "0");
    addArg("Threads", "Number of threads to use when using the CPU for simulation", "0");
    addArg("SimDevice", "CPU or GPU", "GPU");
    addArg("HideGUI", "Hide the GUI by default", "false");
    addArg("DisableRendering", "Hide the window and disable rendering the simulation", "false");
    addArg("ScreenShot", "Take a screenshot at exit", "false");
}

void CmdArgsParser::addArg(std::string name, std::string usageComment, std::string defaultVal)
{
    validArgs_[Utils::sToLower(name)] = std::make_tuple(usageComment, defaultVal);
}

bool CmdArgsParser::hasOption(const std::string& name) const
{
    return options_.find(Utils::sToLower(name)) != options_.end();
}

std::string CmdArgsParser::getOption(const std::string& name) const
{
    std::string optName = Utils::sToLower(name);
    if (hasOption(optName))
        return options_.at(optName);
    else
        return std::get<1>(validArgs_.at(optName));
}

bool CmdArgsParser::flagSet(const std::string& name) const
{
    std::string flagName = Utils::sToLower(name);
    return std::count(flags_.begin(), flags_.end(), flagName) > 0;
}

bool CmdArgsParser::validateArg(const std::string& argName) const
{
    std::string arg = Utils::sToLower(argName);
    return validArgs_.find(arg) != validArgs_.end();
}

void CmdArgsParser::usage() const
{
    std::cout << "Usage: ReactionDiffusion.exe [flags] [heading=value]\nValid inputs are:\n";
    for (auto& pair : validArgs_)
        std::cout << "  " << pair.first // name
        << ": " << std::get<0>(pair.second) // desc
        << " (default=" << std::get<1>(pair.second) << ")\n"; // default val
}