#include "Material.h"


Material::Material(Color color, float shininess, float ambient) 
	: color_(color), shininess_(shininess), ambient_(ambient)
{}

Vec4 Material::getNormalizedColors() const
{
	return Vec4(color_.r / 255.f, color_.g / 255.f, color_.b / 255.f, color_.a / 255.f);
}