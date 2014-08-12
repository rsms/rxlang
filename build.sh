#!/bin/bash
set -e
cd "$(dirname "$0")"

if ! [ -d build ]; then
  pushd deps/build >/dev/null
  DEPS_PREFIX=$(pwd)
  popd >/dev/null
  export CXX="${DEPS_PREFIX}/bin/clang++"
  export CC="${DEPS_PREFIX}/bin/clang"
  mkdir build
  pushd build >/dev/null
  cmake ..  # TODO: Remove build dir if cmake fails, or something like that.
  popd >/dev/null
  # Note that from this point on, `make` will cause cmake to rebuild itself if any CMakeLists.txt
  # files have changes, thus there's no need to re-run cmake while the build directory exists.
fi

if [ "$1" == "test" ]; then
  # Add "tests" group dependency to make sure all tests are built before we actually run test
  make -C build tests $@ # VERBOSE=1
else
  make -C build $@ # VERBOSE=1
fi
