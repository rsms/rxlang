cmake_minimum_required(VERSION 2.8)
add_custom_target(tests)

include_directories(../src)

macro (test target)
  add_executable(test-${target} ${target}.cc)
  use_pch(test-${target} rx_pch)
  target_link_libraries(test-${target} librx ${LIBRX_LIBS})
  add_test(NAME test-${target} COMMAND $<TARGET_FILE:test-${target}>)
  add_dependencies(tests test-${target})
endmacro (test)

test(text-utf8)
test(text-invalid-cat)
