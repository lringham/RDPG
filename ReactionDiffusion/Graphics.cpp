#include "Graphics.h"
#include "Color.h"

#include <iostream>
#include <string>


void Graphics::queryGLVersion() const
{
    std::string version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    std::string glslver = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    std::string renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

    std::cout << " OpenGL\t\t[" << version << "]\n";
    std::cout << " GLSL\t\t[" << glslver << "]\n";
    std::cout << " GPU\t\t[" << renderer << "]\n\n";
}

void Graphics::toggleWireframe()
{
    wireframe_ = !wireframe_;
    if (wireframe_)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Graphics::toggleCullFace()
{
    glFrontFace(GL_CCW);
    cullface_ = !cullface_;
    if (cullface_)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void Graphics::toggleDepthTest()
{
    depthTest_ = !depthTest_;
    if (depthTest_)
    {
        bufferBits_ = bufferBits_ | GL_DEPTH_BUFFER_BIT;
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }
    else
    {
        bufferBits_ = bufferBits_ & (~GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
    }
}

void Graphics::toggleDepthClamp()
{
    depthClamp_ = !depthClamp_;
    if (depthClamp_)
        glEnable(GL_DEPTH_CLAMP);
    else
        glDisable(GL_DEPTH_CLAMP);
}

void Graphics::toggleFrontFace()
{
    if (frontFaceCCW_)
        glFrontFace(GL_CW);
    else
        glFrontFace(GL_CCW);
    frontFaceCCW_ = !frontFaceCCW_;
}

bool Graphics::wireframeEnabled() { return wireframe_; }
bool Graphics::cullfaceEnabled() { return cullface_; }
bool Graphics::depthTestEnabled() { return depthTest_; }
bool Graphics::depthClampEnabled() { return depthClamp_; }
bool Graphics::isFrontFaceCCW() { return frontFaceCCW_; }

void Graphics::setBackgroundColor(const Color& color)
{
    glClearColor((float)color.r / 255.f, (float)color.g / 255.f, (float)color.b / 255.f, 1.f);
}

void Graphics::setBackgroundColor(unsigned char r, unsigned char g, unsigned char b)
{
    glClearColor((float)r / 255.f, (float)g / 255.f, (float)b / 255.f, 1.f);
}

void Graphics::setViewport(GLint left, GLint bottom, GLint right, GLint top)
{
    glViewport(left, bottom, right, top);
}

void Graphics::clear()
{
    glClear(bufferBits_);
    checkGLErrors();
}

void Graphics::clearDepthBuff()
{
    glClear(GL_DEPTH_BUFFER_BIT);
}

bool checkGLErrors()
{
    bool error = false;
    for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
    {
        std::cout << "OpenGL ERROR:  ";
        switch (flag)
        {
        case GL_INVALID_ENUM:
            std::cout << "GL_INVALID_ENUM\n"; break;
        case GL_INVALID_VALUE:
            std::cout << "GL_INVALID_VALUE\n"; break;
        case GL_INVALID_OPERATION:
            std::cout << "GL_INVALID_OPERATION\n"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            std::cout << "GL_INVALID_FRAMEBUFFER_OPERATION\n"; break;
        case GL_OUT_OF_MEMORY:
            std::cout << "GL_OUT_OF_MEMORY\n"; break;
        default:
            std::cout << "[unknown error code]\n";
        }
        error = true;
    }
    return error;
}