#include "GridShader.h"
#include "Utils.h"
#include "ResourceLocations.h"


GridShader::GridShader()
{
    textures["colorMapOutside"] = 0;
    textures["colorMapInside"] = 0;
    textures["morphogensTexture"] = 0;
    addShaderStage(GL_VERTEX_SHADER, R"(
#version 410

// ins
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexture;
layout(location = 3) in vec4 vertexColor;

// outs
out vec3 fragNorm;
out vec4 fragColor;
out vec2 fragUV;

// uniforms
uniform mat4 proj;
uniform mat4 viewModel;

void main()
{
	gl_Position = proj * viewModel * vec4(vertexPosition, 1.0);
	fragColor = vertexColor;
	fragUV = vertexTexture;
	fragNorm = (viewModel * vec4(vertexNormal, 0.f)).xyz;
}
)");

    addShaderStage(GL_FRAGMENT_SHADER, R"(
#version 410

// ins
in vec4 fragColor;
in vec3 fragNorm;
in vec2 fragUV;

// outs
out vec4 FragmentColor;

// Uniforms
uniform sampler2D morphogensTexture;

uniform sampler1D colorMapOutside;
uniform sampler1D colorMapInside;
uniform float normCoef;
uniform int disableColormap;
uniform int disableShading;
uniform int selectingCells;

void main(void)
{
	vec4 color;
	vec3 N = normalize(fragNorm);
	vec3 L = vec3(0, 0, 1);
	float NdotL = dot(N, L);

	vec4 morphs = texture(morphogensTexture, fragUV);

	// "inside" fragment
	if(NdotL < 0) 
	{
		if(selectingCells == 1)
			color = texture(colorMapInside, morphs.r / normCoef) * morphs.g;
		else if(disableColormap == 0)
			color = texture(colorMapInside, morphs.r / normCoef);		
		else
			color = fragColor / normCoef;
	}
	// "outside" fragment
	else 
	{
		if(selectingCells == 1)
			color = texture(colorMapOutside, morphs.r / normCoef) * morphs.g;
		else if(disableColormap == 0)
			color = texture(colorMapOutside, morphs.r / normCoef);		
		else
			color = fragColor / normCoef;
	}

	// Apply simple diffuse if shading enabled
	if(disableShading == 0)
		color = color * .1f + color * abs(NdotL) * .9f;	

	FragmentColor = color;	
})");

    createShaderProgram();

    glUseProgram(program);

    glUniform1f(getUniformLoc("normCoef"), 1.f);
    glUseProgram(0);
}

void GridShader::startRender(const Drawable* drawable, const Camera& camera)
{
    glUseProgram(program);

    glUniform1i(getUniformLoc("colorMapOutside"), 0);
    glUniform1i(getUniformLoc("colorMapInside"), 1);
    glUniform1i(getUniformLoc("morphogensTexture"), 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, textures["colorMapOutside"]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, textures["colorMapInside"]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textures["morphogensTexture"]);

    glUniform1i(getUniformLoc("disableColormap"), disableColormap);
    glUniform1i(getUniformLoc("disableShading"), disableShading);
    glUniform1i(getUniformLoc("selectingCells"), selectingCells);


    setProjUniform(camera.getProjectionData());
    setViewModelUniform((camera.getViewMatrix() * drawable->transform_.getTransformMat()).elements);

}

void GridShader::endRender()
{
    glUseProgram(0);
}

void GridShader::setProjUniform(const float projMat[])
{
    glUniformMatrix4fv(getUniformLoc("proj"), 1, GL_TRUE, projMat);
}

void GridShader::setViewModelUniform(const float viewModelMat[])
{
    glUniformMatrix4fv(getUniformLoc("viewModel"), 1, GL_TRUE, viewModelMat);
}