#!/bin/bash
set -e
cd "$(dirname "$0")"

svn_require() {
  if ! [ -d $1 ]; then
    echo svn checkout http://llvm.org/svn/llvm-project/$2 $1
    svn checkout http://llvm.org/svn/llvm-project/$2 $1
  fi
}

svn_require llvm                          llvm/tags/RELEASE_34/final
svn_require llvm/tools/clang              cfe/tags/RELEASE_34/final
svn_require llvm/tools/clang/tools/extra  clang-tools-extra/tags/RELEASE_34/final
svn_require llvm/projects/compiler-rt     compiler-rt/tags/RELEASE_34/final
svn_require llvm/projects/libcxx          libcxx/tags/RELEASE_342/final

pushd .. >/dev/null
PREFIX=$(pwd)/build
mkdir -p "${PREFIX}"
popd >/dev/null

# build llvm and clang
mkdir -p build-llvm
pushd build-llvm >/dev/null
../llvm/configure --prefix="${PREFIX}" \
  --enable-cxx11 --disable-assertions --enable-optimized --enable-libcpp --without-python
make
make install
popd >/dev/null

export CXX="${PREFIX}/bin/clang++"
export CC="${PREFIX}/bin/clang"
export TRIPLE=-apple-

# Build libc++ (llvm partially builds libc++ but not the actual library for some reason)
rm -rf build-libcxx
mkdir -p build-libcxx
pushd build-libcxx >/dev/null

cmake ../llvm/projects/libcxx
make

if (ls "$PREFIX"/lib/libc++* >/dev/null 2>&1); then
  rm -rf "$PREFIX"/lib/libc++*
fi
cp -af lib/libc++* "$PREFIX/lib/"

cmake ../llvm/projects/libcxx -DLIBCXX_ENABLE_SHARED=OFF
make
cp -af lib/libc++* "$PREFIX/lib/"

popd >/dev/null
rm -rf build-libcxx

# Note: For subsequent Clang development, you can just do make at the clang directory level.
