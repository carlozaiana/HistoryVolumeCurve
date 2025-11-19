cmake_minimum_required(VERSION 3.22)
project(SmoothVolumeHistory VERSION 1.0.0 LANGUAGES CXX)

# macOS 10.13 + Universal binary
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

# === Download JUCE automatically ===
include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        origin/master
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(juce)

# === Plugin definition ===
juce_add_plugin(SmoothVolumeHistory
    COMPANY_NAME              "YourName"
    PLUGIN_MANUFACTURER_CODE  Xxxx
    PLUGIN_CODE               VolH
    FORMATS                   VST3
    PRODUCT_NAME              "Smooth Volume History"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   TRUE
    VST3_CATEGORIES           "Analyzer" "Utility"
)

target_sources(SmoothVolumeHistory
    PRIVATE
        PluginProcessor.cpp
        PluginProcessor.h
        PluginEditor.cpp
        PluginEditor.h
)

target_compile_definitions(SmoothVolumeHistory
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(SmoothVolumeHistory
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_l10n_flags
        juce::juce_recommended_warning_flags
)