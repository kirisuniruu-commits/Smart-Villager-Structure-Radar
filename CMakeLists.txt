cmake_minimum_required(VERSION 3.22)
project(villager_radar LANGUAGES CXX)

# Smart Villager & Structure Radar - created by ChrisNiruu

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# NDK 30's libc++ no longer includes <cstdlib> transitively, which breaks fmt (malloc/free undeclared).
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include cstdlib")

option(RADAR_USE_SCHEMA "Publish the rich config schema (sections + credits node)" OFF)
option(RADAR_BUILD_TESTS "Build host-side unit tests (non-Android only)" OFF)

# Pure logic tests run on a PC: cmake -S . -B build-host -DRADAR_BUILD_TESTS=ON
if(RADAR_BUILD_TESTS AND NOT ANDROID)
  enable_testing()
  add_executable(radar_tests tests/test_radar.cpp)
  target_include_directories(radar_tests PRIVATE include)
  add_test(NAME radar_tests COMMAND radar_tests)
  return()
endif()

if(NOT ANDROID)
  message(FATAL_ERROR "Build with the Android NDK toolchain (arm64-v8a), or use -DRADAR_BUILD_TESTS=ON on a PC.")
endif()

set(PRELOADER_ANDROID_ROOT "${CMAKE_CURRENT_LIST_DIR}/third_party/preloader-android"
    CACHE PATH "preloader-android checkout (only include/pl is needed)")
if(NOT EXISTS "${PRELOADER_ANDROID_ROOT}/include/pl/ModMenu.hpp")
  message(FATAL_ERROR "Missing ${PRELOADER_ANDROID_ROOT}/include/pl/ModMenu.hpp")
endif()

include(FetchContent)
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 11.2.0)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(fmt)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/${ANDROID_ABI}")

add_library(villager_radar SHARED src/RadarMod.cpp src/GameBridge.cpp)
target_include_directories(villager_radar PRIVATE
  include src "${PRELOADER_ANDROID_ROOT}/include")
target_compile_options(villager_radar PRIVATE -Wall -Wextra -fvisibility=hidden -fvisibility-inlines-hidden)
if(RADAR_USE_SCHEMA)
  target_compile_definitions(villager_radar PRIVATE RADAR_USE_SCHEMA=1)
endif()
find_library(LOG_LIB log)
target_link_libraries(villager_radar PRIVATE fmt::fmt ${LOG_LIB})
# preloader symbols (pl::modmenu::*, pl::memory::*) are resolved by the launcher at load time
target_link_options(villager_radar PRIVATE -Wl,--unresolved-symbols=ignore-all)
