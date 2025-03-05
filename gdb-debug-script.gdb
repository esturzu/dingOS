# NOTE: This is a custom script that sets 3 breakpoints.
# 1st breakpoint: beginning of kernelMain()
# 2nd breakpoint: what you want to debug (e.g., HashMap constructor in this case)
# 3rd breakpoint: typing 'continue' into GDB will continue to this point (if it doesn't hang)

# USAGE: Plug this script into 'GDB_SCRIPT' inside debug.sh or whatever other debug script.
# Set your 2nd breakpoint to after whatever location that is hanging. If it hangs,
# the 'echo' will never execute, and now you can examine the state of the system.

# Connect to QEMU
target remote :1234

# Load symbols
add-symbol-file kernel/build/kernel.elf 0x80000

# Kill QEMU on GDB exit
define hook-quit
  shell pkill -f qemu-system-aarch64
end

# Set a breakpoint at kernelMain
b kernelMain

# Set a breakpoint at the HashMap constructor
b "HashMap<int, int, DefaultIntegerHash>::HashMap(int, double, DefaultIntegerHash const&)"

# Set a breakpoint after all HashMap tests are done
b hashmapTests.h:34

# Define commands for the HashMap constructor breakpoint:
#  - "silent" means don't print normal GDB messages
#  - print a helpful message, then continue again
commands 2
  silent
  echo "\n[DEBUG] >>> Hit HashMap constructor => not stuck in 'new' or 'malloc'.\n"
  continue
end

# After loading, immediately continue from kernelMain
continue
