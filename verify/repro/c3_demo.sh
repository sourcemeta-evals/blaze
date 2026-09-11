#!/usr/bin/env bash
# Run from the repo root of the C3 branch (evalon/blaze-conf-40b85c12) with a
# configured ./build tree (cmake -S . -B ./build -DBLAZE_TESTS=ON ...):
#   bash verify/repro/c3_demo.sh
#
# 1. Shows which C++ test files the branch changed relative to the merge base
#    (evaluator_2019_09_test.cc is untouched).
# 2. Lists, per suite, the annotation-enabled (Tweaks.annotations) fast-mode
#    tests whose schema is an exact property set (`additionalProperties: false`)
#    and which assert either a FAILURE (wrong-name case) or a SUCCESS (valid
#    control). 2020-12 has both; 2019-09 has neither.
# 3. Runs the whole committed 2019-09 suite, then appends the two missing
#    2019-09 cases (verify/repro/c3_probe_2019_09_test.cc), rebuilds, and runs
#    them so the expected behaviour is demonstrated. The edit is reverted.
set -u
cd "$(dirname "$0")/../.."
BASE=$(git merge-base HEAD origin/main)
UNIT=./build/test/evaluator/sourcemeta_blaze_evaluator_unit
TESTCC=test/evaluator/evaluator_2019_09_test.cc
cp "$TESTCC" "$TESTCC.c3.bak"
trap 'mv "$TESTCC.c3.bak" "$TESTCC"' EXIT

echo "[c3] merge-base with origin/main: $BASE"
echo "[c3] C++ test files changed by this branch:"
git diff --stat "$BASE" HEAD -- test/evaluator/'*.cc' | cat

for f in test/evaluator/evaluator_2019_09_test.cc test/evaluator/evaluator_2020_12_test.cc; do
  echo "[c3] $f: annotation-enabled fast-mode tests on exact property-set schemas"
  awk '/^TEST\(/{name=$0; ann=0; closed=0; kind=""} /tweaks.annotations/{ann=1} /"additionalProperties": false/{closed=1} /_FAST_SUCCESS_TWEAKED\(/{kind="SUCCESS (valid control)"} /_FAST_FAILURE_TWEAKED\(/{kind="FAILURE (invalid case)"} /^}/{if(ann&&closed&&kind!="") print "   "name" -> "kind}' "$f"
  echo "   total: $(awk '/^TEST\(/{ann=0; closed=0; fast=0} /tweaks.annotations/{ann=1} /"additionalProperties": false/{closed=1} /_FAST_(SUCCESS|FAILURE)_TWEAKED\(/{fast=1} /^}/{if(ann&&closed&&fast) c++} END{print c+0}' "$f")"
done

echo "[c3] every annotation-enabled fast-mode 2019-09 test and the macro it uses:"
awk '/^TEST\(/{name=$0; ann=0} /tweaks.annotations/{ann=1} /_FAST_(SUCCESS|FAILURE)_TWEAKED\(/{if(ann) print "   "name" :: "$0}' test/evaluator/evaluator_2019_09_test.cc

cmake --build ./build --config Debug --target sourcemeta_blaze_evaluator_unit --parallel "$(nproc)" > /dev/null
echo "[c3] committed 2019-09 suite:"
"$UNIT" -f evaluator_2019_09. 2>&1 | grep -E '^# [0-9]+ passed'
echo "[c3] committed 2020-12 pair the branch added:"
"$UNIT" -f evaluator_2020_12.properties_closed_type_strict 2>&1 | grep -E '^(ok|not ok)|^# [0-9]+ passed'

echo "[c3] appending the two missing 2019-09 cases and running them:"
cat verify/repro/c3_probe_2019_09_test.cc >> "$TESTCC"
cmake --build ./build --config Debug --target sourcemeta_blaze_evaluator_unit --parallel "$(nproc)" > /dev/null
"$UNIT" -f evaluator_2019_09.c3_probe 2>&1 | grep -E '^\[c3-probe\]|^(ok|not ok)|^# [0-9]+ passed'
