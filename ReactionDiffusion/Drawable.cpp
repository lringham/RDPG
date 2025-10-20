#include "Drawable.h"

#include <algorithm>


Drawable::~Drawable()
{
    destroyVBOs();
}

void Drawable::destroyVBOs()
{
    if (posID_) glDeleteBuffers(1, &posID_);
    if (norID_) glDeleteBuffers(1, &norID_);
    if (texID_) glDeleteBuffers(1, &texID_);
    if (colID_) glDeleteBuffers(1, &colID_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
}

void Drawable::clearBuffers()
{
    positions_.clear();
    normals_.clear();
    textureCoords_.clear();
    colors_.clear();
    indices_.clear();
}

bool Drawable::hasChild(Drawable* child)
{
    return std::find(childern_.begin(), childern_.end(), child) != childern_.end();
}

void Drawable::render(const Camera& camera, std::unique_ptr<Shader>& overrideShader)
{
    if (!visible_)
        return;

    overrideShader->startRender(this, camera);

    glBindVertexArray(vao_);
    glDrawElements(
        drawMode_,		    // mode
        indexCount_,         // count
        GL_UNSIGNED_INT,    // type
        (void*)0);		    // offset
    glBindVertexArray(0);

    overrideShader->endRender();

    for (Drawable* d : childern_)
    {
        d->transform_.setCustomTransform(transform_.getTransformMat() * d->transform_.getTransformMat());

        if (d->shader_ == nullptr)
            d->render(camera, overrideShader);
        else
            d->render(camera);

        d->transform_.useCustomTransform(false);
    }
}

void Drawable::render(const Camera& camera)
{
    if (!visible_)
        return;

    if (shader_ != nullptr)
        shader_->startRender(this, camera);

    glBindVertexArray(vao_);

    glDrawElements(
        drawMode_,			// mode
        indexCount_,			// count
        GL_UNSIGNED_INT,	// type
        (void*)0);			// offset
    glBindVertexArray(0);

    if (shader_ != nullptr)
        shader_->endRender();

    for (Drawable* d : childern_)
    {
        d->transform_.setCustomTransform(transform_.getTransformMat() * d->transform_.getTransformMat());

        if (d->shader_ == nullptr)
            d->render(camera, shader_);
        else
            d->render(camera);

        d->transform_.useCustomTransform(false);
    }
}

void Drawable::initVBOs(GLsizei newIndexCount)
{
    if (newIndexCount == 0)
        this->indexCount_ = static_cast<GLsizei>(indices_.size());
    else
        this->indexCount_ = newIndexCount;

    posID_ = createVBO(GL_ARRAY_BUFFER, positions_);
    norID_ = createVBO(GL_ARRAY_BUFFER, normals_);
    texID_ = createVBO(GL_ARRAY_BUFFER, textureCoords_);
    colID_ = createVBO(GL_ARRAY_BUFFER, colors_);
    ebo_ = createVBO(GL_ELEMENT_ARRAY_BUFFER, indices_);
    vao_ = createVAO();

    glBindVertexArray(vao_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    setupVertexAttrib<Vec3>(posID_, 0, GL_FLOAT, sizeof(float));
    setupVertexAttrib<Vec3>(norID_, 1, GL_FLOAT, sizeof(float), GL_TRUE);
    setupVertexAttrib<Vec2>(texID_, 2, GL_FLOAT, sizeof(float));
    setupVertexAttrib<Vec4>(colID_, 3, GL_FLOAT, sizeof(float));

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint Drawable::createVAO()
{
    GLuint vaoID;
    glGenVertexArrays(1, &vaoID);
    return vaoID;
}

void Drawable::updatePositionVBO()
{
    updateVBO(posID_, positions_);
}

void Drawable::updateNormalVBO()
{
    updateVBO(norID_, normals_);
}

void Drawable::updateTextureVBO()
{
    updateVBO(texID_, textureCoords_);
}

void Drawable::updateColorVBO()
{
    updateVBO(colID_, colors_);
}

std::pair<Vec3, Vec3> calculateBounds(const Drawable& d)
{
    if (d.positions_.size() == 0)
        return std::pair<Vec3, Vec3>(Vec3(0), Vec3(0));

    Vec3 minCorner = d.positions_[0];
    Vec3 maxCorner = d.positions_[0];
    for (const Vec3& p : d.positions_)
    {
        if (p.x < minCorner.x)
            minCorner.x = p.x;

        if (p.y < minCorner.y)
            minCorner.y = p.y;

        if (p.z < minCorner.z)
            minCorner.z = p.z;

        if (p.x > maxCorner.x)
            maxCorner.x = p.x;

        if (p.y > maxCorner.y)
            maxCorner.y = p.y;

        if (p.z > maxCorner.z)
            maxCorner.z = p.z;
    }
    return std::pair<Vec3, Vec3>(minCorner, maxCorner);
}
