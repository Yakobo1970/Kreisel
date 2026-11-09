cmake_minimum_required(VERSION 3.22)
project(Kreisel VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_DIR "$ENV{USERPROFILE}/JUCE" CACHE PATH "Path to the JUCE source tree")
add_subdirectory(${JUCE_DIR} JUCE)

set(KREISEL_CORE
    Source/Sequencer.cpp
    Source/Presets.cpp
    Source/MidiExport.cpp
    Source/SoundEngine.cpp)

juce_add_plugin(Kreisel
    COMPANY_NAME "Un(d)AbtanzBAR"
    PRODUCT_NAME "Kreisel"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Yako
    PLUGIN_CODE KrsL
    VST3_CATEGORIES Instrument Generator
    FORMATS VST3 Standalone
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon.png"
    COPY_PLUGIN_AFTER_BUILD FALSE)

set(KREISEL_HOST
    Source/HostSettings.cpp
    Source/PluginHosting.cpp)

target_sources(Kreisel PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    ${KREISEL_HOST}
    ${KREISEL_CORE})

target_compile_definitions(Kreisel PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if (MSVC)
    target_compile_options(Kreisel PRIVATE /utf-8)
endif()

target_link_libraries(Kreisel
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Engine regression test (console, no plugin wrapper)
juce_add_console_app(EngineTest PRODUCT_NAME "EngineTest")
target_sources(EngineTest PRIVATE Tools/EngineTest.cpp ${KREISEL_CORE})
target_compile_definitions(EngineTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
if (MSVC)
    target_compile_options(EngineTest PRIVATE /utf-8)
endif()
target_link_libraries(EngineTest PRIVATE juce::juce_audio_basics juce::juce_data_structures
    PUBLIC juce::juce_recommended_config_flags)

# Renders the editor to a PNG (UI checks without a DAW)
juce_add_gui_app(UiShot PRODUCT_NAME "UiShot")
target_sources(UiShot PRIVATE Tools/UiShot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp ${KREISEL_HOST} ${KREISEL_CORE})
target_compile_definitions(UiShot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1
    JucePlugin_Name="Kreisel" JucePlugin_VersionString="${PROJECT_VERSION}")
if (MSVC)
    target_compile_options(UiShot PRIVATE /utf-8)
endif()
target_link_libraries(UiShot PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)

# Minimal VST3 host test (loads the built plugin like a DAW)
juce_add_console_app(HostTest PRODUCT_NAME "HostTest")
target_sources(HostTest PRIVATE Tools/HostTest.cpp)
target_compile_definitions(HostTest PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(HostTest PRIVATE juce::juce_audio_processors juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags)

# Out-of-process plug-in scanner (one process per plug-in file; crashes cannot take Kreisel down)
juce_add_console_app(KreiselScanner PRODUCT_NAME "KreiselScanner")
target_sources(KreiselScanner PRIVATE Tools/KreiselScanner.cpp)
target_compile_definitions(KreiselScanner PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(KreiselScanner PRIVATE juce::juce_audio_processors PUBLIC juce::juce_recommended_config_flags)
if (MSVC)
    set_property(TARGET KreiselScanner PROPERTY LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:wmainCRTStartup")
endif()
# built after the plug-in, then copied next to the standalone and into the VST3 bundle
add_dependencies(KreiselScanner Kreisel_VST3 Kreisel_Standalone)
add_custom_command(TARGET KreiselScanner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/Kreisel_artefacts/$<CONFIG>/VST3/Kreisel.vst3/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:KreiselScanner>" "${CMAKE_BINARY_DIR}/Kreisel_artefacts/$<CONFIG>/VST3/Kreisel.vst3/Contents/Resources/"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:KreiselScanner>" "${CMAKE_BINARY_DIR}/Kreisel_artefacts/$<CONFIG>/Standalone/")
