#!/usr/bin/env bash

TESTBENCH="$1"
INPUTFILE="$2"
shift 2

REQUIRED_OUTPUT="`mktemp /tmp/ecm-XXXXXXXX.ref`"
INPUTNUMBERS="`mktemp /tmp/ecm-XXXXXXXX.in`"
# First word on each line is the input number
sed 's/ *#.*$//' < "${INPUTFILE}" | grep . | cut -d " " -f 1 > "${INPUTNUMBERS}" || exit 1
# Remaining words are the required output
sed 's/ *#.*$//' < "${INPUTFILE}" | grep . | cut -d " " -f 2- > "${REQUIRED_OUTPUT}" || exit 1

ACTUAL_OUTPUT="`mktemp /tmp/ecm-XXXXXXXX.out`"
"${TESTBENCH}" -inp "${INPUTNUMBERS}" "$@" > "${ACTUAL_OUTPUT}"

if ! diff -b "${REQUIRED_OUTPUT}" "${ACTUAL_OUTPUT}" > /dev/null
then
  echo "testbench produced output in file \"${ACTUAL_OUTPUT}\", but expected result as in \"${REQUIRED_OUTPUT}\", input numbers are in \"${INPUTNUMBERS}\""
  # diff "${REQUIRED_OUTPUT}" "${ACTUAL_OUTPUT}"
  exit 1
fi

rm -f "${REQUIRED_OUTPUT}" "${INPUTNUMBERS}" "${ACTUAL_OUTPUT}"
exit 0
