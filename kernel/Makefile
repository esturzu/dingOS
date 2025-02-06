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
QEMU_FLAGS := -no-reboot -M raspi3b -nographic
CCFLAGS := -march=armv8-a -mcpu=cortex-a53 -ffreestanding -nostdlib
CXXFLAGS := -march=armv8-a -mcpu=cortex-a53 -ffreestanding -nostdlib -mno-outline-atomics -fno-builtin -fno-stack-protector -fno-exceptions -fno-rtti -nodefaultlibs -nostartfiles

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

.PHONY: all clean debug qemu

# Build an operating system
all: $(IMG)

# Set the debug flags and compile
debug: OPT_FLAGS = -O0
debug: CCFLAGS += $(DEBUG_FLAGS)
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: ASFLAGS += $(DEBUG_FLAGS)
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

# -------------------
# Test-specific section
# -------------------

TEST_SRCS := $(wildcard tests/*.cpp)
TEST_OBJS := $(patsubst tests/%.cpp, $(BUILD)/%.o, $(TEST_SRCS))
TEST_ELFS := $(patsubst tests/%.cpp, $(BUILD)/%.elf, $(TEST_SRCS))
TEST_IMGS := $(patsubst tests/%.cpp, $(BUILD)/%.img, $(TEST_SRCS))

TEST_NAME ?= test_main
TEST_OBJ := $(BUILD)/$(TEST_NAME).o
TEST_ELF := $(BUILD)/$(TEST_NAME).elf
TEST_IMG := $(BUILD)/$(TEST_NAME).img

run_test: $(TEST_IMG)
	@echo "Running test $(TEST_NAME) in QEMU..."
	LD_LIBRARY_PATH=$(QEMU_LIBS) $(QEMU) $(QEMU_FLAGS) -kernel $< -serial mon:stdio

debug_test: $(TEST_IMG)
	@echo "Starting QEMU for debugging test $(TEST_NAME)..."
	LD_LIBRARY_PATH=$(QEMU_LIBS) $(QEMU) $(QEMU_FLAGS) -kernel $< -serial mon:stdio -s -S

run_all_tests: $(TEST_IMGS)
	@for img in $(TEST_IMGS); do \
		echo "Running test $${img} in QEMU..."; \
		LD_LIBRARY_PATH=$(QEMU_LIBS) $(QEMU) $(QEMU_FLAGS) -kernel $$img -serial mon:stdio; \
	done

$(BUILD)/%.o: tests/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(INCDIR)

$(BUILD)/%.elf: $(BUILD)/%.o $(filter-out $(BUILD)/kernel.o, $(OBJS))
	$(CXX) -T linker.ld $(CXXFLAGS) -o $@ $(filter-out $(BUILD)/kernel.o, $(OBJS)) $<

$(BUILD)/%.img: $(BUILD)/%.elf
	$(OBJCOPY) $< -O binary $@
