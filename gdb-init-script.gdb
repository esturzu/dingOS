# Connect to QEMU
target remote :1234

# Dynamically load symbols based on the test being debugged
if $test_name != ""
    add-symbol-file kernel/build/$test_name.elf 0x80000
else
    add-symbol-file kernel/build/kernel.elf 0x80000
end

# Set breakpoints
b kernelMain

# Kill QEMU on GDB exit
define hook-quit
  shell pkill -f qemu-system-aarch64
end
