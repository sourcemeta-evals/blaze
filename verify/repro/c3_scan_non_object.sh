#!/bin/sh
# Run: sh verify/repro/c3_scan_non_object.sh <checkout of evalon/blaze-conf-0307525d>   (prints the instances used by the annotation-enabled exact-property regression; no non-object instance appears)
cd "${1:-.}" || exit 1
echo "== tests added by the branch:"; git diff 2f5ba6fd HEAD -- test/evaluator/*.cc | grep '^+TEST'
echo "== instances validated by annotation_properties_exact_set (lines after TEST):"
awk '/^TEST\(annotation_properties_exact_set\)/{p=1} p&&/validate\(/{print NR": "$0} p&&/^}/{exit}' test/evaluator/evaluator_2020_12_test.cc
echo "== every tests file location naming 'non_object' together with 'annotations':"; grep -n "non_object" test/evaluator/*.cc || echo "(none in C++ tests)"
echo "== corpus entries naming non_object added by the branch (corpus cannot enable properties annotations):"; git diff 2f5ba6fd HEAD -- test/evaluator/*.json | grep '^+.*"description".*non_object' || echo "(none)"
