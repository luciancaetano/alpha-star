include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Test framework (always).
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.12
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(doctest)

if(NOT ALPHA_WITH_RUNTIME_DEPS)
  return()
endif()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.10
  GIT_SHALLOW TRUE)

set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG v3.1.0
  GIT_SHALLOW TRUE)

FetchContent_Declare(quickjs
  GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
  GIT_TAG v0.10.1
  GIT_SHALLOW TRUE)

FetchContent_Declare(EnTT
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.15.0
  GIT_SHALLOW TRUE)

# Images: PNG/JPG/BMP via the vendored stb backend; no system image libraries.
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
foreach(fmt AVIF JXL TIF WEBP)
  set(SDLIMAGE_${fmt} OFF CACHE BOOL "" FORCE)
endforeach()
FetchContent_Declare(SDL3_image
  GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
  GIT_TAG release-3.2.4
  GIT_SUBMODULES ""  # stb backend needs none of the external codecs
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(SDL3 box2d quickjs EnTT SDL3_image)

# Third-party headers are not ours to warn about; our code stays at /W4 /WX.
set_target_properties(qjs PROPERTIES SYSTEM TRUE)
