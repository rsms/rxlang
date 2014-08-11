cmake_minimum_required (VERSION 2.8)
project(rx)

set(DEPS_DIR    "${PROJECT_SOURCE_DIR}/deps")
set(DEPS_PREFIX "${DEPS_DIR}/build")
# include(CMakeForceCompiler)
# cmake_force_cxx_compiler("${DEPS_PREFIX}/bin/clang++" CLang)
set(CMAKE_CXX_COMPILER        "${DEPS_PREFIX}/bin/clang++")
set(CMAKE_C_COMPILER          "${DEPS_PREFIX}/bin/clang")
set(CMAKE_AR                  "${DEPS_PREFIX}/bin/llvm-ar" CACHE STRING "" )
set(CMAKE_RANLIB              "${DEPS_PREFIX}/bin/llvm-ranlib" CACHE STRING "" )
set(CMAKE_STRIP               "${DEPS_PREFIX}/bin/strip" CACHE STRING "" )
# set(CMAKE_C_COMPILER          "${DEPS_PREFIX}/bin/clang" )
# set(CMAKE_CXX_COMPILER        "${DEPS_PREFIX}/bin/clang++" )
# # set(CMAKE_C_FLAGS              "-march=x86-64 -m64 -msse3" CACHE STRING "" )
# # set(CMAKE_CXX_FLAGS            "-march=x86-64 -m64 -msse3" CACHE STRING "" )
# set(CMAKE_C_FLAGS_RELEASE      "-Ofast -flto" CACHE STRING "" )
# set(CMAKE_CXX_FLAGS_RELEASE    "-Ofast -flto" CACHE STRING "" )
# # set(CMAKE_C_LINK_EXECUTABLE "${DEPS_PREFIX}/llvm-ld <OBJECTS> -o  <TARGET> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "${DEPS_PREFIX}/bin/clang <OBJECTS> -o <TARGET> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES>")


set(RX_CXX_FLAGS_LIST
  -g
  -Wall
  -std=c++1y
  -fno-pic
  -fno-rtti
  -fvisibility-inlines-hidden
  -fno-exceptions
  -fno-cxx-exceptions
  -fno-autolink
  -fdiagnostics-show-option
  -fcolor-diagnostics
  -resource-dir '${DEPS_PREFIX}/lib/clang/3.4'
  -isystem '${DEPS_PREFIX}/include/c++/v1'
  -nostdinc++
  -nostdlib
  -D__STDC_CONSTANT_MACROS
  -D__STDC_LIMIT_MACROS
  -D_GNU_SOURCE
  -D__STDC_CONSTANT_MACROS
  # -v
)
string(REPLACE ";" " " RX_CXX_FLAGS "${RX_CXX_FLAGS_LIST}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RX_CXX_FLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -std=c11 -fno-common")

set(LINK_FLAGS "-dead_strip") # -L'${DEPS_PREFIX}/lib'
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -nostdlib -fno-autolink -isystem '${DEPS_PREFIX}/include/c++/v1'")

# Include clang and llvm
include_directories("${DEPS_PREFIX}/include")

include(cmake/pch.cmake)
add_pch(rx_pch src/prefix.hh src/prefix.cc)

add_executable(rx
  src/async.cc
  src/asyncgroup.cc
  src/compiler.cc
  src/deps.cc
  src/fs.cc
  src/hash.cc
  src/lex.cc
  src/net.cc
  src/rx.cc
  src/srcfile.cc
  src/text.cc
  src/time.cc
)
use_pch(rx rx_pch)

execute_process(
  COMMAND sh -c
  "cd '${DEPS_PREFIX}/include/c++/v1' && find -E . -depth 1 -type f -regex '.+/[a-z]+' -exec echo '{}' ';' | sed -E 's/^\\.\\/(.+)\$/  PKGENTRY(\\1, \"\\1\")/'"
  OUTPUT_VARIABLE STDPKG_ENTRIES)

# message("STDPKG_ENTRIES: ${STDPKG_ENTRIES}")
configure_file(
  "${PROJECT_SOURCE_DIR}/src/stdpkgmap.inc.in"
  "${PROJECT_BINARY_DIR}/generated-include/stdpkgmap.inc")
include_directories("${PROJECT_BINARY_DIR}/generated-include")

include(cmake/FindLibLLVM.cmake)
target_link_libraries(rx ${LIBLLVM_LIB_FILES})

include(cmake/FindLibClang.cmake)
target_link_libraries(rx ${LIBCLANG_STATIC_LIBS})

target_link_libraries(rx "${DEPS_PREFIX}/lib/libc++.dylib")

target_link_libraries(rx "${DEPS_PREFIX}/lib/libuv.a")

find_library(LIB_curses curses "${DEPS_PREFIX}/lib")
target_link_libraries(rx ${LIB_curses})

find_library(LIB_c "c")
target_link_libraries(rx c)

find_library(LIB_z "m")
target_link_libraries(rx ${LIB_m})

find_library(LIB_z "z")
target_link_libraries(rx ${LIB_z})


# message("CLANG_LIB:           ${CLANG_LIB}")
# message("rx_SOURCE_DIR:       ${rx_SOURCE_DIR}")
# message("rx_BINARY_DIR:       ${rx_BINARY_DIR}")
# message("PROJECT_SOURCE_DIR:  ${PROJECT_SOURCE_DIR}")
# message("CMAKE_LIBRARY_PATH:  ${CMAKE_LIBRARY_PATH}")
# message("CMAKE_CXX_FLAGS:     ${CMAKE_CXX_FLAGS}")
# message("LINK_LIBRARIES:      ${LINK_LIBRARIES}")
