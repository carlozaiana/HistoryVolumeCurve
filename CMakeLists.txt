cmake_minimum_required(VERSION 3.15)

project(HistoryVolumeCurve VERSION 0.0.1)

# ==========================================================
# JUCE path setup
# ==========================================================
# Option 1: JUCE as submodule
# add_subdirectory(JUCE)

# Option 2: Installed JUCE package
find_package(JUCE CONFIG REQUIRED)

# ==========================================================
# Target: Audio Plugin
# ==========================================================
juce_add_plugin(HistoryVolumeCurve
    VERSION                 0.0.1
    COMPANY_NAME            "YourCompany"
    COMPANY_WEBSITE         "https://example.com"
    BUNDLE_ID               com.yourcompany.historyvolume
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    FORMATS                 VST3
    PRODUCT_NAME            "HistoryVolumeCurve"
)

# ==========================================================
# Sources
# ==========================================================
target_sources(HistoryVolumeCurve PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

# ==========================================================
# JUCE Modules
# ==========================================================
target_link_libraries(HistoryVolumeCurve PRIVATE
    juce::juce_audio_utils
    juce::juce_opengl         # <-- required for OpenGL rendering
)

# ==========================================================
# macOS configuration
# ==========================================================
if(APPLE)
    # Allow High-DPI rendering in hosts like Logic/Live
    target_compile_definitions(HistoryVolumeCurve PRIVATE
        JUCE_USE_COREIMAGE_LOADER=1
        JUCE_COREGRAPHICS_DRAW_ASYNC=1
    )

    # Enable OpenGL MSAA (Multi-Sample Anti-Aliasing)
    set_property(TARGET HistoryVolumeCurve APPEND PROPERTY COMPILE_DEFINITIONS
        JUCE_OPENGL_MULTISAMPLING=1
        JUCE_OPENGL3=1
    )
endif()

# ==========================================================
# Optimization encouraged
# ==========================================================
target_compile_features(HistoryVolumeCurve PRIVATE cxx_std_17)
