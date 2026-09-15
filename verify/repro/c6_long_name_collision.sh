#!/bin/sh
# Run: sh verify/repro/c6_long_name_collision.sh <probe> <hash_equal>  (both from verify/repro/build_probe.sh); C6 equal-hash long-name substitution in fast/exhaustive mode
set -eu
PROBE="$1"; HASH_EQUAL="$2"
cd "$(dirname "$0")/fixtures"
echo "== hash_equal"
"$HASH_EQUAL" notification_webhook_endpoint_url_for_production_alerts notification_webhook_endpoint_url_fXr_production_alerts
run() { # schema instance
  echo "-- $2: $(tr -d '\n' < "$2.json")"
  "$PROBE" "$1.json" "$2.json" fast
  "$PROBE" "$1.json" "$2.json" fast --callback
  "$PROBE" "$1.json" "$2.json" exhaustive
}
echo "== c6_schema (two long names, closed, all required)"
"$PROBE" c6_schema.json c6_valid.json fast --dump | grep -v '^mode='
run c6_schema c6_valid
run c6_schema c6_substituted
run c6_schema c6_substituted_reordered
run c6_schema c6_task_instance
echo "== c6_items_schema (items, three long names -> Hash3)"
"$PROBE" c6_items_schema.json c6_items_valid.json fast --dump | grep -v '^mode='
run c6_items_schema c6_items_valid
run c6_items_schema c6_items_substituted
run c6_items_schema c6_items_substituted_reordered
echo "== c6_mixed_schema (one short + one long name)"
"$PROBE" c6_mixed_schema.json c6_mixed_valid.json fast --dump | grep -v '^mode='
run c6_mixed_schema c6_mixed_valid
run c6_mixed_schema c6_mixed_substituted
run c6_mixed_schema c6_mixed_substituted_reordered
