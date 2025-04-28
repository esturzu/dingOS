#!/bin/bash

# This script will continuously run 'make qemu' until the last line of
# kernel output is different than expected.

MAX_SECS=4                                        # How many seconds to let QEMU run each time before timing out
EXPECTED_LINE="Test 2: HashMap resizing"          # Change this to the line you expect at the end
LOGFILE="kernel_output.log"                       # File to kernel output
DEBUG_ENABLED_FLAG=1                              # Enable debug prints
MAX_ITER=100

COUNTER=0

while true; do
  # Remove old logs
  rm -f "$LOGFILE"

  # Make clean
  make -s clean

  # Run QEMU for MAX_SECS seconds
  # Need --foreground otherwise QEMU arguments won't be passed in
  timeout --foreground ${MAX_SECS}s make -s DEBUG_ENABLED=$DEBUG_ENABLED_FLAG QEMU_LOG="$LOGFILE" qemu

  # Reset terminal back to cooked mode
  reset

  COUNTER=$((COUNTER + 1))
  echo "=============================================="
  echo "Run #$COUNTER"
  echo "=============================================="

  # Find the PID of a qemu-system-aarch64 process
  PID=$(pgrep -f qemu-system-aarch64)

  # Kill the QEMU process running in the background since
  # timeout doesn't kill everything
  if [ -n "$PID" ]; then
    echo "Found QEMU PID: $PID"
    kill "$PID"
  else
    echo "No matching QEMU process found."
  fi

  # Grab the last line in the kernel output file
  LAST_LINE="$(tail -n 1 "$LOGFILE")"

  echo "Last line of output: '$LAST_LINE'"

  # If it's not the expected line, something unexpected happened
  if [ "$LAST_LINE" != "$EXPECTED_LINE" ]; then
    echo "Unexpected behavior detected in run #$COUNTER!"
    echo "Exiting..."
    exit 1
  fi

  echo "Run #$COUNTER was successful (i.e., ended with '$EXPECTED_LINE'). Retrying..."

  # Break if over MAX_ITER rounds ran successfully
  if [ "$COUNTER" -gt "$MAX_ITER" ]; then
    echo "$MAX_ITER rounds ran successfully."
    break
  fi
done
