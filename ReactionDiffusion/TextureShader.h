#pragma once
#include "Shader.h"
#include "Camera.h"

class TextureShader :
    public Shader
{
public:
    TextureShader(Camera* cam = nullptr, unsigned textureID = 0);

    void setProjUniform(float projMat[]);
    void setViewModelUniform(float viewModelMat[]);
    void startRender(const Drawable* drawable) override;
    void endRender(const Drawable* drawable) override;
    
    unsigned textureID_ = 0;
};
