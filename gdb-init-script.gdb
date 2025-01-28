# Connect to QEMU's GDB server
target remote :1234

# Set the program counter (PC) to the entry point
set $pc = 0x80000

# Set a breakpoint at kernelMain
b kernelMain

# Define a cleanup hook to kill QEMU on GDB exit
define hook-quit
  shell pkill -f qemu-system-aarch64
end
