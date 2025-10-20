#pragma once
#include "Graphics.h"
#include "Shader.h"
#include "Material.h"
#include "Camera.h"
#include "Transform.h"
#include "Mat4.h"
#include "Vec3.h"
#include "Vec2.h"

#include <glad/glad.h>
#include <vector>
#include <memory>


class Shader;
class Drawable
{
public:
    Drawable() = default;
    virtual ~Drawable();

    Drawable(Drawable& drawable) = delete;
    Drawable(Drawable&& drawable) = delete;
    Drawable& operator=(Drawable& drawable) = delete;
    Drawable& operator=(Drawable&& drawable) = delete;


    void render(const Camera& camera, std::unique_ptr<Shader>& shader);
    void render(const Camera& camera);
    void initVBOs(GLsizei indexCount = 0);
    void destroyVBOs();
    void clearBuffers();
    bool hasChild(Drawable* child);
    GLuint createVAO();
    void updatePositionVBO();
    void updateNormalVBO();
    void updateTextureVBO();
    void updateColorVBO();

    std::vector<Drawable*> childern_;
    std::vector<Vec3> positions_;
    std::vector<Vec3> normals_;
    std::vector<Vec4> colors_;
    std::vector<Vec2> textureCoords_;
    std::vector<unsigned> indices_;
    Transform transform_;
    Material material_;
    GLint drawMode_ = GL_TRIANGLES;
    std::unique_ptr<Shader> shader_;
    bool visible_ = true;
    unsigned indexCount_ = 0;

    GLuint posID_ = 0;
    GLuint norID_ = 0;
    GLuint texID_ = 0;
    GLuint colID_ = 0;
    GLuint tanID_ = 0;
    GLuint ebo_ = 0;
    GLuint vao_ = 0;

    template<typename T>
    GLuint createVBO(GLint buffType, unsigned sizeOfDataInBytes, T* data, GLint memoryHint = GL_DYNAMIC_DRAW)
    {
        GLuint attribID;
        glGenBuffers(1, &attribID);
        glBindBuffer(buffType, attribID);
        glBufferData(buffType, sizeOfDataInBytes, data, memoryHint);
        return attribID;
    }

    template<typename T>
    GLuint createVBO(GLint buffType, std::vector<T>& data, GLint memoryHint = GL_DYNAMIC_DRAW)
    {
        GLuint attribID;
        glGenBuffers(1, &attribID);
        glBindBuffer(buffType, attribID);
        glBufferData(buffType, data.size() * sizeof(T), data.data(), memoryHint);
        glBindBuffer(buffType, 0);
        return attribID;
    }

    template<typename T>
    void updateVBO(GLint targetID, std::vector<T>& data, unsigned offset = 0, size_t count = 0)
    {
        if (count == 0)
            count = data.size();

        glBindBuffer(GL_ARRAY_BUFFER, targetID);
        glBufferSubData(GL_ARRAY_BUFFER, offset * sizeof(T), count * sizeof(T), &data[offset]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    template<typename T>
    void setupVertexAttrib(GLuint attribID, GLuint attribLayoutIndex, GLuint elementType, unsigned sizeOfElement, bool normalize = GL_FALSE, void* offset = 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, attribID);
        glVertexAttribPointer(attribLayoutIndex, sizeof(T) / sizeOfElement, elementType, normalize, sizeof(T), offset);
        glEnableVertexAttribArray(attribLayoutIndex);
    }
};

std::pair<Vec3, Vec3> calculateBounds(const Drawable& d);