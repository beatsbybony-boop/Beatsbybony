#include "OrtRuntime.h"

#if defined(_WIN32)
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <delayimp.h>
 #include <string>

namespace
{
    std::wstring g_ortPath;

    FARPROC WINAPI bonyDelayLoadHook (unsigned dliNotify, PDelayLoadInfo info)
    {
        // The plugin links onnxruntime.dll lazily. When Windows asks for it, hand back OUR copy
        // (the one shipped inside the .vst3 folder) instead of whatever else is on the system.
        if (dliNotify == dliNotePreLoadLibrary && info != nullptr && info->szDll != nullptr
            && _stricmp (info->szDll, "onnxruntime.dll") == 0 && ! g_ortPath.empty())
            return (FARPROC) LoadLibraryExW (g_ortPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

        return nullptr;
    }
}

extern "C"
{
   #if defined(DELAYIMP_INSECURE_WRITABLE_HOOKS)
    PfnDliHook __pfnDliNotifyHook2 = bonyDelayLoadHook;
   #else
    const PfnDliHook __pfnDliNotifyHook2 = bonyDelayLoadHook;
   #endif
}

namespace bony
{
bool initOnnxRuntime (const std::string& dir, std::string& err)
{
    std::wstring wdir (dir.begin(), dir.end());
    // directory may contain non-ASCII characters: convert properly
    int len = MultiByteToWideChar (CP_UTF8, 0, dir.c_str(), -1, nullptr, 0);
    if (len > 0) { wdir.assign ((size_t) len - 1, L'\0'); MultiByteToWideChar (CP_UTF8, 0, dir.c_str(), -1, &wdir[0], len); }

    g_ortPath = wdir + L"\\onnxruntime.dll";
    if (GetFileAttributesW (g_ortPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        err = "onnxruntime.dll was not found next to the plugin.";
        g_ortPath.clear();
        return false;
    }
    return true;
}
}
#else
namespace bony
{
bool initOnnxRuntime (const std::string&, std::string&) { return true; }
}
#endif
