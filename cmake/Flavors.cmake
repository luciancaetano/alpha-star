# Build flavors: one place turns CMake options into compile definitions.
# Every dev/debug/agent-only feature sits behind one of these flags and is
# absent from shipping builds (checked by tools/check_shipping.py).
#
#   ALPHA_DEVTOOLS     Dear ImGui engine dev tools (F1, --devtools)
#   ALPHA_MCP          MCP server for agents (step 6; nothing implements it yet)
#   ALPHA_DEBUG_TOOLS  headless/debug CLI flags (--frames --screenshot --tap
#                      --tap-every --devtools), the --test runner + alpha/test
#                      module and its natives, extra diagnostic logging
#   ALPHA_SHIPPING     umbrella for player builds: forces the three above OFF,
#                      IPO/LTO, stripped, no console window, asserts off;
#                      console.log goes to a log file in the per-user data dir
#
# Code tests them with `#if ALPHA_X` (never #ifdef): each is always defined as
# 0 or 1 on every target of ours (alpha_flavor is linked by alpha_core).
option(ALPHA_DEVTOOLS "Build the Dear ImGui dev tools into the engine" OFF)
option(ALPHA_MCP "Build the MCP server for agents" OFF)
option(ALPHA_DEBUG_TOOLS "Headless CLI flags, --test runner, alpha/test, diagnostics" ON)
option(ALPHA_SHIPPING "Player build: no dev/debug/agent features, LTO, stripped, no console" OFF)

if(ALPHA_SHIPPING)
  foreach(flag ALPHA_DEVTOOLS ALPHA_MCP ALPHA_DEBUG_TOOLS)
    set(${flag} OFF CACHE BOOL "forced OFF by ALPHA_SHIPPING" FORCE)
  endforeach()
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ALPHA_IPO OUTPUT ALPHA_IPO_WHY LANGUAGES C CXX)
  if(NOT ALPHA_IPO)
    message(STATUS "alpha-star: IPO/LTO not supported here: ${ALPHA_IPO_WHY}")
  endif()
endif()

add_library(alpha_flavor INTERFACE)
foreach(flag ALPHA_DEVTOOLS ALPHA_MCP ALPHA_DEBUG_TOOLS ALPHA_SHIPPING)
  target_compile_definitions(alpha_flavor INTERFACE ${flag}=$<BOOL:${${flag}}>)
endforeach()
if(ALPHA_SHIPPING)
  target_compile_definitions(alpha_flavor INTERFACE NDEBUG SDL_ASSERT_LEVEL=0)
endif()

# Per-target shipping settings, applied by alpha_set_warnings to every target of ours.
function(alpha_apply_flavor target)
  if(NOT ALPHA_SHIPPING)
    return()
  endif()
  if(ALPHA_IPO)
    set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
  endif()
  get_target_property(kind ${target} TYPE)
  if(kind STREQUAL "EXECUTABLE")
    # WIN32: no console window (SDL_main provides WinMain). Strip symbols; MSVC
    # Release links without /DEBUG, so no PDB is produced.
    set_property(TARGET ${target} PROPERTY WIN32_EXECUTABLE ON)
    if(MSVC)
      target_link_options(${target} PRIVATE /DEBUG:NONE)
    elseif(NOT APPLE)
      target_link_options(${target} PRIVATE -s)
    endif()
  endif()
endfunction()
