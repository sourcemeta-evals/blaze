#!/bin/sh
# Run: verify/repro/c5_annotations_accept_non_objects.sh <path-to-probe-binary>
# (build the probe with verify/repro/build_probe.sh against a checkout of
# evalon/blaze-conf-335d54f2 that has build/dist installed).
# Expected on that branch: with Tweaks.annotations = {"properties"} in fast
# mode, the closed all-required object schema accepts null, booleans, numbers,
# strings and arrays; without the tweak (or in exhaustive mode) they are
# rejected. The dump shows AssertionDefinesExactly and no object type check.
set -eu
PROBE="$1"
TMP="$(mktemp -d)"
cat > "$TMP/schema.json" <<'EOF'
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "foo": { "type": "string" },
    "bar": { "type": "string" }
  },
  "required": ["foo", "bar"],
  "additionalProperties": false
}
EOF
echo "# compiled fast-mode template with annotations={properties}"
echo 'null' > "$TMP/null.json"
"$PROBE" "$TMP/schema.json" "$TMP/null.json" fast --annotations properties --dump | grep -v '^mode='
for value in 'null' 'true' 'false' '42' '3.5' '"foo"' '[]' '["foo","bar"]' '{"foo":"x","bar":"y"}' '{}' '{"foo":"x","qux":"y"}' '{"foo":"x","bar":1}'; do
  echo "$value" > "$TMP/instance.json"
  echo "# instance $value"
  "$PROBE" "$TMP/schema.json" "$TMP/instance.json" fast --annotations properties
  "$PROBE" "$TMP/schema.json" "$TMP/instance.json" fast
  "$PROBE" "$TMP/schema.json" "$TMP/instance.json" exhaustive --annotations properties
done
rm -rf "$TMP"
