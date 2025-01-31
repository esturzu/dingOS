#!/bin/bash

# Step 1: Define paths
GDB_PATH="/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gdb"
GDB_SCRIPT="gdb-init-script.gdb"
KERNEL_ELF="kernel/build/kernel.elf"

pkill -f qemu-system-aarch64 2>/dev/null

# Step 3: Start a new tmux session
SESSION="debug_session"

# Kill the session if it exists
tmux kill-session -t $SESSION 2>/dev/null

# Create a new tmux session
tmux new-session -d -s $SESSION

# Step 4: Run QEMU in the first (left) pane
tmux send-keys -t $SESSION "clear; make clean debug" C-m

# Step 5: Split the tmux window vertically (right pane for GDB)
tmux split-window -h -t $SESSION

# Step 6: Resize the left pane to 1 column
tmux resize-pane -t $SESSION:0.0 -x 1  # Shrink the left pane to 1 column

# Step 7: Run GDB in the right pane
tmux send-keys -t $SESSION:0.1 "$GDB_PATH -tui -x $GDB_SCRIPT $KERNEL_ELF" C-m

# Step 8: Attach to the tmux session
tmux attach -t $SESSION
