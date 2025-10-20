#include "LineLoop.h"
#include "Vec3.h"


LineLoop::LineLoop()
{
	drawMode_ = GL_LINE_LOOP;
}

LineLoop::LineLoop(const std::vector<Vec3>& points)
{
	drawMode_ = GL_LINE_LOOP;
	create(points);
}

void LineLoop::create(const std::vector<Vec3>& points)
{
	for (unsigned i = 0; i < points.size(); ++i)
	{
		positions_.push_back(points[i]);
		indices_.push_back(i);
		colors_.push_back(Vec4(1.f, 0.f, 0.f, 0.f));
		normals_.push_back(Vec3(0.f, 0.f, 1.f));
		textureCoords_.push_back(Vec2(
			(float) i / (float) (points.size() - 1.f), 
			0.f));
	}
	initVBOs();
}