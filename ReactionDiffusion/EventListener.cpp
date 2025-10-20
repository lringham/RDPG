#include "EventListener.h"
#include "EventHandler.h"

EventListener::EventListener()
{
	EventHandler::subscribe(this);
}

EventListener::~EventListener()
{
	EventHandler::unsubscribe(this);
}