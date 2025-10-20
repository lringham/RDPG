#pragma once
#include <string>


class App
{
protected:
    virtual int shutdown() = 0;

    int errorCode_ = 0;
    bool initialized_ = false;
    std::string appName_ = "App";

public:
    App() = default;
    virtual ~App() = default;
    virtual void init(int argc, char** argv, const std::string& name) = 0;
    virtual int run() = 0;

    virtual int load(const std::string& filename) = 0;
    virtual void pause(bool paused) = 0;
};
