# Citations in Makefile
# https://makefiletutorial.com/
# https://wiki.osdev.org/Raspberry_Pi_Bare_Bones
# Info on implicit rules and naming conventions:
# https://www.gnu.org/software/make/manual/make.html#Implicit-Rules Info

# Programs
ARMBIN := /u/gheith/public/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin
QEMUBIN := /u/hill/Coursework/CS378/tools/qemu/bin
QEMU_LIBS := /u/hill/Coursework/CS378/tools/glib/lib/x86_64-linux-gnu
AS := $(ARMBIN)/aarch64-none-linux-gnu-as # Assembler
CC := $(ARMBIN)/aarch64-none-linux-gnu-gcc # C Compiler
CXX := $(ARMBIN)/aarch64-none-linux-gnu-g++ # C++ Compiler
OBJCOPY := $(ARMBIN)/aarch64-none-linux-gnu-objcopy # Objcopy
GDB := $(ARMBIN)/aarch64-none-linux-gnu-gdb # GDB
QEMU := $(QEMUBIN)/qemu-system-aarch64
# Flags and params
QEMU_FLAGS := -no-reboot -M raspi3b -smp 4 -nographic -drive file=../test.dd,if=sd,format=raw -cpu cortex-a72 -m 1G \
	# --trace "sd*" 
CCFLAGS := -march=armv8-a -mcpu=cortex-a53 -ffreestanding -nostdlib
CXXFLAGS := -march=armv8-a -mcpu=cortex-a53 -ffreestanding -nostdlib -mno-outline-atomics -fno-builtin -fno-stack-protector -fno-exceptions -fno-rtti -nodefaultlibs -nostartfiles -DQEMU_ENABLED=$(QEMU_ENABLED) -DDEBUG_ENABLED=$(DEBUG_ENABLED)

ASFLAGS :=
DEBUG_FLAGS := -g
OPT_FLAGS := -O2
# Desired object files, executables names, etc.
BUILD = build
SRCDIR = src
INCDIR = include
VPATH = $(SRCDIR) $(INCDIR)
# NOTE(Nate): The wildcard and the vpath could fuck us up here when used
# together
CSRC = $(wildcard $(SRCDIR)/*.c)
COBJ = $(patsubst $(SRCDIR)/%.c,%.o,$(CSRC))
CXXSRC = $(wildcard $(SRCDIR)/*.cpp)
CXXOBJ = $(patsubst $(SRCDIR)/%.cpp,%.o,$(CXXSRC))
ASSRC = $(wildcard $(SRCDIR)/*.s)
ASOBJ = $(patsubst $(SRCDIR)/%.s,%.o,$(ASSRC))
SRCS = $(CSRC) $(CXXSRC) $(ASSRC)
OBJS = $(addprefix $(BUILD)/, $(COBJ) $(CXXOBJ) $(ASOBJ))
EXEC = $(BUILD)/kernel.elf
IMG = $(BUILD)/kernel8.img
TMP_IMG = $(BUILD)/kernel8_tmp.img
# flags for macros
QEMU_ENABLED ?= 1
DEBUG_ENABLED ?= 0

.PHONY: all clean debug qemu

# Build an operating system
all: $(IMG)

# Set the debug flags and compile
debug: OPT_FLAGS = -O0
debug: CCFLAGS += $(DEBUG_FLAGS)
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: ASFLAGS += $(DEBUG_FLAGS)
debug: DEBUG_ENABLED = 1
debug: $(IMG)
	@echo "To debug via gdb: type into another terminal"
	@echo "$(GDB) kernel/build/kernel.elf"
	@echo "target remote :1234"
	LD_LIBRARY_PATH=$(QEMU_LIBS) $(QEMU) $(QEMU_FLAGS) -kernel $(IMG) -s -S

qemu: $(IMG)
	LD_LIBRARY_PATH=$(QEMU_LIBS) $(QEMU) $(QEMU_FLAGS) -kernel $(IMG)

# The executable, a .elf file (eg. kernel.elf)
$(EXEC): linker.ld $(OBJS)
	$(CXX) -T $< $(CXXFLAGS) -o $@ $(OBJS)

# THe image, a .img file (eg. kernel.img)
$(IMG): $(EXEC)
	$(OBJCOPY) $< -O binary $@

# NOTE(Nate): Implicit rules are okay... but explicit would be good

# Implicit rule for creating object files from c files
$(BUILD)/%.o : %.c | $(BUILD)
	$(CC) $(CCFLAGS) $(OPT_FLAGS) -c $< -o $@ -I $(INCDIR)

# Implicit rule for creating object files from c++ files
$(BUILD)/%.o : %.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $(OPT_FLAGS) -c $< -o $@ -I $(INCDIR)

# Implicit rule for creating object files from assembly files
$(BUILD)/%.o : %.s | $(BUILD)
	$(AS) $(ASFLAGS) -c $< -o $@

# Make the build directory if doesn't exist
$(BUILD):
	mkdir $@

# All files are in the build dir
clean:
	$(RM) -rf $(BUILD)
