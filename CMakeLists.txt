cmake_minimum_required(VERSION 3.28)
project(alpha_star VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Release: static C/C++ runtime, so the shipped alpha_star.exe needs no VC++
# redistributable. Debug keeps the DLL runtime, which the coverage collector needs.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

option(ALPHA_WITH_RUNTIME_DEPS "Fetch SDL3, Box2D, QuickJS-ng, EnTT" ON)
option(ALPHA_BUILD_TESTS "Build tests" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(Warnings)
include(Dependencies)

add_subdirectory(src)

if(ALPHA_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
