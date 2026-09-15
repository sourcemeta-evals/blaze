#!/usr/bin/env python3
# Run: python3 verify/repro/c3_hash3_branch_coverage.py <probe-binary> <worktree of evalon/blaze-conf-0e182701>  (probe from verify/repro/build_probe.sh against that worktree)
"""Map committed corpus cases that compile (fast mode) to typed-hash /
exact-property instructions onto the coverage categories of claim C3.

Usage: c3_map.py <probe-binary> <worktree>
The probe's --dump prints, for typed-hash instructions, the declared property
names in the order the compiler stored them (the order the Hash3 branches use).
"""
import itertools
import json
import os
import subprocess
import sys
import tempfile

probe, worktree = sys.argv[1], sys.argv[2]
suite_dir = os.path.join(worktree, "test/evaluator")
corpora = sorted(os.path.join(suite_dir, f) for f in os.listdir(suite_dir)
                 if f.startswith("evaluator_") and f.endswith(".json"))

PERMS = list(itertools.permutations(range(3)))
hash3_branches = {p: {"conforming": [], "single_substitution": []} for p in PERMS}
four_prop = {"conforming": [], "invalid": []}
declared_collision_controls = []
typed_hash_cases = 0


def compile_dump(schema):
    with tempfile.TemporaryDirectory() as tmp:
        sp, ip = os.path.join(tmp, "s.json"), os.path.join(tmp, "i.json")
        with open(sp, "w") as f:
            json.dump(schema, f)
        with open(ip, "w") as f:
            f.write("null")
        out = subprocess.run([probe, sp, ip, "fast", "--dump"],
                             capture_output=True, text=True)
        if out.returncode != 0:
            return []
        return [l.strip() for l in out.stdout.splitlines() if l.startswith("  ")]


def hash_of(name):
    # Mirrors PropertyHashJSON: names shorter than 32 bytes are memcpy'd into a
    # zeroed buffer (trailing NULs vanish); longer names keep the first 31
    # bytes plus one byte derived from length, first and last byte.
    b = name.encode()
    if len(b) < 32:
        return ("short", b.rstrip(b"\0"))
    return ("long", b[:31], 1 + (len(b) + b[0] + b[-1]) % 255)


def objects_in(instance):
    if isinstance(instance, dict):
        yield instance
    elif isinstance(instance, list):
        for item in instance:
            if isinstance(item, dict):
                yield item


for corpus in corpora:
    with open(corpus) as f:
        cases = json.load(f)
    for case in cases:
        if "fast" not in case:
            continue
        dump = compile_dump(case["schema"])
        typed = [l for l in dump if "[" in l and "Exactly" in l]
        if not typed:
            continue
        typed_hash_cases += 1
        label = f"{os.path.basename(corpus)}:{case['description']}"
        valid = case["valid"]
        instance = case["instance"]
        for line in typed:
            names = json.loads("[" + line.split("[", 1)[1])
            if line.startswith("LoopItemsPropertiesExactlyTypeStrictHash3"):
                for item in objects_in(instance):
                    keys = list(item.keys())
                    if len(keys) != 3:
                        continue
                    matched = False
                    for p in PERMS:
                        expected = [names[i] for i in p]
                        diff = [i for i in range(3) if expected[i] != keys[i]]
                        if not diff and valid:
                            hash3_branches[p]["conforming"].append(label + f" item={json.dumps(item)}")
                            matched = True
                        elif len(diff) == 1 and not valid:
                            hash3_branches[p]["single_substitution"].append(
                                label + f" item={json.dumps(item)} ({json.dumps(keys[diff[0]])} for {json.dumps(expected[diff[0]])})")
            if len(names) == 4:
                four_prop["conforming" if valid else "invalid"].append(
                    label + f" instruction={line.split(' ')[0]} instance={json.dumps(instance)}")
            hashes = [hash_of(n) for n in names]
            if len(set(hashes)) < len(hashes) and valid:
                declared_collision_controls.append(
                    label + f" names={json.dumps(names)} instance={json.dumps(instance)}")

print(f"fast-mode corpus cases compiling to an exact-property instruction: {typed_hash_cases}")
print("== Hash3 branches (LoopItemsPropertiesExactlyTypeStrictHash3, instance key order vs stored name order)")
for p in PERMS:
    print(f"branch {p}:")
    for kind in ("conforming", "single_substitution"):
        entries = hash3_branches[p][kind]
        print(f"  {kind}: {len(entries)}")
        for e in entries:
            print(f"    - {e}")
print("== Four-property exact-property instructions")
for kind, entries in four_prop.items():
    print(f"  {kind}: {len(entries)}")
    for e in entries:
        print(f"    - {e}")
print("== Conforming controls whose declared names have colliding hashes")
print(f"  {len(declared_collision_controls)}")
for e in declared_collision_controls:
    print(f"    - {e}")
