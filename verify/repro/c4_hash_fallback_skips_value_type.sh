#!/bin/sh
# Run: verify/repro/c4_hash_fallback_skips_value_type.sh <path-to-probe-binary>
# (build the probe with verify/repro/build_probe.sh against a checkout of
# evalon/blaze-conf-5818f820 that has build/dist installed).
# Expected on that branch: fast mode accepts {"b":"x","a":1} (callbackless and
# with callback) although "a" must be a string; exhaustive mode rejects it.
set -eu
PROBE="$1"
TMP="$(mktemp -d)"
cat > "$TMP/schema.json" <<'EOF'
{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","properties":{"a":{"type":"string"},"b":{"type":"string"}},"required":["a","b"],"additionalProperties":false}
EOF
echo '{"b":"x","a":1}' > "$TMP/invalid.json"
echo '{"b":"x","a":"y"}' > "$TMP/control.json"
echo '{"a":1,"b":"x"}' > "$TMP/invalid_ordered.json"
echo "# compiled fast-mode template"
"$PROBE" "$TMP/schema.json" "$TMP/invalid.json" fast --dump | sed -n '1,2p'
for instance in invalid control invalid_ordered; do
  echo "# instance $instance: $(cat "$TMP/$instance.json")"
  "$PROBE" "$TMP/schema.json" "$TMP/$instance.json" fast
  "$PROBE" "$TMP/schema.json" "$TMP/$instance.json" fast --callback
  "$PROBE" "$TMP/schema.json" "$TMP/$instance.json" exhaustive
done
rm -rf "$TMP"
