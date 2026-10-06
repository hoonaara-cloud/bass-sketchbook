cmake_minimum_required(VERSION 3.22)

project(GrooveSketchbook VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime: the .vst3 runs on machines without a dev setup.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE is fetched automatically from GitHub (no manual install needed).
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GrooveSketchbook
    COMPANY_NAME "Hoonaar Audio"
    PLUGIN_MANUFACTURER_CODE Hona
    PLUGIN_CODE Grsk
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Groove Sketchbook"
    DESCRIPTION "Pencil-sketch bass multi-effect"
    VERSION 0.1.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Fx"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

# Embedded assets (handwritten font). Include as "BinaryData.h" in sources.
juce_add_binary_data(GrooveSketchbookAssets
    NAMESPACE BinaryData
    SOURCES Assets/Caveat-Regular.ttf)

target_sources(GrooveSketchbook PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/BassDSP.cpp
    Source/SketchLookAndFeel.cpp
    Source/Panels.cpp)

target_compile_definitions(GrooveSketchbook PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GrooveSketchbook PRIVATE
    GrooveSketchbookAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# Headless DSP smoke test (developers only, off by default).
option(GROOVE_BUILD_TESTS "Build headless DSP smoke test" OFF)
if (GROOVE_BUILD_TESTS)
    juce_add_console_app(GrooveDSPTest)
    target_sources(GrooveDSPTest PRIVATE
        Tests/dsp_smoke_test.cpp
        Source/BassDSP.cpp)
    target_include_directories(GrooveDSPTest PRIVATE Source)
    target_compile_definitions(GrooveDSPTest PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(GrooveDSPTest PRIVATE
        juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
