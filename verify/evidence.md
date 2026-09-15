# Evidence — audit run v-625abd89

Audited solution branch: `blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching-perfect`
(checked out as `verify/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching-v-625abd89`,
HEAD `c9c2e1c17f1be066a6bdb7ec3ccab6852f25f248`, base `2f5ba6fdedef602197b6b2b4eaa9a17fb7839863`).

Claim-target branches were fetched from the authoritative repository into separate worktrees:

```sh
cd ~/repos/blaze
git remote add evalrepo https://github.com/sourcemeta-evals/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching.git
for b in 4bb08cf5 9c9905db 0e182701 5818f820 335d54f2; do
  git fetch evalrepo evalon/blaze-conf-$b:refs/remotes/evalrepo/evalon/blaze-conf-$b
  git worktree add ~/repos/wt-$b evalrepo/evalon/blaze-conf-$b
done
for b in 4bb08cf5 9c9905db 0e182701 5818f820 335d54f2; do git -C ~/repos/wt-$b rev-parse HEAD; done
```

```console
cf256a09d07ddc3e063cb6339c1dc1dc07c1d1ae   # C1  wt-4bb08cf5
e2f67e16761e614b523a70b2b34f9c5485c59fa7   # C2  wt-9c9905db
25a2e96cc3ae0c9fbafa614a31f9da8b5c7f0396   # C3  wt-0e182701
0f13fd95928c58b2f888f9d3a334921c3db5fad9   # C4  wt-5818f820
6555acfc542eafedf7b1fe59bed04b54a4319092   # C5  wt-335d54f2
```

Instrumentation (all under `verify/repro/`, no production code touched):

- `probe.cc` — compiles a schema in `fast`/`exhaustive` mode (optionally with `Tweaks.annotations`) and validates an
  instance with or without a callback; prints `mode=<m> callback=<0|1> valid=<0|1>`, every emitted annotation as
  `annotation result=<0|1> <evaluate_path> <instance_location> <value>` under `--callback`, and the compiled
  instruction types (with declared property names for typed-hash / typed-property instructions) under `--dump`.
- `hash_equal.cc` — prints `PropertyHashJSON` hashes of two names and whether they collide.
- `build_probe.sh <checkout> <out-dir>` — builds both against a checkout's `build/dist` install.

Every checkout used below was built and installed the same way (commands shown verbatim in each section), and the
probe was built per checkout with e.g.

```sh
sh ~/repos/blaze/verify/repro/build_probe.sh ~/repos/wt-5818f820 ~/audit/probe-5818f820
# -> probe: /home/ubuntu/audit/probe-5818f820/build/probe
```

## C1

Target: `evalon/blaze-conf-4bb08cf5` (worktree `~/repos/wt-4bb08cf5`, HEAD `cf256a09d07ddc3e063cb6339c1dc1dc07c1d1ae`).
The claim's exact workflow was run verbatim (the `Makefile`'s `configure` target is what `make configure` expands to;
the expansion is echoed at the top of the log):

```sh
cd ~/repos/wt-4bb08cf5
( echo "START $(date)"; rm -rf ./build && make configure && \
  find ./build -name "CTestTestfile.cmake" -exec sed -i -E 's/TIMEOUT "[0-9]+"/TIMEOUT "3600"/g' {} \; && \
  cmake --build ./build --config Debug --parallel $(nproc) && \
  cmake --install ./build --prefix ./build/dist --config Debug --component sourcemeta_core && \
  cmake --install ./build --prefix ./build/dist --config Debug --component sourcemeta_core_dev && \
  cmake --install ./build --prefix ./build/dist --config Debug --component sourcemeta_blaze && \
  cmake --install ./build --prefix ./build/dist --config Debug --component sourcemeta_blaze_dev && \
  cmake -E env UBSAN_OPTIONS=print_stacktrace=1 ctest --test-dir ./build --build-config Debug --output-on-failure --parallel --no-tests=error; \
  echo "EXIT_STATUS=$?"; echo "END $(date)" ) 2>&1 | tee ~/audit/c1_workflow.log
```

Key output (`~/audit/c1_workflow.log`):

```console
START Tue Sep 15 21:16:53 UTC 2026
cmake -S . -B ./build \
	-DCMAKE_BUILD_TYPE:STRING=Debug \
	-DCMAKE_COMPILE_WARNING_AS_ERROR:BOOL=ON \
	-DBLAZE_TESTS:BOOL=ON \
	-DBLAZE_BENCHMARK:BOOL=ON \
	-DBLAZE_CONTRIB:BOOL=ON \
	-DBLAZE_DOCS:BOOL=ON \
	-DBUILD_SHARED_LIBS:BOOL=OFF
-- The CXX compiler identification is GNU 15.2.0
...
-- Installing: /home/ubuntu/repos/wt-4bb08cf5/./build/dist/lib/cmake/core/core-config-version.cmake
-- Installing: /home/ubuntu/repos/wt-4bb08cf5/./build/dist/lib/libsourcemeta_core_io.a
...
Test project /home/ubuntu/repos/wt-4bb08cf5/build
      Start 73: packaging.find_package_configure
      Start  1: blaze.foundation
...
16/74 Test  #7: blaze.evaluator ...........................................................................   Passed    1.68 sec
...
73/74 Test #74: packaging.find_package_build ..............................................................   Passed    7.51 sec
74/74 Test #11: blaze.evaluator_trace_suite_canonical .....................................................   Passed   28.87 sec

100% tests passed out of 74

Total Test time (real) =  28.98 sec
EXIT_STATUS=0
END Tue Sep 15 21:19:59 UTC 2026
```

Checks on the log / filesystem:

```sh
grep -c "Passed" ~/audit/c1_workflow.log; grep -c "Installing:.*blaze" ~/audit/c1_workflow.log; ls ~/repos/wt-4bb08cf5/build/dist
```

```console
74
89
include
lib
```

All four `cmake --install` component invocations printed `-- Installing:` lines and returned 0 (the `&&` chain
reached `ctest`); ctest reported 74/74 passed, overall `EXIT_STATUS=0`. The workflow completes successfully.

## C2

Target: `evalon/blaze-conf-9c9905db` (worktree `~/repos/wt-9c9905db`, HEAD `e2f67e16761e614b523a70b2b34f9c5485c59fa7`).

```sh
cd ~/repos/wt-9c9905db
( echo "START $(date)"; rm -rf ./build-shared && \
  cmake -S . -B ./build-shared -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_COMPILE_WARNING_AS_ERROR:BOOL=ON \
    -DBLAZE_TESTS:BOOL=ON -DBLAZE_BENCHMARK:BOOL=ON -DBLAZE_CONTRIB:BOOL=ON -DBLAZE_DOCS:BOOL=ON -DBUILD_SHARED_LIBS:BOOL=ON && \
  cmake --build ./build-shared --config Debug --parallel $(nproc); \
  echo "EXIT_STATUS=$?"; echo "END $(date)" ) 2>&1 | tee ~/audit/c2_shared.log
```

Key output (`~/audit/c2_shared.log`):

```console
START Tue Sep 15 21:20:45 UTC 2026
...
-- Configuring done (3.8s)
-- Generating done (0.2s)
-- Build files have been written to: /home/ubuntu/repos/wt-9c9905db/build-shared
...
[100%] Building CXX object test/alterschema/CMakeFiles/sourcemeta_blaze_alterschema_unit.dir/alterschema_wrap_test.cc.o
[100%] Linking CXX executable sourcemeta_blaze_alterschema_unit
[100%] Built target sourcemeta_blaze_alterschema_unit
EXIT_STATUS=0
END Tue Sep 15 21:23:23 UTC 2026
```

```sh
grep -c "Built target" ~/audit/c2_shared.log; echo "warnings: $(grep -c 'warning:' ~/audit/c2_shared.log)"
grep -E "^(BLAZE_TESTS|BLAZE_BENCHMARK|BLAZE_CONTRIB|BLAZE_DOCS|BUILD_SHARED_LIBS|CMAKE_BUILD_TYPE|CMAKE_COMPILE_WARNING_AS_ERROR):" ~/repos/wt-9c9905db/build-shared/CMakeCache.txt
ls ~/repos/wt-9c9905db/build-shared/src/evaluator/*.so*
```

```console
89
warnings: 0
BLAZE_BENCHMARK:BOOL=ON
BLAZE_CONTRIB:BOOL=ON
BLAZE_DOCS:BOOL=ON
BLAZE_TESTS:BOOL=ON
BUILD_SHARED_LIBS:BOOL=ON
CMAKE_BUILD_TYPE:STRING=Debug
CMAKE_COMPILE_WARNING_AS_ERROR:BOOL=ON
/home/ubuntu/repos/wt-9c9905db/build-shared/src/evaluator/libsourcemeta_blaze_evaluator.so
/home/ubuntu/repos/wt-9c9905db/build-shared/src/evaluator/libsourcemeta_blaze_evaluator.so.0
/home/ubuntu/repos/wt-9c9905db/build-shared/src/evaluator/libsourcemeta_blaze_evaluator.so.0.0.1
```

Configuration and compilation of the Debug shared-library build with warnings-as-errors and tests, benchmarks,
contrib and docs enabled completed successfully (exit 0, zero compiler warnings, shared objects produced).

## C3

Target: `evalon/blaze-conf-0e182701` (worktree `~/repos/wt-0e182701`, HEAD `25a2e96cc3ae0c9fbafa614a31f9da8b5c7f0396`,
single commit `25a2e96c chore: apply eval changes` on top of the base).

Build + install (needed for the probe and for running the tests):

```sh
cd ~/repos/wt-0e182701
( rm -rf ./build && make configure && cmake --build ./build --config Debug --parallel $(nproc) && \
  for c in sourcemeta_core sourcemeta_core_dev sourcemeta_blaze sourcemeta_blaze_dev; do \
    cmake --install ./build --prefix ./build/dist --config Debug --component $c; done; \
  echo "EXIT_STATUS=$?"; echo "END $(date)" ) > ~/audit/c3_build.log 2>&1
tail -2 ~/audit/c3_build.log
sh ~/repos/blaze/verify/repro/build_probe.sh ~/repos/wt-0e182701 ~/audit/probe-0e182701
```

```console
EXIT_STATUS=0
END Tue Sep 15 21:40:33 UTC 2026
probe: /home/ubuntu/audit/probe-0e182701/build/probe
```

### Category 1 — Hash3 order branches (conforming + single-substitution pairs)

`LoopItemsPropertiesExactlyTypeStrictHash3` (evaluator_dispatch.h on the target branch) has six explicit arms, one per
permutation of the item's key order against the stored declared-name order (`value_1/2/3` are the item's entries in
order, `value.second.first[i]` the stored names):

```cpp
if ((value_1.key_equals(...[0]...) && value_2.key_equals(...[1]...) && value_3.key_equals(...[2]...)) ||   // (0,1,2)
    (value_1.key_equals(...[0]...) && value_2.key_equals(...[2]...) && value_3.key_equals(...[1]...)) ||   // (0,2,1)
    (value_1.key_equals(...[1]...) && value_2.key_equals(...[0]...) && value_3.key_equals(...[2]...)) ||   // (1,0,2)
    (value_1.key_equals(...[1]...) && value_2.key_equals(...[2]...) && value_3.key_equals(...[0]...)) ||   // (1,2,0)
    (value_1.key_equals(...[2]...) && value_2.key_equals(...[0]...) && value_3.key_equals(...[1]...)) ||   // (2,0,1)
    (value_1.key_equals(...[2]...) && value_2.key_equals(...[1]...) && value_3.key_equals(...[0]...))) {   // (2,1,0)
  continue;
}
```

To map committed corpus cases onto those arms I compiled every fast-mode corpus case on the target branch with the
probe's `--dump` (which prints the stored declared-name order of the emitted instruction) and classified each object
item by (a) which arm its key order hits when conforming, or (b) which arm it *would* hit when exactly one key is
replaced by an undeclared name (single-property substitution). Script: `verify/repro/c3_hash3_branch_coverage.py`.

```sh
cd ~/repos/blaze
python3 verify/repro/c3_hash3_branch_coverage.py ~/audit/probe-0e182701/build/probe ~/repos/wt-0e182701 | tee ~/audit/c3_map.log
```

```console
fast-mode corpus cases compiling to an exact-property instruction: 57
== Hash3 branches (LoopItemsPropertiesExactlyTypeStrictHash3, instance key order vs stored name order)
branch (0, 1, 2):
  conforming: 0
  single_substitution: 1
    - evaluator_draft4.json:exact_properties_items_hash3_alias_replaces_required item={"a": "foo", "a\u0000": "foo", "ccc": "foo"} ("a\u0000" for "bb")
branch (0, 2, 1):
  conforming: 0
  single_substitution: 0
branch (1, 0, 2):
  conforming: 0
  single_substitution: 0
branch (1, 2, 0):
  conforming: 0
  single_substitution: 0
branch (2, 0, 1):
  conforming: 2
    - evaluator_draft4.json:additionalProperties_21 item={"foo": 12345678910111213141516171819202122232425262728293031, "bar": 12345678910111213141516171819202122232425262728293031, "baz": 12345678910111213141516171819202122232425262728293031}
    - evaluator_draft4.json:exact_properties_items_hash3_distinct_nul_names item={"bb": "foo", "a": "foo", "a\u0000": "foo"}
  single_substitution: 0
branch (2, 1, 0):
  conforming: 1
    - evaluator_draft4.json:exact_properties_items_hash3_reordered item={"cc": "foo", "b": "foo", "": "foo"}
  single_substitution: 0
== Four-property exact-property instructions
  conforming: 2
    - evaluator_draft4.json:exact_properties_hash_reordered_valid instruction=LoopPropertiesExactlyTypeStrictHash instance={"dddd": "foo", "ccc": "foo", "bb": "foo", "a": "foo"}
    - evaluator_draft4.json:exact_properties_items_hash4_reordered instruction=LoopItemsPropertiesExactlyTypeStrictHash instance=[{"ddd": "foo", "cc": "foo", "b": "foo", "": "foo"}]
  invalid: 6
    - evaluator_draft4.json:exact_properties_hash_ordered_nul instruction=LoopPropertiesExactlyTypeStrictHash instance={"a": "foo", "bb": "foo", "ccc": "foo", "dddd\u0000": "foo"}
    - evaluator_draft4.json:exact_properties_hash_reordered_nul instruction=LoopPropertiesExactlyTypeStrictHash instance={"dddd\u0000": "foo", "ccc": "foo", "bb": "foo", "a": "foo"}
    - evaluator_draft4.json:exact_properties_hash_reordered_wrong_type instruction=LoopPropertiesExactlyTypeStrictHash instance={"dddd": "foo", "ccc": 1, "bb": "foo", "a": "foo"}
    - evaluator_draft4.json:exact_properties_hash_alias_replaces_required instruction=LoopPropertiesExactlyTypeStrictHash instance={"a": "foo", "a\u0000": "foo", "ccc": "foo", "dddd": "foo"}
    - evaluator_draft4.json:exact_properties_items_hash4_nul instruction=LoopItemsPropertiesExactlyTypeStrictHash instance=[{"\u0000": "foo", "b\u0000": "foo", "cc\u0000": "foo", "ddd\u0000": "foo"}]
    - evaluator_draft4.json:exact_properties_items_hash4_later_nul instruction=LoopItemsPropertiesExactlyTypeStrictHash instance=[{"": "foo", "b": "foo", "cc": "foo", "ddd": "foo"}, {"ddd\u0000": "foo", "cc\u0000": "foo", "b\u0000": "foo", "\u0000": "foo"}]
== Conforming controls whose declared names have colliding hashes
  2
    - evaluator_draft4.json:exact_properties_hash_distinct_nul_names names=["a", "a\u0000", "bb"] instance={"bb": "foo", "a\u0000": "foo", "a": "foo"}
    - evaluator_draft4.json:exact_properties_items_hash3_distinct_nul_names names=["a", "a\u0000", "bb"] instance=[{"bb": "foo", "a": "foo", "a\u0000": "foo"}]
```

The direct C++ tests were checked for additional Hash3 cases outside the corpus:

```sh
cd ~/repos/wt-0e182701 && grep -rn "Hash3" test/ | grep -v "\.json:"
```

```console
test/evaluator/evaluator_draft4_test.cc:889:  EVALUATE_TRACE_PRE(0, LoopItemsPropertiesExactlyTypeStrictHash3, "/items",
test/evaluator/evaluator_draft4_test.cc:891:  EVALUATE_TRACE_POST_FAILURE(0, LoopItemsPropertiesExactlyTypeStrictHash3,
test/evaluator/evaluator_draft4_test.cc:3375:                LoopItemsPropertiesExactlyTypeStrictHash3);
```

Line 889 belongs to `additionalProperties_23`, whose item `{"foo","bar","baz"}` fails on the value type (huge reals
vs `integer`) before any name arm decides — it is neither a conforming nor a single-substitution case. Line 3375 is
`items_exact_properties_hash_unrolled_branches`, which only asserts the compiled type is Hash3 and then rewrites the
instruction to the generic `LoopItemsPropertiesExactlyTypeStrictHash` to test its 1- and 3-property branches
(conforming `[object]` valid, `[object, collision]` invalid) — it never evaluates the Hash3 handler.

Result for category 1: only 3 of the 6 arms — `(2,0,1)`, `(2,1,0)` and, via substitution only, `(0,1,2)` — are hit
at all; arms `(0,2,1)`, `(1,0,2)`, `(1,2,0)` have no committed conforming or substitution case, and **no** arm has both
halves of the required pair. Category 1 lacks the specified cases.

### Category 2 — four-property conforming + invalid

From the mapping above: 2 conforming (`exact_properties_hash_reordered_valid`, `exact_properties_items_hash4_reordered`)
and 6 invalid (`..._ordered_nul`, `..._reordered_nul`, `..._reordered_wrong_type`, `..._alias_replaces_required`,
`..._items_hash4_nul`, `..._items_hash4_later_nul`) four-property exact-property cases exist. Category 2 is covered.

### Category 3 — legitimately declared colliding names, conforming controls

From the mapping above: `exact_properties_hash_distinct_nul_names` and `exact_properties_items_hash3_distinct_nul_names`
declare `"a"`, `"a\u0000"` and `"bb"` (the first two share a `PropertyHashJSON` hash since short hashes are
NUL-padded) and are `valid: true`. Category 3 is covered.

### Category 4 — callbackless JavaScript validation for every fast-mode corpus scenario

```sh
cd ~/repos/wt-0e182701 && git diff 2f5ba6fdedef602197b6b2b4eaa9a17fb7839863 HEAD -- ports/javascript/trace.test.mjs | grep "^[-+]"
sed -n 36,37p ports/javascript/trace.test.mjs; sed -n 51,61p ports/javascript/trace.test.mjs
```

```console
--- a/ports/javascript/trace.test.mjs
+++ b/ports/javascript/trace.test.mjs
+          assert.equal(evaluator.validate(testCase.instance), testCase.valid,
+            `Validation result mismatch without a callback`);
      for (const mode of ['fast', 'exhaustive']) {
        if (!testCase[mode]) continue;
          const result = evaluator.validate(testCase.instance, (type, valid, instruction, evaluatePath, instanceLocation, annotation) => {
            ...
          });

          assert.equal(result, testCase.valid, `Validation result mismatch`);
          assert.equal(evaluator.validate(testCase.instance), testCase.valid,
            `Validation result mismatch without a callback`);
```

The callbackless assertion sits in the same `it()` as the callback-based validation, inside the loop over every
`evaluator_*.json` file and every mode present, so each callback-based fast-mode scenario also gets a callbackless run.
Demonstration run (the trace suite compiles each schema with the freshly built `sourcemeta_blaze_contrib_compile`):

```sh
cd ~/repos/wt-0e182701/ports/javascript && npm ci && node --test trace.test.mjs > ~/audit/c3_js_trace.log 2>&1; tail -8 ~/audit/c3_js_trace.log
grep -c "_fast (" ~/audit/c3_js_trace.log; grep -c "_exhaustive (" ~/audit/c3_js_trace.log
```

```console
ℹ tests 3042
ℹ suites 8
ℹ pass 3042
ℹ fail 0
ℹ cancelled 0
ℹ skipped 0
ℹ todo 0
ℹ duration_ms 66630.683962
1521
1521
```

Category 4 is covered.

### Committed C++ suites on the target branch

```sh
cd ~/repos/wt-0e182701 && ctest --test-dir ./build --build-config Debug --output-on-failure -R "blaze.evaluator"
```

```console
100% tests passed out of 5

Total Test time (real) =  33.25 sec
```

### Verdict reasoning

Categories 2, 3 and 4 hold; category 1 does not (three Hash3 arms have no case at all and none has the paired
conforming + single-substitution instances). The claim is a disjunction ("at least one of these categories"), so it is
substantiated. Impact: the three untested permutation arms of the fused three-property items handler are exactly the
kind of hand-unrolled code where a copy/paste index slip would silently accept a mis-named property in fast mode and
nothing in the committed suite would catch it.

## C4

Target: `evalon/blaze-conf-5818f820` (worktree `~/repos/wt-5818f820`, HEAD `0f13fd95928c58b2f888f9d3a334921c3db5fad9`).

Build + install + probe:

```sh
cd ~/repos/wt-5818f820
( rm -rf ./build && make configure && cmake --build ./build --config Debug --parallel $(nproc) && \
  for c in sourcemeta_core sourcemeta_core_dev sourcemeta_blaze sourcemeta_blaze_dev; do \
    cmake --install ./build --prefix ./build/dist --config Debug --component $c; done; echo "EXIT_STATUS=$?" ) > ~/audit/c4_build.log 2>&1
tail -1 ~/audit/c4_build.log
sh ~/repos/blaze/verify/repro/build_probe.sh ~/repos/wt-5818f820 ~/audit/probe-5818f820
```

```console
EXIT_STATUS=0
probe: /home/ubuntu/audit/probe-5818f820/build/probe
```

Repro (schema is the claim's schema plus a `$schema` of 2020-12, which Blaze needs to resolve the dialect):

```sh
cd ~/repos/blaze && sh verify/repro/c4_hash_fallback_skips_value_type.sh ~/audit/probe-5818f820/build/probe
```

```console
# compiled fast-mode template
template:
  LoopPropertiesExactlyTypeStrictHash ["a", "b"]
# instance invalid: {"b":"x","a":1}
mode=fast callback=0 valid=1
mode=fast callback=1 valid=1
mode=exhaustive callback=0 valid=0
# instance control: {"b":"x","a":"y"}
mode=fast callback=0 valid=1
mode=fast callback=1 valid=1
mode=exhaustive callback=0 valid=1
# instance invalid_ordered: {"a":1,"b":"x"}
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
```

The same repro on the audited solution branch (probe built against `~/repos/blaze/build/dist`):

```sh
cd ~/repos/blaze && sh verify/repro/c4_hash_fallback_skips_value_type.sh ~/audit/probe-primary/build/probe
```

```console
# compiled fast-mode template
template:
  LoopPropertiesExactlyTypeStrictHash ["a", "b"]
# instance invalid: {"b":"x","a":1}
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
# instance control: {"b":"x","a":"y"}
mode=fast callback=0 valid=1
mode=fast callback=1 valid=1
mode=exhaustive callback=0 valid=1
# instance invalid_ordered: {"a":1,"b":"x"}
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
```

Part *Fallback value-type enforcement* — the handler on the target branch (`src/evaluator/include/sourcemeta/blaze/evaluator_dispatch.h:2061-2112`):

```cpp
    std::size_t index{0};
    for (const auto &entry : object) {
      if (effective_type_strict_real(entry.second) != value.first) {
        EVALUATE_END(LoopPropertiesExactlyTypeStrictHash);
      }
      if (!property_matches_hash(entry, value.second.first[index])) {
        break;
      }
      index += 1;
    }

    result = true;
    if (index < size) {
      auto iterator = object.cbegin();
      std::advance(iterator, index);
      for (; iterator != object.cend(); ++iterator) {
        if (std::ranges::none_of(
                value.second.first, [&iterator](const auto &entry) -> bool {
                  return property_matches_hash(*iterator, entry);
                })) {
          result = false;
          break;
        }
      }
    }
```

```cpp
inline auto property_matches_hash(const Entry &entry,
    const std::pair<ValueStringSet::hash_type, ValueString> &expected) noexcept -> bool {
  return entry.hash == expected.first &&
         entry.first.size() == expected.second.size();
}
```

The unordered fallback loop (`if (index < size)`) checks only `property_matches_hash` (hash + length); the
`effective_type_strict_real(...) != value.first` check lives only in the ordered loop, which is left by `break` at the
first order mismatch. Part holds.

Part *Callbackless reachability* — for `{"b":"x","a":1}` against stored order `["a","b"]`: entry `"b"` is a string
(type check passes), its hash differs from `"a"` → `break` with `index == 0`; the fallback then matches `"b"` and
`"a"` by name only and `result` stays `true`, so `1` is never type-checked. The dump shows the fast template is a
single `LoopPropertiesExactlyTypeStrictHash`, and the observed `mode=fast callback=0 valid=1` (vs `valid=0` for the
ordered `{"a":1,"b":"x"}`, which the ordered loop catches) shows callbackless validation takes exactly this path. The
callback-based fast run gives the same wrong answer; exhaustive mode rejects. Part holds.

Impact: any closed object schema whose properties all share one primitive type (an extremely common shape) is
validated by this instruction in fast mode; any instance whose keys arrive in a different order than the compiled
order (JSON producers do not guarantee key order) has *all* of its value types unchecked, so type-violating data is
accepted while exhaustive mode rejects it. The audited solution branch does not exhibit this (its fallback re-checks
type and full names).

## C5

Target: `evalon/blaze-conf-335d54f2` (worktree `~/repos/wt-335d54f2`, HEAD `6555acfc542eafedf7b1fe59bed04b54a4319092`).

Build + install + probe:

```sh
cd ~/repos/wt-335d54f2
( rm -rf ./build && make configure && cmake --build ./build --config Debug --parallel $(nproc) && \
  for c in sourcemeta_core sourcemeta_core_dev sourcemeta_blaze sourcemeta_blaze_dev; do \
    cmake --install ./build --prefix ./build/dist --config Debug --component $c; done; echo "EXIT_STATUS=$?" ) > ~/audit/c5_build.log 2>&1
tail -1 ~/audit/c5_build.log
sh ~/repos/blaze/verify/repro/build_probe.sh ~/repos/wt-335d54f2 ~/audit/probe-335d54f2
```

```console
EXIT_STATUS=0
probe: /home/ubuntu/audit/probe-335d54f2/build/probe
```

Repro — for each instance the three lines are: fast with `Tweaks.annotations={"properties"}`, fast without the tweak,
exhaustive with the tweak:

```sh
cd ~/repos/blaze && sh verify/repro/c5_annotations_accept_non_objects.sh ~/audit/probe-335d54f2/build/probe
```

```console
# compiled fast-mode template with annotations={properties}
template:
  AssertionDefinesExactly
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
# instance null
mode=fast callback=0 valid=1
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance true
mode=fast callback=0 valid=1
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance false
mode=fast callback=0 valid=1
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance 42
mode=fast callback=0 valid=1
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance 3.5
mode=fast callback=0 valid=1
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance "foo"
mode=fast callback=0 valid=1
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance []
mode=fast callback=0 valid=1
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance ["foo","bar"]
mode=fast callback=0 valid=1
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance {"foo":"x","bar":"y"}
mode=fast callback=0 valid=1
mode=fast callback=0 valid=1
mode=exhaustive callback=0 valid=1
# instance {}
mode=fast callback=0 valid=0
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance {"foo":"x","qux":"y"}
mode=fast callback=0 valid=0
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance {"foo":"x","bar":1}
mode=fast callback=0 valid=0
mode=fast callback=0 valid=1
mode=exhaustive callback=0 valid=0
```

Every non-object category (null, both booleans, integer, real, string, empty and non-empty array) is accepted by
annotation-enabled fast validation and rejected by plain fast and by exhaustive; the valid object is accepted in all
three and the empty/substituted objects rejected in all three (the last line, plain fast accepting `{"foo":"x","bar":1}`,
is the separate C4-style hash-fallback defect also present on this branch and is not part of C5).

The same repro on the audited solution branch:

```sh
cd ~/repos/blaze && sh verify/repro/c5_annotations_accept_non_objects.sh ~/audit/probe-primary/build/probe
```

```console
# compiled fast-mode template with annotations={properties}
template:
  AssertionDefinesExactlyStrict
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
# instance null
mode=fast callback=0 valid=0
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance true
mode=fast callback=0 valid=0
...
# instance ["foo","bar"]
mode=fast callback=0 valid=0
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance {"foo":"x","bar":"y"}
mode=fast callback=0 valid=1
mode=fast callback=0 valid=1
mode=exhaustive callback=0 valid=1
# instance {}
mode=fast callback=0 valid=0
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance {"foo":"x","qux":"y"}
mode=fast callback=0 valid=0
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
# instance {"foo":"x","bar":1}
mode=fast callback=0 valid=0
mode=fast callback=0 valid=0
mode=exhaustive callback=0 valid=0
```

Part *Required-property instruction selection* — `compile_required_assertions` on the target branch
(`src/compiler/default_compiler_draft3.h:204-273`):

```cpp
  } else if (properties_set.size() > 1) {
    if (is_closed_properties_required(schema_context.schema, properties_set)) {
      if (context.mode == Mode::FastValidation && assume_object &&
          !(context.collects_annotations &&
            annotations_enabled(context, KEYWORD_PROPERTIES))) {
        ...   // `properties` takes over; may return {} / Hash3 / AssertionDefinesExactlyStrict
      } else {
        return {
            make(sourcemeta::blaze::InstructionIndex::AssertionDefinesExactly,
                 context, schema_context, dynamic_context,
                 std::move(properties_set))};
      }
```

With `properties` annotations enabled the guard is false, so the `else` branch emits the non-strict
`AssertionDefinesExactly`, whose handler begins with
`EVALUATE_BEGIN_NON_STRING(AssertionDefinesExactly, target.is_object());` (evaluator_dispatch.h:377-378) — i.e. it
passes vacuously for any non-object. The dump above confirms `AssertionDefinesExactly` is what the template contains.
Part holds.

Part *Object-type assertion omission* — `compiler_draft3_validation_type` (same file, lines 2208-2216):

```cpp
      // A non-empty `required` rejects a non-object on its own, so the type
      // assertion is redundant. An empty `required` asserts nothing, so the
      // type check must stay
      if (!is_draft3 && context.mode == Mode::FastValidation &&
          schema_context.schema.defines("required") &&
          schema_context.schema.at("required").is_array() &&
          !schema_context.schema.at("required").empty()) {
        return {};
      }
```

The `type: object` assertion is dropped whenever `required` is non-empty, on the assumption that the emitted
`required` instruction is strict — an assumption the annotation path breaks. The dump shows no `AssertionTypeStrict`
in the template. Part holds.

Impact: any consumer that asks for `properties` annotations in fast mode (e.g. to learn which properties were
evaluated) gets `valid=true` for `null`, numbers, strings, booleans and arrays against a schema that says
`"type": "object"` — a silently wrong "valid" result on ordinary schemas. The audited solution branch emits
`AssertionDefinesExactlyStrict` in this situation and rejects every non-object.

## C6

Audited on the solution branch (`~/repos/blaze`, HEAD `c9c2e1c17f1be066a6bdb7ec3ccab6852f25f248`). Build + install +
probe:

```sh
cd ~/repos/blaze
( rm -rf ./build && make configure && cmake --build ./build --config Debug --parallel $(nproc) && \
  for c in sourcemeta_core sourcemeta_core_dev sourcemeta_blaze sourcemeta_blaze_dev; do \
    cmake --install ./build --prefix ./build/dist --config Debug --component $c; done; echo "EXIT_STATUS=$?"; echo "END $(date)" ) > ~/audit/primary_build.log 2>&1
tail -2 ~/audit/primary_build.log
sh verify/repro/build_probe.sh ~/repos/blaze ~/audit/probe-primary
```

```console
EXIT_STATUS=0
END Tue Sep 15 21:36:01 UTC 2026
probe: /home/ubuntu/audit/probe-primary/build/probe
```

Name construction: `notification_webhook_endpoint_url_for_production_alerts` (declared) vs
`notification_webhook_endpoint_url_fXr_production_alerts` (undeclared; byte 34 `o`→`X`). Both are 55 bytes, share the
first 31 bytes `notification_webhook_endpoint_u` and end in `s`. `PropertyHashJSON` for names ≥ 32 bytes hashes the
first 31 bytes and ORs in `1 + (size + first byte + last byte) % 255` (`vendor/core/src/core/json/include/sourcemeta/core/json_hash.h`), so the hashes must collide:

```sh
~/audit/probe-primary/build/hash_equal notification_webhook_endpoint_url_for_production_alerts notification_webhook_endpoint_url_fXr_production_alerts
```

```console
name1 size=55 perfect=0 hash=1a6e6f74696669636174696f6e5f776562686f6f6b5f656e64706f696e745f75
name2 size=55 perfect=0 hash=1a6e6f74696669636174696f6e5f776562686f6f6b5f656e64706f696e745f75
equal_hash=1 same_prefix31=1 same_size=1 same_last=1 distinct_names=1
```

Schemas/instances (files under `verify/repro/fixtures/`, printed by the runner):

```sh
cd ~/repos/blaze && sh verify/repro/c6_long_name_collision.sh ~/audit/probe-primary/build/probe ~/audit/probe-primary/build/hash_equal
```

```console
== c6_schema (two long names, closed, all required)
template:
  LoopPropertiesExactlyTypeStrict ["notification_webhook_endpoint_url_for_production_alerts", "notification_webhook_endpoint_url_for_staging_alerts"]
-- c6_valid: {"notification_webhook_endpoint_url_for_production_alerts":"foo","notification_webhook_endpoint_url_for_staging_alerts":"bar"}
mode=fast callback=0 valid=1
mode=fast callback=1 valid=1
mode=exhaustive callback=0 valid=1
-- c6_substituted: {"notification_webhook_endpoint_url_fXr_production_alerts":"foo","notification_webhook_endpoint_url_for_staging_alerts":"bar"}
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
-- c6_substituted_reordered: {"notification_webhook_endpoint_url_for_staging_alerts":"bar","notification_webhook_endpoint_url_fXr_production_alerts":"foo"}
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
-- c6_task_instance: {"a_property_name_we_never_declared_anywhere_in_the_schema":"foo","another_property_name_that_should_definitely_be_rejected":"bar"}
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
== c6_items_schema (items, three long names -> Hash3)
template:
  LoopItemsFrom
    LoopPropertiesExactlyTypeStrict ["notification_webhook_endpoint_url_for_developer_alerts", "notification_webhook_endpoint_url_for_production_alerts", "notification_webhook_endpoint_url_for_staging_alerts"]
  AssertionTypeStrict
-- c6_items_valid: [{"notification_webhook_endpoint_url_for_production_alerts":"a","notification_webhook_endpoint_url_for_staging_alerts":"b","notification_webhook_endpoint_url_for_developer_alerts":"c"}]
mode=fast callback=0 valid=1
mode=fast callback=1 valid=1
mode=exhaustive callback=0 valid=1
-- c6_items_substituted: [{"notification_webhook_endpoint_url_fXr_production_alerts":"a","notification_webhook_endpoint_url_for_staging_alerts":"b","notification_webhook_endpoint_url_for_developer_alerts":"c"}]
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
-- c6_items_substituted_reordered: [{"notification_webhook_endpoint_url_for_developer_alerts":"c","notification_webhook_endpoint_url_fXr_production_alerts":"a","notification_webhook_endpoint_url_for_staging_alerts":"b"}]
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
== c6_mixed_schema (one short + one long name)
template:
  LoopPropertiesExactlyTypeStrict ["id", "notification_webhook_endpoint_url_for_production_alerts"]
-- c6_mixed_valid: {"id":"x","notification_webhook_endpoint_url_for_production_alerts":"foo"}
mode=fast callback=0 valid=1
mode=fast callback=1 valid=1
mode=exhaustive callback=0 valid=1
-- c6_mixed_substituted: {"id":"x","notification_webhook_endpoint_url_fXr_production_alerts":"foo"}
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
-- c6_mixed_substituted_reordered: {"notification_webhook_endpoint_url_fXr_production_alerts":"foo","id":"x"}
mode=fast callback=0 valid=0
mode=fast callback=1 valid=0
mode=exhaustive callback=0 valid=0
```

The schemas are closed (`additionalProperties: false`), all declared names are required, all values are strings, and
the substituted instances preserve property count and value types. In every configuration the conforming control is
accepted and the undeclared equal-hash substitution is rejected in both fast (with and without callback) and
exhaustive mode. Long names are not "perfect" hashes (`perfect=0`), so the compiler emits the non-hash
`LoopPropertiesExactlyTypeStrict`, which compares full names via `key_equals(name, hash)` in the solution's dispatch.
The claimed acceptance does not occur.

## C7

Audited on the solution branch (`~/repos/blaze`, HEAD `c9c2e1c17f1be066a6bdb7ec3ccab6852f25f248`), probe built as in
C6 (`~/audit/probe-primary/build/probe`, build `EXIT_STATUS=0`). Callback-based fast-mode validation with
`Tweaks.annotations={"properties"}`; each `annotation result=1 ...` line is a successful `properties` annotation
reported through the callback (`EvaluationType::Post`, `result == true`, non-null annotation value).

```sh
cd ~/repos/blaze && sh verify/repro/c7_properties_annotations.sh ~/audit/probe-primary/build/probe | tee ~/audit/c7_primary.log
```

Draft 2019-09:

```console
=== 2019-09 c7_2019-09_schema.json
schema: {"$schema":"https://json-schema.org/draft/2019-09/schema","type":"object","properties":{"foo":{"type":"string"},"bar":{"type":"string"}},"required":["foo","bar"],"additionalProperties":false}
template:
  AssertionDefinesExactlyStrict
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
-- c7_two_valid: {"foo":"x","bar":"y"}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_two_valid_reordered: {"bar":"y","foo":"x"}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_two_missing: {"foo":"x"}
mode=fast callback=1 valid=0
-- c7_two_substituted: {"foo":"x","qux":"y"}
mode=fast callback=1 valid=0
=== 2019-09 c7_2019-09_three_schema.json
schema: {"$schema":"https://json-schema.org/draft/2019-09/schema","type":"object","properties":{"foo":{"type":"string"},"bar":{"type":"string"},"baz":{"type":"string"}},"required":["foo","bar","baz"],"additionalProperties":false}
template:
  AssertionDefinesExactlyStrictHash3 ["bar", "baz", "foo"]
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
-- c7_three_valid: {"foo":"x","bar":"y","baz":"z"}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "baz"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_three_valid_reordered: {"baz":"z","foo":"x","bar":"y"}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "baz"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_three_missing: {"foo":"x","bar":"y"}
mode=fast callback=1 valid=0
-- c7_three_substituted: {"foo":"x","bar":"y","qux":"z"}
mode=fast callback=1 valid=0
=== 2019-09 c7_2019-09_mixed_schema.json
schema: {"$schema":"https://json-schema.org/draft/2019-09/schema","type":"object","properties":{"foo":{"type":"string"},"bar":{"type":"integer"},"baz":{"type":"boolean"}},"required":["foo","bar","baz"],"additionalProperties":false}
template:
  AssertionDefinesExactlyStrictHash3 ["bar", "baz", "foo"]
  AssertionPropertyType
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
-- c7_mixed_valid: {"foo":"x","bar":1,"baz":true}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "baz"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_mixed_missing: {"foo":"x","bar":1}
mode=fast callback=1 valid=0
-- c7_mixed_substituted: {"foo":"x","bar":1,"qux":true}
mode=fast callback=1 valid=0
=== 2019-09 c7_2019-09_long_schema.json
schema: {"$schema":"https://json-schema.org/draft/2019-09/schema","type":"object","properties":{"notification_webhook_endpoint_url_for_production_alerts":{"type":"string"},"notification_webhook_endpoint_url_for_staging_alerts":{"type":"string"}},"required":["notification_webhook_endpoint_url_for_production_alerts","notification_webhook_endpoint_url_for_staging_alerts"],"additionalProperties":false}
template:
  AssertionDefinesExactlyStrict
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
-- c6_valid: {"notification_webhook_endpoint_url_for_production_alerts":"foo","notification_webhook_endpoint_url_for_staging_alerts":"bar"}
annotation result=1 /properties  "notification_webhook_endpoint_url_for_production_alerts"
annotation result=1 /properties  "notification_webhook_endpoint_url_for_staging_alerts"
mode=fast callback=1 valid=1
-- c6_substituted: {"notification_webhook_endpoint_url_fXr_production_alerts":"foo","notification_webhook_endpoint_url_for_staging_alerts":"bar"}
mode=fast callback=1 valid=0
```

Draft 2020-12:

```console
=== 2020-12 c7_2020-12_schema.json
schema: {"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","properties":{"foo":{"type":"string"},"bar":{"type":"string"}},"required":["foo","bar"],"additionalProperties":false}
template:
  AssertionDefinesExactlyStrict
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
-- c7_two_valid: {"foo":"x","bar":"y"}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_two_valid_reordered: {"bar":"y","foo":"x"}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_two_missing: {"foo":"x"}
mode=fast callback=1 valid=0
-- c7_two_substituted: {"foo":"x","qux":"y"}
mode=fast callback=1 valid=0
=== 2020-12 c7_2020-12_three_schema.json
schema: {"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","properties":{"foo":{"type":"string"},"bar":{"type":"string"},"baz":{"type":"string"}},"required":["foo","bar","baz"],"additionalProperties":false}
template:
  AssertionDefinesExactlyStrictHash3 ["bar", "baz", "foo"]
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
-- c7_three_valid: {"foo":"x","bar":"y","baz":"z"}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "baz"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_three_valid_reordered: {"baz":"z","foo":"x","bar":"y"}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "baz"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_three_missing: {"foo":"x","bar":"y"}
mode=fast callback=1 valid=0
-- c7_three_substituted: {"foo":"x","bar":"y","qux":"z"}
mode=fast callback=1 valid=0
=== 2020-12 c7_2020-12_mixed_schema.json
schema: {"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","properties":{"foo":{"type":"string"},"bar":{"type":"integer"},"baz":{"type":"boolean"}},"required":["foo","bar","baz"],"additionalProperties":false}
template:
  AssertionDefinesExactlyStrictHash3 ["bar", "baz", "foo"]
  AssertionPropertyType
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
-- c7_mixed_valid: {"foo":"x","bar":1,"baz":true}
annotation result=1 /properties  "bar"
annotation result=1 /properties  "baz"
annotation result=1 /properties  "foo"
mode=fast callback=1 valid=1
-- c7_mixed_missing: {"foo":"x","bar":1}
mode=fast callback=1 valid=0
-- c7_mixed_substituted: {"foo":"x","bar":1,"qux":true}
mode=fast callback=1 valid=0
=== 2020-12 c7_2020-12_long_schema.json
schema: {"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","properties":{"notification_webhook_endpoint_url_for_production_alerts":{"type":"string"},"notification_webhook_endpoint_url_for_staging_alerts":{"type":"string"}},"required":["notification_webhook_endpoint_url_for_production_alerts","notification_webhook_endpoint_url_for_staging_alerts"],"additionalProperties":false}
template:
  AssertionDefinesExactlyStrict
  AssertionPropertyTypeStrict
  AnnotationEmit
  AssertionPropertyTypeStrict
  AnnotationEmit
-- c6_valid: {"notification_webhook_endpoint_url_for_production_alerts":"foo","notification_webhook_endpoint_url_for_staging_alerts":"bar"}
annotation result=1 /properties  "notification_webhook_endpoint_url_for_production_alerts"
annotation result=1 /properties  "notification_webhook_endpoint_url_for_staging_alerts"
mode=fast callback=1 valid=1
-- c6_substituted: {"notification_webhook_endpoint_url_fXr_production_alerts":"foo","notification_webhook_endpoint_url_for_staging_alerts":"bar"}
mode=fast callback=1 valid=0
```

For every conforming case in both drafts (two-, three-property, mixed-type and long-name schemas, in declared and
reordered key order) the successful `/properties` annotations name exactly the declared set — `{bar, foo}`,
`{bar, baz, foo}`, `{..._production_alerts, ..._staging_alerts}` — with no missing or extra names, and the
missing-name and substituted-name controls are rejected (`valid=0`). The claimed failure does not occur.
