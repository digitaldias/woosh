cmake_minimum_required(VERSION 3.21)
project(Woosh VERSION 0.1.0 LANGUAGES CXX)

# vcpkg toolchain auto-detect
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
  add_compile_options(/permissive- /W4 /EHsc /Zc:__cplusplus)
  add_link_options(/INCREMENTAL)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Qt6 COMPONENTS Widgets Multimedia REQUIRED)
find_package(SndFile CONFIG REQUIRED)
find_package(mpg123 CONFIG REQUIRED)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(WOOSH_SOURCES
  ${SRC_ROOT}/main.cpp
  ${SRC_ROOT}/ui/MainWindow.cpp
  ${SRC_ROOT}/ui/BatchDialog.cpp
  ${SRC_ROOT}/ui/WaveformView.cpp
  ${SRC_ROOT}/audio/AudioEngine.cpp
  ${SRC_ROOT}/audio/AudioClip.cpp
  ${SRC_ROOT}/audio/Formats/WavCodec.cpp
  ${SRC_ROOT}/audio/Formats/Mp3Codec.cpp
  ${SRC_ROOT}/utils/FileScanner.cpp
  ${SRC_ROOT}/utils/DSP.cpp
)

add_executable(Woosh WIN32 ${WOOSH_SOURCES})
target_include_directories(Woosh PRIVATE ${SRC_ROOT})
target_link_libraries(Woosh PRIVATE Qt6::Widgets Qt6::Multimedia SndFile::sndfile MPG123::libmpg123)
target_compile_definitions(Woosh PRIVATE QT_NO_KEYWORDS)
set_target_properties(Woosh PROPERTIES
  WIN32_EXECUTABLE TRUE
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

enable_testing()
add_executable(WooshTests ${SRC_ROOT}/tests/AudioEngineTests.cpp
  ${SRC_ROOT}/audio/AudioEngine.cpp
  ${SRC_ROOT}/audio/AudioClip.cpp
  ${SRC_ROOT}/audio/Formats/WavCodec.cpp
  ${SRC_ROOT}/audio/Formats/Mp3Codec.cpp
  ${SRC_ROOT}/utils/DSP.cpp
)
target_include_directories(WooshTests PRIVATE ${SRC_ROOT})
target_link_libraries(WooshTests PRIVATE SndFile::sndfile MPG123::libmpg123)
add_test(NAME WooshTests COMMAND WooshTests)

install(TARGETS Woosh
  RUNTIME DESTINATION bin
  BUNDLE DESTINATION .
)

message(STATUS "Woosh configured with Qt6, libsndfile, mpg123")



