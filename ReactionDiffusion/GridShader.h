#pragma once
#include "Shader.h"


class GridShader : 
    public Shader
{
public:
    GridShader();

    void setProjUniform(const float projMat[]);
    void setViewModelUniform(const float viewModelMat[]);
    void startRender(const Drawable* drawable, const Camera& camera) override;
    void endRender() override;
};

