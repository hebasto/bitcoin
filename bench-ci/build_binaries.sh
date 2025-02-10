#!/usr/bin/env bash
set -euxo pipefail

if [ $# -ne 2 ]; then
    echo "Usage: $0 <base_commit> <head_commit>"
    exit 1
fi

# Save current state of git
initial_ref=$(git symbolic-ref -q HEAD || git rev-parse HEAD)
if git symbolic-ref -q HEAD >/dev/null; then
    initial_state="branch"
    initial_branch=${initial_ref#refs/heads/}
else
    initial_state="detached"
fi

base_commit="$1"
head_commit="$2"

mkdir -p binaries/base
mkdir -p binaries/head

for build in "base:${base_commit}" "head:${head_commit}"; do
  name="${build%%:*}"
  commit="${build#*:}"
  git checkout "$commit"
  # Use environment variables if set, otherwise use defaults
  HOSTS="${HOSTS:-x86_64-linux-gnu}" \
  SOURCES_PATH="${SOURCES_PATH:-/data/SOURCES_PATH}" \
  BASE_CACHE="${BASE_CACHE:-/data/BASE_CACHE}" \
  taskset -c 2-15 chrt -f 1 bench-ci/guix/guix-build

  # Truncate commit hash to 12 characters
  short_commit=$(echo "$commit" | cut -c 1-12)

  # Extract the Guix output
  tar -xzf "guix-build-${short_commit}/output/x86_64-linux-gnu/bitcoin-${short_commit}-x86_64-linux-gnu.tar.gz"

  # Copy the binary to our binaries directory
  cp "bitcoin-${short_commit}/bin/bitcoind" "binaries/${name}/bitcoind"

  # Cleanup extracted files
  rm -rf "bitcoin-${short_commit}"
done

# Restore initial git state
if [ "$initial_state" = "branch" ]; then
    git checkout "$initial_branch"
else
    git checkout "$initial_ref"
fi
