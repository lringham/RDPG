#pragma once
#include "EventListener.h"
#include "GUI.h"

#include <vector>
#include <GLFW/glfw3.h>


class EventHandler
{
public:	
	static void init(GLFWwindow* window);
	static void subscribe(EventListener* listener);
	static void unsubscribe(EventListener* listener);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseMoveCallback(GLFWwindow* window, double x, double y);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
	static void cursorEnterCallback(GLFWwindow* window, int entered);
	static void monitorCallback(GLFWmonitor* monitor, int event);
	static void charCallback(GLFWwindow* window, unsigned int c);


	static EventListener::Action modToAction(int mod);
	static void setGUI(GUI* gui);

private:
	static std::vector<EventListener*> listeners_;
	static GUI* gui_;
};

