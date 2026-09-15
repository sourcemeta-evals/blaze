#!/bin/sh
# Run: sh verify/repro/c7_properties_annotations.sh <probe>  (from verify/repro/build_probe.sh); callback-based fast-mode validation with
# Tweaks.annotations={"properties"} for drafts 2019-09 and 2020-12.
set -eu
P="$1"
cd "$(dirname "$0")/fixtures"
for d in 2019-09 2020-12; do
  for kind in two three mixed long; do
    case "$kind" in
      two) s=c7_${d}_schema.json; insts="c7_two_valid c7_two_valid_reordered c7_two_missing c7_two_substituted";;
      long) s=c7_${d}_long_schema.json; insts="c6_valid c6_substituted";;
      *) s=c7_${d}_${kind}_schema.json; insts="c7_${kind}_valid c7_${kind}_missing c7_${kind}_substituted"; [ "$kind" = three ] && insts="c7_three_valid c7_three_valid_reordered c7_three_missing c7_three_substituted";;
    esac
    echo "=== $d $s"
    echo "schema: $(tr -d ' \n' < $s)"
    "$P" "$s" c7_two_valid.json fast --annotations properties --dump | grep -v '^mode='
    for i in $insts; do
      echo "-- $i: $(cat $i.json)"
      "$P" "$s" "$i.json" fast --annotations properties --callback
    done
  done
done
