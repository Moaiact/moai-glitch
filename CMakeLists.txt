cmake_minimum_required(VERSION 3.22)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS mínimo")
project(MoaiGlitch VERSION 1.1.0)

# JUCE 9.0.4 (se descarga solo; o usa -DJUCE_DIR=/ruta/a/JUCE)
include(FetchContent)
if(JUCE_DIR)
  add_subdirectory(${JUCE_DIR} JUCE)
else()
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 9.0.4 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

set(FORMATS VST3)
if(APPLE)
  list(APPEND FORMATS AU)
endif()

juce_add_plugin(MoaiGlitch
    COMPANY_NAME "Moaiact"
    BUNDLE_ID "com.moaiact.moaiglitch"
    PLUGIN_MANUFACTURER_CODE Moai
    PLUGIN_CODE MglA
    PRODUCT_NAME "Moai Glitch"
    DESCRIPTION "Tempo-synced stutter effect"
    VERSION 1.1.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
    FORMATS ${FORMATS}
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(MoaiGlitch PRIVATE
    Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/MoaiCommon.cpp)

juce_add_binary_data(MoaiGlitchData SOURCES Assets/knob_body.png Assets/title_chrome.png)
set_target_properties(MoaiGlitchData PROPERTIES POSITION_INDEPENDENT_CODE ON)
juce_generate_juce_header(MoaiGlitch)

target_compile_definitions(MoaiGlitch PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MoaiGlitch PRIVATE MoaiGlitchData
    juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
