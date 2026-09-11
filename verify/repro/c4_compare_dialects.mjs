// Run from the repo root of the C4 branch (evalon/blaze-conf-8a277501):
//   node verify/repro/c4_compare_dialects.mjs [merge-base-ref]
// Diffs each of the six official dialect corpus files against the merge base,
// lists the scenarios each file added, buckets them into the four matrices
// named by C4 (long keys / missing names / name collisions / item handling),
// and compares schema+instance+valid across dialects after normalising only
// dialect-required syntax ($schema value; draft-03 property-level
// `required: true` -> `required` array). Prints which dialects match exactly
// and which scenario/dialect pairs differ materially.
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';

const base = process.argv[2] ??
  execFileSync('git', ['merge-base', 'HEAD', 'origin/main']).toString().trim();
const files = ['draft3', 'draft4', 'draft6', 'draft7', '2019_09', '2020_12']
  .map((d) => [d, `test/evaluator/evaluator_${d}.json`]);

const sortKeys = (v) => {
  if (Array.isArray(v)) return v.map(sortKeys);
  if (v && typeof v === 'object') {
    return Object.fromEntries(Object.keys(v).sort().map((k) => [k, sortKeys(v[k])]));
  }
  return v;
};

// Normalise only dialect-required syntax differences
const normaliseSchema = (schema) => {
  const walk = (node) => {
    if (Array.isArray(node)) return node.map(walk);
    if (!node || typeof node !== 'object') return node;
    const out = {};
    for (const [k, v] of Object.entries(node)) {
      if (k === '$schema') continue;
      out[k] = walk(v);
    }
    // draft-03: `required: true` inside each property -> `required` array
    if (out.properties && typeof out.properties === 'object') {
      const required = [];
      for (const [name, sub] of Object.entries(out.properties)) {
        if (sub && typeof sub === 'object' && sub.required === true) {
          required.push(name);
          delete sub.required;
        }
      }
      if (required.length > 0 && !Array.isArray(out.required)) out.required = required;
      if (Array.isArray(out.required)) out.required = [...out.required].sort();
    }
    return out;
  };
  return sortKeys(walk(schema));
};

const bucket = (description) => {
  if (description.includes('long_keys') || description.includes('mixed_keys')) return 'long keys';
  if (description.includes('missing') || description.includes('empty')) return 'missing names';
  if (description.includes('collision')) return 'name collisions';
  return 'other';
};

const added = {};
for (const [dialect, file] of files) {
  const before = JSON.parse(execFileSync('git', ['show', `${base}:${file}`]).toString());
  const after = JSON.parse(readFileSync(file, 'utf8'));
  const beforeDescriptions = new Set(before.map((t) => t.description));
  added[dialect] = after.filter((t) => !beforeDescriptions.has(t.description));
  const buckets = {};
  for (const t of added[dialect]) {
    const item = t.description.startsWith('items_') ? 'item handling' : 'object';
    const key = `${item}/${bucket(t.description)}`;
    buckets[key] = (buckets[key] ?? 0) + 1;
  }
  console.log(`${file}: +${added[dialect].length} scenarios  ${JSON.stringify(buckets)}`);
}

const descriptionSets = Object.values(added).map((list) => list.map((t) => t.description).join('\n'));
console.log(`\nidentical added-description lists across all six files: ${descriptionSets.every((s) => s === descriptionSets[0])}`);

const canonical = (t) => JSON.stringify({
  schema: normaliseSchema(t.schema),
  instance: sortKeys(t.instance),
  valid: t.valid,
});
const reference = Object.fromEntries(added['2020_12'].map((t) => [t.description, canonical(t)]));
let material = 0;
for (const [dialect] of files) {
  const diffs = added[dialect].filter((t) => reference[t.description] !== canonical(t));
  console.log(`${dialect}: ${added[dialect].length - diffs.length}/${added[dialect].length} scenarios match 2020-12 after dialect-syntax normalisation` +
    (diffs.length ? `; differing: ${diffs.map((t) => t.description).join(', ')}` : ''));
  material += diffs.length;
}
console.log(`\nmaterial (non-syntax) differences in schema/instance/valid across dialects: ${material}`);

// Raw (un-normalised) schema differences, to show what the dialect syntax delta is
const raw = (t) => JSON.stringify(sortKeys(t.schema));
for (const [dialect] of files) {
  const rawDiffs = added[dialect].filter((t, i) => raw(t) !== raw(added['2020_12'][i])).length;
  console.log(`${dialect}: ${rawDiffs}/${added[dialect].length} raw schemas differ from 2020-12 before normalisation`);
}

// What exactly differs before normalisation: for draft4/6/7/2019-09 only the
// `$schema` value; for draft3 additionally the placement of `required`.
const stripSchemaKeyword = (node) => {
  if (Array.isArray(node)) return node.map(stripSchemaKeyword);
  if (!node || typeof node !== 'object') return node;
  return Object.fromEntries(Object.entries(node).filter(([k]) => k !== '$schema').map(([k, v]) => [k, stripSchemaKeyword(v)]));
};
const rawNoSchema = (t) => JSON.stringify(sortKeys(stripSchemaKeyword(t.schema)));
for (const [dialect] of files) {
  const diffs = added[dialect].filter((t, i) => rawNoSchema(t) !== rawNoSchema(added['2020_12'][i])).length;
  console.log(`${dialect}: ${diffs}/${added[dialect].length} raw schemas still differ from 2020-12 once every $schema keyword (top-level and nested) is ignored`);
}

// Recorded instruction traces (the corpus' own expected `post` sequences) do
// differ between dialects because each dialect compiles differently; this is
// listed so the reader can judge it separately from the scenario inputs above.
for (const mode of ['fast', 'exhaustive']) {
  const seq = (t) => (t[mode]?.post ?? []).map((p) => p[1]).join(',');
  let same = 0;
  const differing = [];
  added['2020_12'].forEach((ref, i) => {
    const sequences = files.map(([d]) => `${d}:${seq(added[d][i])}`);
    if (sequences.every((s) => s.split(':')[1] === sequences[0].split(':')[1])) same++;
    else differing.push(`${ref.description} -> ${sequences.join(' | ')}`);
  });
  console.log(`\n${mode}: ${same}/${added['2020_12'].length} scenarios have identical recorded post-trace instruction sequences across all six dialects; ${differing.length} differ`);
  for (const line of differing) console.log(`  ${line}`);
}

console.log('\nadded scenario descriptions (identical list in every file):');
for (const t of added['2020_12']) console.log(`  ${t.description}`);
