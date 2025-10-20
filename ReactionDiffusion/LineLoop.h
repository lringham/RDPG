#pragma once
#include "Drawable.h"
#include "Vec3.h"

#include <vector>


class LineLoop :
	public Drawable
{
public:
	LineLoop();
	LineLoop(const std::vector<Vec3>& points);
	
	void create(const std::vector<Vec3>& points);
};

