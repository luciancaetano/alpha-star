set(ALPHA_TEST_SOURCES
  test_main.cpp
  core/audio_test.cpp
  core/input_test.cpp
  core/time_test.cpp
  core/values_test.cpp
  core/version_test.cpp)
set(ALPHA_TEST_LIBS alpha_core)

if(ALPHA_WITH_RUNTIME_DEPS)
  list(APPEND ALPHA_TEST_SOURCES
    ecs/world_test.cpp
    physics/physics_world_test.cpp
    platform/assets_test.cpp
    platform/sdl_input_test.cpp
    render/renderer_test.cpp
    render/tessellate_test.cpp
    script/error_overlay_test.cpp
    script/host_test.cpp
    script/host_limits_test.cpp
    script/host_fixture.cpp)
  set(ALPHA_TEST_LIBS alpha_engine)
endif()

add_executable(alpha_tests ${ALPHA_TEST_SOURCES})
target_link_libraries(alpha_tests PRIVATE ${ALPHA_TEST_LIBS} doctest::doctest)
target_compile_definitions(alpha_tests PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)
alpha_set_warnings(alpha_tests)
add_test(NAME alpha_tests COMMAND alpha_tests)
