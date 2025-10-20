#pragma once


class EventListener
{
public:
	enum class Action
	{
		press,
		release,
		unknown
	};

	EventListener();
	virtual ~EventListener();
	virtual void notifyKeyEvent(int key, Action state, bool guiUsedEvent) = 0;
	virtual void notifyMouseMoveEvent(double x, double y) = 0;
	virtual void notifyMouseButtonEvent(int button, Action state, int mods) = 0;
	virtual void notifyMouseScrollEvent(double xOffset, double yOffset) = 0;
	virtual void notifyMouseEntered(bool hasEntered) = 0;
};