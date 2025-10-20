#pragma once
#include "Shader.h"


class ComputeShader : public Shader
{
public:
    ComputeShader(const std::map<std::string, std::string> & includes, const std::string & shaderFilename);
    ~ComputeShader();
    void printComputeInfo();
    void dispatch(GLuint globalGroupsX, GLuint workGroupsY = 1, GLuint workGroupsZ = 1);
    void wait(GLuint barrierType = GL_ALL_BARRIER_BITS);

    void maxGlobalWorkgroupSize(int& x, int& y, int& z);
    void maxLocalWorkgroupSize(int& x, int& y, int& z);
    int maxLocalInvocations();

    template<typename T>
    GLuint createSSBO(const std::vector<T>& data) const
    {
        GLuint id;
        glGenBuffers(1, &id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(T), nullptr, GL_DYNAMIC_COPY);
        {
            T* ssboData = (T*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, data.size() * sizeof(T), GL_MAP_WRITE_BIT);
            for (unsigned i = 0; i < data.size(); ++i)
                ssboData[i] = data[i];
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return id;
    }

    GLuint createSSBO(unsigned sizeInBytes) const
    {
        GLuint id;
        glGenBuffers(1, &id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeInBytes, nullptr, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return id;
    }
};

template<typename T>
struct GPUBuffer
{
    GPUBuffer() = default;

    GPUBuffer(const std::vector<T>& data, GLint buffCreateHint = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT)
    {
        init(data, buffCreateHint);
    }

    GPUBuffer(const GPUBuffer&) = delete;
    GPUBuffer(GPUBuffer&&) = delete;
    GPUBuffer& operator=(const GPUBuffer&) = delete;
    GPUBuffer& operator=(GPUBuffer&&) = delete;

    virtual ~GPUBuffer()
    {
        unmap();
        glDeleteBuffers(1, &buffName_);
    }

    void init(const std::vector<T>& data)
    {
        this->buffCreateHint_ = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT;
        this->count_ = static_cast<GLsizeiptr>(data.size());
        glCreateBuffers(1, &buffName_);
        glNamedBufferStorage(buffName_, sizeof(T) * count_, data.data(), this->buffCreateHint_);
    }

    bool map(GLsizeiptr sizeInBytes = 0, GLintptr offset = 0)
    {
        if (sizeInBytes == 0)
            sizeInBytes = getSizeInBytes();

        ASSERT(validRange(sizeInBytes, offset), "OUT OF VRAM");
        ptr_ = (T*)glMapNamedBufferRange(buffName_, offset, sizeInBytes, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
        ASSERT(ptr_ != nullptr, "Map failed");

        if (ptr_ != nullptr)
        {
            mappedCount_ = sizeInBytes / sizeof(T);
            mappedOffset_ = offset;
            return true;
        }
        else
            return false;
    }

    bool unmap()
    {
        ptr_ = nullptr;
        mappedCount_ = 0;
        mappedOffset_ = 0;
        bool result = (bool) glUnmapNamedBuffer(buffName_);
        //ASSERT(result, "Buffer corrupt " << buffName_); // Not always correct for all gfx cards
        return result;
    }

    void saveBuffer(std::string filename)
    {
        std::ofstream ofile(filename);
        if (ofile.is_open())
        {
            map();

            for (int i = 0; i < count_; ++i)
                ofile << ptr_[i] << "\n";
            ofile.close();

            unmap();
        }
    }

    GLsizeiptr getSizeInBytes() const
    {
        return sizeof(T) * count_;
    }

    GLuint GLid() const
    {
        return buffName_;
    }

    size_t count() const
    {
        return count_;
    }

    GLint buffCreateHint() const
    {
        return buffCreateHint_;
    }

    bool validRange(GLsizeiptr sizeInBytes, GLintptr offset = 0) const
    {
        return getSizeInBytes() >= offset + sizeInBytes;
    }

    T& operator [](int i)
    {
        ASSERT(i >= mappedOffset_ && i < mappedCount_ + mappedOffset_, "Index out of range!");
        return ptr_[i];
    }

private:
    GLuint buffName_ = 0;
    GLsizeiptr count_ = 0;
    GLsizeiptr mappedCount_ = 0;
    GLsizeiptr mappedOffset_ = 0;
    GLint buffCreateHint_ = 0;
    T* ptr_ = nullptr;
};