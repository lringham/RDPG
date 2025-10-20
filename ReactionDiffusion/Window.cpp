#include "Window.h"
#include "EventHandler.h"
#include "Utils.h"
#include "LOG.h"

#include <iostream>


// #define DEBUG_GL
bool Window::glLoaded_ = false;
std::unordered_map<GLFWwindow*, Window*> Window::windows_;

Window::Window(const std::string& title, int width, int height, bool fullscreen)
{
    gui_ = nullptr;
    init(title, width, height, fullscreen);
}

Window::Window(const std::string& title, int width, int height, int x, int y)
{
    init(title, width, height);
    gui_ = nullptr;
    setPos(x, y);
}

Window::~Window()
{
    if (gui_ != nullptr)
        delete gui_;

    glfwDestroyWindow(window_);
    glfwTerminate();
}

void Window::errorCallback(int error, const char* description)
{
    std::cout << "GLFW ERROR " << error << ":" << std::endl;
    std::cout << description << std::endl;
}

void Window::resizeCallback(GLFWwindow* window, int width, int height)
{
    windows_[window]->graphics_.setViewport(0, 0, width, height);
    if(windows_[window]->resizeCallback_)
        windows_[window]->resizeCallback_(width, height);
}

void Window::swapBuffers() const
{
    glfwSwapBuffers(window_);
}

bool Window::shouldClose() const
{
    if (glfwWindowShouldClose(window_) == 1 && !gui_->exitProgram_)
    {
        gui_->showExitConf_ = true;
        gui_->setVisible(true);
    }

    return gui_->exitProgram_;
}

void Window::init(const std::string& title, int width, int height, bool fullscreen)
{
    glfwSetErrorCallback(errorCallback);

    // init glfw
    if (!glfwInit())
        exit(EXIT_FAILURE);

#ifdef _WIN32	    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    LOG("GL VER 4.6");
#elif __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    LOG("GL VER 4.1");
#elif __linux__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    LOG("GL VER 4.5");
#endif

    glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef DEBUG_GL
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    if (fullscreen)
        window_ = glfwCreateWindow(mode->width, mode->height, title.c_str(), glfwGetPrimaryMonitor(), nullptr);
    else
        window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!window_)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // make current
    makeCurrent();

    // set callbacks
    glfwSetWindowSizeCallback(window_, resizeCallback);

    // set refresh rate
    glfwSwapInterval(0);

    // focus window
    glfwFocusWindow(window_);

    if (!glLoaded_)
    {
        glLoaded_ = true;
        if (!gladLoadGL())
            std::cout << "Can't initialize glad" << std::endl;
    }

#ifdef DEBUG_GL
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
#endif

    glEnable(GL_MULTISAMPLE);
    createGUI();

    windows_[window_] = this;
    EventHandler::init(window_);
}

void Window::pollEvents() const
{
    glfwPollEvents();
}

void Window::waitEvents(bool forceUpdate) const
{
    if (forceUpdate)
        glfwPostEmptyEvent();
    glfwWaitEvents();
}

void Window::makeCurrent() const
{
    glfwMakeContextCurrent(window_);
}

GUI* Window::createGUI()
{
    assert(gui_ == nullptr);
    gui_ = new GUI(window_);
    return gui_;
}

void Window::queryMonitors() const
{
    int width, height;
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    glfwGetMonitorPhysicalSize(primaryMonitor, &width, &height);

    LOG("Monitor\t[" << glfwGetMonitorName(primaryMonitor) << "]\t(" << width << ", " << height << ", " << float(width) / float(height) << ") -- Primary");

    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    for (int i = 0; i < count; ++i)
    {
        GLFWmonitor* monitor = monitors[i];
        if (monitor != primaryMonitor)
        {
            glfwGetMonitorPhysicalSize(monitor, &width, &height);
            LOG("Monitor\t[" << glfwGetMonitorName(primaryMonitor) << "]\t(" << width << ", " << height << ", " << float(width) / float(height) << ")");
        }
    }
}

void Window::setTitle(const std::string& title) const
{
    glfwSetWindowTitle(window_, title.c_str());
}

void Window::getPos(int* x, int* y) const
{
    glfwGetWindowPos(window_, x, y);
}

void Window::setPos(int x, int y)
{
    glfwSetWindowPos(window_, x, y);
}

int Window::getWidth() const
{
    int width = 0;
    glfwGetWindowSize(window_, &width, NULL);
    return width;
}

int Window::getHeight() const
{
    int height = 0;
    glfwGetWindowSize(window_, NULL, &height);
    return height;
}

void Window::getSize(int& width, int& height) const
{
    glfwGetWindowSize(window_, &width, &height);
}

void Window::setSize(int width, int height) const
{
    glfwSetWindowSize(window_, width, height);
}

void Window::getHalfSize(int& width, int& height) const
{
    glfwGetWindowSize(window_, &width, &height);
    width = width / 2;
    height = height / 2;
}

float Window::getAspectRatio() const
{
    int width = 0, height = 0;
    getSize(width, height);
    return (float)width / (float)height;
}

void Window::getCursorPos(int& x, int& y)
{
    double xd, yd;
    glfwGetCursorPos(window_, &xd, &yd);
    x = static_cast<int>(xd);
    y = static_cast<int>(yd);
}

void Window::screenshot(const std::string& filename) const
{
    int width, height;
    getSize(width, height);
    const unsigned numOfComponents = 3; //RGB
    unsigned char* pixels = new unsigned char[width * height * numOfComponents];
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    unsigned char r, g, b;
    unsigned i, j;
    for (int y = 0; y < height / 2; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            //get top pixel
            i = y * width + x;
            i *= numOfComponents;

            //get bottom pixel
            j = (height - 1 - y) * width + x;
            j *= numOfComponents;

            //swap them
            r = pixels[i];
            g = pixels[i + 1];
            b = pixels[i + 2];

            pixels[i] = pixels[j];
            pixels[i + 1] = pixels[j + 1];
            pixels[i + 2] = pixels[j + 2];

            pixels[j] = r;
            pixels[j + 1] = g;
            pixels[j + 2] = b;
        }
    }

    Utils::saveImage(filename.c_str(), width, height, pixels, numOfComponents);

    delete[] pixels;
}

void Window::setShouldExit(bool shouldClose)
{
    glfwSetWindowShouldClose(window_, shouldClose ? 1 : 0);
}

void Window::hide() const
{
    glfwHideWindow(window_);
}

void Window::show() const
{
    glfwShowWindow(window_);
}

void Window::setBackgroundColor(const Color& backgroundColor) 
{
    if (glLoaded_)
        graphics_.setBackgroundColor(backgroundColor);
}

GUI* Window::getGUI() const
{
    return gui_;
}

#ifdef DEBUG_GL
void APIENTRY glDebugOutput(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204 || id == 131076 || id == 131186)
        return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}
#endif
