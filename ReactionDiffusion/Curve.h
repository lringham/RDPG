#pragma once
#include "Vec3.h"
#include "Drawable.h"
#include "ColorShader.h"
#include "Triangle.h"

#include <vector>


class Curve 
{
protected:
	std::vector<Vec3> controlPoints_;
	
public:
	Curve() = default;

	Curve(const std::vector<Vec3>& controlPoints)
	{
		setControlPoints(controlPoints);
	}
	
	virtual ~Curve() = default;

	virtual void initDrawable(Drawable& drawable)
	{
		// reset curve geometry
		drawable.destroyVBOs();
		drawable.clearBuffers();
		drawable.drawMode_ = GL_LINE_STRIP;
		for (auto* c : drawable.childern_)
			delete c;
		drawable.childern_.clear();

		// create control point geometry
		for (auto& p : controlPoints_)
		{
			float triSize = .1f;
			drawable.childern_.push_back(new Triangle(
				p + Vec3{ -triSize, -triSize, -0.001f },
				p + Vec3{ 0, 0, 0.f },
				p + Vec3{ triSize, -triSize, -0.001f })
			);
			drawable.childern_.back()->material_.color_.set(255, 0, 0);
		}

		// sample the curve
		int numSamples = 1000;
		for (int i = 0; i <= numSamples; ++i)
		{
			float u = (float)i / (float)numSamples;
			drawable.positions_.push_back(sample(u));
			drawable.indices_.push_back(i);
			drawable.colors_.push_back(Vec4(1.f, 0.f, 0.f, 0.f));
			drawable.normals_.push_back(Vec3(0.f, 0.f, 1.f));
			drawable.textureCoords_.push_back(Vec2(u, 0.f));
		}

		drawable.initVBOs();

		if(!drawable.shader_)
			drawable.shader_ = std::make_unique<ColorShader>();
	}

	virtual void setControlPoints(const std::vector<Vec3>& controlPoints) 
	{
		controlPoints_ = controlPoints;
	}

	virtual std::vector<Vec3> controlPoints() const
	{
		return controlPoints_;
	}

	virtual void setControlPoint(size_t i, const Vec3& v)
	{
		controlPoints_.at(i) = v;
	}

	virtual Vec3 getControlPoint(size_t i) const
	{
		return controlPoints_.at(i);
	}

	virtual Vec3 sample(float u) const = 0;
};