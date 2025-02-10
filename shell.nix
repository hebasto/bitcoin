# Copyright 0xB10C, willcl-ark
{ pkgs ? import
  (fetchTarball "https://github.com/nixos/nixpkgs/archive/nixos-24.11.tar.gz")
  { }, }:
let
  inherit (pkgs.lib) optionals strings;
  inherit (pkgs) stdenv;

  # Override the default cargo-flamegraph with a custom fork
  cargo-flamegraph = pkgs.rustPlatform.buildRustPackage rec {
    pname =
      "flamegraph"; # Match the name in Cargo.toml, doesn't seem to work otherwise
    version = "bitcoin-core";

    src = pkgs.fetchFromGitHub {
      owner = "willcl-ark";
      repo = "flamegraph";
      rev = "bitcoin-core";
      sha256 = "sha256-tQbr3MYfAiOxeT12V9au5KQK5X5JeGuV6p8GR/Sgen4=";
    };

    doCheck = false;
    cargoHash = "sha256-QWPqTyTFSZNJNayNqLmsQSu0rX26XBKfdLROZ9tRjrg=";

    useFetchCargoVendor = true;

    nativeBuildInputs =
      pkgs.lib.optionals stdenv.hostPlatform.isLinux [ pkgs.makeWrapper ];
    buildInputs = pkgs.lib.optionals stdenv.hostPlatform.isDarwin
      [ pkgs.darwin.apple_sdk.frameworks.Security ];

    postFixup = pkgs.lib.optionalString stdenv.hostPlatform.isLinux ''
      wrapProgram $out/bin/cargo-flamegraph \
        --set-default PERF ${pkgs.linuxPackages.perf}/bin/perf
      wrapProgram $out/bin/flamegraph \
        --set-default PERF ${pkgs.linuxPackages.perf}/bin/perf
    '';
  };

in pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    autoconf
    automake
    boost
    ccache
    clang_18
    cmake
    libevent
    libtool
    pkg-config
    sqlite
    zeromq
  ];
  buildInputs = with pkgs; [
    just
    bash
    git
    shellcheck
    python310
    uv

    # Benchmarking
    cargo-flamegraph
    flamegraph
    hyperfine
    jq
    linuxKernel.packages.linux_6_6.perf
    perf-tools
    util-linux

    # Binary patching
    patchelf

    # Guix
    curl
    getent
  ];

  shellHook = ''
    echo "Bitcoin Core build nix-shell"
    echo ""
    echo "Setting up python venv"

    # fixes libstdc++ issues and libgl.so issues
    export LD_LIBRARY_PATH=${stdenv.cc.cc.lib}/lib/:$LD_LIBRARY_PATH

    uv venv --python 3.10
    source .venv/bin/activate
    uv pip install -r pyproject.toml

    patch-binary() {
      if [ -z "$1" ]; then
        echo "Usage: patch-binary <binary-path>"
        return 1
      fi
      patchelf --set-interpreter "$(cat $NIX_CC/nix-support/dynamic-linker)" "$1"
    }
    echo "Added patch-binary command"
    echo "    Usage: 'patch-binary <binary_path>'"
  '';
}
