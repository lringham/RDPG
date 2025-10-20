#include "EventHandler.h"
#include "imgui_impl_glfw.h"

std::vector<EventListener*> EventHandler::listeners_ = std::vector<EventListener*>();
GUI* EventHandler::gui_ = nullptr;

void EventHandler::init(GLFWwindow* window)
{
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetCursorEnterCallback(window, cursorEnterCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetMonitorCallback(monitorCallback);
}

void EventHandler::subscribe(EventListener* listener)
{
    if (std::find(listeners_.begin(), listeners_.end(), listener) == listeners_.end())
        listeners_.push_back(listener);
}

void EventHandler::unsubscribe(EventListener* listener)
{
    auto loc = std::find(listeners_.begin(), listeners_.end(), listener);
    if (loc == listeners_.end())
        listeners_.erase(loc);
}

EventListener::Action EventHandler::modToAction(int mod)
{
    EventListener::Action state;
    switch (mod)
    {
    case GLFW_PRESS:
        state = EventListener::Action::press;
        break;
    case GLFW_RELEASE:
        state = EventListener::Action::release;
        break;
    default:
        state = EventListener::Action::unknown;
        break;
    }
    return state;
}

void EventHandler::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    //determine action
    EventListener::Action state = modToAction(action);

    //dispatch state information
    for (size_t i = 0; i < listeners_.size(); ++i)
        listeners_[i]->notifyKeyEvent(key, state, gui_->hasUsedEvent());
}

void EventHandler::mouseMoveCallback(GLFWwindow* window, double x, double y)
{
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);
    if (gui_ != nullptr && gui_->hasUsedEvent())
        return;

    for (size_t i = 0; i < listeners_.size(); ++i)
        listeners_[i]->notifyMouseMoveEvent(x, y);
}

void EventHandler::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    if (gui_ != nullptr && gui_->hasUsedEvent())
        return;

    for (size_t i = 0; i < listeners_.size(); ++i)
        listeners_[i]->notifyMouseButtonEvent(button, modToAction(action), mods);
}

void EventHandler::mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);

    if (gui_ != nullptr && gui_->hasUsedEvent())
        return;

    for (size_t i = 0; i < listeners_.size(); ++i)
        listeners_[i]->notifyMouseScrollEvent(xOffset, yOffset);
}

void EventHandler::cursorEnterCallback(GLFWwindow* window, int hasEntered)
{
    ImGui_ImplGlfw_CursorEnterCallback(window, hasEntered);
    for (size_t i = 0; i < listeners_.size(); ++i)
        listeners_[i]->notifyMouseEntered(hasEntered);
}

void EventHandler::monitorCallback(GLFWmonitor* monitor, int event)
{
    ImGui_ImplGlfw_MonitorCallback(monitor, event);
}

void EventHandler::charCallback(GLFWwindow* window, unsigned int c)
{
    ImGui_ImplGlfw_CharCallback(window, c);
}

void EventHandler::setGUI(GUI* gui)
{
    EventHandler::gui_ = gui;
}