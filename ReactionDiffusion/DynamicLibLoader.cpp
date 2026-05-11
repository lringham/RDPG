#include "DynamicLibLoader.h"

#include "LOG.h"


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
    if (handle_ == nullptr) {
        char* msg = nullptr;
        DWORD err = GetLastError();
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&msg,
            0,
            nullptr
        );
        MessageBoxA(
            nullptr,
            "Failed to load PDES.dll",
            "Error",
            MB_OK | MB_ICONINFORMATION
        );
        LOG(msg);
        LocalFree(msg);
    }
#elif __APPLE__
    handle_ = dlopen(libName.c_str(), RTLD_LAZY);
#elif __linux__
    handle_ = dlopen(("./"+libName).c_str(), RTLD_LAZY);
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
        dlclose(handle_);
#endif
        loaded_ = false;
        handle_ = nullptr;
    }
}

bool DynamicLibLoader::initialized() const
{
    return loaded_ && handle_ != nullptr;
}