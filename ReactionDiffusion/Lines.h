#pragma once
#include "Drawable.h"

class Lines 
    : public Drawable
{
public:
    Lines();
    
    void addLine(Vec3 start, Vec3 end, Vec3 startColor = Vec3(0.2f, 0.2f, 0.2f), Vec3 endColor = Vec3(1, 1, 1));
    size_t numLines() const;
    void finalize();
    const Vec3& getStart(unsigned index);
    const Vec3& getEnd(unsigned index);
};

