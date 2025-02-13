#!/usr/bin/env bash

set -euxo pipefail

# Helper function to check and clean datadir
clean_datadir() {
  set -euxo pipefail

  local TMP_DATADIR="$1"

  # Create the directory if it doesn't exist
  mkdir -p "${TMP_DATADIR}"

  # If we're in CI, clean without confirmation
  if [ -n "${CI:-}" ]; then
    rm -Rf "${TMP_DATADIR:?}"/*
  else
    read -rp "Are you sure you want to delete everything in ${TMP_DATADIR}? [y/N] " response
    if [[ "$response" =~ ^[Yy]$ ]]; then
      rm -Rf "${TMP_DATADIR:?}"/*
    else
      echo "Aborting..."
      exit 1
    fi
  fi
}

# Helper function to clear logs
clean_logs() {
  set -euxo pipefail

  local TMP_DATADIR="$1"
  local logfile="${TMP_DATADIR}/debug.log"

  echo "Checking for ${logfile}"
  if [ -e "${logfile}" ]; then
    echo "Removing ${logfile}"
    rm "${logfile}"
  fi
}

# Execute CMD before each set of timing runs.
setup_run() {
  set -euxo pipefail

  local TMP_DATADIR="$1"
  local commit="$2"
  clean_datadir "${TMP_DATADIR}"
}

# Execute CMD before each timing run.
prepare_run() {
  set -euxo pipefail

  local TMP_DATADIR="$1"
  local ORIGINAL_DATADIR="$2"

  # Run the actual preparation steps
  clean_datadir "${TMP_DATADIR}"
  # Don't copy hidden files so use *
  taskset -c 0-15 cp -r "$ORIGINAL_DATADIR"/* "$TMP_DATADIR"
  clean_logs "${TMP_DATADIR}"
}

# Executed after each timing run
conclude_run() {
  set -euxo pipefail

  local commit="$1"
  local TMP_DATADIR="$2"
  local PNG_DIR="$3"

  # Search in subdirs e.g. $datadir/signet
  debug_log=$(find "${TMP_DATADIR}" -name debug.log -print -quit)
  if [ -n "${debug_log}" ]; then
    echo "Generating plots from ${debug_log}"
    if [ -x "bench-ci/parse_and_plot.py" ]; then
      bench-ci/parse_and_plot.py "${debug_log}" "${PNG_DIR}"
    else
      ls -al "bench-ci/"
      echo "parse_and_plot.py not found or not executable, skipping plot generation"
    fi
  else
    ls -al "${TMP_DATADIR}/"
    echo "debug.log not found, skipping plot generation"
  fi

  # Move flamegraph if exists
  if [ -e flamegraph.svg ]; then
    mv flamegraph.svg "${commit}"-flamegraph.svg
  fi
}

# Execute CMD after the completion of all benchmarking runs for each individual
# command to be benchmarked.
cleanup_run() {
  set -euxo pipefail

  local TMP_DATADIR="$1"

  # Clean up the datadir
  clean_datadir "${TMP_DATADIR}"
}

run_benchmark() {
  local base_commit="$1"
  local head_commit="$2"
  local TMP_DATADIR="$3"
  local ORIGINAL_DATADIR="$4"
  local results_file="$5"
  local png_dir="$6"
  local chain="$7"
  local stop_at_height="$8"
  local connect_address="$9"
  local dbcache="${10}"
  local BINARIES_DIR="${11}"

  # Export functions so they can be used by hyperfine
  export -f setup_run
  export -f prepare_run
  export -f conclude_run
  export -f cleanup_run
  export -f clean_datadir
  export -f clean_logs

  # Debug: Print all variables being used
  echo "=== Debug Information ==="
  echo "TMP_DATADIR: ${TMP_DATADIR}"
  echo "ORIGINAL_DATADIR: ${ORIGINAL_DATADIR}"
  echo "BINARIES_DIR: ${BINARIES_DIR}"
  echo "base_commit: ${base_commit}"
  echo "head_commit: ${head_commit}"
  echo "results_file: ${results_file}"
  echo "png_dir: ${png_dir}"
  echo "chain: ${chain}"
  echo "stop_at_height: ${stop_at_height}"
  echo "connect_address: ${connect_address}"
  echo "dbcache: ${dbcache}"
  echo "\n"

  # Run hyperfine
  hyperfine \
    --shell=bash \
    --setup "setup_run ${TMP_DATADIR} {commit}" \
    --prepare "prepare_run ${TMP_DATADIR} ${ORIGINAL_DATADIR}" \
    --conclude "conclude_run {commit} ${TMP_DATADIR} ${png_dir}" \
    --cleanup "cleanup_run ${TMP_DATADIR}" \
    --runs 1 \
    --export-json "${results_file}" \
    --show-output \
    --command-name "base (${base_commit})" \
    --command-name "head (${head_commit})" \
    "taskset -c 1 flamegraph --palette bitcoin --title 'bitcoind IBD@{commit}' -c 'record -F 101 --call-graph fp' -- taskset -c 2-15 chrt -r 1 ${BINARIES_DIR}/{commit}/bitcoind -datadir=${TMP_DATADIR} -connect=${connect_address} -daemon=0 -prune=10000 -chain=${chain} -stopatheight=${stop_at_height} -dbcache=${dbcache} -printtoconsole=0 -debug=coindb -debug=leveldb -debug=bench -debug=validation" \
    -L commit "base,head"
}

# Main execution
if [ "$#" -ne 11 ]; then
  echo "Usage: $0 base_commit head_commit TMP_DATADIR ORIGINAL_DATADIR results_dir png_dir chain stop_at_height connect_address dbcache BINARIES_DIR"
  exit 1
fi

run_benchmark "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" "${10}" "${11}"
