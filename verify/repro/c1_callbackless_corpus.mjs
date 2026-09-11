// Run from the repo root after building the compile CLI (sourcemeta_blaze_contrib_compile):
//   node verify/repro/c1_callbackless_corpus.mjs [test/evaluator/evaluator_2020_12.json ...]
// Compiles every fast-mode corpus scenario and evaluates it through the public
// `Blaze.validate(instance)` API with NO trace callback (the path the committed
// JS tests never take), reporting any scenario whose result differs from `valid`.
import { readFileSync, readdirSync } from 'node:fs';
import { join, resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { Blaze } from '../../ports/javascript/index.mjs';
import { compileSchema } from '../../ports/javascript/compile.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
const CORPUS_DIR = join(ROOT, 'test', 'evaluator');
const files = process.argv.length > 2
  ? process.argv.slice(2).map((file) => resolve(file))
  : readdirSync(CORPUS_DIR)
      .filter((file) => file.startsWith('evaluator_') && file.endsWith('.json'))
      .sort()
      .map((file) => join(CORPUS_DIR, file));

let scenarios = 0;
let mismatches = 0;
for (const file of files) {
  const corpus = JSON.parse(readFileSync(file, 'utf8'), Blaze.reviver);
  for (let index = 0; index < corpus.length; index++) {
    const entry = corpus[index];
    if (!entry.fast) continue;
    scenarios += 1;
    const template = compileSchema(file, { mode: 'fast', path: `/${index}/schema` });
    const evaluator = new Blaze(template);
    const result = evaluator.validate(entry.instance);
    if (result !== entry.valid) {
      mismatches += 1;
      console.log(`MISMATCH ${file}#${index} ${entry.description}: expected ${entry.valid}, got ${result}`);
    }
  }
}
console.log(`callbackless fast-mode scenarios checked: ${scenarios}, mismatches: ${mismatches}`);
process.exit(mismatches === 0 ? 0 : 1);
