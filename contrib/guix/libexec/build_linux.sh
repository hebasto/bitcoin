#!/usr/bin/env bash
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
export LC_ALL=C
set -e -o pipefail

# Environment variables for determinism
export TAR_OPTIONS="--no-same-owner --owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ=UTC

# Although Guix _does_ set umask when building its own packages (in our case,
# this is all packages in manifest.scm), it does not set it for `guix
# shell`. It does make sense for at least `guix shell --container`
# to set umask, so if that change gets merged upstream and we bump the
# time-machine to a commit which includes the aforementioned change, we can
# remove this line.
#
# This line should be placed before any commands which creates files.
umask 0022

if [ -n "$V" ]; then
    # Print both unexpanded (-v) and expanded (-x) forms of commands as they are
    # read from this file.
    set -vx
    # Set VERBOSE for CMake-based builds
    export VERBOSE="$V"
fi

# Check that required environment variables are set
cat << EOF
Required environment variables as seen inside the container:
    DIST_ARCHIVE_BASE: ${DIST_ARCHIVE_BASE:?not set}
    DISTNAME: ${DISTNAME:?not set}
    HOST: ${HOST:?not set}
    SOURCE_DATE_EPOCH: ${SOURCE_DATE_EPOCH:?not set}
    JOBS: ${JOBS:?not set}
    DISTSRC: ${DISTSRC:?not set}
    OUTDIR: ${OUTDIR:?not set}
EOF

ACTUAL_OUTDIR="${OUTDIR}"
OUTDIR="${DISTSRC}/output"

#####################
# Environment Setup #
#####################

# The depends folder also serves as a base-prefix for depends packages for
# $HOSTs after successfully building.
BASEPREFIX="${PWD}/depends"

# Given a package name and an output name, return the path of that output in our
# current guix environment
store_path() {
    grep --extended-regexp "/[^-]{32}-${1}-[^-]+${2:+-${2}}" "${GUIX_ENVIRONMENT}/manifest" \
        | head --lines=1 \
        | sed --expression='s|\x29*$||' \
              --expression='s|^[[:space:]]*"||' \
              --expression='s|"[[:space:]]*$||'
}


# Set environment variables to point the NATIVE toolchain to the right
# includes/libs
NATIVE_GCC="$(store_path gcc-toolchain)"

unset LIBRARY_PATH
unset CPATH
unset C_INCLUDE_PATH
unset CPLUS_INCLUDE_PATH
unset OBJC_INCLUDE_PATH
unset OBJCPLUS_INCLUDE_PATH

# Set native toolchain
build_CC="${NATIVE_GCC}/bin/gcc -isystem ${NATIVE_GCC}/include"
build_CXX="${NATIVE_GCC}/bin/g++ -isystem ${NATIVE_GCC}/include/c++ -isystem ${NATIVE_GCC}/include"

NATIVE_GCC_STATIC="$(store_path gcc-toolchain static)"
export LIBRARY_PATH="${NATIVE_GCC}/lib:${NATIVE_GCC_STATIC}/lib"

# Set environment variables to point the CROSS toolchain to the right
# includes/libs for $HOST
CROSS_GLIBC="$(store_path "glibc-cross-${HOST}")"
CROSS_GLIBC_STATIC="$(store_path "glibc-cross-${HOST}" static)"
CROSS_KERNEL="$(store_path "linux-libre-headers-cross-${HOST}")"
CROSS_GCC="$(store_path "gcc-cross-${HOST}")"
CROSS_GCC_LIB_STORE="$(store_path "gcc-cross-${HOST}" lib)"
CROSS_GCC_LIBS=( "${CROSS_GCC_LIB_STORE}/lib/gcc/${HOST}"/* ) # This expands to an array of directories...
CROSS_GCC_LIB="${CROSS_GCC_LIBS[0]}" # ...we just want the first one (there should only be one)

export CROSS_C_INCLUDE_PATH="${CROSS_GCC_LIB}/include:${CROSS_GCC_LIB}/include-fixed:${CROSS_GLIBC}/include:${CROSS_KERNEL}/include"
export CROSS_CPLUS_INCLUDE_PATH="${CROSS_GCC}/include/c++:${CROSS_GCC}/include/c++/${HOST}:${CROSS_GCC}/include/c++/backward:${CROSS_C_INCLUDE_PATH}"
export CROSS_LIBRARY_PATH="${CROSS_GCC_LIB_STORE}/lib:${CROSS_GCC_LIB}:${CROSS_GLIBC}/lib:${CROSS_GLIBC_STATIC}/lib"

# Sanity check CROSS_*_PATH directories
IFS=':' read -ra PATHS <<< "${CROSS_C_INCLUDE_PATH}:${CROSS_CPLUS_INCLUDE_PATH}:${CROSS_LIBRARY_PATH}"
for p in "${PATHS[@]}"; do
    if [ -n "$p" ] && [ ! -d "$p" ]; then
        echo "'$p' doesn't exist or isn't a directory... Aborting..."
        exit 1
    fi
done

# Disable Guix ld auto-rpath behavior
export GUIX_LD_WRAPPER_DISABLE_RPATH=yes

# Make /usr/bin if it doesn't exist
[ -e /usr/bin ] || mkdir -p /usr/bin

# Symlink env to a conventional path
[ -e /usr/bin/env ]  || ln -s --no-dereference "$(command -v env)"  /usr/bin/env

# Determine the correct value for -Wl,--dynamic-linker for the current $HOST

glibc_dynamic_linker=$(
    case "$HOST" in
        x86_64-linux-gnu)      echo /lib64/ld-linux-x86-64.so.2 ;;
        arm-linux-gnueabihf)   echo /lib/ld-linux-armhf.so.3 ;;
        aarch64-linux-gnu)     echo /lib/ld-linux-aarch64.so.1 ;;
        riscv64-linux-gnu)     echo /lib/ld-linux-riscv64-lp64d.so.1 ;;
        powerpc64-linux-gnu)   echo /lib64/ld64.so.1;;
        powerpc64le-linux-gnu) echo /lib64/ld64.so.2;;
        *)                     exit 1 ;;
    esac
)

####################
# Depends Building #
####################

# Build the depends tree, overriding variables that assume multilib gcc
make -C depends --jobs="$JOBS" HOST="$HOST" \
                                   ${V:+V=1} \
                                   ${SOURCES_PATH+SOURCES_PATH="$SOURCES_PATH"} \
                                   ${BASE_CACHE+BASE_CACHE="$BASE_CACHE"} \
                                   ${SDK_PATH+SDK_PATH="$SDK_PATH"} \
                                   ${build_CC+build_CC="$build_CC"} \
                                   ${build_CXX+build_CXX="$build_CXX"} \
                                   x86_64_linux_CC=x86_64-linux-gnu-gcc \
                                   x86_64_linux_CXX=x86_64-linux-gnu-g++ \
                                   x86_64_linux_AR=x86_64-linux-gnu-gcc-ar \
                                   x86_64_linux_RANLIB=x86_64-linux-gnu-gcc-ranlib \
                                   x86_64_linux_NM=x86_64-linux-gnu-gcc-nm \
                                   x86_64_linux_STRIP=x86_64-linux-gnu-strip \
                                   NO_QT=1

###########################
# Source Tarball Building #
###########################

GIT_ARCHIVE="${DIST_ARCHIVE_BASE}/${DISTNAME}.tar.gz"

# Create the source tarball if not already there
if [ ! -e "$GIT_ARCHIVE" ]; then
    mkdir -p "$(dirname "$GIT_ARCHIVE")"
    git archive --prefix="${DISTNAME}/" --output="$GIT_ARCHIVE" HEAD
fi

mkdir -p "$OUTDIR"

###########################
# Binary Tarball Building #
###########################

# CONFIGFLAGS
CONFIGFLAGS="-DREDUCE_EXPORTS=ON -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF -DCMAKE_SKIP_RPATH=TRUE"

# CFLAGS
HOST_CFLAGS="-O2 -g"
HOST_CFLAGS+=$(find /gnu/store -maxdepth 1 -mindepth 1 -type d -exec echo -n " -ffile-prefix-map={}=/usr" \;)
HOST_CFLAGS+=" -fdebug-prefix-map=${DISTSRC}/src=."

# CXXFLAGS
HOST_CXXFLAGS="$HOST_CFLAGS"

case "$HOST" in
    arm-linux-gnueabihf) HOST_CXXFLAGS="${HOST_CXXFLAGS} -Wno-psabi" ;;
esac

# LDFLAGS
HOST_LDFLAGS="-Wl,--as-needed -Wl,--dynamic-linker=$glibc_dynamic_linker -Wl,-O2"

# EXE FLAGS
case "$HOST" in
    *linux*)  CMAKE_EXE_LINKER_FLAGS="-DCMAKE_EXE_LINKER_FLAGS=${HOST_LDFLAGS} -static-libstdc++ -static-libgcc" ;;
esac

mkdir -p "$DISTSRC"
(
    cd "$DISTSRC"

    # Extract the source tarball
    tar --strip-components=1 -xf "${GIT_ARCHIVE}"

    # Configure this DISTSRC for $HOST
    # shellcheck disable=SC2086
    env CFLAGS="${HOST_CFLAGS}" CXXFLAGS="${HOST_CXXFLAGS}" LDFLAGS="${HOST_LDFLAGS}" \
    cmake -S . -B build \
          --toolchain "${BASEPREFIX}/${HOST}/toolchain.cmake" \
          -DWITH_CCACHE=OFF \
          -Werror=dev \
          ${CONFIGFLAGS} \
          "${CMAKE_EXE_LINKER_FLAGS}"

    # Build Bitcoin Core
    cmake --build build -j "$JOBS" ${V:+--verbose}

    # Setup the directory where our Bitcoin Core build for HOST will be
    # installed. This directory will also later serve as the input for our
    # binary tarballs.
    INSTALLPATH="${PWD}/installed/${DISTNAME}"
    mkdir -p "${INSTALLPATH}"
    # Install built Bitcoin Core to $INSTALLPATH
    cmake --install build --prefix "${INSTALLPATH}" ${V:+--verbose}
)

rm -rf "$DISTSRC"/build

exit 0
