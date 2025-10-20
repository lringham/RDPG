#pragma once
#include <string>
#ifdef _WIN32	
#include <Windows.h>
#else
#include <dlfcn.h>
#endif


class DynamicLibLoader
{
public:
    DynamicLibLoader() = default;
    DynamicLibLoader(const std::string& libName);
    ~DynamicLibLoader();

    DynamicLibLoader(const DynamicLibLoader&) = delete;
    DynamicLibLoader(DynamicLibLoader&&) = delete;
    DynamicLibLoader& operator=(const DynamicLibLoader&) = delete;
    DynamicLibLoader& operator=(DynamicLibLoader&&) = delete;

    void loadLib(const std::string& handleName);
    void freeLib();

    template<typename T>
    T loadFunc(const std::string& funcName)
    {
#ifdef _WIN32	
        return (T)GetProcAddress(handle_, funcName.c_str());
#else
        return (T)dlsym(handle_, funcName.c_str());
#endif
    }

private:

#ifdef _WIN32	
    HMODULE handle_ = nullptr;
#else
    void* handle_ = nullptr;
#endif

    bool loaded_ = false;
};

