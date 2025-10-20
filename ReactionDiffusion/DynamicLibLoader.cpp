#include "DynamicLibLoader.h"


DynamicLibLoader::DynamicLibLoader(const std::string& libName)
{
    loadLib(libName);
}

DynamicLibLoader::~DynamicLibLoader()
{
    freeLib();
}

void DynamicLibLoader::loadLib(const std::string& libName)
{
    freeLib();

#ifdef _WIN32	    
    handle_ = LoadLibrary(libName.c_str());
#elif __APPLE__
    handle = dlopen(libName.c_str(), RTLD_LAZY);
#elif __linux__
    handle = dlopen(("./"+libName).c_str(), RTLD_LAZY);
#endif

    loaded_ = handle_ != nullptr;
}

void DynamicLibLoader::freeLib()
{
    if (loaded_)
    {
#ifdef _WIN32	    
        FreeLibrary(handle_);
#elif else
        dlclose(handle);
#endif
        loaded_ = false;
        handle_ = nullptr;
    }
}