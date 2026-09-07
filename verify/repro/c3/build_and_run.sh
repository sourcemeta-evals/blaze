#!/usr/bin/env bash
# C3 repro: compiles annotation_elision_probe.cc against an already-built Blaze
# worktree and runs it. The probe compiles a closed, single-type object schema
# with `required` in fast mode, once with default tweaks and once with
# `Tweaks::annotations = {"properties"}`, and prints the emitted template plus
# the validation outcome of four instances.
#
# Usage: build_and_run.sh <blaze-worktree-with-build-dir>
# The worktree must have been configured and built, e.g.:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBLAZE_CONTRIB=ON
#   cmake --build build --config Release --target sourcemeta_blaze_compiler sourcemeta_blaze_evaluator
set -euo pipefail
W="$1"
HERE="$(cd "$(dirname "$0")" && pwd)"
INC=$(find "$W/build" -type d -name include | sed 's/^/-I/' | tr '\n' ' ')
VINC=$(find "$W/src" "$W/vendor" -type d -name include | sed 's/^/-I/' | tr '\n' ' ')
LIBS=$(find "$W/build" -name "*.a" | tr '\n' ' ')
OUT="${TMPDIR:-/tmp}/annotation_elision_probe.$$"
# shellcheck disable=SC2086
g++ -std=c++23 -O1 -flto $INC $VINC "$HERE/annotation_elision_probe.cc" -o "$OUT" \
  -Wl,--start-group $LIBS -Wl,--end-group -lpthread 2>&1 | grep -E "error|undefined" || true
"$OUT"
rm -f "$OUT"
