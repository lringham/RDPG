#pragma once
#include "Color.h"
#include "Vec4.h"


class Material
{
public:
	Material(Color color = Color(255, 255, 255), float shininess = .5f, float ambient = .1f);
	Vec4 getNormalizedColors() const;

	Color color_;
	float shininess_;
	float ambient_;
};

