## Setup

All claim-target branches live in the sibling repository `sourcemeta-evals/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching` (not on `origin`); each was checked out into its own worktree. Builds used the eval's Debug configuration (`-DCMAKE_COMPILE_WARNING_AS_ERROR=ON -DBLAZE_TESTS=ON`) and only the targets needed for each claim.

```sh
cd ~/repos/blaze
git remote add claims https://github.com/sourcemeta-evals/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching
for b in 3ee9a13e 0307525d 423d1e31 5a54d79d; do
  git fetch claims evalon/blaze-conf-$b && git worktree add -f ../blaze-wt-$b FETCH_HEAD
done
for b in 5a54d79d 3ee9a13e 0307525d 423d1e31; do
  d=~/repos/blaze-wt-$b
  cmake -S $d -B $d/build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_COMPILE_WARNING_AS_ERROR=ON -DBLAZE_TESTS=ON -DBLAZE_BENCHMARK=OFF -DBLAZE_CONTRIB=ON -DBLAZE_DOCS=OFF
  cmake --build $d/build --target sourcemeta_blaze_evaluator_unit sourcemeta_blaze_evaluator_trace_suite_unit --parallel 8
done
cmake --build ~/repos/blaze-wt-3ee9a13e/build --target sourcemeta_blaze_contrib_compile --parallel 8   # for the JS runner
```

Every claim branch is a single commit on top of the shared base `2f5ba6fd` (`git log --oneline -2` in each worktree), so "added by the branch" below always means `git diff 2f5ba6fd HEAD`.

## C1

Branch: `evalon/blaze-conf-3ee9a13e` (commit `c982844d`).

Fixtures added by the branch (every corpus file), listed with `python3` over `git diff 2f5ba6fd HEAD`:

```console
$ cd ~/repos/blaze-wt-3ee9a13e && git diff 2f5ba6fd HEAD -- test/evaluator/*.json | grep '^+.*"description"' | sort | uniq -c
      6 "exact_properties_declared_names_valid"      (2019-09, 2020-12, draft4, draft6, draft7 ... valid=true, plain names)
      6 "exact_properties_hash_unordered_type_reject"
      6 "exact_properties_partially_declared_reject"
      6 "exact_properties_undeclared_names_reject"
      6 "hash_length_exact_properties_reject"        instance {"aa\u0000": "foo", "bb": "bar"} against schema declaring aa, bb
      5 "hash_length_exact_properties_valid"
      3 "hash_length_items_exact_properties_2_reject" instance [{"aa\u0000": "foo", "bb": "bar"}]                      (1 item)
      3 "hash_length_items_exact_properties_3_reject" instance [{"aa": "foo", "bb\u0000": "bar", "cc": "baz"}]        (1 item)
      3 "hash_length_items_exact_properties_4_reject" instance [{"aa": "foo", "bb": "bar", "cc": "baz", "dd\u0000": "qux"}] (1 item)
      3 "hash_length_items_exact_properties_2_valid"  instance [{"aa": "foo", "bb": "bar"}, {"bb": "bar", "aa": "foo"}] (valid=true)
      3 "hash_length_items_exact_properties_4_valid"  instance [{"dd": "qux", "cc": "baz", "bb": "bar", "aa": "foo"}]  (valid=true)
```

Per-dialect C++ tests added by the branch: `required_with_properties_annotations_fast_{empty,undeclared,valid}` in `evaluator_2020_12_test.cc` and `evaluator_2019_09_test.cc` (all use the long `notification_webhook_*` names; instances are objects).

Scanner over the *whole* committed suite on the branch (repro: `verify/repro/c1_scan_missing_fixtures.py`): part (a) = a valid fixture whose schema declares two or more NUL-containing property names; part (b) = an empty string inside any `required`; part (c) = an invalid array fixture with two or more items under an `items` object schema with `additionalProperties: false`.

```console
$ python3 ~/repos/blaze/verify/repro/c1_scan_missing_fixtures.py ~/repos/blaze-wt-3ee9a13e
[cc] '\u0000' occurrences in per-dialect C++ tests: 0
[cc] empty-string required entries in per-dialect C++ tests: 0
summary: part(a) multi-NUL positive=0 part(b) empty-required=0 part(c) invalid-after-valid-prefix=0
```

Scanner sanity check — the same script on a different branch (`evalon/blaze-conf-0307525d`) does find such fixtures, so empty output on 3ee9a13e is not a scanner defect:

```console
$ python3 ~/repos/blaze/verify/repro/c1_scan_missing_fixtures.py ~/repos/blaze-wt-0307525d
[b] .../evaluator_2020_12.json exact_properties_empty_name_nul_alias: empty required name, valid=False
[c] .../evaluator_draft4.json exact_properties_items_4_later_invalid: valid=False, array of 2 items: [{"a": "foo", "bb": "foo", "ccc": "foo", "dddd": "foo"}, {"a": "foo", "bb": "foo", "ccc": "foo", "extra": "foo"}]
```

(That branch's only schema-declared NUL fixtures declare a single NUL name, `a\u0000`, so part (a) is zero there too; irrelevant to this claim.)

Demonstration run of the branch's suite (all committed fixtures execute and pass, i.e. the gap is in what is asserted, not in a broken runner):

```console
$ cd ~/repos/blaze-wt-3ee9a13e && ./build/test/evaluator/sourcemeta_blaze_evaluator_trace_suite_unit | tail -1
# 3082 passed, 0 failed
$ ./build/test/evaluator/sourcemeta_blaze_evaluator_trace_suite_unit | grep -c "hash_length_items_exact_properties"
30
$ ./build/test/evaluator/sourcemeta_blaze_evaluator_unit --filter evaluator_2020_12.required_with_properties_annotations | tail -1
# 3 passed, 0 failed
```

Verdict: CONFIRMED (all three parts hold). Impact reasoning: the branch changed `LoopItemsPropertiesExactlyTypeStrictHash`/`Hash3` (per-item fused checks) and the size-plus-hash comparison in `evaluator_dispatch.h`, yet no committed fixture (i) proves a legitimately declared pair such as `"a"` and `"a\u0000"` is still accepted after the new length comparisons, (ii) touches the empty property name, or (iii) shows a later array item being rejected once earlier items passed — the very traversal the fused `items` handlers implement. Any regression in those paths would go unnoticed by `ctest` or `node --test`.

## C2

Branch: `evalon/blaze-conf-3ee9a13e` (commit `c982844d`).

JavaScript part — the corpus runner is unchanged by the branch and has exactly one `validate` call, which passes a callback:

```console
$ cd ~/repos/blaze-wt-3ee9a13e && git diff --stat 2f5ba6fd HEAD -- ports/javascript/trace.test.mjs
(no output: file unchanged)
$ grep -n "validate(" ports/javascript/trace.test.mjs
51:          const result = evaluator.validate(testCase.instance, (type, valid, instruction, evaluatePath, instanceLocation, annotation) => {
$ grep -c "validate(testCase.instance)" ports/javascript/trace.test.mjs
0
```

The callbackless JS path is a separate implementation (`*_fast` handlers), so this is untested code, not an alias of the tested code:

```console
$ grep -n "^function LoopPropertiesExactlyTypeStrict" ports/javascript/index.mjs
2142:function LoopPropertiesExactlyTypeStrict(instruction, instance, depth, template, evaluator) {
2176:function LoopPropertiesExactlyTypeStrictHash(instruction, instance, depth, template, evaluator) {
3714:function LoopPropertiesExactlyTypeStrict_fast(instruction, instance, depth, template, evaluator) {
3731:function LoopPropertiesExactlyTypeStrictHash_fast(instruction, instance, depth, template, evaluator) {
```

Demonstration run of the committed JS runner (passes, callback mode only):

```console
$ node --test ports/javascript/trace.test.mjs 2>&1 | grep -E "^ℹ (tests|pass|fail)"
ℹ tests 3082
ℹ pass 3082
ℹ fail 0
```

Side probe (not part of the committed suite): running the branch's own `exact_properties_*`/`hash_length_*` fixtures through callbackless `evaluator.validate(instance)` in JS gives `callbackless checks: 72 mismatches: 0`, i.e. the untested path currently behaves correctly on those fixtures.

C++ part — the annotation regressions added by the branch all go through `EVALUATE_WITH_TRACE_FAST_*_TWEAKED`, whose only `validate` call passes a callback:

```console
$ grep -n "^TEST(required_with_properties_annotations" test/evaluator/evaluator_2020_12_test.cc test/evaluator/evaluator_2019_09_test.cc
test/evaluator/evaluator_2020_12_test.cc:5642:TEST(required_with_properties_annotations_fast_empty) {
test/evaluator/evaluator_2020_12_test.cc:5674:TEST(required_with_properties_annotations_fast_undeclared) {
test/evaluator/evaluator_2020_12_test.cc:5712:TEST(required_with_properties_annotations_fast_valid) {
test/evaluator/evaluator_2019_09_test.cc:4373:TEST(required_with_properties_annotations_fast_empty) {
test/evaluator/evaluator_2019_09_test.cc:4405:TEST(required_with_properties_annotations_fast_undeclared) {
test/evaluator/evaluator_2019_09_test.cc:4443:TEST(required_with_properties_annotations_fast_valid) {
$ awk -F: '$1>5634' <(grep -n "EVALUATE_WITH_TRACE\|validate(" test/evaluator/evaluator_2020_12_test.cc)
5661:  EVALUATE_WITH_TRACE_FAST_FAILURE_TWEAKED(schema, instance, 1, "", tweaks);
5696:  EVALUATE_WITH_TRACE_FAST_FAILURE_TWEAKED(schema, instance, 1, "", tweaks);
5734:  EVALUATE_WITH_TRACE_FAST_SUCCESS_TWEAKED(schema, instance, 5, "", tweaks);
$ sed -n 25,27p test/evaluator/evaluator_utils.h
  sourcemeta::blaze::Evaluator evaluator;                                      \
  const auto result{evaluator.validate(                                        \
      schema_template, instance,                                               \
$ git diff --stat 2f5ba6fd HEAD -- test/evaluator/evaluator_test.cc test/evaluator/tracesuite.cc
(no output: neither file changed; tracesuite.cc also only calls EVALUATE_WITH_TRACE)
```

The only callbackless `validate(compiled_schema, x)` calls in `evaluator_2020_12_test.cc` are at lines 1369/1373 inside the pre-existing `unevaluated_properties_annotations_none_still_tracks` (base file is 5634 lines; unrelated to exact-property matching). Demonstration run: `--filter evaluator_2020_12.required_with_properties_annotations` → `# 3 passed, 0 failed`; same for `evaluator_2019_09`.

Verdict: CONFIRMED (both parts). Impact reasoning: `Evaluator::validate` without a callback instantiates the dispatch templates with `HasCallback=false` and the JS port routes to distinct `*_fast` functions; the task statement required the fix to "hold in every evaluation mode", but the committed suites never execute that mode for the corpus. (Contrast: branch 0307525d adds `EXPECT_EQ(evaluator.validate(compiled_schema, instance), expected_valid)` to `tracesuite.cc` and a callbackless assertion to `trace.test.mjs`.)

## C3

Branch: `evalon/blaze-conf-0307525d` (commit `1e3bd5ca`).

Tests added by the branch and the instances the annotation-enabled test validates:

```console
$ cd ~/repos/blaze-wt-0307525d && git diff 2f5ba6fd HEAD -- test/evaluator/*.cc | grep '^+TEST'
+TEST(annotation_properties_exact_set) {
+TEST(items_exact_hash_small_collections) {
$ awk '/^TEST\(annotation_properties_exact_set\)/{p=1} p&&/validate\(/{print NR": "$0} p&&/^}/{exit}' test/evaluator/evaluator_2020_12_test.cc
5696:       EXPECT_TRUE(evaluator.validate(compiled_schema, valid, callback));
5701:       EXPECT_TRUE(evaluator.validate(compiled_schema, valid));
5703:         EXPECT_FALSE(evaluator.validate(compiled_schema, invalid, callback));
5704:         EXPECT_FALSE(evaluator.validate(compiled_schema, invalid));
```

`valid`, `missing`, `replaced`, `extra` are all built with `JSON::make_object()` (lines 5657–5670); no null/boolean/integer/string/array instance exists in the test. Other tests in the branch that whitelist `"properties"` annotations (`annotation_properties_closed_object_fast_keeps_closure` line 1292, `annotation_fast_properties` line 5067, and the 2019-09/draft7 equivalents) are pre-existing and use object instances. The corpus cannot express annotation whitelists (the test's own comment: "Annotation selection cannot be expressed in the shared trace corpus"); the branch's only `non_object` fixture is `exact_properties_items_2_non_object` in `evaluator_draft4.json`, an `items` schema with no annotations.

```console
$ grep -n "non_object" test/evaluator/*.cc
(no output)
$ ./build/test/evaluator/sourcemeta_blaze_evaluator_unit --filter evaluator_2020_12.annotation_properties_exact_set | tail -1
# 1 passed, 0 failed
```

Behavioural probe (audit-only test appended to the worktree copy of `evaluator_2020_12_test.cc`, patch in this section's notes; not committed anywhere): same shape of schema with `tweaks.annotations = {"properties"}`:

```console
$ ./build/test/evaluator/sourcemeta_blaze_evaluator_unit --filter evaluator_2020_12.zz_verify_c3_probe
[C3-PROBE] mode=fast instructions: AssertionDefinesExactlyStrict AssertionPropertyTypeStrict AnnotationEmit AssertionPropertyTypeStrict AnnotationEmit
[C3-PROBE]   instance=null callbackless=false callback=false
[C3-PROBE]   instance=true callbackless=false callback=false
[C3-PROBE]   instance=1 callbackless=false callback=false
[C3-PROBE]   instance="s" callbackless=false callback=false
[C3-PROBE]   instance=[] callbackless=false callback=false
[C3-PROBE]   instance={"a":"x","bb":"y"} callbackless=true callback=true
[C3-PROBE] mode=exhaustive instructions: AssertionDefinesExactly LogicalWhenType LoopPropertiesExcept AssertionTypeStrict
[C3-PROBE]   instance=null callbackless=false callback=false
... (all five non-object instances rejected in both modes)
```

Verdict: CONFIRMED — the coverage gap is real; the probe shows the behaviour itself is currently correct (non-objects are rejected by `AssertionDefinesExactlyStrict`), so this is a latent, not an exercised, omission.

## C4

Branch: `evalon/blaze-conf-423d1e31` (commit `9a20b01a`).

Mechanism (source on the branch, `src/evaluator/include/sourcemeta/blaze/evaluator_dispatch.h` lines 2045–2103): the first loop checks `effective_type_strict_real(entry.second) != value.first` **before** comparing hashes and `break`s on the first ordering mismatch; the fallback loop that resumes from `index` only tests `none_of(hash == && size ==)` — no type check on the remaining values:

```cpp
    result = true;
    if (index < size) {
      auto iterator = object.cbegin();
      // Continue where we left
      std::advance(iterator, index);
      for (; iterator != object.cend(); ++iterator) {
        if (std::ranges::none_of(
                value.second.first, [&iterator](const auto &entry) -> bool {
                  return entry.first == iterator->hash &&
                         entry.second.size() == iterator->first.size();
                })) {
          result = false;
          break;
        }
      }
    }
```

Instrumentation (audit-only, applied to the worktree; `verify/repro/c4_instrumentation.patch`) prints a line on `std::cerr` when the fallback is entered and for every property it accepts, including the observed and expected `ValueType`. Probe test (`verify/repro/c4_probe_test.cc`, appended to the worktree's `evaluator_2020_12_test.cc`) compiles the claim's schema and validates `{"aa":"x","b":1}` with and without a callback, plus the canonical ordering as a control.

```console
$ cd ~/repos/blaze-wt-423d1e31 && cmake --build build --target sourcemeta_blaze_evaluator_unit --parallel 4
$ ./build/test/evaluator/sourcemeta_blaze_evaluator_unit --filter evaluator_2020_12.zz_verify_c4_probe
[C4-PROBE] mode=fast has_$schema=1 first_instruction=LoopPropertiesExactlyTypeStrictHash instruction_count=1
[C4-PROBE] callbackless validate({"aa":"x","b":1}) => [C4-PROBE] LoopPropertiesExactlyTypeStrictHash ordering-mismatch fallback entered index=0 size=2 HasCallback=0
[C4-PROBE] fallback accepted property "aa" (membership only, value type=4 expected=4)
[C4-PROBE] fallback accepted property "b" (membership only, value type=2 expected=4)
true
[C4-PROBE] callback validate({"aa":"x","b":1}) => [C4-PROBE] LoopPropertiesExactlyTypeStrictHash ordering-mismatch fallback entered index=0 size=2 HasCallback=1
[C4-PROBE] fallback accepted property "aa" (membership only, value type=4 expected=4)
[C4-PROBE] fallback accepted property "b" (membership only, value type=2 expected=4)
true
[C4-PROBE]   post valid=1 LoopPropertiesExactlyTypeStrictHash
[C4-PROBE] control callbackless validate({"b":1,"aa":"x"}) => false
[C4-PROBE] mode=exhaustive has_$schema=1 first_instruction=AssertionDefinesExactly instruction_count=4
[C4-PROBE] callbackless validate({"aa":"x","b":1}) => false
[C4-PROBE] callback validate({"aa":"x","b":1}) => false
```

(`ValueType` 4 = string, 2 = integer. The same probe without a `$schema` keyword throws "Could not determine the base dialect of the schema", so the repro keeps the 2020-12 `$schema`.)

Reachability part: the compiled fast-mode template is exactly one `LoopPropertiesExactlyTypeStrictHash` instruction, and the callbackless run (`HasCallback=0`) enters the ordering-mismatch fallback at `index=0`. Mechanism part: the fallback accepts `"b"` although its value type (integer) differs from the required strict type (string). Validation result recorded separately: **fast mode returns `true` for an instance every other mode rejects** (exhaustive mode → `false`; reordering the instance to match the schema order → `false`). The JS port on the same branch is not affected (`[C4-JS] callbackless validate({aa:"x",b:1}) => false` in both modes via `ports/javascript/compile.mjs` + `Blaze.validate`). The audited `*-perfect` branch does contain the type check in the fallback (`if (effective_type_strict_real(iterator->second) != value.first)` inside the "Continue where we left" loop), so this defect is specific to the 423d1e31 implementation (and inherited from base `2f5ba6fd`, whose fallback also lacks it).

Verdict: CONFIRMED (both parts). Impact reasoning: any closed object schema whose two-or-more required properties share one strict type compiles to this instruction in fast mode; an instance that lists the properties in a different order than the schema and carries a wrong-typed value is accepted. Property order differing from the schema is the everyday case (serialisers sort keys, humans reorder), so wrong-typed data passes fast validation while exhaustive validation and other validators reject it — the same class of bug the task asked to eliminate.

## C5

Branch: `evalon/blaze-conf-5a54d79d` (commit `6433c289`).

Committed test text (`test/evaluator/evaluator_2020_12_test.cc`, lines 5638–5727):

```console
$ cd ~/repos/blaze-wt-5a54d79d && grep -n "TEMPORARY_DUMP_BEGIN\|TEMPORARY_DUMP_END\|^TEST(zz_temporary_dump)\|std::cerr <<\|temporary_dump(" test/evaluator/evaluator_2020_12_test.cc
5638:// TEMPORARY_DUMP_BEGIN
5640:static auto temporary_dump(const sourcemeta::core::JSON &schema,
5650:  std::cerr << "##### " << label << " result=" << result << "\n";
5653:    std::cerr << "PRE " << index << " "
5662:    std::cerr << "POST " << index << " " << std::get<0>(entry) << " "
5669:    std::cerr << "\n  DESC: "
5678:TEST(zz_temporary_dump) {
5720:  temporary_dump(schema, invalid, none, "long invalid none");
5721:  temporary_dump(schema, invalid, title, "long invalid title");
5722:  temporary_dump(schema, invalid, properties, "long invalid properties");
5723:  temporary_dump(schema, valid, properties, "long valid properties");
5724:  temporary_dump(short_schema, short_invalid, none, "short invalid none");
5725:  temporary_dump(short_schema, short_invalid, properties, "short invalid properties");
5727:// TEMPORARY_DUMP_END
```

The function has no `EXPECT_*`, no environment/option guard, and no failure condition; it writes unconditionally. Execution with stderr captured:

```console
$ ./build/test/evaluator/sourcemeta_blaze_evaluator_unit --filter evaluator_2020_12.zz_temporary_dump 2>stderr.txt; echo "exit=$?"
TAP version 14
1..1
ok 1 - evaluator_2020_12.zz_temporary_dump
# 1 passed, 0 failed
exit=0
$ wc -l < stderr.txt
39
$ grep -n "^#####" stderr.txt
1:##### long invalid none result=0
5:##### long invalid title result=0
9:##### long invalid properties result=1
16:##### long valid properties result=1
29:##### short invalid none result=0
33:##### short invalid properties result=1
$ head -4 stderr.txt
##### long invalid none result=0
PRE 0 LoopPropertiesExactlyTypeStrict "/properties" "#/properties" ""
POST 0 0 LoopPropertiesExactlyTypeStrict "/properties" "#/properties" "" ann=null
  DESC: The required object properties were expected to be of type string
```

Full binary run: `./build/test/evaluator/sourcemeta_blaze_evaluator_unit >out 2>err` → `# 1131 passed, 0 failed`, `ok 432 - evaluator_2020_12.zz_temporary_dump`, and `err` contains the same 39 lines (the only stderr output of the whole binary).

Verdict: CONFIRMED. Impact reasoning: every `ctest`/unit run of the 2020-12 evaluator tests on this branch emits 39 lines of validation results plus complete pre/post traces to stderr on success, with no diagnostic option or failure gate. Side observation from that very dump: `##### long invalid properties result=1` and `##### short invalid properties result=1` show that on this branch the original bug's invalid instances still validate in fast mode when `properties` annotations are collected — the dump documents an unfixed mode rather than asserting on it.
