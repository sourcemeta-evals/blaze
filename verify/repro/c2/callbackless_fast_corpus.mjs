// The test that the committed JavaScript suite lacks: every `fast` scenario of
// test/evaluator/evaluator_*.json compiled in fast mode and validated through the
// public callbackless `Blaze.validate(instance)`, compared against `valid`.
//
// Run from the root of a Blaze checkout that has build/contrib/sourcemeta_blaze_contrib_compile:
//   node ./verify/repro/c2/callbackless_fast_corpus.mjs
import { readFileSync, readdirSync } from 'node:fs';
import { resolve } from 'node:path';

const root = process.cwd();
const { Blaze } = await import(resolve(root, 'ports/javascript/index.mjs'));
const { compileSchema } = await import(resolve(root, 'ports/javascript/compile.mjs'));
const dir = resolve(root, 'test/evaluator');

let total = 0;
let mismatches = 0;
const files = readdirSync(dir)
  .filter((file) => file.startsWith('evaluator_') && file.endsWith('.json'))
  .sort();
for (const file of files) {
  const corpusPath = resolve(dir, file);
  const corpus = JSON.parse(readFileSync(corpusPath, 'utf8'), Blaze.reviver);
  for (let index = 0; index < corpus.length; index++) {
    const entry = corpus[index];
    if (!entry.fast) continue;
    total++;
    const template = compileSchema(corpusPath, { mode: 'fast', path: `/${index}/schema` });
    const result = new Blaze(template).validate(entry.instance);
    if (result !== entry.valid) {
      mismatches++;
      console.log(`MISMATCH ${file} #${index} ${entry.description}: got ${result} expected ${entry.valid}`);
    }
  }
}
console.log(`callbackless fast corpus scenarios: ${total}, mismatches: ${mismatches}`);
process.exitCode = mismatches === 0 ? 0 : 1;
