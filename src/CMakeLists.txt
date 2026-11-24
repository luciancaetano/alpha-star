# core: pure, dependency-free domain code. Tested by the fast harness preset.
add_library(alpha_core
  core/version.cpp)
target_include_directories(alpha_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(alpha_core PUBLIC ALPHA_VERSION="${PROJECT_VERSION}")
alpha_set_warnings(alpha_core)

if(NOT ALPHA_WITH_RUNTIME_DEPS)
  return()
endif()

# runtime: modules that touch SDL3 / Box2D / QuickJS-ng / EnTT.
add_executable(alpha_star main.cpp)
target_link_libraries(alpha_star PRIVATE
  alpha_core SDL3::SDL3 box2d::box2d qjs EnTT::EnTT)
alpha_set_warnings(alpha_star)
