cmake_minimum_required(VERSION 3.21)
project(Woosh VERSION 0.2.0 LANGUAGES CXX)

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
  add_compile_options(/permissive- /W4 /EHsc /Zc:__cplusplus)
  add_link_options(/INCREMENTAL)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# Dependencies
# ============================================================================
find_package(Qt6 COMPONENTS Widgets Multimedia REQUIRED)
find_package(SndFile CONFIG REQUIRED)
find_package(mpg123 CONFIG REQUIRED)

# ============================================================================
# Source Files
# ============================================================================
set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(WOOSH_SOURCES
  ${SRC_ROOT}/main.cpp
  # UI components
  ${SRC_ROOT}/ui/MainWindow.cpp
  ${SRC_ROOT}/ui/ClipTableModel.cpp
  ${SRC_ROOT}/ui/ProcessingPanel.cpp
  ${SRC_ROOT}/ui/OutputPanel.cpp
  ${SRC_ROOT}/ui/TransportPanel.cpp
  ${SRC_ROOT}/ui/WaveformView.cpp
  ${SRC_ROOT}/ui/SettingsDialog.cpp
  # Audio core
  ${SRC_ROOT}/audio/AudioEngine.cpp
  ${SRC_ROOT}/audio/AudioClip.cpp
  ${SRC_ROOT}/audio/AudioPlayer.cpp
  ${SRC_ROOT}/audio/Formats/WavCodec.cpp
  ${SRC_ROOT}/audio/Formats/Mp3Codec.cpp
  # Utilities
  ${SRC_ROOT}/utils/FileScanner.cpp
  ${SRC_ROOT}/utils/DSP.cpp
  # Resources
  ${SRC_ROOT}/resources/woosh.qrc
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
  SndFile::sndfile 
  MPG123::libmpg123
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
# Tests
# ============================================================================
enable_testing()

add_executable(WooshTests 
  ${SRC_ROOT}/tests/AudioEngineTests.cpp
  ${SRC_ROOT}/audio/AudioEngine.cpp
  ${SRC_ROOT}/audio/AudioClip.cpp
  ${SRC_ROOT}/audio/Formats/WavCodec.cpp
  ${SRC_ROOT}/audio/Formats/Mp3Codec.cpp
  ${SRC_ROOT}/utils/DSP.cpp
)

target_include_directories(WooshTests PRIVATE 
  ${SRC_ROOT}
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(WooshTests PRIVATE 
  SndFile::sndfile 
  MPG123::libmpg123
)

add_test(NAME WooshTests COMMAND WooshTests)

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
