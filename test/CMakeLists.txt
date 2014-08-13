cmake_minimum_required(VERSION 2.8)
# add_custom_target(tests)

include_directories(../src)

macro(test target)
  # Usage: test(lex [<arg passed to test program> ...])
  add_executable(test-${target} ${target}.cc)
  use_pch(test-${target} rx_pch)
  target_link_libraries(test-${target} librx ${LIBRX_LIBS})
  # set(extra_macro_args ${ARGN})
  # add_test(NAME test-${target} COMMAND $<TARGET_FILE:test-${target}> ${extra_macro_args})
  add_test(NAME test-${target} COMMAND $<TARGET_FILE:test-${target}> ${ARGN}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  # add_dependencies(tests test-${target})
endmacro(test)

test(text-utf8)
test(text-invalid-cat)
test(lex)
