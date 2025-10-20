#pragma once
#include <string>
#include <vector>
#include <map>


class CmdArgsParser
{
    std::map<std::string, std::string> options_;
    std::vector<std::string> flags_;
    std::map<std::string, std::tuple<std::string, std::string>> validArgs_;
    bool valid_ = true;
    bool initialized_ = false;

    void addDefaultArgs();
    void addArg(std::string name, std::string usageComment, std::string defaultValue);
    bool validateArg(const std::string& arg) const;

public:
    CmdArgsParser();
    CmdArgsParser(int argc, char** argv);

    bool parseArgs(int argc, char** argv);
    bool hasOption(const std::string& name) const;
    std::string getOption(const std::string& name) const;
    bool flagSet(const std::string& name) const;
    void usage() const;
};