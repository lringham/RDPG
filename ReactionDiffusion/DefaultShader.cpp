#include "DefaultShader.h"
#include "Utils.h"
#include "ResourceLocations.h"


DefaultShader::DefaultShader()
{
    textures["colorMapOutside"] = 0;
    textures["colorMapInside"] = 0;

    addShaderStage(GL_VERTEX_SHADER, R"(
#version 410

// ins
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexture;
layout(location = 3) in vec4 vertexColor;

// outs
out vec3 fragNorm;
out vec3 fragPos;
out vec4 fragColor;
out vec2 fragUV;

// uniforms
uniform mat4 proj;
uniform mat4 viewModel;

void main()
{
	gl_Position = proj * viewModel * vec4(vertexPosition, 1.0);
	fragNorm = (viewModel * vec4(vertexNormal, 0.f)).xyz;
	fragPos = (viewModel * vec4(vertexPosition, 1.0)).xyz;
	fragUV = vertexTexture;
	fragColor = vertexColor;
}
)");

    addShaderStage(GL_FRAGMENT_SHADER, R"(
#version 410

// ins
in vec4 fragColor;
in vec3 fragNorm;
in vec3 fragPos;
in vec2 fragUV;

// outs
out vec4 FragmentColor;

// Uniforms
uniform sampler1D colorMapOutside;
uniform sampler1D colorMapInside;
uniform float normCoef;
uniform int disableColormap;
uniform int disableShading;
uniform int selectingCells;

uniform sampler2D background;
uniform float backgroundThreshold;
uniform float backgroundOffset;
uniform int useBackgroundTexture;

vec4 getColor(sampler1D textureID, vec4 morphs)
{
	vec4 color;
	if(selectingCells == 1)
		color = texture(textureID, morphs.x / normCoef) * morphs.y;
	else if(disableColormap == 0)
		color = texture(textureID, morphs.x / normCoef);		
	else
		color = morphs / normCoef;
	return color;
}

void main(void)
{
	vec4 color;
	vec3 N = normalize(fragNorm);
	vec3 L = vec3(0, 0, 1);
	float NdotL = dot(N, L);
	float isOutside = dot(-fragPos, N);
	vec4 morphs = fragColor;
	
	if (useBackgroundTexture != 0) 
	{
		if(isOutside <= 0.0f)
		{
			color = getColor(colorMapInside, morphs);
		}
		else 
		{
			color = getColor(colorMapOutside, morphs);
		}

		// Normalize the morph conc.
		// Mix texture with conc
		float morph = morphs.x / normCoef;
		morph = min(max((morph - backgroundOffset) / backgroundThreshold, 0.0), 1.0);
		color = mix(texture(background, fragUV), color, morph);
	}
	else if(isOutside <= 0.0f)
	{
		color = getColor(colorMapInside, morphs);
	}
	else 
	{
		color = getColor(colorMapOutside, morphs);
	}

	// Apply simple diffuse if shading enabled
	if(disableShading == 0)
		color = color * .4f + color * abs(NdotL) * .6f;	

	FragmentColor = color;
})");

    createShaderProgram();

    glUseProgram(program);
    glUniform1f(getUniformLoc("normCoef"), 1.f);
    glUseProgram(0);
}

void DefaultShader::startRender(const Drawable* drawable, const Camera& camera)
{
    glUseProgram(program);

    glUniform1i(getUniformLoc("colorMapOutside"), 0);
    glUniform1i(getUniformLoc("colorMapInside"), 1);
	glUniform1i(getUniformLoc("background"), 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, textures["colorMapOutside"]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, textures["colorMapInside"]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, textures["background"]);

    glUniform1i(getUniformLoc("disableColormap"), disableColormap);
    glUniform1i(getUniformLoc("disableShading"), disableShading);
    glUniform1i(getUniformLoc("selectingCells"), selectingCells);
	
    setProjUniform(camera.getProjectionData());
    setViewModelUniform((camera.getViewMatrix() * drawable->transform_.getTransformMat()).elements);
}

void DefaultShader::endRender()
{
    glUseProgram(0);
}

void DefaultShader::setProjUniform(const float projMat[])
{
    glUniformMatrix4fv(getUniformLoc("proj"), 1, GL_TRUE, projMat);
}

void DefaultShader::setViewModelUniform(const float viewModelMat[])
{
    glUniformMatrix4fv(getUniformLoc("viewModel"), 1, GL_TRUE, viewModelMat);
}