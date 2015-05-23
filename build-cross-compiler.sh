#!/usr/bin/env bash

set -vex

export PREFIX=$HOME/opt/cross
export TARGET=i386-elf
export PATH=$PREFIX/bin:$PATH
export CC="gcc"
export CXX="g++"
export CPP="cpp"
export LD="gcc"

idonthave() {
    md5=$(md5sum "$1" 2>/dev/null | cut -f1 -d\ )
    if [ -f "$1" -a "$md5" = "$2" ]; then
        return 1
    fi
    return 0
}

build_dir=$HOME/src/cross

binutils_ver=binutils-2.25
binutils_arc=$binutils_ver.tar.bz2
binutils_md5="d9f3303f802a5b6b0bb73a335ab89d66"
binutils_url=ftp://ftp.gnu.org/gnu/binutils/$binutils_arc

# libiconv_ver=libiconv-1.14
# libiconv_arc=$libiconv_ver.tar.gz
# libiconv_md5="e34509b1623cec449dfeb73d7ce9c6c6"
# libiconv_url=ftp://ftp.gnu.org/gnu/libiconv/$libiconv_arc

gcc_ver=gcc-5.1.0
gcc_arc=$gcc_ver.tar.bz2
gcc_md5="d5525b1127d07d215960e6051c5da35e"
gcc_url=ftp://ftp.gnu.org/gnu/gcc/$gcc_ver/$gcc_arc

test -d $build_dir || mkdir -p $build_dir

cd $build_dir

# Download
if idonthave $binutils_arc $binutils_md5; then
    curl -LO $binutils_url
fi
if idonthave $gcc_arc $gcc_md5; then
    curl -LO $gcc_url
fi
# if idonthave $libiconv_arc $libiconv_md5; then
#     curl -LO $libiconv_url
# fi

# Unpack
cd $build_dir
test -d $binutils_ver || tar xjf $binutils_arc
test -d $gcc_ver || tar xjf $gcc_arc
# test -d $libiconv_ver || tar xzf $libiconv_arc

# Shuffle
cd $build_dir
# test -d $gcc_ver/libiconv || mv $libiconv_ver $gcc_ver/libiconv
cd $gcc_ver
# echo "language=c" > gcc/ada/config-lang.in
# cp -va gcc/ada/gcc-interface/Make-lang.in gcc/ada/Make-lang.in
./contrib/download_prerequisites

# Build
cd $build_dir
test -d binutils-build || mkdir binutils-build
cd binutils-build
../$binutils_ver/configure --target=$TARGET --prefix="$PREFIX" --disable-werror --disable-nls
make
make install

cd $build_dir
test -d gcc-build || mkdir gcc-build
cd gcc-build
../$gcc_ver/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
