# core: pure, dependency-free domain code. Tested by the fast harness preset.
add_library(alpha_core
  core/axes.cpp
  core/cli.cpp
  core/color.cpp
  core/controls.cpp
  core/fixed_step.cpp
  core/input.cpp
  core/log_ring.cpp
  core/mixer.cpp
  core/pad_slots.cpp
  core/random.cpp
  core/source_map.cpp
  core/synth.cpp
  core/tags.cpp
  core/timers.cpp
  core/version.cpp)
target_include_directories(alpha_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(alpha_core PUBLIC ALPHA_VERSION="${PROJECT_VERSION}")
target_link_libraries(alpha_core PUBLIC alpha_flavor)
alpha_set_warnings(alpha_core)

if(NOT ALPHA_WITH_RUNTIME_DEPS)
  return()
endif()

# Script runtime JS is embedded in the binary: the player ships one executable.
set(ALPHA_JS_FILES alpha alpha_test runtime)
foreach(js ${ALPHA_JS_FILES})
  file(READ script/${js}.js ALPHA_JS_${js})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS script/${js}.js)
endforeach()
configure_file(script/embedded_js.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/script/embedded_js.hpp @ONLY)

# engine: everything the binary runs. Games are JS; nothing game-specific here.
add_library(alpha_engine
  ecs/entity_view.cpp
  ecs/world.cpp
  physics/physics_world.cpp
  platform/assets.cpp
  platform/audio_out.cpp
  platform/frame_latency.cpp
  platform/gamepads.cpp
  platform/log_file.cpp
  platform/sdl_input.cpp
  render/renderer.cpp
  render/tessellate.cpp
  render/text_layout.cpp
  script/asset_bindings.cpp
  script/error_overlay.cpp
  script/host.cpp
  script/bundle_map.cpp
  script/input_bindings.cpp
  script/instance_bindings.cpp
  script/instances.cpp
  script/js_engine.cpp
  script/js_value.cpp
  script/spec_parser.cpp
  script/synth_parser.cpp
  script/system_bindings.cpp
  script/ui_bindings.cpp
  script/world_bindings.cpp
  ui/input.cpp
  ui/library.cpp
  ui/pad_nav.cpp
  ui/rml_renderer.cpp
  ui/ui.cpp)
target_include_directories(alpha_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(alpha_engine PUBLIC alpha_core SDL3::SDL3 box2d::box2d EnTT::EnTT qjs SDL3_image::SDL3_image
  RmlUi::Core alpha_rmlui_sdl)
alpha_set_warnings(alpha_engine)

# devtools: Dear ImGui engine dev tools (F1). Only with ALPHA_DEVTOOLS; a
# release build compiles and links nothing of it.
if(ALPHA_DEVTOOLS)
  add_library(alpha_devtools
    devtools/dev_tools.cpp
    devtools/inspector.cpp
    devtools/panels.cpp
    devtools/routing.cpp
    devtools/snapshot.cpp
    devtools/time_control.cpp)
  target_link_libraries(alpha_devtools PUBLIC alpha_engine PRIVATE alpha_imgui)
  alpha_set_warnings(alpha_devtools)
endif()

# mcp: the agent control channel (--control PORT). Only with ALPHA_MCP; shipping
# compiles and links nothing of it.
if(ALPHA_MCP)
  add_library(alpha_mcp
    mcp/agent_input.cpp
    mcp/base64.cpp
    mcp/control.cpp
    mcp/dispatch.cpp
    mcp/game_session.cpp
    mcp/json.cpp
    mcp/socket.cpp
    mcp/state_json.cpp)
  target_link_libraries(alpha_mcp PUBLIC alpha_engine)
  if(WIN32)
    target_link_libraries(alpha_mcp PRIVATE ws2_32)
  endif()
  alpha_set_warnings(alpha_mcp)
endif()

add_executable(alpha_star main.cpp)
target_link_libraries(alpha_star PRIVATE alpha_engine $<$<BOOL:${ALPHA_DEVTOOLS}>:alpha_devtools>
  $<$<BOOL:${ALPHA_MCP}>:alpha_mcp>)
alpha_set_warnings(alpha_star)
