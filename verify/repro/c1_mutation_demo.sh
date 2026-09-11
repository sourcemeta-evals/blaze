#!/usr/bin/env bash
# Run from the repo root after building the compile CLI (sourcemeta_blaze_contrib_compile):
#   bash verify/repro/c1_mutation_demo.sh
#
# Demonstrates the impact of the missing callback-less coverage: the public
# `Blaze.validate(instance)` API (no callback) routes exact-property loops
# through the `*_fast` handlers / native validator, a code path distinct from
# the callback-mode handlers the committed trace tests exercise. We temporarily
# delete the property-name check from `LoopPropertiesExactlyTypeStrict_fast`,
# then show that the committed JS test file still passes while the callback-less
# corpus run catches the regression. The mutation is reverted afterwards.
set -u
cd "$(dirname "$0")/../.."
FILE=ports/javascript/index.mjs
cp "$FILE" "$FILE.c1.bak"
trap 'mv "$FILE.c1.bak" "$FILE"' EXIT

node - "$FILE" <<'EOF'
const fs = require('node:fs');
const file = process.argv[2];
const src = fs.readFileSync(file, 'utf8');
const marker = 'function LoopPropertiesExactlyTypeStrict_fast(';
const start = src.indexOf(marker);
if (start < 0) throw new Error('handler not found');
const end = src.indexOf('\n}\n', start);
const body = src.slice(start, end);
const check = "  for (let index = 0; index < strings.length; index++) {\n    if (!Object.hasOwn(target, strings[index])) return false;\n  }\n";
if (!body.includes(check)) throw new Error('name check not found in fast handler');
fs.writeFileSync(file, src.slice(0, start) + body.replace(check, '') + src.slice(end));
console.log('[c1-mutation] removed property-name check from LoopPropertiesExactlyTypeStrict_fast');
EOF

echo "[c1-mutation] running committed JS trace tests (ports/javascript/trace.test.mjs) against mutant"
node --test ports/javascript/trace.test.mjs 2>&1 | grep -E '^ℹ (tests|pass|fail)'

echo "[c1-mutation] running callback-less corpus repro against mutant"
node verify/repro/c1_callbackless_corpus.mjs test/evaluator/evaluator_2020_12.json
