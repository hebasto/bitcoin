set shell := ["bash", "-uc"]

os := os()

default:
    just --list

# Build base and head binaries for CI
[group('ci')]
build-assumeutxo-binaries-guix base_commit head_commit:
    ./bench-ci/build_binaries.sh {{ base_commit }} {{ head_commit }}

# Run uninstrumented benchmarks on mainnet
[group('ci')]
run-mainnet-ci base_commit head_commit TMP_DATADIR ORIGINAL_DATADIR results_file dbcache png_dir binaries_dir:
    ./bench-ci/run-benchmark.sh {{ base_commit }} {{ head_commit }} {{ TMP_DATADIR }} {{ ORIGINAL_DATADIR }} {{ results_file }} {{ png_dir }} main 855000 "148.251.128.115:33333" {{ dbcache }} {{ binaries_dir }}

# Run instrumented benchmarks on mainnet
[group('ci')]
run-mainnet-ci-instrumented base_commit head_commit TMP_DATADIR ORIGINAL_DATADIR results_file dbcache png_dir binaries_dir:
    ./bench-ci/run-benchmark-instrumented.sh {{ base_commit }} {{ head_commit }} {{ TMP_DATADIR }} {{ ORIGINAL_DATADIR }} {{ results_file }} {{ png_dir }} main 855000 "148.251.128.115:33333" {{ dbcache }} {{ binaries_dir }}

# Cherry-pick commits from a bitcoin core PR onto this branch
[group('git')]
pick-pr pr_number:
    #!/usr/bin/env bash
    set -euxo pipefail

    if ! git remote get-url upstream 2>/dev/null | grep -q "bitcoin/bitcoin"; then
        echo "Error: 'upstream' remote not found or doesn't point to bitcoin/bitcoin"
        echo "Please add it with: `git remote add upstream https://github.com/bitcoin/bitcoin.git`"
        exit 1
    fi

    git fetch upstream pull/{{ pr_number }}/head:bench-{{ pr_number }} && git cherry-pick $(git rev-list --reverse bench-{{ pr_number }} --not upstream/master)
