# SDL_shadercross CLI (`shadercross`): HLSL -> SPIR-V / DXIL / MSL for custom
# shaders (assets.shaders). A dev/build-time TOOL next to the engine, never linked
# into it: the release engine runs it to compile on load (ALPHA_DEBUG_TOOLS) and
# `alpha export` runs it to put blobs into packs. Shipping and Android engines
# never build it.
# Dependencies: SPIRV-Cross from source (static, small), DXC as Microsoft's
# prebuilt release (building DXC = LLVM from source takes ~1 h and GBs).
# Pins are shadercross's own (its download script and submodule commits).
option(ALPHA_SHADERCROSS "Build the shadercross tool (custom shader compiler)" ON)
if(NOT ALPHA_SHADERCROSS OR ALPHA_SHIPPING OR ANDROID OR APPLE)
  return()
endif()

if(WIN32)
  FetchContent_Declare(dxc_bin
    URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2602/dxc_2026_02_20.zip
    URL_HASH SHA256=a1e89031421cf3c1fca6627766ab3020ca4f962ac7e2caa7fab2b33a8436151e)
else()
  FetchContent_Declare(dxc_bin
    URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2602/linux_dxc_2026_02_20.x86_64.tar.gz
    URL_HASH SHA256=a1d3e3b5e1c5685b3eb27d5e8890e41d87df45def05112a2d6f1a63a931f7d60)
endif()
FetchContent_MakeAvailable(dxc_bin)

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
add_library(spirv_cross_c ALIAS spirv-cross-c)  # what shadercross looks for before find_package
# shadercross exports its static library for install; its link deps must be exported too.
set(spirv_targets)
foreach(t c core glsl hlsl msl cpp reflect util)
  if(TARGET spirv-cross-${t})
    list(APPEND spirv_targets spirv-cross-${t})
  endif()
endforeach()
export(TARGETS ${spirv_targets} FILE alpha-spirv-cross-targets.cmake)

set(SDLSHADERCROSS_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_STATIC ON CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_CLI ON CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_CLI_STATIC ON CACHE BOOL "" FORCE)
set(SDLSHADERCROSS_INSTALL OFF CACHE BOOL "" FORCE)
# shadercross points its finder at its own folder: hand it the prebuilt's paths as cache results.
set(dxc ${dxc_bin_SOURCE_DIR})
find_path(DirectXShaderCompiler_INCLUDE_PATH dxcapi.h PATHS ${dxc}/inc ${dxc}/include ${dxc}/include/dxc NO_DEFAULT_PATH)
if(WIN32)
  find_file(DirectXShaderCompiler_dxcompiler_BINARY dxcompiler.dll PATHS ${dxc}/bin/x64 NO_DEFAULT_PATH)
  find_library(DirectXShaderCompiler_dxcompiler_LIBRARY dxcompiler PATHS ${dxc}/lib/x64 NO_DEFAULT_PATH)
  find_file(DirectXShaderCompiler_dxil_BINARY dxil.dll PATHS ${dxc}/bin/x64 NO_DEFAULT_PATH)
  set(dxc_runtime ${DirectXShaderCompiler_dxcompiler_BINARY} ${DirectXShaderCompiler_dxil_BINARY})
else()
  find_library(DirectXShaderCompiler_dxcompiler_LIBRARY dxcompiler PATHS ${dxc}/lib NO_DEFAULT_PATH)
  find_library(DirectXShaderCompiler_dxil_LIBRARY dxil PATHS ${dxc}/lib NO_DEFAULT_PATH)
  set(dxc_runtime ${DirectXShaderCompiler_dxcompiler_LIBRARY} ${DirectXShaderCompiler_dxil_LIBRARY})
endif()
FetchContent_Declare(shadercross
  GIT_REPOSITORY https://github.com/libsdl-org/SDL_shadercross.git
  GIT_TAG 1ff05bec573988a98ef9e0260b4da44f512b8367
  GIT_SUBMODULES ""  # dependencies come from above, not its vendored submodules
  SYSTEM EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(shadercross)

# The tool lands next to alpha_star with the DXC libraries it loads.
set_target_properties(shadercross PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:alpha_star> BUILD_RPATH "$ORIGIN")
add_custom_target(alpha_shadercross
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dxc_runtime} $<TARGET_FILE_DIR:alpha_star>
  DEPENDS shadercross
  COMMENT "shadercross + DXC next to the engine")
add_dependencies(alpha_star alpha_shadercross)  # building the engine builds its tool
