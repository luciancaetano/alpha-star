# SDL_shadercross's CLI (`shadercross`): the SPIR-V that glslang makes -> MSL (Metal) for
# custom shaders (assets.shaders). A dev/build-time TOOL next to the engine, never linked
# into it, like glslang: the release engine runs it to compile on load (ALPHA_DEBUG_TOOLS)
# and `alpha export` runs it to put .msl files into packs. Shipping and Android engines
# never build it. Built WITHOUT DXC (SDLSHADERCROSS_DXC OFF): its SPIR-V input path needs
# only SPIRV-Cross, so no HLSL input and no DXIL output. A D3D12/Xbox target would add
# SPIR-V -> HLSL here (shadercross does it without DXC) and the platform's own compiler.
# Pins: shadercross main and the SPIRV-Cross commit of its submodule.
option(ALPHA_SHADERCROSS "Build the shadercross tool (SPIR-V -> MSL for custom shaders)" ON)
if(NOT ALPHA_SHADERCROSS OR ALPHA_SHIPPING OR ANDROID OR APPLE)
  return()
endif()

set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(spirv_cross
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
  GIT_TAG 1a6169566c73d3da552748fc372fe2bbb856e46e
  SYSTEM EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(spirv_cross)
add_library(spirv_cross_c ALIAS spirv-cross-c)  # the name shadercross takes before find_package
# shadercross exports its static library (build tree); its link dependencies must be exported too.
set(spirv_targets)
foreach(t c core glsl hlsl msl cpp reflect util)
  if(TARGET spirv-cross-${t})
    list(APPEND spirv_targets spirv-cross-${t})
  endif()
endforeach()
export(TARGETS ${spirv_targets} FILE alpha-spirv-cross-targets.cmake)

set(SDLSHADERCROSS_DXC OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_STATIC ON CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_CLI ON CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_CLI_STATIC ON CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_INSTALL OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(shadercross
  GIT_REPOSITORY https://github.com/libsdl-org/SDL_shadercross.git
  GIT_TAG 1ff05bec573988a98ef9e0260b4da44f512b8367
  GIT_SUBMODULES ""  # SPIRV-Cross comes from above; DXC and SPIRV-Tools are not needed
  SYSTEM EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(shadercross)

# The tool lands next to alpha_star; building the engine builds its tool.
set_target_properties(shadercross PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:alpha_star>)
add_dependencies(alpha_star shadercross)
