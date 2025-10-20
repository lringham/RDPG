#pragma once
#include "Image.h"

#include <glad/glad.h>
#include <vector>
#include <string>

struct Textures
{
public:
    class Texture
    {
        GLuint id_ = 0, target_ = 0;
        
    public:
        Texture() = default;

        Texture(GLuint id, GLuint target) :
            id_(id), target_(target)
        {}

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& rhs) noexcept
        {
            id_ = rhs.id_;
            target_ = rhs.target_;

            rhs.id_ = 0;
            rhs.target_ = 0;
        }

        Texture& operator=(Texture&& rhs) noexcept
        {
            id_ = rhs.id_;
            target_ = rhs.target_;

            rhs.id_ = 0;
            rhs.target_ = 0;

            return *this;
        }

        virtual ~Texture()
        {
            glDeleteTextures(1, &id_);
        }

        GLuint id() const 
        {
            return id_;
        }

        GLuint target() const
        {
            return id_;
        }
    };

    class Texture1D : 
        public Texture
    {
        int width_ = 0;

    public:
        Texture1D() = default;

        Texture1D(int width, GLuint id) :
            Texture(id, GL_TEXTURE_1D), width_(width)
        {}

        int width() const
        {
            return width_;
        }
    };

    class Texture2D :
        public Texture
    {
        int width_ = 0, height_ = 0;
    
    public:
        Texture2D() = default;

        Texture2D(int width, int height, GLuint id) :
            Texture(id, GL_TEXTURE_2D), width_(width), height_(height)
        {}

        int width() const
        {
            return width_;
        }

        int height() const
        {
            return height_;
        }
    };

    class Texture2DArray :
        public Texture
    {
        int width_ = 0, height_ = 0, numLayers_ = 0;

    public:
        Texture2DArray() = default;

        Texture2DArray(int width, int height, int numLayers, GLuint id) :
            Texture(id, GL_TEXTURE_2D_ARRAY), width_(width), height_(height), numLayers_(numLayers)
        {}

        int width() const
        {
            return width_;
        }

        int height() const
        {
            return height_;
        }

        int numLayers() const
        {
            return numLayers_;
        }
    };

    class Texture3D :
        public Texture
    {
        int width_ = 0, height_ = 0, depth_ = 0;

    public:
        Texture3D() = default;

        Texture3D(int width, int height, int depth, GLuint id) :
            Texture(id, GL_TEXTURE_3D), width_(width), height_(height), depth_(depth)
        {}

        int width() const
        {
            return width_;
        }

        int height() const
        {
            return height_;
        }

        int depth() const
        {
            return depth_;
        }
    };

    template<typename T>
    static Texture1D create1DTexture(const std::vector<T>& data, GLuint filter = GL_LINEAR, GLuint wrap = GL_CLAMP_TO_EDGE, GLuint imageFormat = GL_RGB, GLuint internalFormat = GL_RGB, char numComponents = 3, GLuint type = GL_UNSIGNED_BYTE)
    {
        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_1D, id);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, wrap);

        GLsizei width = static_cast<GLsizei>(data.size()) / static_cast<GLsizei>(numComponents);
        glTexImage1D(GL_TEXTURE_1D, 0, internalFormat, width, 0, imageFormat, type, &data[0]);
        glBindTexture(GL_TEXTURE_1D, 0);

        return Texture1D(width, id);
    }

    template<typename T>
    static Texture2D create2DTexture(const std::vector<T>& data, int width, int height, GLuint filter = GL_LINEAR, GLuint wrap = GL_CLAMP_TO_EDGE, GLuint imageFormat = GL_RGB, GLuint internalFormat = GL_RGB, GLuint type = GL_UNSIGNED_BYTE)
    {
        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, imageFormat, type, &data[0]);
        glBindTexture(GL_TEXTURE_2D, 0);

        return Texture2D(width, height, id);
    }

    static Texture2D create2DTexture(
        const Image& image, 
        GLuint filter = GL_LINEAR, 
        GLuint wrap = GL_CLAMP_TO_EDGE, 
        GLuint imageFormat = GL_RGB, 
        GLuint internalFormat = GL_RGB, 
        GLuint type = GL_UNSIGNED_BYTE)
    {
        return create2DTexture<unsigned char>(image.data_, image.width_, image.height_, filter, wrap, imageFormat, internalFormat, type);
    }

    template<typename T>
    static Texture2DArray create2DArrayTexture(const std::vector<T>& data, GLsizei width, GLsizei height, GLsizei numLayers, GLuint filter = GL_LINEAR, GLuint wrap = GL_CLAMP_TO_EDGE, GLuint imageFormat = GL_RGB, GLuint internalFormat = GL_RGB, char numComponents = 3, GLuint type = GL_UNSIGNED_BYTE)
    {
        GLsizei mipLevelCount = 1; // 1 = no mip maps
        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D_ARRAY, id);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, filter);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, internalFormat, width, height, numLayers);

        for (GLsizei i = 0; i < numLayers; ++i)
        {
            glTexSubImage3D(
                GL_TEXTURE_2D_ARRAY,
                0,                 // mipmap number
                0, 0, i,           // offset into volume
                width, height, 1,
                imageFormat,
                type,
                &data[0]);
        }

        return Texture2DArray(width, height, numLayers, id);
    }
};

