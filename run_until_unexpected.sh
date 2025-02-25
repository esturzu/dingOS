#!/bin/bash

# This script will continuously run 'make qemu' until the last line of
# kernel output is different than expected.

MAX_SECS=5   # How many seconds to let QEMU run each time before timing out
COUNTER=0

while true; do
  COUNTER=$((COUNTER + 1))
  echo "=============================================="
  echo "Run #$COUNTER"
  echo "=============================================="

  # Remove old logs
  rm -f kernel/kernel_output.log

  # Run QEMU via "make qemu" for MAX_SECS seconds
  # All kernel Output is captured to kernel/kernel_output.log
  timeout ${MAX_SECS}s make qemu

  # Grab the last line of QEMU's output
  LAST_LINE="$(tail -n 1 qemu_output.log)"

  echo "Last line of output: '$LAST_LINE'"

  # If it's not "HERE 4", we assume we've hit the deadlock scenario
  if [ "$LAST_LINE" != "HERE 4" ]; then
    echo "Deadlock (or unexpected exit) detected in run #$COUNTER!"
    echo "Exiting..."
    exit 1
  fi

  echo "Run #$COUNTER was successful (i.e. ended with 'HERE 4'). Retrying..."
done
