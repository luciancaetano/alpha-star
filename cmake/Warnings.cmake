option(ALPHA_COVERAGE "Instrument our targets for gcov (GCC/Clang)" OFF)

# Warnings are errors: never silence the compiler. Also applies coverage
# instrumentation, so every target of ours gets both from one call.
function(alpha_set_warnings target)
  if(ALPHA_COVERAGE AND NOT MSVC)
    target_compile_options(${target} PRIVATE --coverage -O0)
    target_link_options(${target} PRIVATE --coverage)
  endif()
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror)
  endif()
endfunction()
