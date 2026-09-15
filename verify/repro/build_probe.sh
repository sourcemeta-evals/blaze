#!/bin/sh
# Run: verify/repro/build_probe.sh <blaze-checkout-with-build/dist> <out-dir>
# Builds verify/repro/probe.cc against the `build/dist` install of the given
# checkout (produce it via `make configure && cmake --build ./build && cmake
# --install ./build --prefix ./build/dist --component ...`, see verify/evidence.md).
set -eu
CHECKOUT="$1"
OUT="$2"
HERE="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$OUT"
cat > "$OUT/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.18)
project(blaze_probe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Blaze REQUIRED)
add_executable(probe "$HERE/probe.cc")
target_link_libraries(probe PRIVATE sourcemeta::core::json)
target_link_libraries(probe PRIVATE sourcemeta::core::jsonpointer)
target_link_libraries(probe PRIVATE sourcemeta::blaze::compiler)
target_link_libraries(probe PRIVATE sourcemeta::blaze::evaluator)
add_executable(hash_equal "$HERE/hash_equal.cc")
target_link_libraries(hash_equal PRIVATE sourcemeta::core::json)
EOF
cmake -S "$OUT" -B "$OUT/build" -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$CHECKOUT/build/dist" >/dev/null
cmake --build "$OUT/build" --parallel "$(nproc)"
echo "probe: $OUT/build/probe"
