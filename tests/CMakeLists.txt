set(ALPHA_TEST_SOURCES
  test_main.cpp
  core/audio_test.cpp
  core/cli_test.cpp
  core/axes_test.cpp
  core/camera_test.cpp
  core/glyph_layout_test.cpp
  core/window_test.cpp
  core/particles_test.cpp
  core/input_test.cpp
  core/log_ring_test.cpp
  core/pack_watch_test.cpp
  core/source_map_test.cpp
  core/sprite_test.cpp
  core/time_test.cpp
  core/values_test.cpp
  core/version_test.cpp)
set(ALPHA_TEST_LIBS alpha_core)

if(ALPHA_WITH_RUNTIME_DEPS)
  list(APPEND ALPHA_TEST_SOURCES
    ecs/sprite_world_test.cpp
    ecs/world_test.cpp
    physics/contacts_test.cpp
    physics/physics_world_test.cpp
    physics/queries_joints_test.cpp
    platform/assets_test.cpp
    platform/frame_latency_test.cpp
    platform/gamepads_test.cpp
    platform/log_file_test.cpp
    platform/sdl_input_test.cpp
    platform/window_test.cpp
    render/effects_render_test.cpp
    render/renderer_test.cpp
    render/tessellate_test.cpp
    script/error_overlay_test.cpp
    script/host_test.cpp
    script/host_limits_test.cpp
    script/input_script_test.cpp
    script/hot_reload_test.cpp
    script/reload_test.cpp
    script/map_script_test.cpp
    script/simulate_test.cpp
    script/effects_script_test.cpp
    script/entity_links_test.cpp
    script/physics_script_test.cpp
    script/pixel_mode_test.cpp
    script/sprite_script_test.cpp
    script/test_report_test.cpp
    script/console_test.cpp
    script/host_fixture.cpp
    script/ui_script_test.cpp
    ui/pad_nav_test.cpp
    ui/ui_test.cpp)
  set(ALPHA_TEST_LIBS alpha_engine)
endif()

if(ALPHA_WITH_RUNTIME_DEPS AND ALPHA_DEVTOOLS)
  list(APPEND ALPHA_TEST_SOURCES
    devtools/dev_tools_test.cpp
    devtools/inspector_test.cpp
    devtools/routing_test.cpp
    devtools/snapshot_test.cpp
    devtools/time_control_test.cpp)
  list(APPEND ALPHA_TEST_LIBS alpha_devtools)
endif()

if(ALPHA_WITH_RUNTIME_DEPS AND ALPHA_MCP)
  list(APPEND ALPHA_TEST_SOURCES
    mcp/control_test.cpp
    mcp/dispatch_test.cpp
    mcp/game_session_test.cpp
    mcp/json_test.cpp
    mcp/mcp_fixture.cpp
    mcp/socket_test.cpp)
  list(APPEND ALPHA_TEST_LIBS alpha_mcp)
endif()

add_executable(alpha_tests ${ALPHA_TEST_SOURCES})
target_link_libraries(alpha_tests PRIVATE ${ALPHA_TEST_LIBS} doctest::doctest)
target_include_directories(alpha_tests PRIVATE script)
target_compile_definitions(alpha_tests PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS
  ALPHA_TEST_FONT="${PROJECT_SOURCE_DIR}/examples/flappy/assets/fonts/PressStart2P-Regular.ttf")
alpha_set_warnings(alpha_tests)
add_test(NAME alpha_tests COMMAND alpha_tests)
