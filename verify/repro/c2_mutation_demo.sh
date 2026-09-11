#!/usr/bin/env bash
# Run from the repo root with a configured ./build tree (make configure):
#   bash verify/repro/c2_mutation_demo.sh
#
# 1. Lists every annotation-enabled (Tweaks.annotations) fast-mode test in the
#    three C++ files named by C2 and shows which of them assert a failure.
# 2. Reverts the evaluator fix (src/evaluator/.../evaluator_dispatch.h) back to
#    the merge-base state, rebuilds the evaluator unit binary, and runs every
#    test in those three files: they all still pass, i.e. none of them is a
#    wrong-property-name regression for this change (annotation-enabled or not).
# 3. Appends verify/repro/c2_probe_test.cc (the missing test) to
#    test/evaluator/evaluator_test.cc on the fixed source, rebuilds and runs it.
#    On this branch the probe FAILS: with `properties` annotations enabled the
#    fast compiler emits only per-property type assertions + AnnotationEmit and
#    drops the required/additionalProperties closure, so a wrong-named object
#    is accepted (exhaustive mode and fast mode without annotations reject it).
# All production edits are reverted at the end.
set -u
cd "$(dirname "$0")/../.."
BASE=$(git merge-base HEAD origin/main)
UNIT=./build/test/evaluator/sourcemeta_blaze_evaluator_unit
DISPATCH=src/evaluator/include/sourcemeta/blaze/evaluator_dispatch.h
TESTCC=test/evaluator/evaluator_test.cc
cp "$DISPATCH" "$DISPATCH.c2.bak"
cp "$TESTCC" "$TESTCC.c2.bak"
trap 'mv "$DISPATCH.c2.bak" "$DISPATCH"; mv "$TESTCC.c2.bak" "$TESTCC"' EXIT

echo "[c2] merge-base with origin/main: $BASE"
echo "[c2] C++ test files touched by this branch vs merge-base:"
git diff --stat "$BASE" HEAD -- test/evaluator/'*.cc' | cat
echo "[c2] (empty above == no C++ test added or changed)"

echo "[c2] annotation-enabled fast-mode tests per file (TWEAKED macro line shown):"
for f in test/evaluator/evaluator_2019_09_test.cc test/evaluator/evaluator_2020_12_test.cc $TESTCC; do
  echo "== $f"
  awk '/^TEST\(/{name=$0} /tweaks.annotations/{ann=1} /_FAST_(SUCCESS|FAILURE)_TWEAKED\(/{if(ann) print name" :: "$0} /^}/{ann=0}' "$f" | grep FAILURE || echo "   (no annotation-enabled fast-mode test asserting a failure)"
done

echo "[c2] reverting the evaluator fix to the merge-base version"
git show "$BASE:$DISPATCH" > "$DISPATCH"
cmake --build ./build --config Debug --target sourcemeta_blaze_evaluator_unit --parallel "$(nproc)" > /dev/null
echo "[c2] running all tests from the three named files against the UNFIXED evaluator"
for prefix in evaluator_2019_09. evaluator_2020_12. evaluator.; do
  "$UNIT" -f "$prefix" 2>&1 | grep -E '^# [0-9]+ passed' | sed "s/^/   $prefix /"
done

echo "[c2] restoring the fix and appending the missing probe test"
cp "$DISPATCH.c2.bak" "$DISPATCH"
cat verify/repro/c2_probe_test.cc >> "$TESTCC"
cmake --build ./build --config Debug --target sourcemeta_blaze_evaluator_unit --parallel "$(nproc)" > /dev/null
"$UNIT" -f evaluator.c2_probe 2>&1 | grep -E '^\[c2-probe\]|^(ok|not ok)|^# [0-9]+ passed'

echo "[c2] running the probe against the UNFIXED evaluator too"
git show "$BASE:$DISPATCH" > "$DISPATCH"
cmake --build ./build --config Debug --target sourcemeta_blaze_evaluator_unit --parallel "$(nproc)" > /dev/null
"$UNIT" -f evaluator.c2_probe 2>&1 | grep -E '^\[c2-probe\]|^(ok|not ok)|^# [0-9]+ passed'
