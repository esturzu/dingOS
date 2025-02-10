#!/bin/bash

# Paths to tools and files
GDB_PATH="/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gdb"
GDB_SCRIPT="gdb-init-script.gdb"
KERNEL_DIR="kernel"
KERNEL_BUILD_DIR="$KERNEL_DIR/build"
DEFAULT_TEST_NAME="test_main"

# Function to print usage instructions
usage() {
    echo "Usage: $0 [test_name]"
    echo "       If no test_name is provided, defaults to '$DEFAULT_TEST_NAME'"
    exit 1
}

# Parse input arguments
TEST_NAME=${1:-$DEFAULT_TEST_NAME}
TEST_ELF="$KERNEL_BUILD_DIR/$TEST_NAME.elf"
TEST_IMG="$KERNEL_BUILD_DIR/$TEST_NAME.img"

# Check if the test ELF file exists
if [ ! -f "$TEST_ELF" ]; then
    echo "Error: Test ELF file '$TEST_ELF' not found. Make sure the test is built."
    exit 1
fi

# Kill any running QEMUs
pkill -f qemu-system-aarch64 2>/dev/null

# Clean and start QEMU with the test
echo "Starting QEMU for test: $TEST_NAME"
make -C "$KERNEL_DIR" debug_test TEST_NAME="$TEST_NAME" > qemu_log.log 2>&1 &

# Check if QEMU started correctly
if [ $? -ne 0 ]; then
    echo "Error: Failed to start QEMU. Check qemu_log.log for details."
    exit 1
fi

# Start GDB
echo "Starting GDB for test: $TEST_NAME"
exec "$GDB_PATH" -x "$GDB_SCRIPT" "$TEST_ELF"
