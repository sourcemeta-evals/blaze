#!/bin/sh
# Run: sh verify/repro/c2_scan_callbackless.sh <checkout of evalon/blaze-conf-3ee9a13e>   (lists every validate() call in the JS corpus runner and in the C++ annotation regressions; a callbackless call would be `validate(x)` with a single argument)
cd "${1:-.}" || exit 1
echo "== ports/javascript/trace.test.mjs validate() calls:"; grep -n "validate(" ports/javascript/trace.test.mjs
echo "== C++ annotation regressions added by the branch:"; grep -n "^TEST(required_with_properties_annotations" test/evaluator/evaluator_2020_12_test.cc test/evaluator/evaluator_2019_09_test.cc
echo "== macros they use (all route through EVALUATE_WITH_TRACE, which passes a callback):"; grep -n "EVALUATE_WITH_TRACE" test/evaluator/evaluator_2020_12_test.cc | awk -F: '$1>5634' ; grep -n "EVALUATE_WITH_TRACE" test/evaluator/evaluator_2019_09_test.cc | awk -F: '$1>4367'
echo "== all callbackless validate(...) calls in those two files (evaluator_2020_12_test.cc lines <= 5634 and evaluator_2019_09_test.cc lines <= 4367 pre-date the branch):"; grep -n "validate(compiled_schema, [a-z_]*)" test/evaluator/evaluator_2020_12_test.cc test/evaluator/evaluator_2019_09_test.cc
echo "== evaluator_utils.h EVALUATE_WITH_TRACE definition:"; sed -n 25,27p test/evaluator/evaluator_utils.h
