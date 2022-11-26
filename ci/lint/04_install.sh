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
  ${CI_RETRY_EXE} apt-get install -y build-essential libbz2-dev libffi-dev libncursesw5-dev libreadline-dev libssl-dev libsqlite3-dev liblzma-dev zlib1g-dev
  curl -sL https://github.com/pyenv/pyenv-installer/raw/master/bin/pyenv-installer | bash
  "${PYENV_ROOT}/bin/pyenv" install "$PYTHON_VERSION"
fi
export PATH="${PYTHON_BIN_PATH}:${PATH}"
command -v python
python --version

ls -l "${PYTHON_BIN_PATH}"
command -v pip

echo =================================== LINE 24
${CI_RETRY_EXE} python -m pip install codespell==2.2.1
echo =================================== LINE 26
${CI_RETRY_EXE} python -m pip install flake8==4.0.1
echo =================================== LINE 28
${CI_RETRY_EXE} python -m pip install mypy==0.942
echo =================================== LINE 30
${CI_RETRY_EXE} python -m pip install pyzmq==22.3.0
echo =================================== LINE 32
${CI_RETRY_EXE} python -m pip install vulture==2.3
echo =================================== LINE 34

SHELLCHECK_VERSION=v0.8.0
curl -sL "https://github.com/koalaman/shellcheck/releases/download/${SHELLCHECK_VERSION}/shellcheck-${SHELLCHECK_VERSION}.linux.x86_64.tar.xz" | tar --xz -xf - --directory /tmp/
export PATH="/tmp/shellcheck-${SHELLCHECK_VERSION}:${PATH}"
