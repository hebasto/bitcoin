#!/usr/bin/env bash
#
# Copyright (c) 2018-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

${CI_RETRY_EXE} apt-get update
${CI_RETRY_EXE} apt-get install -y curl git gawk jq

export PYENV_ROOT="${HOME}/.pyenv"
PYTHON_VERSION=$(cat "${BASE_ROOT_DIR}/.python-version")
PYTHON_BIN_PATH="${PYENV_ROOT}/versions/${PYTHON_VERSION}/bin"
if [ ! -d "$PYTHON_BIN_PATH" ]; then
  ${CI_RETRY_EXE} apt-get install -y build-essential libbz2-dev libncursesw5-dev libreadline-dev libssl-dev libsqlite3-dev liblzma-dev zlib1g-dev
  curl -sL https://github.com/pyenv/pyenv-installer/raw/master/bin/pyenv-installer | bash
  "${PYENV_ROOT}/bin/pyenv" install "$PYTHON_VERSION"
fi
export PATH="${PYTHON_BIN_PATH}:${PATH}"
command -v python
python --version

(
  # Temporary workaround for https://github.com/bitcoin/bitcoin/pull/26130#issuecomment-1260499544
  # Can be removed once the underlying image is bumped to something that includes git2.34 or later
  sed -i -e 's/bionic/jammy/g' /etc/apt/sources.list
  ${CI_RETRY_EXE} apt-get update
  ${CI_RETRY_EXE} apt-get install -y --reinstall git
)

${CI_RETRY_EXE} python -m pip install codespell==2.2.1
${CI_RETRY_EXE} python -m pip install flake8==4.0.1
${CI_RETRY_EXE} python -m pip install mypy==0.942
${CI_RETRY_EXE} python -m pip install pyzmq==22.3.0
${CI_RETRY_EXE} python -m pip install vulture==2.3

SHELLCHECK_VERSION=v0.8.0
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
export PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"
