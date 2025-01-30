# Connect to QEMU
target remote :1234

# load symbols
add-symbol-file kernel/build/kernel.elf 0x80000

# set breakpoints
b kernelMain

# kill qemu on gdb exit
define hook-quit
  shell pkill -f qemu-system-aarch64
end
