\
cmake_minimum_required(VERSION 3.15)
project(HistoryVolumeCurve VERSION 0.0.3)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 7.0.10
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(HistoryVolumeCurve
    PLUGIN_NAME "HistoryVolumeCurve"
    COMPANY_NAME "YourCompany"
    BUNDLE_ID com.yourcompany.historyvolumecurve
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS VST3
    PRODUCT_NAME "HistoryVolumeCurve"
)

target_sources(HistoryVolumeCurve PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(HistoryVolumeCurve
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_OPENGL_MULTISAMPLING=1
)

target_link_libraries(HistoryVolumeCurve PRIVATE
    juce::juce_audio_utils
    juce::juce_opengl
)
