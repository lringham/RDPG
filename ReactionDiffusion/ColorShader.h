#pragma once
#include "Shader.h"


class ColorShader :
    public Shader
{
public:
    ColorShader();
    ~ColorShader();

    void startRender(const Drawable* drawable, const Camera& camera) override;
    void endRender() override;
    void setProjUniform(const float projMat[]);
    void setViewModelUniform(float viewModelMat[]);
    void setColor(float r, float g, float b);
};

