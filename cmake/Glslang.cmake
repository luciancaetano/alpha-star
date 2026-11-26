# glslang's standalone compiler (`glslang`): Vulkan GLSL fragment shaders ->
# SPIR-V for custom shaders (assets.shaders). A dev/build-time TOOL next to the
# engine, never linked into it: the release engine runs it to compile on load
# (ALPHA_DEBUG_TOOLS) and `alpha export` runs it to put .spv files into packs.
# Shipping and Android engines never build it. Built from source at a pinned
# tag, without SPIRV-Tools (no optimizer) and without its HLSL front end.
option(ALPHA_GLSLANG "Build the glslang tool (custom shader compiler)" ON)
if(NOT ALPHA_GLSLANG OR ALPHA_SHIPPING OR ANDROID OR APPLE)
  return()
endif()

set(ENABLE_GLSLANG_BINARIES ON CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_JS OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glslang
  GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
  GIT_TAG 16.6.0
  GIT_SHALLOW TRUE
  SYSTEM EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(glslang)

# The tool lands next to alpha_star; building the engine builds its tool.
set_target_properties(glslang-standalone PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:alpha_star>)
add_dependencies(alpha_star glslang-standalone)
