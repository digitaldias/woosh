cmake_minimum_required(VERSION 3.21)
project(Woosh VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Version and Product Information
# ============================================================================
set(WOOSH_PRODUCT_NAME "Woosh")
set(WOOSH_DESCRIPTION "Audio Batch Editor for Game Developers")
set(WOOSH_AUTHOR "Pedro G. Dias")
set(WOOSH_AUTHOR_HANDLE "digitaldias")
set(WOOSH_COPYRIGHT "Copyright (c) 2025 ${WOOSH_AUTHOR} (${WOOSH_AUTHOR_HANDLE})")
set(WOOSH_HOMEPAGE "https://github.com/digitaldias/woosh")

# Generate version header
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h"
  @ONLY
)

# ============================================================================
# vcpkg toolchain auto-detect
# ============================================================================
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" 
      CACHE STRING "Vcpkg toolchain file")
endif()

# ============================================================================
# Compiler Settings
# ============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
  # Use all available CPU cores for compilation
  add_compile_options(/MP)
  
  # Common flags
  add_compile_options(/permissive- /W4 /EHsc /Zc:__cplusplus)
  
  # Configuration-specific optimizations
  # Debug: Keep incremental linking for fast iteration
  add_link_options($<$<CONFIG:Debug>:/INCREMENTAL>)
  
  # Release: Maximum speed optimizations for audio processing
  add_compile_options($<$<CONFIG:Release>:/O2>)           # Maximum speed optimization
  add_compile_options($<$<CONFIG:Release>:/Oi>)           # Enable intrinsic functions
  add_compile_options($<$<CONFIG:Release>:/Ot>)           # Favor fast code
  add_compile_options($<$<CONFIG:Release>:/GL>)           # Whole program optimization
  add_compile_options($<$<CONFIG:Release>:/fp:fast>)      # Fast floating-point (great for audio DSP)
  add_compile_options($<$<CONFIG:Release>:/arch:AVX2>)    # Enable AVX2 SIMD instructions
  add_link_options($<$<CONFIG:Release>:/LTCG>)            # Link-time code generation
  add_link_options($<$<CONFIG:Release>:/OPT:REF>)         # Remove unreferenced code
  add_link_options($<$<CONFIG:Release>:/OPT:ICF>)         # Identical COMDAT folding
  
  # RelWithDebInfo: Speed with debug symbols
  add_compile_options($<$<CONFIG:RelWithDebInfo>:/O2>)
  add_compile_options($<$<CONFIG:RelWithDebInfo>:/Oi>)
  add_compile_options($<$<CONFIG:RelWithDebInfo>:/fp:fast>)
  
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
  
  # Release optimizations for Clang/GCC
  add_compile_options($<$<CONFIG:Release>:-O3>)
  add_compile_options($<$<CONFIG:Release>:-ffast-math>)
  add_compile_options($<$<CONFIG:Release>:-march=native>)
  add_compile_options($<$<CONFIG:Release>:-flto>)
  add_link_options($<$<CONFIG:Release>:-flto>)
endif()

# ============================================================================
# Dependencies
# ============================================================================
find_package(Qt6 COMPONENTS Widgets Multimedia Concurrent REQUIRED)
find_package(SndFile CONFIG REQUIRED)
find_package(mpg123 CONFIG REQUIRED)
find_package(mp3lame CONFIG REQUIRED)

# mpg123 target name varies by vcpkg version
if(TARGET MPG123::libmpg123)
  set(MPG123_TARGET MPG123::libmpg123)
elseif(TARGET MPG123::mpg123)
  set(MPG123_TARGET MPG123::mpg123)
elseif(TARGET mpg123::mpg123)
  set(MPG123_TARGET mpg123::mpg123)
else()
  message(FATAL_ERROR "Could not find mpg123 target. Available targets from mpg123 package not recognized.")
endif()
message(STATUS "Using mpg123 target: ${MPG123_TARGET}")

# ============================================================================
# Source Files
# ============================================================================
set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(WOOSH_SOURCES
  ${SRC_ROOT}/main.cpp
  # Core
  ${SRC_ROOT}/core/Project.cpp
  ${SRC_ROOT}/core/ProjectManager.cpp
  # UI components
  ${SRC_ROOT}/ui/MainWindow.cpp
  ${SRC_ROOT}/ui/ClipTableModel.cpp
  ${SRC_ROOT}/ui/ProcessingPanel.cpp
  ${SRC_ROOT}/ui/OutputPanel.cpp
  ${SRC_ROOT}/ui/TransportPanel.cpp
  ${SRC_ROOT}/ui/ToggleSwitch.cpp
  ${SRC_ROOT}/ui/WaveformView.cpp
  ${SRC_ROOT}/ui/SettingsDialog.cpp
  ${SRC_ROOT}/ui/VuMeterWidget.cpp
  ${SRC_ROOT}/ui/NewProjectDialog.cpp
  ${SRC_ROOT}/ui/ProjectSettingsDialog.cpp
  # Audio core
  ${SRC_ROOT}/audio/AudioEngine.cpp
  ${SRC_ROOT}/audio/AudioClip.cpp
  ${SRC_ROOT}/audio/AudioPlayer.cpp
  ${SRC_ROOT}/audio/Formats/WavCodec.cpp
  ${SRC_ROOT}/audio/Formats/Mp3Codec.cpp
  ${SRC_ROOT}/audio/Formats/Mp3Encoder.cpp
  # Utilities
  ${SRC_ROOT}/utils/FileScanner.cpp
  ${SRC_ROOT}/utils/DSP.cpp
  # Resources
  ${SRC_ROOT}/resources/woosh.qrc
)

set(WOOSH_TEST_SOURCES
  ${SRC_ROOT}/tests/AudioClipTests.cpp
  ${SRC_ROOT}/tests/AudioEngineTests.cpp
  ${SRC_ROOT}/tests/ProjectTests.cpp
  ${SRC_ROOT}/tests/DSPTests.cpp
  ${SRC_ROOT}/tests/WaveformViewHelpersTests.cpp
)

# ============================================================================
# Platform-specific Resources
# ============================================================================
if(WIN32)
  # Generate Windows RC file with version info
  configure_file(
    "${SRC_ROOT}/media/woosh.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/woosh.rc"
    @ONLY
  )
  set(WOOSH_RC "${CMAKE_CURRENT_BINARY_DIR}/woosh.rc")
  
  # Copy icon to build directory (RC file references it relatively)
  file(COPY "${SRC_ROOT}/media/Woosh_icon.ico" 
       DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif()

if(APPLE)
  set(MACOSX_BUNDLE_ICON_FILE Woosh.icns)
  set(MACOSX_BUNDLE_BUNDLE_NAME ${WOOSH_PRODUCT_NAME})
  set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION} - ${WOOSH_DESCRIPTION}")
  set(MACOSX_BUNDLE_COPYRIGHT ${WOOSH_COPYRIGHT})
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.digitaldias.woosh")
  
  # Icon file for macOS bundle (if exists)
  if(EXISTS "${SRC_ROOT}/media/Woosh.icns")
    set(WOOSH_ICON "${SRC_ROOT}/media/Woosh.icns")
    set_source_files_properties(${WOOSH_ICON} PROPERTIES 
      MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND WOOSH_SOURCES ${WOOSH_ICON})
  endif()
endif()

# ============================================================================
# Main Executable
# ============================================================================
add_executable(Woosh WIN32 MACOSX_BUNDLE ${WOOSH_SOURCES} ${WOOSH_RC})

target_include_directories(Woosh PRIVATE 
  ${SRC_ROOT}
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(Woosh PRIVATE 
  Qt6::Widgets 
  Qt6::Multimedia 
  Qt6::Concurrent
  SndFile::sndfile 
  ${MPG123_TARGET}
  mp3lame::mp3lame
)

target_compile_definitions(Woosh PRIVATE 
  QT_NO_KEYWORDS
  WOOSH_VERSION="${PROJECT_VERSION}"
)

set_target_properties(Woosh PROPERTIES
  WIN32_EXECUTABLE TRUE
  MACOSX_BUNDLE TRUE
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_NAME "Woosh"
)

# ============================================================================
# Deploy Qt DLLs (Windows)
# ============================================================================
if(WIN32)
  # Find windeployqt executable
  get_target_property(_qt6_qmake_location Qt6::qmake IMPORTED_LOCATION)
  get_filename_component(_qt6_bin_dir "${_qt6_qmake_location}" DIRECTORY)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt6_bin_dir}" NO_CMAKE_FIND_ROOT_PATH)
  
  if(WINDEPLOYQT_EXECUTABLE)
    # Use Qt's deployment tool to copy required DLLs after build
    add_custom_command(TARGET Woosh POST_BUILD
      COMMAND "${WINDEPLOYQT_EXECUTABLE}"
              $<$<CONFIG:Debug>:--debug>
              $<$<CONFIG:Release>:--release>
              --no-translations
              --no-system-d3d-compiler
              --no-opengl-sw
              "$<TARGET_FILE:Woosh>"
      COMMENT "Deploying Qt runtime dependencies..."
    )
  else()
    message(WARNING "windeployqt not found - Qt DLLs will not be automatically deployed")
  endif()
endif()

# ============================================================================
# Tests
# ============================================================================
enable_testing()

# Common test sources (shared dependencies)
set(TEST_COMMON_SOURCES
  ${SRC_ROOT}/audio/AudioEngine.cpp
  ${SRC_ROOT}/audio/AudioClip.cpp
  ${SRC_ROOT}/audio/Formats/WavCodec.cpp
  ${SRC_ROOT}/audio/Formats/Mp3Codec.cpp
  ${SRC_ROOT}/audio/Formats/Mp3Encoder.cpp
  ${SRC_ROOT}/utils/DSP.cpp
)

# --- AudioEngine Tests ---
add_executable(AudioEngineTests 
  ${SRC_ROOT}/tests/AudioEngineTests.cpp
  ${TEST_COMMON_SOURCES}
)
target_include_directories(AudioEngineTests PRIVATE 
  ${SRC_ROOT}
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_link_libraries(AudioEngineTests PRIVATE 
  SndFile::sndfile 
  ${MPG123_TARGET}
  mp3lame::mp3lame
)
add_test(NAME AudioEngineTests COMMAND AudioEngineTests)

# --- DSP Tests ---
add_executable(DSPTests 
  ${SRC_ROOT}/tests/DSPTests.cpp
  ${SRC_ROOT}/utils/DSP.cpp
)
target_include_directories(DSPTests PRIVATE 
  ${SRC_ROOT}
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_link_libraries(DSPTests PRIVATE)
add_test(NAME DSPTests COMMAND DSPTests)

# --- AudioClip Tests ---
add_executable(AudioClipTests 
  ${SRC_ROOT}/tests/AudioClipTests.cpp
  ${SRC_ROOT}/audio/AudioClip.cpp
)
target_include_directories(AudioClipTests PRIVATE 
  ${SRC_ROOT}
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_link_libraries(AudioClipTests PRIVATE)
add_test(NAME AudioClipTests COMMAND AudioClipTests)

# --- Project Tests ---
add_executable(ProjectTests 
  ${SRC_ROOT}/tests/ProjectTests.cpp
  ${SRC_ROOT}/core/Project.cpp
)
target_include_directories(ProjectTests PRIVATE 
  ${SRC_ROOT}
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_link_libraries(ProjectTests PRIVATE)
add_test(NAME ProjectTests COMMAND ProjectTests)

# --- WaveformViewHelpers Tests ---
add_executable(WaveformViewHelpersTests 
  ${SRC_ROOT}/tests/WaveformViewHelpersTests.cpp
  ${SRC_ROOT}/ui/WaveformViewHelpers.cpp
)
target_include_directories(WaveformViewHelpersTests PRIVATE 
  ${SRC_ROOT}
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_link_libraries(WaveformViewHelpersTests PRIVATE)
add_test(NAME WaveformViewHelpersTests COMMAND WaveformViewHelpersTests)

# Aggregate target to build all tests
add_custom_target(WooshTests DEPENDS AudioEngineTests DSPTests AudioClipTests ProjectTests WaveformViewHelpersTests)

# ============================================================================
# Installation
# ============================================================================
install(TARGETS Woosh
  RUNTIME DESTINATION bin
  BUNDLE DESTINATION .
)

# ============================================================================
# CPack Configuration for Installers
# ============================================================================
set(CPACK_PACKAGE_NAME ${WOOSH_PRODUCT_NAME})
set(CPACK_PACKAGE_VENDOR ${WOOSH_AUTHOR_HANDLE})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${WOOSH_DESCRIPTION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "Pedro G. Dias <https://github.com/digitaldias>")
set(CPACK_PACKAGE_HOMEPAGE_URL ${WOOSH_HOMEPAGE})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# Package file name format
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

if(WIN32)
  # Windows: Use NSIS or ZIP
  set(CPACK_GENERATOR "ZIP;NSIS")
  set(CPACK_NSIS_DISPLAY_NAME ${WOOSH_PRODUCT_NAME})
  set(CPACK_NSIS_PACKAGE_NAME ${WOOSH_PRODUCT_NAME})
  set(CPACK_NSIS_HELP_LINK ${WOOSH_HOMEPAGE})
  set(CPACK_NSIS_URL_INFO_ABOUT ${WOOSH_HOMEPAGE})
  set(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
  set(CPACK_NSIS_MODIFY_PATH OFF)
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
  set(CPACK_NSIS_MUI_ICON "${SRC_ROOT}/media/Woosh_icon.ico")
  set(CPACK_NSIS_MUI_UNIICON "${SRC_ROOT}/media/Woosh_icon.ico")
elseif(APPLE)
  # macOS: Use DragNDrop DMG
  set(CPACK_GENERATOR "DragNDrop")
  set(CPACK_DMG_VOLUME_NAME ${WOOSH_PRODUCT_NAME})
  set(CPACK_DMG_FORMAT "UDBZ")
else()
  # Linux: Use TGZ and optionally AppImage
  set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

# ============================================================================
# Status Messages
# ============================================================================
message(STATUS "")
message(STATUS "=== Woosh Build Configuration ===")
message(STATUS "  Version:     ${PROJECT_VERSION}")
message(STATUS "  Author:      ${WOOSH_AUTHOR} (${WOOSH_AUTHOR_HANDLE})")
message(STATUS "  Platform:    ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:    ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Qt6:         Found")
message(STATUS "  libsndfile:  Found")
message(STATUS "  mpg123:      Found")
message(STATUS "=================================")
message(STATUS "")
