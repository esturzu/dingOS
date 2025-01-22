# Citations in Makefile
# https://makefiletutorial.com/
# https://wiki.osdev.org/Raspberry_Pi_Bare_Bones
# Info on implicit rules and naming conventions:
# https://www.gnu.org/software/make/manual/make.html#Implicit-Rules Info

# Programs
ARMBIN := /u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin
QEMUBIN := /u/hill/Coursework/CS378/tools/qemu/bin
AS := $(ARMBIN)/aarch64-none-elf-as # Assembler
CC := $(ARMBIN)/aarch64-none-elf-gcc # C Compiler
CXX := $(ARMBIN)/aarch64-none-elf-g++ # C++ Compiler
OBJCOPY := $(ARMBIN)/aarch64-none-elf-objcopy # Objcopy
GDB := $(ARMBIN)/aarch64-none-elf-gdb # GDB
QEMU := $(QEMUBIN)/qemu-system-aarch64 
# Flags and params
QEMU_FLAGS := -no-reboot -M raspi4b -nographic
CCFLAGS := -ffreestanding -nostdlib
CXXFLAGS := -ffreestanding -nostdlib
ASFLAGS := 
DEBUG_FLAGS := -g
OPT_FLAGS := -O2
# Desired object files, executables names, etc.
BUILD = build
VPATH = src
# NOTE(Nate): The wildcard and the vpath could fuck us up here when used
# together
CSRC = $(wildcard *.c)
COBJ = $(patsubst %.c,%.o,$(CSRC))
CXXSRC = $(wildcard *.cc)
CXXOBJ = $(patsubst %.cc,%.o,$(CXXSRC))
ASSRC = $(wildcard *.s)
ASOBJ = $(patsubst %.s,%.o,$(ASSRC))
SRCS = $(CSRC) $(CXXSRC) $(ASSRC)
OBJS = $(addprefix $(BUILD)/, $(COBJ) $(CXXOBJ) $(ASOBJ))
EXEC = $(BUILD)/kernel.elf
IMG = $(BUILD)/kernel.img

# Build an operating system
all: $(IMG)
	@echo "Objects are $(OBJS)"
	$(QEMU) $(QEMU_FLAGS) -kernel $(IMG)

# Set the debug flags and compile
debug: OPT_FLAGS = -O0
debug: CFLAGS += $(DEBUG_FLAGS)
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: ASFLAGS += $(DEBUG_FLAGS)
debug: all
	@echo "To debug via gdb: type into another terminal"
	@echo "$(GDB) kernel/build/kernel.elf"
	@echo "target remote :1234"

	#mkdir -p kernel/build

	#/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-as -c kernel/boot.s -o kernel/build/boot.o -g
	#/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gcc -ffreestanding -c ${CC_FILES} -o kernel/build/kernel.o -g
	#/u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gcc -T kernel/linker.ld -o kernel/build/kernel.elf -ffreestanding -O2 -nostdlib kernel/build/boot.o kernel/build/kernel.o -g


	#/u/hill/Coursework/CS378/tools/qemu/bin/qemu-system-aarch64 ${QEMU_FLAGS} -kernel kernel/build/kernel.elf -s

# The executable, a .elf file (eg. kernel.elf)
$(EXEC): linker.ld $(OBJS)
	$(CC) -T $< $(CCFLAGS) -o $@ $(OBJS)

# THe image, a .img file (eg. kernel.img)
$(IMG): $(EXEC)
	$(OBJCOPY) $< -O binary $@

# NOTE(Nate): Implicit rules are okay... but explicit would be good

# Implicit rule for creating object files from c files
$(BUILD)/%.o : %.c | $(BUILD)
	@echo here
	$(CC) $(CFLAGS) $(OPT_FLAGS) -c $< -o $@

# Implicit rule for creating object files from c++ files
$(BUILD)/%.o : %.cc | $(BUILD)
	@echo i
	$(CXX) $(CXXFLAGS) $(OPT_FLAGS) -c $< -o $@

# Implicit rule for creating object files from assembly files
$(BUILD)/%.o : %.s | $(BUILD)
	@echo am
	$(AS) $(ASFLAGS) -c $< -o $@

# Make the build directory if doesn't exist
$(BUILD):
	mkdir $@

# All files are in the build dir
clean:
	$(RM) -rf $(BUILD)

.PHONY: clean
