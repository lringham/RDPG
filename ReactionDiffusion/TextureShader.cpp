#include "TextureShader.h"


TextureShader::TextureShader(Camera* cam, unsigned textureID) :
    textureID_(textureID)
{
    addShaderStage(GL_VERTEX_SHADER,
        R"***(#version 410
// ins
layout(location = 0) in vec3 vertexPosition;
layout(location = 2) in vec2 uvs;

// outs
out vec2 fragUVs;

// uniforms
uniform mat4 proj;
uniform mat4 viewModel;

void main()
{
	gl_Position = proj * viewModel * vec4(vertexPosition, 1.0);
    fragUVs = uvs;
})***");


    addShaderStage(GL_FRAGMENT_SHADER,
        R"***(#version 410
// ins
in vec2 fragUVs;

// outs
out vec4 FragmentColor;

// Uniforms
uniform sampler2D ourTexture;

void main(void)
{
	FragmentColor = texture(ourTexture, fragUVs);
}
)***");

    createShaderProgram();
    this->cam = cam;
}

void TextureShader::startRender(const Drawable* drawable)
{
    glUseProgram(program);
    //glBindTexture(GL_TEXTURE_2D, textureID_);
    if (cam != nullptr)
    {
        setProjUniform(cam->getProjectionData());
        setViewModelUniform((cam->getViewMatrix().elements * drawable->transform.getTransformMat()).elements);
    }
}

void TextureShader::endRender(const Drawable*)
{
   // glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void TextureShader::setProjUniform(float projMat[])
{
    glUniformMatrix4fv(getUniformLoc("proj"), 1, GL_TRUE, projMat);
}

void TextureShader::setViewModelUniform(float viewModelMat[])
{
    glUniformMatrix4fv(getUniformLoc("viewModel"), 1, GL_TRUE, viewModelMat);
}