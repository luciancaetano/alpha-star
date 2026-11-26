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

project(dr_mp3-populate NONE)



include(ExternalProject)
ExternalProject_Add(dr_mp3-populate
                     "UPDATE_DISCONNECTED" "False" "EXTERNALPROJECT_INTERNAL_ARGUMENT_SEPARATOR" "URL" "https://raw.githubusercontent.com/mackron/dr_libs/aec2dd20667f54acfdb66031217df6d240f56c52/dr_mp3.h" "URL_HASH" "SHA256=f5c8020eeebb79877184432efdd2651f24869f5bb8f379d45450fd0aed1ee915" DOWNLOAD_NO_EXTRACT YES
                    SOURCE_DIR          "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/dr_mp3-src"
                    BINARY_DIR          "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/dr_mp3-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
                    USES_TERMINAL_PATCH     YES
)


ExternalProject_Get_Property(dr_mp3-populate DOWNLOADED_FILE)
get_filename_component(dlFileName "${DOWNLOADED_FILE}" NAME)

ExternalProject_Add_Step(dr_mp3-populate copyfile
  COMMAND    "C:/Users/lucia/AppData/Local/Android/Sdk/cmake/3.31.6/bin/cmake.exe" -E copy_if_different
             "<DOWNLOADED_FILE>" "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/dr_mp3-src"
  DEPENDEES  patch
  DEPENDERS  configure
  BYPRODUCTS "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/dr_mp3-src/${dlFileName}"
  COMMENT    "Copying file to SOURCE_DIR"
)

