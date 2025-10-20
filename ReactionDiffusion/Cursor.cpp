#include "Cursor.h"
#include "ColorShader.h"
#include "Utils.h"


void Cursor::init()
{
    // Make a circle
    std::vector<Vec3> points;
    for (float i = 0.f; i < 360.f; ++i)
        points.push_back(Vec3(cosf(Math::degToRad(i)), sinf(Math::degToRad(i)), 0.f));
    
    // Create graphics objects
    create(points);

    shader_ = std::make_unique<ColorShader>();
    transform_.setScale(.025f);
    visible_ = false;
}