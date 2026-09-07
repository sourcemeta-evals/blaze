#!/usr/bin/env python3
"""C4 repro: show that the scenarios added to the Draft 7, 2019-09 and 2020-12
evaluator corpus files are separate copies of one materially equivalent matrix.

Usage:
    python3 compare_corpus_duplication.py <repo> <base-commit> <head-commit>

For each of the three corpus files it loads the base and head revisions via
`git show`, isolates the scenarios added at head, and then compares the added
scenarios across dialects after normalising the `$schema` URI (the only part of
a fixture that is legitimately dialect-specific).
"""
import json
import subprocess
import sys

FILES = [
    "test/evaluator/evaluator_draft7.json",
    "test/evaluator/evaluator_2019_09.json",
    "test/evaluator/evaluator_2020_12.json",
]


def load(repo, rev, path):
    text = subprocess.check_output(["git", "-C", repo, "show", f"{rev}:{path}"])
    return json.loads(text)


def strip_dialect(value):
    if isinstance(value, dict):
        return {
            k: ("<DIALECT>" if k == "$schema" else strip_dialect(v))
            for k, v in value.items()
        }
    if isinstance(value, list):
        return [strip_dialect(v) for v in value]
    return value


def canon(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"))


def main():
    repo, base, head = sys.argv[1:4]
    added = {}
    for path in FILES:
        before = {canon(e) for e in load(repo, base, path)}
        after = load(repo, head, path)
        new = [e for e in after if canon(e) not in before]
        added[path] = new
        print(f"{path}: base={len(before)} head={len(after)} added={len(new)}")

    names = [[e["description"] for e in added[p]] for p in FILES]
    print("\nadded descriptions identical across the 3 files:",
          names[0] == names[1] == names[2])
    print("added descriptions (in order):")
    for d in names[0]:
        print("  -", d)

    ref = FILES[0]
    for path in FILES[1:]:
        same_full = same_schema = same_inst = same_valid = same_fast = 0
        for a, b in zip(added[ref], added[path]):
            same_full += canon(strip_dialect(a)) == canon(strip_dialect(b))
            same_schema += canon(strip_dialect(a["schema"])) == canon(
                strip_dialect(b["schema"]))
            same_inst += canon(a["instance"]) == canon(b["instance"])
            same_valid += a["valid"] == b["valid"]
            same_fast += canon(a.get("fast")) == canon(b.get("fast"))
        n = len(added[ref])
        print(f"\n{ref} vs {path} (after normalising $schema): "
              f"identical entries={same_full}/{n} schema={same_schema}/{n} "
              f"instance={same_inst}/{n} valid={same_valid}/{n} "
              f"fast-trace={same_fast}/{n}")
        for a, b in zip(added[ref], added[path]):
            if canon(strip_dialect(a)) != canon(strip_dialect(b)):
                keys = [k for k in set(a) | set(b)
                        if canon(strip_dialect(a.get(k))) !=
                        canon(strip_dialect(b.get(k)))]
                print(f"  differs: {a['description']!r} -> keys {keys}")

    # Only the `$schema` URI differs per dialect?
    dialects = set()
    for path in FILES:
        for e in added[path]:
            dialects.add(e["schema"].get("$schema"))
    print("\ndistinct $schema values across added scenarios:", sorted(dialects))


if __name__ == "__main__":
    main()
