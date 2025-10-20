#include "Color.h"


Color::Color(unsigned char rVal, unsigned char gVal, unsigned char bVal, unsigned char aVal)
    : r(rVal), g(gVal), b(bVal), a(aVal)
{}

void Color::set(unsigned char rVal, unsigned char gVal, unsigned char bVal, unsigned char aVal)
{
    this->r = rVal;
    this->g = gVal;
    this->b = bVal;
    this->a = aVal;
}

void Color::set(Color color)
{
    this->r = color.r;
    this->g = color.g;
    this->b = color.b;
    this->a = color.a;
}

Vec4 Color::vec4(bool normalize) const 
{
    if (normalize)
        return Vec4{ r / 255.f, g / 255.f, b / 255.f, a / 255.f };
    else
        return Vec4{ (float)r, (float)g, (float)b, (float)a };
}
