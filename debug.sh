#!/bin/bash
GDB_PATH="/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gdb"

# Init script is a clean debuggin state
GDB_SCRIPT="gdb-init-script.gdb"

# This one is for setting predefined breakpoints for ease of debugging
# GDB_SCRIPT="gdb-debug-script.gdb"

KERNEL_ELF="kernel/build/kernel.elf"

# Kill any running QEMUs
pkill -f qemu-system-aarch64 2>/dev/null

# Start QEMU and reroute output to qemu_log.txt
make clean debug > qemu_log.log 2>&1 &

# Start debugging
exec "$GDB_PATH" -x "$GDB_SCRIPT" "$KERNEL_ELF"
