#!/bin/sh
# Run: sh verify/repro/c5_capture_stderr.sh <checkout of evalon/blaze-conf-5a54d79d>   (builds the evaluator unit tests, runs only zz_temporary_dump, and shows what a passing run writes to stderr)
cd "${1:-.}" || exit 1
grep -n "TEMPORARY_DUMP_BEGIN\|^TEST(zz_temporary_dump)\|std::cerr" test/evaluator/evaluator_2020_12_test.cc
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBLAZE_TESTS=ON -DBLAZE_BENCHMARK=OFF -DBLAZE_CONTRIB=OFF -DBLAZE_DOCS=OFF >/dev/null
cmake --build build --target sourcemeta_blaze_evaluator_unit --parallel >/dev/null
./build/test/evaluator/sourcemeta_blaze_evaluator_unit --filter evaluator_2020_12.zz_temporary_dump 2>stderr.txt
echo "exit=$?  stderr lines=$(wc -l < stderr.txt)"; head -4 stderr.txt
