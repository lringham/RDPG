#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>


class Color;
class Graphics
{
public:
	Graphics() = default;

	void toggleWireframe();
	void toggleCullFace();
	void toggleDepthTest();
	void toggleDepthClamp();
	void toggleFrontFace();
	bool wireframeEnabled();
	bool cullfaceEnabled();
	bool depthTestEnabled();
	bool depthClampEnabled();
	bool isFrontFaceCCW();

	void queryGLVersion() const;
	void setBackgroundColor(unsigned char r, unsigned char g, unsigned char b);
	void setBackgroundColor(const Color& color);
	void setViewport(GLint left, GLint bottom, GLint right, GLint top);
	void clear();
	void clearDepthBuff();

private:
	bool wireframe_ = false, cullface_ = false, depthTest_ = false, depthClamp_ = false, frontFaceCCW_ = true;
	GLuint bufferBits_ = GL_COLOR_BUFFER_BIT;
};

bool checkGLErrors();
