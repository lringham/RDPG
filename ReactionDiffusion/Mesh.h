#pragma once
#include "Drawable.h"


class Mesh : 
	public Drawable
{
public:
	Mesh() = default;
	Mesh(int xRes, int yRes, float width, float height);
	virtual ~Mesh() = default;

	void subdivide(unsigned iterations = 1);
	void calculateNormals(bool useAngleWeights = true);
};