# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-src")
  file(MAKE_DIRECTORY "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-src")
endif()
file(MAKE_DIRECTORY
  "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-build"
  "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-subbuild/quickjs-populate-prefix"
  "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-subbuild/quickjs-populate-prefix/tmp"
  "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-subbuild/quickjs-populate-prefix/src/quickjs-populate-stamp"
  "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-subbuild/quickjs-populate-prefix/src"
  "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-subbuild/quickjs-populate-prefix/src/quickjs-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-subbuild/quickjs-populate-prefix/src/quickjs-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/workspace/alpha-star/examples/survivors/android/app/.cxx/Release/1l6f6d5t/arm64-v8a/_deps/quickjs-subbuild/quickjs-populate-prefix/src/quickjs-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
