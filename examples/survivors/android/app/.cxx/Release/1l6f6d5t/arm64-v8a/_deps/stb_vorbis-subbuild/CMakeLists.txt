# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.31.6)

# Reject any attempt to use a toolchain file. We must not use one because
# we could be downloading it here. If the CMAKE_TOOLCHAIN_FILE environment
# variable is set, the cache variable will have been initialized from it.
unset(CMAKE_TOOLCHAIN_FILE CACHE)
unset(ENV{CMAKE_TOOLCHAIN_FILE})

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(stb_vorbis-populate NONE)



include(ExternalProject)
ExternalProject_Add(stb_vorbis-populate
                     "UPDATE_DISCONNECTED" "False" "EXTERNALPROJECT_INTERNAL_ARGUMENT_SEPARATOR" "URL" "https://raw.githubusercontent.com/nothings/stb/6b5799bae8a1fe3792691ef4b72ae26434c3e799/stb_vorbis.c" "URL_HASH" "SHA256=97a8febc738e6a29fafc8d8bc557c7f17ad3db7ba6544485f59395cc6008b764" DOWNLOAD_NO_EXTRACT YES
                    SOURCE_DIR          "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/stb_vorbis-src"
                    BINARY_DIR          "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/stb_vorbis-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


ExternalProject_Get_Property(stb_vorbis-populate DOWNLOADED_FILE)
get_filename_component(dlFileName "${DOWNLOADED_FILE}" NAME)

ExternalProject_Add_Step(stb_vorbis-populate copyfile
  COMMAND    "C:/Users/lucia/AppData/Local/Android/Sdk/cmake/3.31.6/bin/cmake.exe" -E copy_if_different
             "<DOWNLOADED_FILE>" "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/stb_vorbis-src"
  DEPENDEES  patch
  DEPENDERS  configure
  BYPRODUCTS "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/stb_vorbis-src/${dlFileName}"
  COMMENT    "Copying file to SOURCE_DIR"
)

