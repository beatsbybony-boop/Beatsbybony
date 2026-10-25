#pragma once
#include <string>

namespace bony
{
/** Tells the plugin where to find onnxruntime.dll (Windows). Call before the first model is loaded.
    On other platforms this does nothing. Returns false if the library can't be found. */
bool initOnnxRuntime (const std::string& directoryContainingOrtLibrary, std::string& errorMessage);
}
