
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum macOS version required")

cmake_minimum_required(VERSION 3.15)
project(SimpleReverb VERSION 1.0.0)
add_subdirectory(External/JUCE)

set(formats "VST3")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND formats "Standalone")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" AND APPLE)
    list(APPEND formats "AU")
endif()

juce_add_plugin(SimpleReverb
    VERSION "1.0.0"
    COMPANY_NAME "VeraNois"
    COMPANY_WEBSITE "veranois.online"
    COMPANY_EMAIL "veranoismusica@gmail.com"
    PLUGIN_MANUFACTURER_CODE "Vera"
    PLUGIN_CODE "Vera"
    FORMATS ${formats}
    VST3_CATEGORIES "Fx"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    PLUGIN_NAME "Reververa"
    PRODUCT_NAME "Reververa")

add_subdirectory(Source)
set(SourceFiles
        Source/EditorContent.cpp
        Source/SimpleReverbEditor.cpp
        Source/SimpleReverbProcessor.cpp
        Source/EditorContent.h
        Source/SimpleReverbEditor.h
        Source/SimpleReverbProcessor.h
        Source/ParamNames.h)

target_sources(SimpleReverb PRIVATE ${SourceFiles})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX Source FILES ${SourceFiles})

#target_sources(SimpleReverb
#    PRIVATE
#        EditorContent.cpp
#        SimpleReverbEditor.cpp
#        SimpleReverbProcessor.cpp)

add_subdirectory(Source/GUI)

juce_add_binary_data(SimpleReverbData SOURCES "Resources/pluginBackground.png")

juce_generate_juce_header(SimpleReverb)

target_compile_features(SimpleReverb PUBLIC cxx_std_17)

target_compile_definitions(SimpleReverb
    PUBLIC
        JUCE_ENABLE_REPAINT_DEBUGGING=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SimpleReverb
    PRIVATE
        SimpleReverbData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
