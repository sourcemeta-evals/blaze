#!/usr/bin/env python3
# Run: python3 verify/repro/c1_scan_missing_fixtures.py <checkout of evalon/blaze-conf-3ee9a13e>   (prints the fixtures matching each C1 part; empty = no coverage)
import glob, json, os, re, sys
root = sys.argv[1] if len(sys.argv) > 1 else "."
def walk(o):
    yield o
    if isinstance(o, dict):
        for v in o.values(): yield from walk(v)
    elif isinstance(o, list):
        for v in o: yield from walk(v)
a = b = c = 0
for f in sorted(glob.glob(os.path.join(root, "test/evaluator/evaluator_*.json"))):
    for e in json.load(open(f)):
        s, inst, d = e["schema"], e["instance"], e["description"]
        nul = set()
        for o in walk(s):
            if isinstance(o, dict):
                nul |= {k for k in o if "\x00" in k}
                if isinstance(o.get("required"), list):
                    nul |= {r for r in o["required"] if isinstance(r, str) and "\x00" in r}
        if len(nul) >= 2 and e["valid"]:
            a += 1; print(f"[a] {f} {d}: valid=True, schema declares NUL names {sorted(nul)!r}")
        if any(isinstance(o, dict) and isinstance(o.get("required"), list) and "" in o["required"] for o in walk(s)):
            b += 1; print(f"[b] {f} {d}: empty required name, valid={e['valid']}")
        if isinstance(inst, list) and len(inst) >= 2 and e["valid"] is False and '"items"' in json.dumps(s) and '"additionalProperties": false' in json.dumps(s):
            c += 1; print(f"[c] {f} {d}: valid=False, array of {len(inst)} items: {json.dumps(inst)}")
cc = " ".join(open(p).read() for p in glob.glob(os.path.join(root, "test/evaluator/*_test.cc")))
print("[cc] '\\\\u0000' occurrences in per-dialect C++ tests:", len(re.findall(r"\\u0000", cc)))
print('[cc] empty-string required entries in per-dialect C++ tests:', len(re.findall(r'"required"\s*:\s*\[[^\]]*""', cc)))
print(f"summary: part(a) multi-NUL positive={a} part(b) empty-required={b} part(c) invalid-after-valid-prefix={c}")
