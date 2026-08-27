cmake_minimum_required(VERSION 3.22)

# ------------------------------------------------------------
# Project configuration
# ------------------------------------------------------------

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13"
    CACHE STRING "Minimum OS X deployment target"
)

set(CMAKE_MSVC_RUNTIME_LIBRARY
    "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

project(Sampler VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Organise generated targets/files in IDEs such as Xcode.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Show JUCE modules organised into groups in Xcode.
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON
    CACHE BOOL "Enable JUCE module source groups"
    FORCE
)

# Generate Xcode schemes for CMake targets.
set(CMAKE_XCODE_GENERATE_SCHEME ON)


# ------------------------------------------------------------
# Debug symbols
# ------------------------------------------------------------

# Generate PDB/dSYM files for Release builds.

if(MSVC)

    add_compile_options("$<$<NOT:$<CONFIG:Debug>>:/Zi>")

    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/DEBUG>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:REF>")
    add_link_options("$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>")

else()

    set(CMAKE_CXX_FLAGS_RELEASE
        "${CMAKE_CXX_FLAGS_RELEASE} -g"
    )

    set(
        CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Release]
        "YES"
    )

    set(
        CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release]
        "dwarf-with-dsym"
    )

    set(
        CMAKE_XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release]
        "YES"
    )

endif()


# ------------------------------------------------------------
# CPM
# ------------------------------------------------------------

# We're going to use CPM as our package manager to bring in JUCE.
# Check to see if we have CPM installed already.
# Bring it in if we don't.

set(CPM_DOWNLOAD_VERSION 0.34.0)

set(
    CPM_DOWNLOAD_LOCATION
    "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake"
)

if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})

    message(
        STATUS
        "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}"
    )

    file(
        DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        ${CPM_DOWNLOAD_LOCATION}
    )

endif()

include(${CPM_DOWNLOAD_LOCATION})


# ------------------------------------------------------------
# JUCE
# ------------------------------------------------------------

CPMAddPackage(
    NAME JUCE
    GITHUB_REPOSITORY juce-framework/JUCE
    GIT_TAG 8.0.8
)

# ------------------------------------------------------------
# JUCE headers for Xcode indexing
# ------------------------------------------------------------

file(GLOB_RECURSE JUCE_HEADERS
    CONFIGURE_DEPENDS
    "${JUCE_SOURCE_DIR}/modules/*.h"
)

set_source_files_properties(
    ${JUCE_HEADERS}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

# ------------------------------------------------------------
# Plugin
# ------------------------------------------------------------

juce_add_plugin(${PROJECT_NAME}

    COMPANY_NAME "The Audio Programmer"

    PLUGIN_MANUFACTURER_CODE Tap1
    PLUGIN_CODE Smpl

    BUNDLE_ID "org.theaudioprogrammer.sampler"

    IS_SYNTH TRUE

    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    COPY_PLUGIN_AFTER_BUILD TRUE

    FORMATS
        AU
        VST3
        Standalone

    PRODUCT_NAME "Sampler"
)


# ------------------------------------------------------------
# Source files
# ------------------------------------------------------------

set(SOURCE_FILES

    sources/PluginEditor.h
    sources/PluginEditor.cpp

    sources/PluginProcessor.h
    sources/PluginProcessor.cpp

    sources/SamplerLookAndFeel.h
)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${SOURCE_FILES}
        ${JUCE_HEADERS}
)

# ------------------------------------------------------------
# Xcode source tree
# ------------------------------------------------------------

# Preserve the physical directory hierarchy in Xcode.
# Header and implementation files stay together.

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/sources"
    PREFIX "Source"
    FILES ${SOURCE_FILES}
)

source_group(
    TREE "${JUCE_SOURCE_DIR}/modules"
    PREFIX "JUCE"
    FILES ${JUCE_HEADERS}
)


# ------------------------------------------------------------
# JUCE generated header
# ------------------------------------------------------------

juce_generate_juce_header(${PROJECT_NAME})


# ------------------------------------------------------------
# Binary resources
# ------------------------------------------------------------

juce_add_binary_data(BinaryData

    SOURCES

        assets/sounds/c5.wav
        assets/sounds/d5.wav
        assets/sounds/e5.wav
        assets/sounds/f5.wav
        assets/sounds/g5.wav
        assets/sounds/a5.wav
        assets/sounds/b5.wav
        assets/sounds/c6.wav

        assets/images/BG_1376x1032_1.png
        assets/images/Knob_152x132_129.png

        assets/fonts/Inter-Medium.ttf
        assets/fonts/Inter-Regular.ttf
        assets/fonts/Inter-Semibold.ttf
)


# ------------------------------------------------------------
# Compile definitions
# ------------------------------------------------------------

target_compile_definitions(${PROJECT_NAME}

    PUBLIC

        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_MODAL_LOOPS_PERMITTED=0
        DONT_SET_USING_JUCE_NAMESPACE=1
)


# ------------------------------------------------------------
# Libraries
# ------------------------------------------------------------

target_link_libraries(${PROJECT_NAME}

    PRIVATE

        BinaryData

        juce::juce_animation
        juce::juce_audio_utils
        juce::juce_dsp

    PUBLIC

        juce::juce_recommended_warning_flags
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)
