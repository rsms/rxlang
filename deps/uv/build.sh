#!/bin/bash
set -e
cd "$(dirname "$0")"

pushd ../build >/dev/null
PREFIX=$(pwd)
popd >/dev/null

VERSION=0.11.27

if ! [ -d libuv-v${VERSION} ]; then
  echo curl '-#' -O http://libuv.org/dist/v${VERSION}/libuv-v${VERSION}.tar.gz
       curl '-#' -O http://libuv.org/dist/v${VERSION}/libuv-v${VERSION}.tar.gz
  tar xzf libuv-v${VERSION}.tar.gz
fi

pushd libuv-v${VERSION} >/dev/null

if ! [ -d build/gyp ]; then
  mkdir -p build
  echo git clone https://git.chromium.org/external/gyp.git
       git clone https://git.chromium.org/external/gyp.git build/gyp
fi

BUILDTYPE=Release
# BUILDTYPE=Debug

CC="${PREFIX}"/bin/clang ./gyp_uv.py -f make
make -C out -j8 BUILDTYPE=${BUILDTYPE}

rm -f "${PREFIX}/lib/libuv.a"
cp -a out/${BUILDTYPE}/libuv.a "${PREFIX}/lib/libuv.a"

rm -rf "${PREFIX}/include/uv"
cp -a include "${PREFIX}/include/uv"
