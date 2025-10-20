#include "ColorShader.h"


ColorShader::ColorShader()
{
    addShaderStage(GL_VERTEX_SHADER,
        R"***(#version 410
// ins
layout(location = 0) in vec3 vertexPosition;

// uniforms
uniform mat4 proj;
uniform mat4 viewModel;

void main()
{
	gl_Position = proj * viewModel * vec4(vertexPosition, 1.0);
})***");


    addShaderStage(GL_FRAGMENT_SHADER,
        R"***(#version 410
// outs
out vec4 FragmentColor;

// Uniforms
uniform vec4 color;

void main(void)
{
	FragmentColor = color;
}
)***");

    createShaderProgram();
}


ColorShader::~ColorShader()
{}

void ColorShader::startRender(const Drawable* drawable, const Camera& camera)
{
    glUseProgram(program);
    setProjUniform(camera.getProjectionData());
    setViewModelUniform((camera.getViewMatrix() * drawable->transform_.getTransformMat()).elements);
    Vec4 color = drawable->material_.getNormalizedColors();
    setColor(color.x, color.y, color.z);
}

void ColorShader::endRender()
{
    glBindTexture(GL_TEXTURE_1D, 0);
    glUseProgram(0);
}

void ColorShader::setProjUniform(const float projMat[])
{
    glUniformMatrix4fv(getUniformLoc("proj"), 1, GL_TRUE, projMat);
}

void ColorShader::setViewModelUniform(float viewModelMat[])
{
    glUniformMatrix4fv(getUniformLoc("viewModel"), 1, GL_TRUE, viewModelMat);
}

void ColorShader::setColor(float r, float g, float b)
{
    glUniform4f(getUniformLoc("color"), r, g, b, 1.f);
}