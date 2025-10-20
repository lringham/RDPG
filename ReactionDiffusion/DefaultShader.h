#pragma once
#include "Shader.h"


class DefaultShader : 
    public Shader
{
public:
    DefaultShader();

    void setProjUniform(const float projMat[]);
    void setViewModelUniform(const float viewModelMat[]);
    void startRender(const Drawable* drawable, const Camera& camera) override;
    void endRender() override;
};

