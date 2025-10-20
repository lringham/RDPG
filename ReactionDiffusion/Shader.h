#pragma once
#include "Drawable.h"
#include "Camera.h"

#include <string>
#include <glad/glad.h>
#include <unordered_map>
#include <vector>
#include <map>


class Drawable;
class Shader
{
private:
    std::unordered_map<GLint, std::string> shaderStages;
    GLuint linkProgram(std::vector<GLuint>& shaders);
    GLuint createShader(GLint shaderTarget, const std::string& sourceCode);
    const char* printType(GLint type);
    void updateAttributes();
    void updateUniforms();
    std::string processIncludes(std::string sourceCode, const std::map<std::string, std::string>& includesMap);

public:
    Shader() = default;
    virtual ~Shader();
    virtual void startRender(const Drawable* drawable, const Camera& camera);
    virtual void endRender();
    void bind();
    void unbind();
    void printShaderInfo();
    void setTexture(const std::string& name, GLuint texID);
    void setDisableColormap(bool state);
    void setDisableShading(bool state);
    void setSelectingCells(bool state);
    void setUniform1i(const std::string& uniformName, int val);
    void setUniform1f(const std::string& uniformName, float val);

protected:
    //methods
    void addShaderStage(GLint shaderTarget, const std::string& sourceCode, const std::map<std::string, std::string>& includesMap);
    void addShaderStage(GLint shaderTarget, const std::string& sourceCode);
    void createShaderProgram();

    GLint getUniformLoc(const std::string& name);

    //variables
    std::map<std::string, GLint> attributes;
    GLuint program = 0;
    std::map<std::string, GLint> textures;

    bool disableColormap = false;
    bool disableShading = false;
    bool selectingCells = false;

private:
    std::map<std::string, GLint> uniforms;
};
