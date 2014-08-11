#!/bin/bash
set -e
cd "$(dirname "$0")"

if ! [ -d build ]; then
  mkdir build
  pushd build >/dev/null
  cmake ..  # TODO: Remove build dir if cmake fails, or something like that.
  popd >/dev/null
  # Note that from this point on, `make` will cause cmake to rebuild itself if any CMakeLists.txt
  # files have changes, thus there's no need to re-run cmake while the build directory exists.
fi

make -C build $@ # VERBOSE=1
