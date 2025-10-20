#pragma once


class Counter
{
public:
	Counter() = default;
	Counter(unsigned long long duration);
	
	void count();
	void setDuration(unsigned long long duration);
	void reset();
	bool elapsed() const;
	bool elapsedAndReset();
	bool countElapsedAndReset();
	unsigned long long getDuration() const;
	unsigned long long getCount() const;

private:
	unsigned long long counts_ = 0;
	unsigned long long duration_ = 0;
};

