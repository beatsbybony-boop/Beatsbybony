cmake_minimum_required(VERSION 3.22)
project(BeatsByBony VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    # link the C++ runtime into the plugin so the user doesn't need extra installs for it
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
option(BONY_BUILD_TESTS "Build the developer test program" OFF)

# ---------------------------------------------------------------- JUCE
set(BONY_JUCE_DIR "" CACHE PATH "Use a JUCE folder you already have (leave empty to download it)")
if(BONY_JUCE_DIR)
    add_subdirectory(${BONY_JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------- ONNX Runtime (runs the stem separation models)
set(BONY_ORT_VERSION 1.20.1)
set(BONY_ORT_DIR "" CACHE PATH "Use an ONNX Runtime folder you already have (leave empty to download it)")
if(NOT BONY_ORT_DIR)
    if(WIN32)
        set(_ort_url "https://github.com/microsoft/onnxruntime/releases/download/v${BONY_ORT_VERSION}/onnxruntime-win-x64-${BONY_ORT_VERSION}.zip")
    elseif(APPLE)
        set(_ort_url "https://github.com/microsoft/onnxruntime/releases/download/v${BONY_ORT_VERSION}/onnxruntime-osx-universal2-${BONY_ORT_VERSION}.tgz")
    else()
        set(_ort_url "https://github.com/microsoft/onnxruntime/releases/download/v${BONY_ORT_VERSION}/onnxruntime-linux-x64-${BONY_ORT_VERSION}.tgz")
    endif()
    FetchContent_Declare(onnxruntime URL ${_ort_url} DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(onnxruntime)
    set(BONY_ORT_DIR ${onnxruntime_SOURCE_DIR})
endif()

# ---------------------------------------------------------------- plugin
juce_add_plugin(BeatsByBony
    COMPANY_NAME "BeatsByBony"
    PLUGIN_MANUFACTURER_CODE Bbby
    PLUGIN_CODE Bbn1
    FORMATS VST3
    PRODUCT_NAME "BeatsByBony"
    VST3_CATEGORIES Fx Tools
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    BUNDLE_ID com.beatsbybony.plugin)

target_sources(BeatsByBony PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/midi/NoteDetector.cpp
    Source/midi/GrabEngine.cpp
    Source/stems/ModelManager.cpp
    Source/stems/MdxModel.cpp
    Source/stems/OrtRuntime.cpp
    Source/stems/StemEngine.cpp
    Source/ui/Art.cpp
    Source/ui/Tag.cpp
    Source/ui/BonyLookAndFeel.cpp
    Source/ui/PianoRoll.cpp
    Source/ui/GrabPage.cpp
    Source/ui/SplitPage.cpp
    Source/ui/BoostPage.cpp)

target_include_directories(BeatsByBony PRIVATE Source ${BONY_ORT_DIR}/include)

target_compile_definitions(BeatsByBony PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    _USE_MATH_DEFINES
    NOMINMAX)

if(BONY_BUILD_TESTS)
    target_compile_definitions(BeatsByBony PUBLIC JUCE_MODAL_LOOPS_PERMITTED=1)
endif()

# Developer-only switches for building on a bare Linux box (not used for the Windows plugin)
option(BONY_LINUX_DEV "Disable X11 extras so the code compiles on a minimal Linux box" OFF)
if(BONY_LINUX_DEV)
    target_compile_definitions(BeatsByBony PUBLIC JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0
        JUCE_USE_XRENDER=0 JUCE_USE_XSHM=0 JUCE_ALSA=0 JUCE_JACK=0)
endif()

target_link_libraries(BeatsByBony
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_cryptography
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if(WIN32)
    target_link_libraries(BeatsByBony PUBLIC ${BONY_ORT_DIR}/lib/onnxruntime.lib delayimp)
    target_link_options(BeatsByBony PUBLIC /DELAYLOAD:onnxruntime.dll)
    add_custom_command(TARGET BeatsByBony_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BONY_ORT_DIR}/lib/onnxruntime.dll $<TARGET_FILE_DIR:BeatsByBony_VST3>
        COMMENT "Copying onnxruntime.dll next to the plugin")
elseif(APPLE)
    target_link_libraries(BeatsByBony PUBLIC ${BONY_ORT_DIR}/lib/libonnxruntime.dylib)
else()
    target_link_libraries(BeatsByBony PUBLIC ${BONY_ORT_DIR}/lib/libonnxruntime.so)
    set_target_properties(BeatsByBony PROPERTIES BUILD_RPATH ${BONY_ORT_DIR}/lib)
    add_custom_command(TARGET BeatsByBony_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BONY_ORT_DIR}/lib/libonnxruntime.so.${BONY_ORT_VERSION} $<TARGET_FILE_DIR:BeatsByBony_VST3>)
endif()

# Developer-only test program (not part of the plugin)
if(BONY_BUILD_TESTS)
    add_executable(bony_tests tests/tests.cpp)
    target_link_libraries(bony_tests PRIVATE BeatsByBony juce::juce_audio_utils juce::juce_dsp juce::juce_cryptography juce::juce_recommended_config_flags)
    target_include_directories(bony_tests PRIVATE Source ${BONY_ORT_DIR}/include)
endif()
