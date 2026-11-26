include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Test framework (always).
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.12
  GIT_SHALLOW TRUE
  SYSTEM)
FetchContent_MakeAvailable(doctest)

if(NOT ALPHA_WITH_RUNTIME_DEPS)
  return()
endif()

# Static everywhere except Android, where SDLActivity loads libSDL3.so (with its JNI
# entry points) before the game's libmain.so.
if(ANDROID)
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
else()
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
endif()
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.20
  GIT_SHALLOW TRUE
  SYSTEM)

set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG v3.1.0
  GIT_SHALLOW TRUE
  SYSTEM)

FetchContent_Declare(quickjs
  GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
  GIT_TAG v0.10.1
  GIT_SHALLOW TRUE
  SYSTEM)

FetchContent_Declare(EnTT
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.15.0
  GIT_SHALLOW TRUE
  SYSTEM)

# Images: PNG/JPG/BMP via the vendored stb backend; no system image libraries.
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG_LIBPNG OFF CACHE BOOL "" FORCE)  # 3.4: PNG through stb, not vendored libpng + zlib
foreach(fmt AVIF JXL TIF WEBP)
  set(SDLIMAGE_${fmt} OFF CACHE BOOL "" FORCE)
endforeach()
FetchContent_Declare(SDL3_image
  GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
  GIT_TAG release-3.4.8
  GIT_SUBMODULES ""  # stb backend needs none of the external codecs
  GIT_SHALLOW TRUE
  SYSTEM)

FetchContent_MakeAvailable(SDL3 box2d quickjs EnTT SDL3_image)

# Box2D hardcodes -Werror for GCC; GCC 13 at -O2 reports a false-positive
# -Wmaybe-uninitialized inside Box2D's own distance.c (Release only). Third-party
# warnings are not ours to fix: keep them visible but not fatal for that target.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_compile_options(box2d PRIVATE -Wno-error=maybe-uninitialized)
endif()

# Audio codecs: single-file decoders, pinned by commit and SHA-256.
# stb_vorbis v1.25 (nothings/stb; MIT or public domain) for OGG Vorbis,
# dr_mp3 v0.7.4 (mackron/dr_libs tag mp3-0.7.4; MIT-0 or public domain) for MP3.
# alpha_codecs compiles both implementations; their warnings are not ours
# (no alpha_set_warnings, SYSTEM include dir).
FetchContent_Declare(stb_vorbis
  URL https://raw.githubusercontent.com/nothings/stb/6b5799bae8a1fe3792691ef4b72ae26434c3e799/stb_vorbis.c
  URL_HASH SHA256=97a8febc738e6a29fafc8d8bc557c7f17ad3db7ba6544485f59395cc6008b764
  DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_Declare(dr_mp3
  URL https://raw.githubusercontent.com/mackron/dr_libs/aec2dd20667f54acfdb66031217df6d240f56c52/dr_mp3.h
  URL_HASH SHA256=f5c8020eeebb79877184432efdd2651f24869f5bb8f379d45450fd0aed1ee915
  DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_MakeAvailable(stb_vorbis dr_mp3)
file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/codecs/alpha_codecs.c
  CONTENT "#include \"stb_vorbis.c\"\n#define DR_MP3_IMPLEMENTATION\n#include \"dr_mp3.h\"\n" @ONLY)
add_library(alpha_codecs STATIC ${CMAKE_BINARY_DIR}/codecs/alpha_codecs.c)
target_include_directories(alpha_codecs SYSTEM PUBLIC ${stb_vorbis_SOURCE_DIR} ${dr_mp3_SOURCE_DIR})

# Game UI: RmlUi (HTML/CSS-like .rml/.rcss) with its FreeType font engine.
# FreeType static and self-contained: no zlib/bzip2/png/harfbuzz/brotli from the system.
foreach(dep ZLIB BZIP2 PNG HARFBUZZ BROTLI)
  set(FT_DISABLE_${dep} ON CACHE BOOL "" FORCE)
endforeach()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(freetype
  GIT_REPOSITORY https://github.com/freetype/freetype.git
  GIT_TAG VER-2-14-3
  GIT_SHALLOW TRUE
  SYSTEM)
FetchContent_MakeAvailable(freetype)
# RmlUi looks for the Freetype::Freetype target; ours is the one built above.
add_library(Freetype::Freetype ALIAS freetype)
set(CMAKE_DISABLE_FIND_PACKAGE_Freetype ON)

set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
set(RMLUI_LUA_BINDINGS OFF CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "" FORCE)
set(RMLUI_INSTALL_RUNTIME_DEPENDENCIES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(rmlui
  GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
  GIT_TAG 6.3
  GIT_SHALLOW TRUE
  SYSTEM)
FetchContent_MakeAvailable(rmlui)
# Key/mouse conversion helpers from RmlUi's SDL3 platform backend (not its
# renderer or event handler: the engine routes input and renders itself).
add_library(alpha_rmlui_sdl STATIC ${rmlui_SOURCE_DIR}/Backends/RmlUi_Platform_SDL.cpp)
target_include_directories(alpha_rmlui_sdl SYSTEM PUBLIC ${rmlui_SOURCE_DIR}/Backends)
target_compile_definitions(alpha_rmlui_sdl PUBLIC RMLUI_SDL_VERSION_MAJOR=3)
target_link_libraries(alpha_rmlui_sdl PUBLIC RmlUi::Core SDL3::SDL3)

# Dear ImGui: engine dev tools only (ALPHA_DEVTOOLS), never in shipping builds.
# ImGui has no CMake build; alpha_imgui compiles the core plus the SDL3 platform
# and SDL_Renderer backends. SYSTEM includes: its warnings are not ours.
if(ALPHA_DEVTOOLS)
  FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9
    GIT_SHALLOW TRUE
    SYSTEM)
  FetchContent_MakeAvailable(imgui)
  add_library(alpha_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
  target_include_directories(alpha_imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_compile_definitions(alpha_imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
  target_link_libraries(alpha_imgui PUBLIC SDL3::SDL3)
endif()
