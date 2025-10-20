#include "Shader.h"
#include "Mat4.h"
#include "LOG.h"


Shader::~Shader()
{
    glDeleteProgram(program);
}

void Shader::startRender(const Drawable*, const Camera&)
{
    glUseProgram(program);
}

void Shader::endRender()
{
    glUseProgram(0);
}

void Shader::bind()
{
    glUseProgram(program);
}

void Shader::unbind()
{
    glUseProgram(0);
}

void Shader::createShaderProgram()
{
    std::vector<GLuint> shaders(shaderStages.size());

    int i = 0;
    for (const auto& stage : shaderStages)
        shaders[i++] = createShader(stage.first, stage.second);

    program = linkProgram(shaders);

    updateAttributes();
    updateUniforms();

    shaderStages.clear();
    shaders.clear();
}

GLuint Shader::linkProgram(std::vector<GLuint>& shaders)
{
    // allocate program object name
    GLuint programObject = glCreateProgram();

    // attach 
    for (GLuint shader : shaders)
        glAttachShader(programObject, shader);

    // link
    glLinkProgram(programObject);

    // detach shaders
    for (GLuint shader : shaders)
        glDetachShader(programObject, shader);

    // delete shaders
    for (GLuint shader : shaders)
        glDeleteShader(shader);

    // retrieve link status
    GLint status;
    glGetProgramiv(programObject, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
        std::string info(length, ' ');
        glGetProgramInfoLog(programObject, (GLsizei)info.length(), &length, &info[0]);
        std::cout << "ERROR linking shader program:" << std::endl;
        std::cout << info << std::endl;
    }

    return programObject;
}

GLuint Shader::createShader(const GLint shaderTarget, const std::string& sourceCode)
{
    GLuint shader = 0;

    //Create shader
    const char* code = sourceCode.c_str();
    shader = glCreateShader(shaderTarget);		//create shader
    glShaderSource(shader, 1, &code, NULL);		//attach source code
    glCompileShader(shader);					//compile

    //Check for compile errors
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string info(length, ' ');
        glGetShaderInfoLog(shader, (GLsizei)info.length(), &length, &info[0]);
        std::cout << "ERROR compiling shader:" << std::endl << std::endl;
        std::cout << sourceCode << std::endl;
        std::cout << info << std::endl;
    }

    return shader;
}

void Shader::addShaderStage(const GLint shaderTarget, const std::string& sourceCode, const std::map<std::string, std::string>& includesMap)
{
    shaderStages[shaderTarget] = processIncludes(sourceCode, includesMap);
}

void Shader::addShaderStage(const GLint shaderTarget, const std::string& sourceCode)
{
    shaderStages[shaderTarget] = sourceCode;
}

std::string Shader::processIncludes(std::string sourceCode, const std::map<std::string, std::string>& includesMap)
{
    size_t pos;
    for (auto& incl : includesMap)
    {
        pos = sourceCode.find(incl.first);
        if (pos != std::string::npos)
            sourceCode = sourceCode.replace(pos, incl.first.size(), incl.second);
    }
    return sourceCode;
}

void Shader::updateAttributes()
{
    GLsizei size = 0;
    GLenum type;
    GLint loc;
    GLint numActiveAttribs;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numActiveAttribs);

    GLint attribNameLength = 0;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attribNameLength);
    char* name = new char[attribNameLength];
    for (int i = 0; i < numActiveAttribs; ++i)
    {
        glGetActiveAttrib(program, (GLint)i, attribNameLength, nullptr, &size, &type, name);
        loc = glGetAttribLocation(program, name);
        attributes[name] = loc;
    }
    delete[] name;
    name = nullptr;
}

void Shader::updateUniforms()
{
    GLint loc;
    GLint numActiveUniforms;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numActiveUniforms);
    GLint uniformNameLength = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformNameLength);
    char* name = new char[uniformNameLength];
    for (int i = 0; i < numActiveUniforms; i++)
    {
        glGetActiveUniformName(program, (GLint)i, uniformNameLength, nullptr, name);
        loc = glGetUniformLocation(program, name);
        uniforms[name] = loc;
    }
    delete[] name;
    name = nullptr;
}

GLint Shader::getUniformLoc(const std::string& name)
{
    if(uniforms.count(name) > 0)
        return uniforms.at(name);
    return -1;
}

//void Shader::updateSubroutines()
//{
    // For each shader type....

    // get all subroutine names
//	GLint numSubroutines = 0;
//	GLint maxNameLen = 0;
//	char* name = new char[maxNameLen];
//	
//	glGetProgramStageiv(program, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, &numSubroutines);
//	glGetProgramStageiv(program, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINE_MAX_LENGTH, &maxNameLen);
//	glGetActiveSubroutineName(program, GL_VERTEX_SHADER, index, sizeof(char)*maxNameLen, nullptr, name);
//
//	//For loop 
//
    // get all subroutine uniform names
//	GLint numSubUniforms = 0;
//	glGetProgramStageiv(program, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &numSubUniforms);
//
//	//for loop
//	for (int i = 0; i < numSubUniforms; ++i)
//	{
//		glGetActiveSubroutineUniformiv(program, GL_VERTEX_SHADER, i, , value);
//	}
//
//	delete[] name;
//	name = nullptr;
//}

void Shader::printShaderInfo()
{
    GLsizei size = 0;
    GLenum type;
    GLint loc;

    std::cout << std::endl << "SHADER INFO"   << std::endl;
    std::cout << "--------------------------" << std::endl;

    // Attributes
    {
        GLint numActiveAttribs;
        glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numActiveAttribs);

        GLint attribNameLength = 0;
        glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attribNameLength);
        char* name = new char[attribNameLength];
        std::cout << "Vertex Attributes" << std::endl;
        for (int i = 0; i < numActiveAttribs; ++i)
        {
            glGetActiveAttrib(program, (GLint)i, attribNameLength, nullptr, &size, &type, name);
            loc = glGetAttribLocation(program, name);
            std::cout << " " << printType(type) << " " << "layout(location=" << loc << ") in " << name << " " << size << std::endl;
        }
        delete[] name;
        name = nullptr;
    }

    //Fragment Outputs
    std::cout << std::endl << "Fragment Output" << std::endl;
    std::cout << " FragDataLocation " << glGetFragDataLocation(program, "FragmentColor") << std::endl;
    std::cout << " FragDataIndex " << glGetFragDataIndex(program, "FragmentColor") << std::endl << std::endl;

    // Uniforms
    {
        GLint numActiveUniforms;
        GLint params;
        std::cout << "Uniforms" << std::endl;
        glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numActiveUniforms);
        GLint uniformNameLength = 0;
        glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformNameLength);
        char* name = new char[uniformNameLength];
        for (int i = 0; i < numActiveUniforms; i++)
        {
            glGetActiveUniformName(program, (GLint)i, uniformNameLength, nullptr, name);
            loc = glGetUniformLocation(program, name);
            std::cout << " " << name << std::endl;
            std::cout << " loc " << loc << std::endl;
            std::cout << " UBI " << (GLint)glGetUniformBlockIndex(program, name) << std::endl;

            const GLuint idx = i;
            glGetActiveUniformsiv(program, 1, &idx, GL_UNIFORM_TYPE, &params);
            std::cout << " " << printType(params) << std::endl;
            glGetActiveUniformsiv(program, 1, &idx, GL_UNIFORM_SIZE, &params);
            std::cout << " size " << params << std::endl << std::endl;
        }
        delete[] name;
        name = nullptr;
    }
}

void Shader::setTexture(const std::string& name, GLuint texID)
{
    textures[name] = texID;
}

void Shader::setDisableColormap(bool state)
{
    disableColormap = state;
}

void Shader::setDisableShading(bool state)
{
    disableShading = state;
}

void Shader::setSelectingCells(bool state)
{
    selectingCells = state;
}

void Shader::setUniform1i(const std::string& uniformName, int val)
{
    glUseProgram(program);
    glUniform1i(getUniformLoc(uniformName.c_str()), val);
    glUseProgram(0);
}

void Shader::setUniform1f(const std::string& uniformName, float val)
{
    glUseProgram(program);
    glUniform1f(getUniformLoc(uniformName.c_str()), val);
    glUseProgram(0);
}

const char* Shader::printType(GLint type)
{
    switch (type)
    {
    case GL_SHADER_TYPE:
        return "GL_SHADER_TYPE";
    case GL_FLOAT_VEC2:
        return "GL_FLOAT_VEC2";
    case GL_FLOAT_VEC3:
        return "GL_FLOAT_VEC3";
    case GL_FLOAT_VEC4:
        return "GL_FLOAT_VEC4";
    case GL_INT_VEC2:
        return "GL_INT_VEC2";
    case GL_INT_VEC3:
        return "GL_INT_VEC3";
    case GL_INT_VEC4:
        return "GL_INT_VEC4";
    case GL_BOOL:
        return "GL_BOOL";
    case GL_BYTE:
        return "GL_BYTE";
    case GL_UNSIGNED_BYTE:
        return "GL_UNSIGNED_BYTE";
    case GL_SHORT:
        return "GL_SHORT";
    case GL_UNSIGNED_SHORT:
        return "GL_UNSIGNED_SHORT";
    case GL_INT:
        return "GL_INT";
    case GL_UNSIGNED_INT:
        return "GL_UNSIGNED_INT";
    case GL_FLOAT:
        return "GL_FLOAT";
    case GL_DOUBLE:
        return "GL_DOUBLE";
    case GL_BOOL_VEC2:
        return "GL_BOOL_VEC2";
    case GL_BOOL_VEC3:
        return "GL_BOOL_VEC3";
    case GL_BOOL_VEC4:
        return "GL_BOOL_VEC4";
    case GL_FLOAT_MAT2:
        return "GL_FLOAT_MAT2";
    case GL_FLOAT_MAT3:
        return "GL_FLOAT_MAT3";
    case GL_FLOAT_MAT4:
        return "GL_FLOAT_MAT4";
    case GL_SAMPLER_1D:
        return "GL_SAMPLER_1D";
    case GL_SAMPLER_2D:
        return "GL_SAMPLER_2D";
    case GL_SAMPLER_3D:
        return "GL_SAMPLER_3D";
    case GL_SAMPLER_CUBE:
        return "GL_SAMPLER_CUBE";
    case GL_SAMPLER_1D_SHADOW:
        return "GL_SAMPLER_1D_SHADOW";
    case GL_SAMPLER_2D_SHADOW:
        return "GL_SAMPLER_2D_SHADOW";
    case GL_FLOAT_MAT2x3:
        return "GL_FLOAT_MAT2x3";
    case GL_FLOAT_MAT2x4:
        return "GL_FLOAT_MAT2x4";
    case GL_FLOAT_MAT3x2:
        return "GL_FLOAT_MAT3x2";
    case GL_FLOAT_MAT3x4:
        return "GL_FLOAT_MAT3x4";
    case GL_FLOAT_MAT4x2:
        return "GL_FLOAT_MAT4x2";
    case GL_FLOAT_MAT4x3:
        return "GL_FLOAT_MAT4x3";
    default:
        return "NO_TYPE_FOUND";
    };
}