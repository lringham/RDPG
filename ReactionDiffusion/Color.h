#pragma once
#include "Vec4.h"


class Color
{
public:
    Color(unsigned char rVal = 255, unsigned char gVal = 255, unsigned char bVal = 255, unsigned char aVal = 255);
    void set(unsigned char rVal, unsigned char gVal, unsigned char bVal, unsigned char aVal = 255);
	void set(Color color);
	Vec4 vec4(bool normalize = true) const;

	unsigned char r = 0, g = 0, b = 0, a = 0;
};

