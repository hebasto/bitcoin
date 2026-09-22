# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

{ pkgs ? import (builtins.fetchTarball {
    # Pin to the nixos-25.05 branch, the last release that still ships GCC 12 and 13.
    url = "https://github.com/NixOS/nixpkgs/archive/ac62194c3917d5f474c1a844b6fd6da2db95077d.tar.gz";
  }) {} }:

let
  host = builtins.getEnv "HOST";
  # Select the GCC major version with GCC_VERSION=12 or GCC_VERSION=13 (default).
  gccVersionEnv = builtins.getEnv "GCC_VERSION";
  gccVersion = if gccVersionEnv == "" then "13" else gccVersionEnv;
  supportedGccVersions = [ "12" "13" ];
  crossPkgs = if host == "x86_64-w64-mingw32ucrt"
    then pkgs.pkgsCross.ucrt64
    else if host == "x86_64-w64-mingw32"
      then pkgs.pkgsCross.mingwW64
      else throw "Unsupported HOST: ${host}";
  toolchain = crossPkgs.stdenv.cc.targetPrefix;
  pthreads = crossPkgs.windows.mingw_w64_pthreads;
  # Build GCC with the posix thread model on winpthreads, as Guix does, instead
  # of the nixpkgs default mcfgthread model. GCC 12 has no upstream mcfgthread
  # support and the nixpkgs backport no longer matches the mcfgthread headers.
  #
  # The GCC build activates the threads package for every compiler wrapper,
  # including the native one used for host code. A top-level include/ would
  # then shadow glibc's pthread.h. Present winpthreads through a subdirectory
  # and tell GCC's target flags where to look via the incdir/libdir attrs.
  pthreadsForGcc = pkgs.runCommandLocal "${pthreads.name}-for-gcc" {
    passthru = { incdir = "/winpthreads/include"; libdir = "/winpthreads/lib"; };
  } ''
    mkdir -p $out/winpthreads
    ln -s ${pthreads}/include $out/winpthreads/include
    ln -s ${pthreads}/lib $out/winpthreads/lib
  '';
  threadsCross = { model = "posix"; package = pthreadsForGcc; };
  # The cross compiler runs on the build platform and targets Windows, so it
  # lives in buildPackages (the plain crossPkgs.gccNN attribute is a GCC that
  # runs on Windows). Rewrap the unwrapped compiler with the chosen thread
  # model and make the wrapper add winpthreads instead of mcfgthread.
  mkCrossGcc = version:
    let base = crossPkgs.buildPackages."gcc${version}";
    in base.override {
      cc = base.cc.override { inherit threadsCross; };
      extraPackages = [ pthreads ];
    };
  crossGcc = if builtins.elem gccVersion supportedGccVersions
    then mkCrossGcc gccVersion
    else throw "Unsupported GCC_VERSION: ${gccVersion} (expected one of: ${builtins.concatStringsSep ", " supportedGccVersions})";
in

pkgs.mkShellNoCC {
  packages = [
    crossGcc
    pkgs.nsis
  ];

  shellHook = ''
    export NIX_CFLAGS_COMPILE="-isystem ${pthreads}/include $NIX_CFLAGS_COMPILE"
    export NIX_LDFLAGS="-L${pthreads}/lib $NIX_LDFLAGS"
    export CC=$(command -v ${toolchain}gcc)
    export CXX=$(command -v ${toolchain}g++)
    export LD=$(command -v ${toolchain}ld)
    export AR=$(command -v ${toolchain}ar)
    export AS=$(command -v ${toolchain}as)
    export RANLIB=$(command -v ${toolchain}ranlib)
    export NM=$(command -v ${toolchain}nm)
    export STRIP=$(command -v ${toolchain}strip)
    export OBJCOPY=$(command -v ${toolchain}objcopy)
    export OBJDUMP=$(command -v ${toolchain}objdump)
    export READELF=$(command -v ${toolchain}readelf)
    export SIZE=$(command -v ${toolchain}size)
    export WINDRES=$(command -v ${toolchain}windres)
    export RC=$(command -v ${toolchain}windres)
  '';
}
