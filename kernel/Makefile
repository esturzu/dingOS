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

# Directories
BUILD := build
SRCDIR := src
INCDIR := include
# CHANGE TEST DIR TO BE ABLE TO PUT IN YOUR TESTS
TESTDIR := tests

# Kernel source and object files
SRCS := $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*.s $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(BUILD)/%.o, $(wildcard $(SRCDIR)/*.cpp)) \
        $(patsubst $(SRCDIR)/%.s, $(BUILD)/%.o, $(wildcard $(SRCDIR)/*.s)) \
        $(patsubst $(SRCDIR)/%.c, $(BUILD)/%.o, $(wildcard $(SRCDIR)/*.c))

EXEC := $(BUILD)/kernel.elf
IMG := $(BUILD)/kernel8.img

# Test source and object files
TEST_SRCS := $(wildcard $(TESTDIR)/*.cpp)
TEST_OBJS := $(patsubst $(TESTDIR)/%.cpp, $(BUILD)/%.o, $(TEST_SRCS))
TEST_ELFS := $(patsubst $(TESTDIR)/%.cpp, $(BUILD)/%.elf, $(TEST_SRCS))
TEST_IMGS := $(patsubst $(TESTDIR)/%.cpp, $(BUILD)/%.img, $(TEST_SRCS))

.PHONY: all clean all-tests run_all_tests run_test debug_test

# --- Kernel Build Targets ---

all: $(IMG)

# Kernel ELF build
$(EXEC): linker.ld $(OBJS)
	$(CXX) -T $< $(CXXFLAGS) -o $@ $(OBJS)

# Kernel IMG build
$(IMG): $(EXEC)
	$(OBJCOPY) $< -O binary $@

# Clean all build files (kernel + tests)
clean:
	rm -rf $(BUILD)

# --- Test-Specific Section ---

# Compile all test ELFs (no execution)
all-tests: $(TEST_ELFS)

# Compile and run all test images
run-all-tests: $(TEST_IMGS)
	@for img in $(TEST_IMGS); do \
		echo "Running test $$img..."; \
		expect -c ' \
			spawn $(QEMU) $(QEMU_FLAGS) -kernel $$img -serial mon:stdio; \
			expect "TEST DONE" { exit 0 } timeout 10 { exit 1 }' \
		|| echo "Test $$img failed"; \
	done



# Compile and run a specific test
run-test:
ifndef TEST_NAME
	$(error "TEST_NAME is not set. Please specify a test name using TEST_NAME=<name>")
endif
	@$(MAKE) $(BUILD)/$(TEST_NAME).img
	@echo "Running test $(TEST_NAME)..."
	LD_LIBRARY_PATH=$(QEMU_LIBS) $(QEMU) $(QEMU_FLAGS) -kernel $(BUILD)/$(TEST_NAME).img -serial mon:stdio

# Compile and debug a specific test
debug-test:
ifndef TEST_NAME
	$(error "TEST_NAME is not set. Please specify a test name using TEST_NAME=<name>")
endif
	@$(MAKE) $(BUILD)/$(TEST_NAME).img
	@echo "Starting QEMU for debugging test $(TEST_NAME)..."
	LD_LIBRARY_PATH=$(QEMU_LIBS) $(QEMU) $(QEMU_FLAGS) -kernel $(BUILD)/$(TEST_NAME).img -serial mon:stdio -s -S

# --- Compilation Rules ---

# General object file compilation for kernel components
$(BUILD)/%.o: $(SRCDIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $(OPT_FLAGS) -c $< -o $@ -I $(INCDIR)

$(BUILD)/%.o: $(SRCDIR)/%.s | $(BUILD)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SRCDIR)/%.c | $(BUILD)
	$(CC) $(CCFLAGS) $(OPT_FLAGS) -c $< -o $@ -I $(INCDIR)

# Test-specific object file compilation
$(BUILD)/%.o: $(TESTDIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -I $(INCDIR)

# Linking test ELF files
$(BUILD)/%.elf: $(BUILD)/%.o $(filter-out $(BUILD)/kernel.o, $(OBJS))
	$(CXX) -T linker.ld $(CXXFLAGS) -o $@ $(filter-out $(BUILD)/kernel.o, $(OBJS)) $<

# Convert test ELF files to IMG format
$(BUILD)/%.img: $(BUILD)/%.elf
	$(OBJCOPY) $< -O binary $@

# Create the build directory if it doesn't exist
$(BUILD):
	mkdir -p $@
