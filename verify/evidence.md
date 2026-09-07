# Audit evidence — run `v-5bba9995`

Audited implementation branch: `blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching-perfect`
(checked out as `verify/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching-v-5bba9995`,
HEAD `439783cf`). Every claim below targets a branch of
<https://github.com/sourcemeta-evals/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching>,
fetched through a second remote named `claims` and checked out into its own worktree under `~/wt/<id>`.
All four claim branches share the base commit `2f5ba6fd` ("Confirm key length in perfect-hash
exact-defines membership (#937)"), so `git diff 2f5ba6fd HEAD` in a worktree is "the claim branch's change".

Toolchain: Ubuntu, g++ 15.2.0, cmake 4.4.3, node v24.20.0, 8 cores.

Common setup (run once):

```sh
cd ~/repos/blaze
git remote add claims https://github.com/sourcemeta-evals/blaze-confirm-key-length-and-name-in-perfect-hash-exact-set-matching.git
git fetch claims evalon/blaze-conf-e9256ce6 evalon/blaze-conf-1e7ad14b evalon/blaze-conf-0671d94d evalon/blaze-conf-545f7da7
for id in e9256ce6 1e7ad14b 0671d94d 545f7da7; do
  git worktree add --detach ~/wt/$id claims/evalon/blaze-conf-$id
done
```

Resulting worktree heads:

```text
e9256ce6: 423b994319412df75c94f6669403399139bb7095   (C1)
1e7ad14b: b5b7be3d8f623197012fdb904cd0093554d1c0a6   (C2)
0671d94d: 968a152020c873c632bdd8981d10c090404adab7   (C3)
545f7da7: a59023dc03905159d119f0f4430f0cbeed376b3a   (C4)
```

---

## C1

**Claim.** `LoopPropertiesExactlyTypeStrictHash` accepts a distinct undeclared property whose
`PropertyHashJSON` value and length equal those of a required property.
Target: branch `evalon/blaze-conf-e9256ce6` (worktree `~/wt/e9256ce6`, HEAD `423b9943`).

**Verdict: REFUTED.** Every applicable ordered and fallback path rejects an undeclared name, and the
premise of the claim (two *distinct* names with equal perfect hash *and* equal length) is impossible
for the hashes this handler ever receives.

### C1.1 What the target branch changed in the handler

```sh
cd ~/wt/e9256ce6 && git diff 2f5ba6fd HEAD -- src/evaluator/include/sourcemeta/blaze/evaluator_dispatch.h | grep -n -A3 -B3 "size() !=\|size() ==" | head -40
```

Key lines of the resulting handler (`sed -n 2044,2100p src/evaluator/include/sourcemeta/blaze/evaluator_dispatch.h`):

```cpp
    std::size_t index{0};
    for (const auto &entry : object) {
      if (effective_type_strict_real(entry.second) != value.first) {
        EVALUATE_END(LoopPropertiesExactlyTypeStrictHash);
      }

      if (entry.hash != value.second.first[index].first ||
          entry.first.size() != value.second.first[index].second.size()) {
        break;                                   // ordered path: hash + length
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
                  return entry.first == iterator->hash &&
                         entry.second.size() == iterator->first.size();   // fallback path: hash + length
                })) {
          result = false;
          break;
        }
      }
    }
```

Neither path compares full names, so the claim reduces to: *can two distinct keys reach this handler
with equal hash and equal length?*

### C1.2 Which hashes reach the handler

`default_compiler_draft3.h` on the target branch only selects `LoopPropertiesExactlyTypeStrictHash`
when every required name has a *perfect* hash; otherwise it emits `LoopPropertiesExactlyTypeStrict`
(full-name set membership):

```sh
cd ~/wt/e9256ce6 && sed -n 646,668p src/compiler/default_compiler_draft3.h
```

```cpp
            sourcemeta::core::PropertyHashJSON<ValueString> hasher;
            std::vector<std::pair<ValueString, ValueStringSet::hash_type>>
                perfect_hashes;
            for (const auto &entry : required) {
              assert(required.contains(entry.first, entry.second));
              if (hasher.is_perfect(entry.second)) {
                perfect_hashes.emplace_back(entry.first, entry.second);
              }
            }

            if (perfect_hashes.size() == required.size()) {
              return {make(sourcemeta::blaze::InstructionIndex::
                               LoopPropertiesExactlyTypeStrictHash,
                           ...
            }

            return {make(
                sourcemeta::blaze::InstructionIndex::
                    LoopPropertiesExactlyTypeStrict,
                ...
```

`PropertyHashJSON::perfect` (vendor/core/src/core/json/include/sourcemeta/core/json_hash.h) memcpy's
the key bytes (≤ 31) into a zero-initialised 256-bit value at offset 1, and `is_perfect` is
`(hash.a & 255) == 0`. Keys longer than 31 bytes get a *non-perfect* hash (first 31 bytes plus a
1..255 tag derived from length/first/last byte) and therefore never reach the hash handler.

### C1.3 Hash probe: equal hash + equal length ⇒ identical name (for perfect hashes)

Standalone program (source below) compiled against the target worktree's vendored core headers:

```sh
mkdir -p ~/c1 && cd ~/c1 && W=~/wt/e9256ce6 \
 && INC=$(find $W/vendor/core/src $W/build/vendor/core/src -type d -name include | sed 's/^/-I/' | tr '\n' ' ') \
 && g++ -std=c++20 -O2 $INC hashcheck.cc -o hashcheck && ./hashcheck
```

(the `build/vendor/...` include dirs come from a configured build — `cmake -S . -B build` in the worktree.)

`hashcheck.cc`:

```cpp
#include <sourcemeta/core/json_hash.h>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <random>
using Hasher = sourcemeta::core::PropertyHashJSON<std::string>;
struct HashKey { std::size_t operator()(const Hasher::hash_type &h) const noexcept {
  return std::hash<unsigned long long>{}(static_cast<unsigned long long>(h.a)) ^ (std::hash<unsigned long long>{}(static_cast<unsigned long long>(h.b)) * 31); } };
static void dump(const char *label, const std::string &s, const Hasher::hash_type &h, const Hasher &hasher) {
  std::printf("%s len=%zu perfect=%d hash=", label, s.size(), hasher.is_perfect(h));
  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(&h);
  for (std::size_t i = 0; i < sizeof(h); i++) std::printf("%02x", bytes[i]);
  std::printf("\n");
}
int main() {
  Hasher hasher;
  std::string p = "notification_webhook_endpoint_url_for_production_alerts";
  std::string q = "notification_webhook_endpoint_uXXXXXXXXXXXXXXXXXXXXXXXs";
  auto hp = hasher(p), hq = hasher(q);
  dump("p", p, hp, hasher); dump("q", q, hq, hasher);
  std::printf("p!=q: %d, hash(p)==hash(q): %d, len equal: %d\n", p != q, hp == hq, p.size() == q.size());
  std::string a = "a", a0 = std::string("a\0", 2);
  auto ha = hasher(a), ha0 = hasher(a0);
  dump("\"a\"", a, ha, hasher); dump("\"a\\0\"", a0, ha0, hasher);
  std::printf("hash(a)==hash(a\\0): %d, len equal: %d\n", ha == ha0, a.size() == a0.size());
  // Exhaustive injectivity over all 1-byte and 2-byte keys (any byte values)
  for (std::size_t len = 1; len <= 2; len++) {
    std::unordered_set<Hasher::hash_type, HashKey> seen;
    std::size_t total = 0;
    std::string s(len, '\0');
    auto rec = [&](auto &self, std::size_t pos) -> void {
      if (pos == len) { total++; seen.insert(hasher(s)); return; }
      for (int c = 0; c < 256; c++) { s[pos] = static_cast<char>(c); self(self, pos + 1); }
    };
    rec(rec, 0);
    std::printf("len=%zu: %zu keys -> %zu distinct hashes (injective=%d)\n", len, total, seen.size(), total == seen.size());
  }
  // Randomized: same-length distinct keys for every length 1..31 never collide
  std::mt19937_64 rng(42);
  std::size_t short_collisions = 0, checks = 0;
  for (std::size_t len = 1; len <= 31; len++) {
    for (int trial = 0; trial < 20000; trial++) {
      std::string x(len, 0), y(len, 0);
      for (auto &ch : x) ch = static_cast<char>(rng());
      y = x; y[rng() % len] ^= static_cast<char>(1 + rng() % 255);
      checks++;
      if (hasher(x) == hasher(y)) short_collisions++;
    }
  }
  std::printf("random same-length distinct pairs (len 1..31): %zu checked, %zu collisions\n", checks, short_collisions);
  return 0;
}
```

Output:

```text
p len=55 perfect=0 hash=1a6e6f74696669636174696f6e5f776562686f6f6b5f656e64706f696e745f75
q len=55 perfect=0 hash=1a6e6f74696669636174696f6e5f776562686f6f6b5f656e64706f696e745f75
p!=q: 1, hash(p)==hash(q): 1, len equal: 1
"a" len=1 perfect=1 hash=0061000000000000000000000000000000000000000000000000000000000000
"a\0" len=2 perfect=1 hash=0061000000000000000000000000000000000000000000000000000000000000
hash(a)==hash(a\0): 1, len equal: 0
len=1: 256 keys -> 256 distinct hashes (injective=1)
len=2: 65536 keys -> 65536 distinct hashes (injective=1)
random same-length distinct pairs (len 1..31): 620000 checked, 0 collisions
```

Reading: the only distinct-name collisions are (a) two ≥32-byte keys (`perfect=0`, so they never
reach the hash handler) and (b) perfect hashes of *different* length (`"a"` vs `"a\0"`), which the
new length check separates. Among perfect hashes, equal hash + equal length ⇒ byte-identical name
(exhaustive for lengths 1–2, 620 000 randomized same-length pairs for lengths 1–31, zero collisions;
this follows from `perfect` being a verbatim copy of the key bytes).

### C1.4 Behavioural probes through the trace CLI (ordered and fallback paths)

Build the trace CLI on the target branch:

```sh
cd ~/wt/e9256ce6 && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBLAZE_CONTRIB=ON -DBLAZE_TESTS=OFF \
 && cmake --build build --config Release --target sourcemeta_blaze_contrib_trace -j8
```

Fixtures (written to `~/c1/`):

```json
// schema_short.json  (all names ≤31 bytes → LoopPropertiesExactlyTypeStrictHash)
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object",
 "properties":{"a":{"type":"string"},"bc":{"type":"string"}},"required":["a","bc"],"additionalProperties":false}
// inst_short_valid_ordered.json      {"a": "x", "bc": "y"}
// inst_short_valid_reordered.json    {"bc": "y", "a": "x"}
// inst_short_ordered_nul.json        {"a\u0000": "x", "bc": "y"}      (hash("a\0") == hash("a"), len differs; first entry → ordered path)
// inst_short_fallback_nul.json       {"bc": "y", "a\u0000": "x"}      (same, but reached via the fallback/none_of path)

// schema_long2.json  (55-byte names → non-perfect hashes → LoopPropertiesExactlyTypeStrict)
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object",
 "properties":{"notification_webhook_endpoint_url_for_production_alerts":{"type":"string"},
               "notification_webhook_endpoint_url_for_staging_alerts":{"type":"string"}},
 "required":["notification_webhook_endpoint_url_for_production_alerts","notification_webhook_endpoint_url_for_staging_alerts"],
 "additionalProperties":false}
// inst_long2_valid.json              {"notification_webhook_endpoint_url_for_production_alerts": "foo", "notification_webhook_endpoint_url_for_staging_alerts": "bar"}
// inst_long2_collide_ordered.json    {"notification_webhook_endpoint_uXXXXXXXXXXXXXXXXXXXXXXXs": "foo", "notification_webhook_endpoint_url_for_staging_alerts": "bar"}
// inst_long2_collide_reordered.json  {"notification_webhook_endpoint_url_for_staging_alerts": "bar", "notification_webhook_endpoint_uXXXXXXXXXXXXXXXXXXXXXXXs": "foo"}

// schema_long.json   (single 55-byte required name)
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object",
 "properties":{"notification_webhook_endpoint_url_for_production_alerts":{"type":"string"}},
 "required":["notification_webhook_endpoint_url_for_production_alerts"],"additionalProperties":false}
// inst_long_valid.json               {"notification_webhook_endpoint_url_for_production_alerts": "foo"}
// inst_long_collide.json             {"notification_webhook_endpoint_uXXXXXXXXXXXXXXXXXXXXXXXs": "foo"}

// schema_mixed.json  (one short + one long name)
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object",
 "properties":{"ab":{"type":"string"},"notification_webhook_endpoint_url_for_production_alerts":{"type":"string"}},
 "required":["ab","notification_webhook_endpoint_url_for_production_alerts"],"additionalProperties":false}
// inst_mixed_collide.json            {"ab": "x", "notification_webhook_endpoint_uXXXXXXXXXXXXXXXXXXXXXXXs": "foo"}
```

Runner (`~/c1/run_probe.sh`):

```sh
#!/usr/bin/env bash
set -u
W="$1"; D="$2"
TRACE="$W/build/contrib/sourcemeta_blaze_contrib_trace"
run() {
  local schema="$1" inst="$2"
  echo "### $schema / $inst: $(cat "$D/$inst")"
  "$TRACE" --fast "$D/$schema" "$D/$inst" 2>&1 | grep -E "LoopPropertiesExactly|^(PASS|FAIL)|Result"
  echo "exit=$?"
}
run schema_short.json inst_short_valid_ordered.json
run schema_short.json inst_short_valid_reordered.json
run schema_short.json inst_short_ordered_nul.json
run schema_short.json inst_short_fallback_nul.json
run schema_long2.json inst_long2_valid.json
run schema_long2.json inst_long2_collide_ordered.json
run schema_long2.json inst_long2_collide_reordered.json
run schema_long.json inst_long_valid.json
run schema_long.json inst_long_collide.json
run schema_mixed.json inst_mixed_collide.json
```

```sh
cd ~/c1 && ./run_probe.sh ~/wt/e9256ce6 ~/c1
```

Output:

```text
### schema_short.json / inst_short_valid_ordered.json: {"a": "x", "bc": "y"}
-> (push) "/properties" [LoopPropertiesExactlyTypeStrictHash]
<- (pass) "/properties" [LoopPropertiesExactlyTypeStrictHash]
LoopPropertiesExactlyTypeStrictHash : 1
PASS
### schema_short.json / inst_short_valid_reordered.json: {"bc": "y", "a": "x"}
-> (push) "/properties" [LoopPropertiesExactlyTypeStrictHash]
<- (pass) "/properties" [LoopPropertiesExactlyTypeStrictHash]
LoopPropertiesExactlyTypeStrictHash : 1
PASS
### schema_short.json / inst_short_ordered_nul.json: {"a\u0000": "x", "bc": "y"}
-> (push) "/properties" [LoopPropertiesExactlyTypeStrictHash]
<- (fail) "/properties" [LoopPropertiesExactlyTypeStrictHash]
LoopPropertiesExactlyTypeStrictHash : 1
FAIL
### schema_short.json / inst_short_fallback_nul.json: {"bc": "y", "a\u0000": "x"}
-> (push) "/properties" [LoopPropertiesExactlyTypeStrictHash]
<- (fail) "/properties" [LoopPropertiesExactlyTypeStrictHash]
LoopPropertiesExactlyTypeStrictHash : 1
FAIL
### schema_long2.json / inst_long2_valid.json: {...production_alerts": "foo", ...staging_alerts": "bar"}
-> (push) "/properties" [LoopPropertiesExactlyTypeStrict]
<- (pass) "/properties" [LoopPropertiesExactlyTypeStrict]
PASS
### schema_long2.json / inst_long2_collide_ordered.json: {"notification_webhook_endpoint_uXXXXXXXXXXXXXXXXXXXXXXXs": "foo", ...staging_alerts": "bar"}
-> (push) "/properties" [LoopPropertiesExactlyTypeStrict]
<- (fail) "/properties" [LoopPropertiesExactlyTypeStrict]
FAIL
### schema_long2.json / inst_long2_collide_reordered.json: {...staging_alerts": "bar", "notification_webhook_endpoint_uXXXXXXXXXXXXXXXXXXXXXXXs": "foo"}
-> (push) "/properties" [LoopPropertiesExactlyTypeStrict]
<- (fail) "/properties" [LoopPropertiesExactlyTypeStrict]
FAIL
### schema_long.json / inst_long_valid.json: {"notification_webhook_endpoint_url_for_production_alerts": "foo"}
-> (push) "/properties" [LoopPropertiesExactlyTypeStrict]
<- (pass) "/properties" [LoopPropertiesExactlyTypeStrict]
PASS
### schema_long.json / inst_long_collide.json: {"notification_webhook_endpoint_uXXXXXXXXXXXXXXXXXXXXXXXs": "foo"}
FAIL
### schema_mixed.json / inst_mixed_collide.json: {"ab": "x", "notification_webhook_endpoint_uXXXXXXXXXXXXXXXXXXXXXXXs": "foo"}
-> (push) "/properties" [LoopPropertiesExactlyTypeStrict]
<- (fail) "/properties" [LoopPropertiesExactlyTypeStrict]
FAIL
```

(`exit=0` lines are the exit status of the `grep` pipeline and were dropped above.)

### C1.5 Conclusion

- Ordered path (`inst_short_ordered_nul`) and fallback path (`inst_short_fallback_nul`) of
  `LoopPropertiesExactlyTypeStrictHash` both reject the undeclared `"a\0"` whose perfect hash equals
  `"a"`'s; valid ordered/reordered controls pass.
- The only distinct-name pairs with equal hash **and** equal length are ≥32-byte keys, and those
  schemas compile to `LoopPropertiesExactlyTypeStrict` (full-name membership), which rejects the
  colliding name in ordered, reordered, single-key and mixed-key forms.
- For perfect hashes, equal hash + equal length implies an identical key (exhaustive 1–2 byte check,
  620 000 randomized same-length pairs, zero collisions), so no undeclared distinct property can
  satisfy the claim's premise inside the hash handler. **REFUTED.**

---

## C2

**Claim.** The repository's committed JavaScript tests do not exercise fast corpus scenarios through
the public callbackless `Blaze.validate` API.
Target: branch `evalon/blaze-conf-1e7ad14b` (worktree `~/wt/1e7ad14b`, HEAD `b5b7be3d`).

**Verdict: CONFIRMED.**

### C2.1 What the committed tests call

```sh
cd ~/wt/1e7ad14b && grep -n "validate(" ports/javascript/*.test.mjs
```

```text
ports/javascript/official.test.mjs:100:              const result = evaluator.validate(testCase.data);
ports/javascript/output.test.mjs:24:          const actual = evaluator.validate(testCase.instance, format);
ports/javascript/trace.test.mjs:51:          const result = evaluator.validate(testCase.instance, (type, valid, instruction, evaluatePath, instanceLocation, annotation) => {
```

- `trace.test.mjs` is the only test that iterates `test/evaluator/evaluator_*.json` (the fast
  corpus) in both `fast` and `exhaustive` modes — and it always passes a callback.
- `official.test.mjs` is callbackless but iterates `vendor/jsonschema-test-suite` (line 11:
  `const SUITE_PATH = join(PROJECT_ROOT, 'vendor/jsonschema-test-suite');`), not the corpus.
- `output.test.mjs` iterates `test/output/output_standard_{flag,basic}.json` and always passes a
  format string; the callbackless calls seen below are `runStandard`'s internal
  `evaluator.validate(instance)` for the `flag` format (index.mjs line 4178–4179).

The public API dispatches on the second argument (`ports/javascript/index.mjs`):

```js
  validate(instance, callbackOrFormat) {
    ...
    if (callback === undefined && this._nativeValidate) {
      return this._nativeValidate(instance, this);
    }
    ...
    } else {
      evaluateInstruction = evaluateInstructionFast;   // callbackless → `*_fast` handler table
    }
```

so callback and callbackless validation run through different handler functions
(`LoopPropertiesExactlyTypeStrict` vs `LoopPropertiesExactlyTypeStrict_fast`, etc.).

### C2.2 Instrumented run of the committed tests

Preload hook `verify/repro/c2/count_validate_calls.mjs` wraps `Blaze.prototype.validate` and counts
how each `*.test.mjs` file invokes it (needs `build/contrib/sourcemeta_blaze_contrib_compile`, built
with `cmake -S . -B build -DBLAZE_CONTRIB=ON && cmake --build build --target sourcemeta_blaze_contrib_compile`):

```sh
cd ~/wt/1e7ad14b
for f in ports/javascript/trace.test.mjs ports/javascript/output.test.mjs ports/javascript/official.test.mjs; do
  echo "--- $f"
  node --import ~/repos/blaze/verify/repro/c2/count_validate_calls.mjs --test --test-reporter=tap $f 2>&1 \
    | grep -E "^# (tests|pass|fail)|VALIDATE CALL SUMMARY"
done
```

```text
--- ports/javascript/trace.test.mjs
# VALIDATE CALL SUMMARY {"ports/javascript/trace.test.mjs":{"callback":2994}}
# tests 2994
# pass 2994
# fail 0
--- ports/javascript/output.test.mjs
# VALIDATE CALL SUMMARY {"ports/javascript/output.test.mjs":{"format-string":86,"callbackless":28,"callback":58}}
# tests 86
# pass 86
# fail 0
--- ports/javascript/official.test.mjs
# VALIDATE CALL SUMMARY {"ports/javascript/official.test.mjs":{"callbackless":11144}}
# tests 11149
# pass 11149
# fail 0
```

2994 corpus scenarios × modes, **zero** callbackless calls from `trace.test.mjs`. The 28 callbackless
calls under `output.test.mjs` are the 14 `flag` fixtures × 2 modes re-entering `validate` internally.

### C2.3 The missing test, and proof that the gap is real

`verify/repro/c2/callbackless_fast_corpus.mjs` compiles every `fast` corpus scenario in fast mode and
validates it with `new Blaze(template).validate(instance)`:

```sh
cd ~/wt/1e7ad14b && node ~/repos/blaze/verify/repro/c2/callbackless_fast_corpus.mjs
```

```text
callbackless fast corpus scenarios: 1497, mismatches: 0
```

Then revert **only** the callbackless (`_fast`) half of the branch's fix in `index.mjs` (line 3708,
`LoopPropertiesExactlyTypeStrict_fast`) and re-run both the committed test and the probe:

```sh
cd ~/wt/1e7ad14b && cp ports/javascript/index.mjs /tmp/index.mjs.bak
sed -i '3708s/.*/    if (effectiveTypeStrictReal(target[key]) !== value[0]) return false;/' ports/javascript/index.mjs
git diff | grep "^[-+] "
node --test --test-reporter=tap ports/javascript/trace.test.mjs 2>&1 | grep -E "^# (tests|pass|fail)"
node ~/repos/blaze/verify/repro/c2/callbackless_fast_corpus.mjs
cp /tmp/index.mjs.bak ports/javascript/index.mjs
```

```text
-    if (!value[1].includes(key) || effectiveTypeStrictReal(target[key]) !== value[0]) return false;
+    if (effectiveTypeStrictReal(target[key]) !== value[0]) return false;
# tests 2994
# pass 2994
# fail 0
MISMATCH evaluator_2020_12.json #200 closed_typed_properties_reject_same_sized_unknown_set: got true expected false
callbackless fast corpus scenarios: 1497, mismatches: 1
```

### C2.4 Impact

With the callbackless handler regressed to the exact bug this branch set out to fix (accepting a
same-sized set of unknown properties), the committed JavaScript suite stays fully green
(2994/2994) while the public callbackless `Blaze.validate` — the API most consumers call — returns
`true` for an invalid instance. The corpus scenarios the branch added (`evaluator_2020_12.json`
`#200`, `evaluator_draft4.json`) therefore only guard the callback code path. **CONFIRMED.**

---

## C3

**Claim.** The optimized exact-set and fused-item paths do not document the correctness conditions
governing property counts, hash equality, key length, full-name comparison, and annotation-dependent
required-assertion elision.
Target: branch `evalon/blaze-conf-0671d94d` (worktree `~/wt/0671d94d`, HEAD `968a1520`).

**Verdict: CONFIRMED.** (Claim is explicitly about source comments; source inspection plus a
demonstration run is the evidence.)

### C3.1 Comments present in the four handlers

```sh
cd ~/wt/0671d94d && F=src/evaluator/include/sourcemeta/blaze/evaluator_dispatch.h
for H in LoopPropertiesExactlyTypeStrict LoopPropertiesExactlyTypeStrictHash LoopItemsPropertiesExactlyTypeStrictHash LoopItemsPropertiesExactlyTypeStrictHash3; do
  echo "=== $H"
  awk -v h="INSTRUCTION_HANDLER($H)" 'index($0,h){p=1} p{print NR": "$0} p&&/^}/{exit}' $F | grep -E "^[0-9]+: *//"
done
```

```text
=== LoopPropertiesExactlyTypeStrict
2028:     // Otherwise why emit this instruction?
=== LoopPropertiesExactlyTypeStrictHash
2048:   // TODO: Take advantage of the table of contents structure to speed up checks
2058:     // Otherwise why emit this instruction?
2061:     // The idea is to first assume the object property ordering and the
2062:     // hashes collection aligns. If they don't we do a full comparison
2063:     // from where we left of.
2082:       // Continue where we left
2085:         // NOLINTNEXTLINE(modernize-use-ranges)
=== LoopItemsPropertiesExactlyTypeStrictHash
2379:   // TODO: Take advantage of the table of contents structure to speed up checks
2381:   // Otherwise why emit this instruction?
2404:     // Unroll, for performance reasons, for small collections
2408:             // NOLINTNEXTLINE(bugprone-branch-clone)
2428:             // NOLINTNEXTLINE(bugprone-branch-clone)
2455:             // NOLINTNEXTLINE(bugprone-branch-clone)
=== LoopItemsPropertiesExactlyTypeStrictHash3
2486:   // TODO: Take advantage of the table of contents structure to speed up checks
2489:   // Otherwise why emit this instruction?
```

None of these mention why the size check is sufficient, why hash equality alone is/isn't enough, why
key length is compared, or when a full-name comparison is needed. The branch's diff adds **no**
comment lines at all:

```sh
cd ~/wt/0671d94d && git diff 2f5ba6fd HEAD -- src/evaluator/include/sourcemeta/blaze/evaluator_dispatch.h | grep -cE '^[+-] *//'
git diff 2f5ba6fd HEAD | grep -E '^\+.*//' 
```

```text
0
+      "$schema": "https://json-schema.org/draft/2020-12/schema",
+      "$schema": "https://json-schema.org/draft/2020-12/schema",
+      "$schema": "http://json-schema.org/draft-04/schema#",
+      "$schema": "http://json-schema.org/draft-04/schema#",
```

(the only `//` in added lines are inside `$schema` URLs of the JSON fixtures.)

### C3.2 `compile_required_assertions`

```sh
cd ~/wt/0671d94d && sed -n 225,239p src/compiler/default_compiler_draft3.h
```

```cpp
        if (std::ranges::all_of(properties, [](const auto &property) -> auto {
              return property.second.size() == 1 &&
                     property.second.front().type ==
                         InstructionIndex::AssertionTypeStrict;
            })) {
          std::set<ValueType> types;
          for (const auto &property : properties) {
            types.insert(std::get<ValueType>(property.second.front().value));
          }

          if (types.size() == 1) {
            // Handled in `properties`
            return {};
          }
        }
```

The elision of `required` is justified only by `// Handled in 'properties'`. It does not state the
condition under which `properties` actually handles it: the exact-set instruction is emitted only
when `!emit_annotation` (`default_compiler_draft3.h` line 617,
`if (!emit_annotation && context.mode == Mode::FastValidation && ...`). The `required` elision itself
does **not** check annotations on this branch.

### C3.3 Demonstration that the undocumented annotation condition is load-bearing

`verify/repro/c3/annotation_elision_probe.cc` compiles
`{"type":"object","properties":{"a":{"type":"string"},"b":{"type":"string"}},"required":["a","b"],"additionalProperties":false}`
in `Mode::FastValidation` twice — default tweaks, and `Tweaks::annotations = {"properties"}` — and
validates four instances.

Build the libraries on the target branch, then compile and run the probe:

```sh
cd ~/wt/0671d94d && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBLAZE_CONTRIB=ON -DBLAZE_TESTS=OFF \
 && cmake --build build --config Release --target sourcemeta_blaze_contrib_trace -j8
bash ~/repos/blaze/verify/repro/c3/build_and_run.sh ~/wt/0671d94d
```

```text
### fast, default tweaks (no annotations)
template: [5,false,false,[[[76,["properties"],[],"#/properties",0,[21,[4,[[[[0,24832,0,0],"a"],[[0,25088,0,0],"b"]],[[0,0],[1,2]]]]]]]],[]]
{} -> FAIL
{"a":"x"} -> FAIL
{"a":"x","b":"y"} -> PASS
{"a":"x","b":"y","c":"z"} -> FAIL
### fast, tweaks.annotations={"properties"}
template: [5,false,false,[[[43,["properties","a","type"],["a"],"#/properties/a/type",0,[8,4]],[50,["properties"],[],"#/properties",0,[1,"a"]],[43,["properties","b","type"],["b"],"#/properties/b/type",0,[8,4]],[50,["properties"],[],"#/properties",0,[1,"b"]]]],[]]
{} -> PASS
{"a":"x"} -> PASS
{"a":"x","b":"y"} -> PASS
{"a":"x","b":"y","c":"z"} -> PASS
```

With `properties` annotations enabled, the template contains only per-property type assertions and
annotations: no `required` instruction, no exact-set instruction, no closure. `{}` and an object
with an extra property `c` are accepted.

Contrast with the audited base branch (`~/repos/blaze`, which does carry the guard
`if (types.size() == 1 && !annotations_enabled(context, "properties"))` at
`src/compiler/default_compiler_draft3.h:238`):

```sh
cd ~/repos/blaze && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBLAZE_TESTS=OFF -DBLAZE_CONTRIB=ON \
 && cmake --build build --config Release --target sourcemeta_blaze_compiler sourcemeta_blaze_evaluator -j8
bash verify/repro/c3/build_and_run.sh ~/repos/blaze
```

```text
### fast, tweaks.annotations={"properties"}
template: [5,false,false,[[[6,["required"],[],"#/required",0,[6,["a","b"]]],[43,["properties","a","type"],["a"],"#/properties/a/type",0,[8,4]],[50,["properties"],[],"#/properties",0,[1,"a"]],[43,["properties","b","type"],["b"],"#/properties/b/type",0,[8,4]],[50,["properties"],[],"#/properties",0,[1,"b"]]]],[]]
{} -> FAIL
{"a":"x"} -> FAIL
{"a":"x","b":"y"} -> PASS
{"a":"x","b":"y","c":"z"} -> FAIL
```

### C3.4 Impact

The optimized handlers rely on a chain of non-obvious invariants — object size must equal the
declared set size; for perfect hashes equal hash + equal length ⇔ equal name, so no full-name
compare is needed there but *is* needed for non-perfect hashes (`LoopPropertiesExactlyTypeStrict`);
and `required` may be dropped only when `properties` is guaranteed to emit the exact-set
instruction, which in turn requires annotations to be off. None of this is written down at the
sites that depend on it. The C3 target branch shows the consequence: its `required` elision
silently assumes the exact-set path and, when `properties` annotations are enabled in fast mode,
accepts `{}` against a schema requiring two properties. **CONFIRMED.**

---

## C4

**Claim.** The 2019-09, 2020-12, and Draft 7 corpus files contain separate copies of materially
equivalent long-name, property-hash, and item regression matrices.
Target: branch `evalon/blaze-conf-545f7da7` (worktree `~/wt/545f7da7`, HEAD `a59023dc`).

**Verdict: CONFIRMED.**

### C4.1 Diff shape

```sh
cd ~/wt/545f7da7 && git diff --stat 2f5ba6fd HEAD && git diff --numstat 2f5ba6fd HEAD -- test/evaluator/
```

```text
 ports/javascript/index.mjs                         |   27 +-
 .../include/sourcemeta/blaze/evaluator_dispatch.h  |   88 +-
 test/evaluator/evaluator_2019_09.json              | 1007 +++++++++++++++++++
 test/evaluator/evaluator_2020_12.json              | 1037 ++++++++++++++++++++
 test/evaluator/evaluator_draft7.json               |  920 +++++++++++++++++
 5 files changed, 3054 insertions(+), 25 deletions(-)
1007	0	test/evaluator/evaluator_2019_09.json
1037	0	test/evaluator/evaluator_2020_12.json
920	0	test/evaluator/evaluator_draft7.json
```

### C4.2 Content comparison

`verify/repro/c4/compare_corpus_duplication.py` loads base and head revisions of the three files,
isolates the added scenarios, and compares them across dialects after normalising only `$schema`:

```sh
cd ~/repos/blaze && python3 verify/repro/c4/compare_corpus_duplication.py ~/wt/545f7da7 2f5ba6fd a59023dc
```

```text
test/evaluator/evaluator_draft7.json: base=97 head=115 added=18
test/evaluator/evaluator_2019_09.json: base=188 head=206 added=18
test/evaluator/evaluator_2020_12.json: base=200 head=218 added=18

added descriptions identical across the 3 files: True
added descriptions (in order):
  - exactly_type_strict_long_keys_undeclared
  - exactly_type_strict_long_keys_partial
  - exactly_type_strict_long_keys_wrong_type
  - exactly_type_strict_long_keys_valid
  - exactly_type_strict_long_keys_unordered_valid
  - hash_length_exactly_type_strict_hash_collision
  - hash_length_exactly_type_strict_hash_unordered_collision
  - hash_length_exactly_type_strict_hash_valid
  - hash_length_exactly_type_strict_hash_unordered_valid
  - hash_length_items_exactly_type_strict_hash_1_collision
  - hash_length_items_exactly_type_strict_hash_1_valid
  - hash_length_items_exactly_type_strict_hash_2_collision
  - hash_length_items_exactly_type_strict_hash_2_valid
  - hash_length_items_exactly_type_strict_hash_3_collision
  - hash_length_items_exactly_type_strict_hash_3_valid
  - hash_length_items_exactly_type_strict_hash_4_collision
  - hash_length_items_exactly_type_strict_hash_4_unordered_collision
  - hash_length_items_exactly_type_strict_hash_4_valid

test/evaluator/evaluator_draft7.json vs test/evaluator/evaluator_2019_09.json (after normalising $schema): identical entries=8/18 schema=18/18 instance=18/18 valid=18/18 fast-trace=18/18
  differs: 'exactly_type_strict_long_keys_wrong_type' -> keys ['exhaustive']
  ... (10 entries differ only in 'exhaustive')

test/evaluator/evaluator_draft7.json vs test/evaluator/evaluator_2020_12.json (after normalising $schema): identical entries=4/18 schema=18/18 instance=18/18 valid=18/18 fast-trace=9/18
  differs: 'exactly_type_strict_long_keys_wrong_type' -> keys ['exhaustive']
  ... (14 entries differ only in 'fast' and/or 'exhaustive')

distinct $schema values across added scenarios: ['http://json-schema.org/draft-07/schema#', 'https://json-schema.org/draft/2019-09/schema', 'https://json-schema.org/draft/2020-12/schema']
```

For all 18 × 3 scenarios: same description, same schema (modulo the `$schema` URI), same instance,
same expected validity. The only differences are the recorded traces, which are mechanical dialect
artefacts, e.g. for `hash_length_items_exactly_type_strict_hash_1_valid`:

```sh
cd ~/wt/545f7da7 && python3 - <<'EOF'
import json
d7={e["description"]:e for e in json.load(open("test/evaluator/evaluator_draft7.json"))}
d20={e["description"]:e for e in json.load(open("test/evaluator/evaluator_2020_12.json"))}
n="hash_length_items_exactly_type_strict_hash_1_valid"
print(json.dumps(d7[n]["schema"])); print(json.dumps(d20[n]["schema"]))
print(d7[n]["fast"]["pre"][0], d20[n]["fast"]["pre"][0])
EOF
```

```text
{"$schema": "http://json-schema.org/draft-07/schema#", "type": "array", "items": {"type": "object", "properties": {"aa": {"type": "string"}}, "required": ["aa"], "additionalProperties": false}}
{"$schema": "https://json-schema.org/draft/2020-12/schema", "type": "array", "items": {"type": "object", "properties": {"aa": {"type": "string"}}, "required": ["aa"], "additionalProperties": false}}
['LoopItems', '/items', '#/items', ''] ['LoopItemsFrom', '/items', '#/items', '']
```

(2020-12 compiles `items` to `LoopItemsFrom`, 2019-09/2020-12 exhaustive traces add `Annotation`
steps; the schema, instance and validity under test are identical.)

### C4.3 Demonstration run

Build and run the JSON-driven trace suite on the target branch; the tripled matrix appears as
3 × 18 × 2 (fast/exhaustive) = 108 test cases:

```sh
cd ~/wt/545f7da7 && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBLAZE_TESTS=ON -DBLAZE_CONTRIB=OFF \
 && cmake --build build --config Release --target sourcemeta_blaze_evaluator_trace_suite_unit -j8
B=build/test/evaluator/sourcemeta_blaze_evaluator_trace_suite_unit
$B 2>&1 | grep -E "^ok .*(hash_length_(exactly|items)|exactly_type_strict_long_keys)" \
  | sed -E 's/^ok [0-9]+ - Evaluator_trace_([a-z0-9_]+)\..*/\1/' | sort | uniq -c
$B 2>&1 | tail -1
```

```text
     36 2019_09
     36 2020_12
     36 draft7
# 3094 passed, 0 failed
```

### C4.4 Impact

Eighteen scenarios (five long-key exact-set, four property-hash collision/valid, nine fused-item
hash cases) were pasted three times with only the `$schema` URI changed, adding ~2 960 lines of
fixtures for behaviour that is dialect-independent (the exact-set/hash handlers do not branch on
dialect). Every future adjustment to these scenarios — new collision shapes, renamed instructions,
changed trace expectations — must be made in three places, and a fix applied to one dialect's copy
can silently diverge from the others. **CONFIRMED.**
