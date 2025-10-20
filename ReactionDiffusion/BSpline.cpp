#include "BSpline.h"
#include "ColorShader.h"
#include "Triangle.h"

#include <cassert>


BSpline::BSpline(const std::vector<Vec3>& controlPoints, size_t degree, bool clamped) :
	degree_(degree), clamped_(clamped)
{
	setControlPoints(controlPoints);
}

Vec3 BSpline::sample(float u) const
{	
	return bSpline(u, controlPoints_, knots_, clamped_); 
}

void BSpline::setControlPoints(const std::vector<Vec3>& points)
{
	controlPoints_ = points;
	knots_ = createKnots(degree_, controlPoints_.size(), clamped_);
}

Vec3 bSpline(float u, const std::vector<Vec3>& controlPoints, const std::vector<float>& knots, bool clamped)
{
	size_t degree = knots.size() - controlPoints.size() - 1;

	// FIXME: cannot use u == 1.f with a clamped curve. It causes the last point to be zero
	// because of "u < knots.at(i + 1))" in basis(). And that can't be changed because it
	// causes weird instability (spikes) at curve knots.
	if (clamped)
	{
		if (u <= 0.f)
			return controlPoints[0];
		else if (u >= 1.f) // Can be removed if other FIXMEs are resolved??
			return controlPoints[controlPoints.size() - 1];
	}
	else // if not clamped we assume open
	{
		// if open (standard knot sequence) the beginning and end bases  
		// do not sum to 1 thus, only the middle subset of knot values are valid
		float offset = 0.00001f; 
		if (u < knots[degree])
			u = knots[degree] + offset;
		else if (u > knots[controlPoints.size()])
			u = knots[controlPoints.size()] - offset;
	}

	Vec3 samplePoint;
	for (size_t i = 0; i < controlPoints.size(); ++i)
		samplePoint = samplePoint + controlPoints[i] * basis(static_cast<int>(i), degree, u, knots);

	return samplePoint;
}

float basis(size_t i, size_t degree, float u, const std::vector<float>& knots)
{
	float value = 0.f;
	if (degree == 0)
	{
		// FIXME: Some literature uses u < knots.at(i + 1) but that seems to lead 
		// to a {0,0,0} sample for u = 1.0. Using "u <= knots.at(i + 1)" fixes this 
		// however, that leads to spikes at curve knots. I believe these spikes
		// are caused by overlapping of basis funcs at knot points
		if (knots.at(i) <= u && u < knots.at(i + 1)) 
			value = 1.f;
	}
	else
	{
		float coef1 = knots.at(i + degree) - knots.at(i);
		if (coef1 != 0.f)
		{ 
			coef1 = (u - knots.at(i)) / coef1;
			value = coef1 * basis(i, degree - 1, u, knots);
		}

		float coef2 = knots.at(i + degree + 1) - knots.at(i + 1);
		if (coef2 != 0.f)
		{
			coef2 = (knots.at(i + degree + 1) - u) / coef2;
			value += coef2 * basis(i + 1, degree - 1, u, knots);
		}
	}
	return value;
}

std::vector<float> createKnots(size_t degree, size_t numControlPoints, bool clamped)
{
	size_t numKnots = numControlPoints + degree + 1;
	std::vector<float> knots;

	if (clamped) // Update knots using the standard knot sequence
	{
		size_t numFreeKnots = numKnots - (degree + 1) * 2;
		for (size_t i = 0; i <= degree; ++i)
			knots.push_back(0.f);
		for (size_t i = 1; i <= numFreeKnots; ++i)
			knots.push_back((float)i / (float)(numFreeKnots + 1));
		for (size_t i = 0; i <= degree; ++i)
			knots.push_back(1.f);
	}
	else // Update knots using the uniform knot sequence (open)
	{
		knots.resize(numKnots, 0.f);
		for (size_t i = 0; i < numKnots; i++)
			knots[i] = (float)i / (float)(numKnots - 1.f);
	}
	return knots;
}