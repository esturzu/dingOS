# Citations in Makefile
# https://makefiletutorial.com/
# https://wiki.osdev.org/Raspberry_Pi_Bare_Bones

CC_FILES := $(wildcard kernel/*.cc)
QEMU_FLAGS := -no-reboot -M raspi4b -nographic

debug:
	mkdir -p kernel/build

	/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-as -c kernel/boot.s -o kernel/build/boot.o -g
	/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gcc -ffreestanding -c ${CC_FILES} -o kernel/build/kernel.o -g
	/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gcc -T kernel/linker.ld -o kernel/build/kernel.elf -ffreestanding -O2 -nostdlib kernel/build/boot.o kernel/build/kernel.o -g

	@echo "type into another terminal"
	@echo "/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gdb /u/hill/Coursework/CS378/dingOS/kernel/build/kernel.elf"
	@echo "target remote :1234"

	/u/hill/Coursework/CS378/tools/qemu/bin/qemu-system-aarch64 ${QEMU_FLAGS} -kernel kernel/build/kernel.elf -s

all:
	mkdir -p kernel/build

	/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-as -c kernel/boot.s -o kernel/build/boot.o
	/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gcc -ffreestanding -c ${CC_FILES} -o kernel/build/kernel.o
	/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gcc -T kernel/linker.ld -o kernel/build/kernel.elf -ffreestanding -O2 -nostdlib kernel/build/boot.o kernel/build/kernel.o
	/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-objcopy kernel/build/kernel.elf -O binary kernel/build/kernel.img

	/u/hill/Coursework/CS378/tools/qemu/bin/qemu-system-aarch64 ${QEMU_FLAGS} -kernel kernel/build/kernel.img

clean:
	rm -rf kernel/build