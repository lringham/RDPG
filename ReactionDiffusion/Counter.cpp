#include "Counter.h"


Counter::Counter(unsigned long long duration) : 
	counts_(0), duration_(duration)
{}

void Counter::setDuration(unsigned long long newDuration)
{
	this->duration_ = newDuration;
	counts_ = 0;
}

void Counter::reset()
{
	counts_ = 0;
}

void Counter::count()
{
	counts_++;
}

bool Counter::elapsed() const
{
	return counts_ >= duration_;
}

bool Counter::elapsedAndReset()
{
	if (elapsed())
	{
		counts_ = 0;
		return true;
	}
	return false;
}

bool Counter::countElapsedAndReset()
{
	if (elapsed())
	{
		counts_ = 0;
		return true;
	}
	else
	{
		counts_++;
		return false;
	}
}

unsigned long long Counter::getDuration() const
{
	return duration_;
}

unsigned long long Counter::getCount() const
{
	return counts_;
}