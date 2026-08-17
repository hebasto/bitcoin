#!/usr/bin/env bash
set -o errexit -o nounset -o pipefail

# Download Guix
mkdir -p "$GUIX_BOOTSTRAP_DIR"
guix_tarball="${GUIX_BOOTSTRAP_DIR}/guix-binary-${GUIX_BINARY_VERSION}.x86_64-linux.tar.xz"
if [ ! -e "$guix_tarball" ]; then
  curl --fail --location --silent --show-error \
    --output "$guix_tarball" \
    "https://ftp.gnu.org/gnu/guix/guix-binary-${GUIX_BINARY_VERSION}.x86_64-linux.tar.xz"
fi
echo "${GUIX_BINARY_SHA256}  ${guix_tarball}" | sha256sum --check

# Install Guix
cd /tmp || exit
curl --fail --location --silent --show-error \
  --output guix-install.sh https://guix.gnu.org/guix-install.sh
chmod +x guix-install.sh
(
  set +o pipefail
  yes '' 2>/dev/null | sudo env GUIX_BINARY_FILE_NAME="$guix_tarball" ./guix-install.sh
)

# Configure Bitcoin Core substitute server
substitute_server_key="${RUNNER_TEMP}/guix-fish-signing-key.pub"
curl --fail --location --show-error \
  --output "$substitute_server_key" \
  "$SUBSTITUTE_SERVER"/signing-key.pub
echo "${SUBSTITUTE_SERVER_KEY_SHA256}  ${substitute_server_key}" | sha256sum --check
# shellcheck disable=SC2024 # The key is deliberately read before sudo.
sudo guix archive --authorize < "$substitute_server_key"
