#pragma once
#include "Graphics.h"
#include "GUI.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <string>
#include <functional>


class GUI;
class Window
{
public:
    Window() = default;
    Window(const std::string& title, int width = 1080, int height = 720, bool fullscreen = false);
    Window(const std::string& title, int width, int height, int x, int y);
    ~Window();

    void init(const std::string& title = "", int width = 1080, int height = 720, bool fullscreen = false);

    static void errorCallback(int error, const char* description);
    static void resizeCallback(GLFWwindow* window, int width, int height);

    void setTitle(const std::string& title) const;
    GUI* createGUI();
    void swapBuffers() const;
    void makeCurrent() const;
    bool shouldClose() const;
    void screenshot(const std::string& filename) const;
    void queryMonitors() const;
    void getCursorPos(int& x, int& y);
    void setPos(int x, int y);
    void getPos(int* x, int* y) const;
    void getHalfSize(int& width, int& height) const;
    int getWidth() const;
    int getHeight() const;
    void getSize(int& width, int& height) const;
    void setSize(int width, int height) const;
    float getAspectRatio() const;
    void pollEvents() const;
    void waitEvents(bool forceUpdate) const;
    void setShouldExit(bool shouldClose);
    void hide() const;
    void show() const;
    void setBackgroundColor(const Color& backgroundColor);
    
    GUI* getGUI() const;

    Graphics graphics_;
    std::function<void(int width, int height)> resizeCallback_ = nullptr;

private:
    static std::unordered_map<GLFWwindow*, Window*> windows_;
    static bool glLoaded_;
    GLFWwindow* window_ = nullptr;
    GUI* gui_ = nullptr;
};

void APIENTRY glDebugOutput(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam);
