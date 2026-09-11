## C1

**Claim:** The committed JavaScript tests do not run every fast-mode corpus scenario through the public `Blaze.validate` API without trace callbacks.

**Branch:** `evalon/blaze-conf-61af5600` (remote `claims` = `https://github.com/sourcemeta-evals/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching`), commit `c2e82eaf`, merge-base with `origin/main` `2f5ba6fd`. Worktree `~/wt/c61`, compile CLI built with `cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Debug -DBLAZE_TESTS=ON -DBLAZE_CONTRIB=ON && cmake --build ./build --target sourcemeta_blaze_contrib_compile --parallel $(nproc)`.

**Verdict: CONFIRMED.**

### Committed JS test files on the branch

```sh
cd ~/wt/c61 && git diff --stat 2f5ba6fd HEAD | cat
ls ports/javascript/*.test.mjs
```

```console
 ports/javascript/index.mjs                         |  23 ++-
 .../include/sourcemeta/blaze/evaluator_dispatch.h  |  79 +++++----
 test/evaluator/evaluator_2020_12.json              | 187 +++++++++++++++++++++
 test/evaluator/evaluator_draft4.json               | 129 ++++++++++++++
 4 files changed, 381 insertions(+), 37 deletions(-)
```

The branch adds corpus scenarios and changes the JS evaluator but adds no JS test. The only test that consumes the corpus is `ports/javascript/trace.test.mjs`; its single `validate` call is:

```js
const result = evaluator.validate(testCase.instance, (type, valid, instruction, evaluatePath, instanceLocation, annotation) => {
  ...
});
```

`ports/javascript/index.mjs` takes a different path when the second argument is absent (`callbackOrFormat === undefined` -> generated native validator / `*_fast` handlers) than when a callback is passed (`callbackMode = true`).

### Enumerate fast-mode corpus scenarios

```sh
cd ~/wt/c61 && for f in test/evaluator/evaluator_*.json; do echo "$(basename $f): $(jq '[.[] | select(.fast != null)] | length' $f) fast / $(jq length $f) total"; done
```

```console
evaluator_2019_09.json: 188 fast / 188 total
evaluator_2020_12.json: 205 fast / 205 total
evaluator_draft3.json: 408 fast / 408 total
evaluator_draft4.json: 393 fast / 393 total
evaluator_draft6.json: 198 fast / 198 total
evaluator_draft7.json: 97 fast / 97 total
evaluator_openapi_3_1.json: 6 fast / 6 total
evaluator_openapi_3_2.json: 6 fast / 6 total
```

1501 fast-mode scenarios in total (1489 in the six official dialect files + 12 OpenAPI). The committed suite runs each scenario twice (fast + exhaustive) = 3002 tests.

### Instrument the committed suite (`verify/repro/c1_instrument_validate.mjs`)

Wraps `Blaze.prototype.validate` and counts calls by second-argument kind.

```sh
cd ~/wt/c61 && node --import ./verify/repro/c1_instrument_validate.mjs --test ports/javascript/trace.test.mjs 2>&1 | grep -E '^ℹ (tests|pass|fail)|c1-instrument'
```

```console
[c1-instrument] validate() calls: {"callback":3002,"noCallback":0,"formatString":0}
ℹ tests 3002
ℹ pass 3002
ℹ fail 0
```

Every one of the 3002 `validate` calls made by the committed tests passes a trace callback; **zero** calls go through the public callback-less API. Hence none of the 1501 fast-mode scenarios is run through `Blaze.validate(instance)` without a callback.

### Callback-less run of the corpus (`verify/repro/c1_callbackless_corpus.mjs`)

Compiles every fast-mode scenario and calls `evaluator.validate(entry.instance)` with no callback, comparing to `valid`.

```sh
cd ~/wt/c61 && node verify/repro/c1_callbackless_corpus.mjs
```

```console
Found compile CLI: /home/ubuntu/wt/c61/build/contrib/sourcemeta_blaze_contrib_compile
callbackless fast-mode scenarios checked: 1501, mismatches: 0
```

On the unmodified branch the callback-less path is currently correct for all 1501 scenarios — the finding is about missing coverage, not a present defect.

### Impact demonstration (`verify/repro/c1_mutation_demo.sh`)

Temporarily deletes the property-name check from `LoopPropertiesExactlyTypeStrict_fast` in `ports/javascript/index.mjs` (the handler used only on the callback-less path), runs the committed suite, then the callback-less repro; restores the file via `trap`.

```sh
cd ~/wt/c61 && bash verify/repro/c1_mutation_demo.sh; echo "exit=$?"; git status --short
```

```console
[c1-mutation] removed property-name check from LoopPropertiesExactlyTypeStrict_fast
[c1-mutation] running committed JS trace tests (ports/javascript/trace.test.mjs) against mutant
ℹ tests 3002
ℹ pass 3002
ℹ fail 0
[c1-mutation] running callback-less corpus repro against mutant
Found compile CLI: /home/ubuntu/wt/c61/build/contrib/sourcemeta_blaze_contrib_compile
MISMATCH /home/ubuntu/wt/c61/test/evaluator/evaluator_2020_12.json#200 properties_exactly_type_strict_wrong_names: expected false, got true
MISMATCH /home/ubuntu/wt/c61/test/evaluator/evaluator_2020_12.json#201 properties_exactly_type_strict_partial_names: expected false, got true
callbackless fast-mode scenarios checked: 205, mismatches: 2
exit=1
?? verify/
```

The committed suite stays green (3002/3002) while the public callback-less API accepts the two new wrong-property-name scenarios that this very branch added to the corpus. `git status` shows only the untracked `verify/` directory, i.e. production code was restored.

### Impact reasoning

The task's motivating bug is precisely a fast-path regression (exact property-set matching accepting wrong names). The JS port's exact-property fast handlers (`LoopPropertiesExactlyTypeStrict_fast`, `LoopPropertiesExactlyTypeStrictHash_fast`, native validator) are only reached when a consumer calls `Blaze.validate(instance)` with no callback — the ordinary production call. The committed test suite never takes that path, so the branch's JS fix and the corpus scenarios written for it are not protected against regression in the code that real callers execute. The mutation shows a wrong-name acceptance bug in that path would ship undetected.

## C2

**Claim:** The committed C++ tests do not include a fast-mode wrong-property-name regression that collects `properties` annotations through `Tweaks.annotations`.

**Branch:** `evalon/blaze-conf-61af5600` (remote `claims` = `https://github.com/sourcemeta-evals/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching`), commit `c2e82eaf`, merge-base with `origin/main` `2f5ba6fd`. Worktree `~/wt/c61`, unit binary built with `cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Debug -DBLAZE_TESTS=ON -DBLAZE_CONTRIB=ON && cmake --build ./build --target sourcemeta_blaze_evaluator_unit --parallel $(nproc)`.

**Verdict: CONFIRMED** — and the missing test would fail on this branch (a real correctness bug).

### The branch touches no C++ test file

```sh
cd ~/wt/c61 && git diff --stat 2f5ba6fd HEAD | cat
```

```console
 ports/javascript/index.mjs                         |  23 ++-
 .../include/sourcemeta/blaze/evaluator_dispatch.h  |  79 +++++----
 test/evaluator/evaluator_2020_12.json              | 187 +++++++++++++++++++++
 test/evaluator/evaluator_draft4.json               | 129 ++++++++++++++
 4 files changed, 381 insertions(+), 37 deletions(-)
```

### Inspect + run the three named files, unfixed-evaluator check, and probe (`verify/repro/c2_mutation_demo.sh`)

The script (1) lists every annotation-enabled fast-mode test in the three files and which assert a failure, (2) reverts `src/evaluator/include/sourcemeta/blaze/evaluator_dispatch.h` to the merge-base version, rebuilds, and runs every test in the three files, (3) restores the fix, appends `verify/repro/c2_probe_test.cc` to `test/evaluator/evaluator_test.cc`, rebuilds and runs the probe, then (4) runs the probe against the unfixed evaluator too. All edits are reverted by `trap`.

```sh
cd ~/wt/c61 && bash verify/repro/c2_mutation_demo.sh; git status --short
```

```console
[c2] merge-base with origin/main: 2f5ba6fdedef602197b6b2b4eaa9a17fb7839863
[c2] C++ test files touched by this branch vs merge-base:
[c2] (empty above == no C++ test added or changed)
[c2] annotation-enabled fast-mode tests per file (TWEAKED macro line shown):
== test/evaluator/evaluator_2019_09_test.cc
   (no annotation-enabled fast-mode test asserting a failure)
== test/evaluator/evaluator_2020_12_test.cc
TEST(annotation_properties_closed_object_fast_keeps_closure) { ::   EVALUATE_WITH_TRACE_FAST_FAILURE_TWEAKED(schema, instance, 5, "", tweaks);
== test/evaluator/evaluator_test.cc
   (no annotation-enabled fast-mode test asserting a failure)
[c2] reverting the evaluator fix to the merge-base version
[c2] running all tests from the three named files against the UNFIXED evaluator
   evaluator_2019_09. # 198 passed, 0 failed
   evaluator_2020_12. # 233 passed, 0 failed
   evaluator. # 21 passed, 0 failed
[c2] restoring the fix and appending the missing probe test
[c2-probe] fast+annotations{properties} instructions: AssertionPropertyTypeStrict,AnnotationEmit,AssertionPropertyTypeStrict,AnnotationEmit,
[c2-probe] fast+annotations{properties}: wrong-name -> true; valid -> true
[c2-probe] fast, no annotations instructions: LoopPropertiesExactlyTypeStrictHash,
[c2-probe] fast, no annotations: wrong-name -> false; valid -> true
[c2-probe] exhaustive+annotations{properties}: wrong-name -> false; valid -> true
not ok 1 - evaluator.c2_probe_fast_wrong_property_name_with_properties_annotations
# 0 passed, 1 failed
[c2] running the probe against the UNFIXED evaluator too
[c2-probe] fast+annotations{properties} instructions: AssertionPropertyTypeStrict,AnnotationEmit,AssertionPropertyTypeStrict,AnnotationEmit,
[c2-probe] fast+annotations{properties}: wrong-name -> true; valid -> true
[c2-probe] fast, no annotations instructions: LoopPropertiesExactlyTypeStrictHash,
[c2-probe] fast, no annotations: wrong-name -> false; valid -> true
[c2-probe] exhaustive+annotations{properties}: wrong-name -> false; valid -> true
not ok 1 - evaluator.c2_probe_fast_wrong_property_name_with_properties_annotations
# 0 passed, 1 failed
?? verify/
```

### Why the one existing annotation-enabled failure test does not count

`annotation_properties_closed_object_fast_keeps_closure` (`test/evaluator/evaluator_2020_12_test.cc`, pre-existing at the merge base) is:

```cpp
"type": "object", "required": [ "foo" ], "additionalProperties": false,
"properties": { "foo": { "type": "string" } }
// instance { "foo": "bar", "extra": 1 }
tweaks.annotations = std::unordered_set<sourcemeta::core::JSON::StringView>{"properties"};
EVALUATE_WITH_TRACE_FAST_FAILURE_TWEAKED(schema, instance, 5, "", tweaks);
```

It is an *extra* property (closure) case: the declared property is present and correctly typed, and the failure is produced by `LoopPropertiesExcept` over the additional property. It is not a wrong-property-name case (same property count and types, different names), and it did not change behaviour when the evaluator fix was reverted (233/233 still pass on the unfixed evaluator).

### The probe (`verify/repro/c2_probe_test.cc`)

Schema: `type: object`, `properties: { foo: string, bar: string }`, `required: [foo, bar]`, `additionalProperties: false`. Wrong-name instance `{ "foo": "1", "baz": "2" }` (expected invalid); valid control `{ "foo": "1", "bar": "2" }`. `Tweaks.annotations = {"properties"}`, `Mode::FastValidation`.

### Impact reasoning

Observed on the branch (fixed evaluator): with `properties` annotations enabled in fast mode, the compiler emits only `AssertionPropertyTypeStrict,AnnotationEmit,AssertionPropertyTypeStrict,AnnotationEmit` — no `required`/`additionalProperties` closure instruction at all — and the evaluator **accepts** `{ "foo": "1", "baz": "2" }` against a schema that requires exactly `foo` and `bar`. The same instance is rejected by fast mode without annotations (`LoopPropertiesExactlyTypeStrictHash`) and by exhaustive mode with annotations. This is exactly the "annotations collected in fast mode must stay correct" requirement of the task, and it is broken on this branch; the test C2 asks about would have caught it. For comparison, the C3 target branch (`evalon/blaze-conf-40b85c12`) changes `src/compiler/default_compiler_draft3.h` to guard the closed-`properties` form with `!annotations_enabled(context, properties_keyword)` and adds `properties_closed_type_strict_undeclared_fast_annotations` — this branch has neither.

Any consumer that asks fast mode for `properties` annotations (e.g. to know which properties matched) on an exact-property-set schema silently loses `required` and `additionalProperties: false` enforcement for multi-property objects. Workaround: use exhaustive mode or disable `properties` annotations.

## C3

**Claim:** The committed 2019-09 C++ suite lacks annotation-enabled fast-mode regression coverage for the affected property-name handling. Parts: (a) 2019-09 wrong-name case, (b) 2019-09 valid control.

**Branch:** `evalon/blaze-conf-40b85c12` (remote `claims` = `https://github.com/sourcemeta-evals/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching`), commit `bc2c5601`, merge-base with `origin/main` `2f5ba6fd`. Worktree `~/wt/c40`, unit binary built with `cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Debug -DBLAZE_TESTS=ON && cmake --build ./build --target sourcemeta_blaze_evaluator_unit --parallel $(nproc)`.

**Verdict: CONFIRMED (both parts).**

### What the branch changed

```sh
cd ~/wt/c40 && git diff --stat 2f5ba6fd HEAD | cat
```

```console
 ports/javascript/index.mjs                         |  25 ++-
 src/compiler/default_compiler_draft3.h             |   5 +-
 .../include/sourcemeta/blaze/evaluator_dispatch.h  |  67 ++++---
 test/evaluator/evaluator_2019_09.json              | 209 +++++++++++++++++++++
 test/evaluator/evaluator_2020_12.json              | 209 +++++++++++++++++++++
 test/evaluator/evaluator_2020_12_test.cc           |  97 ++++++++++
 test/evaluator/evaluator_draft4.json               | 126 +++++++++++++
 test/evaluator/evaluator_draft7.json               | 203 ++++++++++++++++++++
 8 files changed, 906 insertions(+), 35 deletions(-)
```

`test/evaluator/evaluator_2019_09_test.cc` is **not** modified. The C++ tests the branch adds are in `evaluator_2020_12_test.cc` only:

```sh
cd ~/wt/c40 && git diff 2f5ba6fd HEAD -- test/evaluator/evaluator_2020_12_test.cc | grep -E '^\+(TEST\(|  tweaks|  EVALUATE_WITH)'
```

```console
+TEST(properties_closed_type_strict_undeclared_fast_annotations) {
+  tweaks.annotations =
+  EVALUATE_WITH_TRACE_FAST_FAILURE_TWEAKED(schema, instance, 1, "", tweaks);
+TEST(properties_closed_type_strict_valid_fast_annotations) {
+  tweaks.annotations =
+  EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 5, "", tweaks);
```

### Inspect + run (`verify/repro/c3_demo.sh`)

The script lists the branch's C++ test changes, enumerates annotation-enabled fast-mode tests on exact property-set schemas (`additionalProperties: false`) per suite, lists every annotation-enabled fast-mode 2019-09 test with its macro, runs the committed 2019-09 suite and the 2020-12 pair, then appends `verify/repro/c3_probe_2019_09_test.cc` (the two missing 2019-09 cases), rebuilds, runs them, and reverts the edit.

```sh
cd ~/wt/c40 && bash verify/repro/c3_demo.sh; git status --short
```

```console
[c3] merge-base with origin/main: 2f5ba6fdedef602197b6b2b4eaa9a17fb7839863
[c3] C++ test files changed by this branch:
 test/evaluator/evaluator_2020_12_test.cc | 97 ++++++++++++++++++++++++++++++++
 1 file changed, 97 insertions(+)
[c3] test/evaluator/evaluator_2019_09_test.cc: annotation-enabled fast-mode tests on exact property-set schemas
   total: 0
[c3] test/evaluator/evaluator_2020_12_test.cc: annotation-enabled fast-mode tests on exact property-set schemas
   TEST(annotation_properties_closed_object_fast_keeps_closure) { -> FAILURE (invalid case)
   TEST(properties_closed_type_strict_undeclared_fast_annotations) { -> FAILURE (invalid case)
   TEST(properties_closed_type_strict_valid_fast_annotations) { -> SUCCESS (valid control)
   total: 3
[c3] every annotation-enabled fast-mode 2019-09 test and the macro it uses:
   TEST(annotation_fast_whitelist_unused_anyof_short_circuits) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 2, "", tweaks);
   TEST(annotation_fast_whitelist_unused_contains_short_circuits) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 2, "", tweaks);
   TEST(annotation_fast_whitelist_unused_properties_fusion) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 1, "", tweaks);
   TEST(annotation_fast_whitelist_anyof_branch_not_short_circuited) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 4, "", tweaks);
   TEST(annotation_fast_metadata_title) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 1, "", tweaks);
   TEST(annotation_fast_format) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 1, "", tweaks);
   TEST(annotation_fast_content_media_type) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 1, "", tweaks);
   TEST(annotation_fast_properties) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 2, "", tweaks);
   TEST(annotation_fast_pattern_properties) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 3, "", tweaks);
   TEST(annotation_fast_additional_properties) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 4, "", tweaks);
   TEST(annotation_fast_items_array) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 5, "", tweaks);
   TEST(annotation_fast_additional_items) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 6, "", tweaks);
   TEST(annotation_fast_items_schema) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 4, "", tweaks);
   TEST(annotation_fast_contains) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 5, "", tweaks);
   TEST(annotation_fast_contains_min_max) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 6, "", tweaks);
   TEST(annotation_fast_contains_nested_annotation) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 5, "", tweaks);
   TEST(annotation_fast_unevaluated_properties) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 4, "", tweaks);
   TEST(annotation_fast_unevaluated_items) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 5, "", tweaks);
   TEST(annotation_fast_unknown_keyword) { ::   EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 1, "", tweaks);
[c3] committed 2019-09 suite:
# 198 passed, 0 failed
[c3] committed 2020-12 pair the branch added:
ok 1 - evaluator_2020_12.properties_closed_type_strict_undeclared_fast_annotations
ok 2 - evaluator_2020_12.properties_closed_type_strict_valid_fast_annotations
# 2 passed, 0 failed
[c3] appending the two missing 2019-09 cases and running them:
[c3-probe] 2019-09 fast+annotations{properties} instructions: AssertionDefinesExactlyStrict,AssertionPropertyTypeStrict,AnnotationEmit,AssertionPropertyTypeStrict,AnnotationEmit,
[c3-probe] 2019-09 wrong-name -> false (expected false)
ok 1 - evaluator_2019_09.c3_probe_2019_09_wrong_name_fast_annotations
[c3-probe] 2019-09 valid control -> true (expected true)
ok 2 - evaluator_2019_09.c3_probe_2019_09_valid_control_fast_annotations
# 2 passed, 0 failed
?? verify/
```

### Per-part result

- **2019-09 wrong-name case — CONFIRMED (absent).** All 19 annotation-enabled fast-mode tests in `evaluator_2019_09_test.cc` use `EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED`; none asserts a failure, and none uses an exact property-set schema (`additionalProperties: false`; count `0` above). The suite passes 198/198 regardless.
- **2019-09 valid control — CONFIRMED (absent).** The only 2019-09 annotation-enabled fast tests over `properties` are `annotation_fast_properties` (`{ "properties": { "foo": ... } }`, no `required`, no `additionalProperties`) and `annotation_fast_whitelist_unused_properties_fusion` (annotations enabled only for `x-test-custom`, not `properties`). Neither exercises the affected property-name handling (closed exact property set with `properties` annotations on).

The branch did add exactly this pair for 2020-12 (`properties_closed_type_strict_undeclared_fast_annotations` / `properties_closed_type_strict_valid_fast_annotations`), so the 2019-09 gap is an omission relative to the branch's own pattern. When the two missing 2019-09 cases are appended they pass on this branch (the behaviour itself is correct here, thanks to the `default_compiler_draft3.h` guard), i.e. the gap is coverage, not a present 2019-09 defect on this branch.

### Impact reasoning

2019-09 and 2020-12 share the compiler path in `default_compiler_draft3.h` that the branch modified (`!annotations_enabled(context, properties_keyword)` guard), so the annotation-enabled fast-mode wrong-name behaviour for 2019-09 is protected only indirectly by the 2020-12 tests. A future change that makes the guard dialect-specific, or a 2019-09-only compiler difference, would regress unnoticed for 2019-09.

## C4

**Claim:** The corpus additions duplicate the same broad long-key, missing-name, name-collision, and item-handling scenario matrices across all six official dialect files.

**Branch:** `evalon/blaze-conf-8a277501` (remote `claims` = `https://github.com/sourcemeta-evals/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching`), commit `9f2b5bb3`, merge-base with `origin/main` `2f5ba6fd`. Worktree `~/wt/c8a`.

**Verdict: CONFIRMED.**

### Diff stat of the corpus files

```sh
cd ~/wt/c8a && git diff --stat 2f5ba6fd HEAD -- test/evaluator/'*.json' | cat
```

```console
 test/evaluator/evaluator_2019_09.json | 1188 +++++++++++++++++++++++++++++++
 test/evaluator/evaluator_2020_12.json | 1233 +++++++++++++++++++++++++++++++++
 test/evaluator/evaluator_draft3.json  | 1148 ++++++++++++++++++++++++++++++
 test/evaluator/evaluator_draft4.json  | 1071 ++++++++++++++++++++++++++++
 test/evaluator/evaluator_draft6.json  | 1062 ++++++++++++++++++++++++++++
 test/evaluator/evaluator_draft7.json  | 1062 ++++++++++++++++++++++++++++
 6 files changed, 6764 insertions(+)
```

(Line counts differ only because the recorded trace arrays differ in length per dialect; the scenario count added is 19 in every file, see below.)

### Compare the added scenarios (`verify/repro/c4_compare_dialects.mjs`)

The script diffs each of the six official dialect files against the merge base, lists the added scenarios, buckets them into the four matrices named by the claim, and compares `schema` + `instance` + `valid` across dialects after normalising **only** dialect-required syntax: the `$schema` value (top-level and nested) and draft-03's property-level `required: true` folded into a `required` array. It then reports what still differs with only `$schema` ignored, and finally the recorded post-trace instruction sequences per dialect.

```sh
cd ~/wt/c8a && node verify/repro/c4_compare_dialects.mjs
```

```console
test/evaluator/evaluator_draft3.json: +19 scenarios  {"object/long keys":7,"object/missing names":1,"object/name collisions":2,"object/other":1,"item handling/name collisions":4,"item handling/other":3,"item handling/long keys":1}
test/evaluator/evaluator_draft4.json: +19 scenarios  {"object/long keys":7,"object/missing names":1,"object/name collisions":2,"object/other":1,"item handling/name collisions":4,"item handling/other":3,"item handling/long keys":1}
test/evaluator/evaluator_draft6.json: +19 scenarios  {"object/long keys":7,"object/missing names":1,"object/name collisions":2,"object/other":1,"item handling/name collisions":4,"item handling/other":3,"item handling/long keys":1}
test/evaluator/evaluator_draft7.json: +19 scenarios  {"object/long keys":7,"object/missing names":1,"object/name collisions":2,"object/other":1,"item handling/name collisions":4,"item handling/other":3,"item handling/long keys":1}
test/evaluator/evaluator_2019_09.json: +19 scenarios  {"object/long keys":7,"object/missing names":1,"object/name collisions":2,"object/other":1,"item handling/name collisions":4,"item handling/other":3,"item handling/long keys":1}
test/evaluator/evaluator_2020_12.json: +19 scenarios  {"object/long keys":7,"object/missing names":1,"object/name collisions":2,"object/other":1,"item handling/name collisions":4,"item handling/other":3,"item handling/long keys":1}

identical added-description lists across all six files: true
draft3: 19/19 scenarios match 2020-12 after dialect-syntax normalisation
draft4: 19/19 scenarios match 2020-12 after dialect-syntax normalisation
draft6: 19/19 scenarios match 2020-12 after dialect-syntax normalisation
draft7: 19/19 scenarios match 2020-12 after dialect-syntax normalisation
2019_09: 19/19 scenarios match 2020-12 after dialect-syntax normalisation
2020_12: 19/19 scenarios match 2020-12 after dialect-syntax normalisation

material (non-syntax) differences in schema/instance/valid across dialects: 0
draft3: 19/19 raw schemas differ from 2020-12 before normalisation
draft4: 19/19 raw schemas differ from 2020-12 before normalisation
draft6: 19/19 raw schemas differ from 2020-12 before normalisation
draft7: 19/19 raw schemas differ from 2020-12 before normalisation
2019_09: 19/19 raw schemas differ from 2020-12 before normalisation
2020_12: 0/19 raw schemas differ from 2020-12 before normalisation
draft3: 19/19 raw schemas still differ from 2020-12 once every $schema keyword (top-level and nested) is ignored
draft4: 0/19 raw schemas still differ from 2020-12 once every $schema keyword (top-level and nested) is ignored
draft6: 0/19 raw schemas still differ from 2020-12 once every $schema keyword (top-level and nested) is ignored
draft7: 0/19 raw schemas still differ from 2020-12 once every $schema keyword (top-level and nested) is ignored
2019_09: 0/19 raw schemas still differ from 2020-12 once every $schema keyword (top-level and nested) is ignored
2020_12: 0/19 raw schemas still differ from 2020-12 once every $schema keyword (top-level and nested) is ignored

fast: 8/19 scenarios have identical recorded post-trace instruction sequences across all six dialects; 11 differ
  exactly_type_strict_long_keys_valid -> draft3:LoopPropertiesExactlyTypeStrict,AssertionTypeStrict | draft4:LoopPropertiesExactlyTypeStrict | draft6:LoopPropertiesExactlyTypeStrict | draft7:LoopPropertiesExactlyTypeStrict | 2019_09:LoopPropertiesExactlyTypeStrict | 2020_12:LoopPropertiesExactlyTypeStrict
  items_exactly_type_strict_hash2_length_collision -> draft3:LoopPropertiesExactlyTypeStrictHash,AssertionTypeStrict,LoopPropertiesExactlyTypeStrictHash,LoopItems | draft4:LoopItemsPropertiesExactlyTypeStrictHash | draft6:LoopItemsPropertiesExactlyTypeStrictHash | draft7:LoopItemsPropertiesExactlyTypeStrictHash | 2019_09:LoopItemsPropertiesExactlyTypeStrictHash | 2020_12:LoopPropertiesExactlyTypeStrictHash,LoopPropertiesExactlyTypeStrictHash,LoopItemsFrom
  ... (9 more item-handling / valid-control lines of the same shape)

exhaustive: 8/19 scenarios have identical recorded post-trace instruction sequences across all six dialects; 11 differ
  ... (2019-09/2020-12 add `Annotation` steps; 2020-12 uses `LoopItemsFrom`/`LogicalWhenArraySizeGreater`; draft3 orders `AssertionTypeStrict` differently)

added scenario descriptions (identical list in every file):
  exactly_type_strict_long_keys_unknown
  exactly_type_strict_long_keys_valid
  exactly_type_strict_long_keys_partial
  exactly_type_strict_mixed_keys_unknown
  exactly_type_strict_mixed_keys_valid
  exactly_type_strict_long_keys_missing
  exactly_type_strict_long_keys_empty
  exactly_type_strict_hash_missing
  exactly_type_strict_hash_length_collision
  exactly_type_strict_hash_length_collision_partial
  exactly_type_strict_hash_reordered_valid
  items_exactly_type_strict_hash1_length_collision
  items_exactly_type_strict_hash2_length_collision
  items_exactly_type_strict_hash2_valid
  items_exactly_type_strict_hash3_length_collision
  items_exactly_type_strict_hash3_valid
  items_exactly_type_strict_hash4_length_collision
  items_exactly_type_strict_hash4_valid
  items_exactly_type_strict_long_keys_unknown
```

### Example of the only draft-03 delta (`exactly_type_strict_long_keys_unknown`)

```sh
cd ~/wt/c8a && jq -c '.[] | select(.description=="exactly_type_strict_long_keys_unknown") | .schema' test/evaluator/evaluator_draft3.json test/evaluator/evaluator_2020_12.json
```

```json
{"$schema":"http://json-schema.org/draft-03/schema#","type":"object","properties":{"notification_webhook_endpoint_url_for_production_alerts":{"type":"string","required":true},"notification_webhook_endpoint_url_for_staging_alerts":{"type":"string","required":true}},"additionalProperties":false}
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","properties":{"notification_webhook_endpoint_url_for_production_alerts":{"type":"string"},"notification_webhook_endpoint_url_for_staging_alerts":{"type":"string"}},"required":["notification_webhook_endpoint_url_for_production_alerts","notification_webhook_endpoint_url_for_staging_alerts"],"additionalProperties":false}
```

### Reasoning

- All six files add the same 19 scenario descriptions in the same order, with the same bucket counts for the four matrices named by the claim (long keys 8, missing names 1, name collisions 6, item handling 8 — counted across the object/item split above).
- For draft4, draft6, draft7 and 2019-09 the added schemas are byte-identical to 2020-12 once every `$schema` keyword is ignored; for draft3 the only additional difference is the dialect-mandated `required: true` placement. Instances and expected `valid` are identical in all six files (`material ... differences: 0`).
- The recorded instruction traces do differ for 11/19 scenarios, but those differences are how the compiler lowers the *same* schema under each dialect (draft-03 emits a separate `AssertionTypeStrict`; 2020-12 lowers `items` to `LoopItemsFrom`; 2019-09/2020-12 emit `Annotation` steps in exhaustive mode). They are not scenario design choices — no scenario uses a dialect-specific keyword, feature, or instance shape — so the scenarios do not "exercise materially different dialect behaviour" in the sense of the refute criterion; they exercise the same evaluator handler with the same inputs six times.
- This is a duplication/maintenance finding, not a correctness defect: the corpus runs pass and the eval's own "no duplicate added scenarios within a file" check is unaffected (it only compares within one file).
