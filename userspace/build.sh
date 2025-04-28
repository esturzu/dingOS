CPU="cortex-a53"
PFX="/u/yifanma/cross"

${PFX}/bin/aarch64-elf-gcc -mcpu=${CPU} -ffreestanding -c crt0.S -o crt0.o
${PFX}/bin/aarch64-elf-gcc -mcpu=${CPU} -ffreestanding -c crt-stubs.c -o crt-stubs.o
${PFX}/bin/aarch64-elf-gcc -mcpu=${CPU} -ffreestanding -c hello.c -o hello.o
${PFX}/bin/aarch64-elf-gcc -mcpu=${CPU} -ffreestanding -c syscalls.c -o syscalls.o

${PFX}/bin/aarch64-elf-gcc \
  -T hello.ld -nostartfiles -static \
  crt0.o crt-stubs.o syscalls.o hello.o \
  -lc -lgcc \
  -o hello.elf

${PFX}/bin/aarch64-elf-objcopy --remove-section hello.elf
