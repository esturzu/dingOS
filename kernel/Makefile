# Citations
# https://makefiletutorial.com/
# https://wiki.osdev.org/Raspberry_Pi_Bare_Bones

# Info on implicit rules and naming conventions:
# https://www.gnu.org/software/make/manual/make.html#Implicit-Rules

# Programs
ARMBIN := /u/gheith/public/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin
QEMUBIN := /u/hill/Coursework/CS378/tools/qemu/bin
QEMU_LIBS := /u/hill/Coursework/CS378/tools/glib/lib/x86_64-linux-gnu

AS := $(ARMBIN)/aarch64-none-linux-gnu-as 						# Assembler
CC := $(ARMBIN)/aarch64-none-linux-gnu-gcc 						# C Compiler
CXX := $(ARMBIN)/aarch64-none-linux-gnu-g++ 					# C++ Compiler
OBJCOPY := $(ARMBIN)/aarch64-none-linux-gnu-objcopy 	# Objcopy
GDB := $(ARMBIN)/aarch64-none-linux-gnu-gdb 					# GDB
QEMU := $(QEMUBIN)/qemu-system-aarch64 								# QEMU

# QEMU flags and params
QEMU_FLAGS := -no-reboot -M raspi3b -nographic -smp 4 -drive file=../ext2.img,if=sd,format=raw -cpu cortex-a72 -m 1G \
	# --trace "sd*" ]

# Optional kernel output log file
QEMU_LOG ?=
ifneq ($(QEMU_LOG),)
QEMU_FLAGS += -serial file:../$(QEMU_LOG)
endif

CCFLAGS := -march=armv8-a -mcpu=cortex-a53 -ffreestanding -nostdlib
CXXFLAGS := -march=armv8-a -mcpu=cortex-a53 -ffreestanding -nostdlib \
						-mno-outline-atomics -fno-builtin -fno-stack-protector \
						-fno-exceptions -fno-rtti -nodefaultlibs -nostartfiles \
						-DDEBUG_ENABLED=$(DEBUG_ENABLED)

# Enable debug prints
DEBUG_ENABLED ?= 1

ASFLAGS :=
DEBUG_FLAGS := -g
OPT_FLAGS := -O2

# For generating dependency files automatically
DEPFLAGS = -MMD -MP

# Desired directories, object files, executables names, etc.
BUILD 	:= build
SRCDIR 	:= src
INCDIR 	:= include

# Dynamically find all subdirectories under include/
INC_SUBDIRS := $(shell find $(INCDIR) -type d)

# For each subdir in include/, add "-I subdir" to the compiler flags
INC_PATHS := $(foreach d, $(INC_SUBDIRS), -I $d)
CCFLAGS   += $(INC_PATHS)
CXXFLAGS  += $(INC_PATHS)

# Search recursively for all files to build
CSRC   	:= $(shell find $(SRCDIR) -name '*.c')
CXXSRC 	:= $(shell find $(SRCDIR) -name '*.cpp')
ASSRC  	:= $(shell find $(SRCDIR) -name '*.s')

# Produce object lists
COBJ 		:= $(patsubst $(SRCDIR)/%.c,%.o,$(CSRC))
CXXOBJ 	:= $(patsubst $(SRCDIR)/%.cpp,%.o,$(CXXSRC))
ASOBJ 	:= $(patsubst $(SRCDIR)/%.s,%.o,$(ASSRC))

OBJS 		:= $(addprefix $(BUILD)/, $(COBJ) $(CXXOBJ) $(ASOBJ))

# The specialized CRT files must come first/last
CRTI := $(BUILD)/crti.o

# Filter them out of the main OBJ list
OTHER_OBJS := $(filter-out $(CRTI), $(OBJS))

EXEC 		:= $(BUILD)/kernel.elf
IMG 		:= $(BUILD)/kernel8.img

# ------------------------------------------------------------------------
.PHONY: all clean debug qemu

# Default rule
all: $(IMG)

# Debug build
debug: OPT_FLAGS = -O0
debug: CCFLAGS += $(DEBUG_FLAGS)
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: ASFLAGS += $(DEBUG_FLAGS)
debug: DEBUG_ENABLED = 1
debug: $(IMG)
	@echo "To debug via gdb:"
	@echo "   $(GDB) $(EXEC)"
	@echo "   (gdb) target remote :1234"
	LD_LIBRARY_PATH=$(QEMU_LIBS) $(QEMU) $(QEMU_FLAGS) -kernel $(IMG) -s -S

# Run under QEMU
qemu: $(IMG)
	LD_LIBRARY_PATH=$(QEMU_LIBS) $(QEMU) $(QEMU_FLAGS) -kernel $(IMG)

# How to build the final ELF from all objects + linker script
$(EXEC): linker.ld $(OBJS)
	$(CXX) -T $< $(CXXFLAGS) -o $@ $(CRTI) -Wl,--start-group $(OTHER_OBJS) -Wl,--end-group

# How to produce the .img from the ELF
$(IMG): $(EXEC)
	$(OBJCOPY) $< -O binary $@

# Rules for building object files from C/C++/Assembly
# We also generate .d dependency files automatically (-MMD -MP)
$(BUILD)/%.o : $(SRCDIR)/%.c | $(BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CCFLAGS) $(OPT_FLAGS) $(DEPFLAGS) -c $< -o $@ -MF $(basename $@).d

$(BUILD)/%.o : $(SRCDIR)/%.cpp | $(BUILD)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OPT_FLAGS) $(DEPFLAGS) -c $< -o $@ -MF $(basename $@).d

$(BUILD)/%.o : $(SRCDIR)/%.s | $(BUILD)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Create the build directory if needed
$(BUILD):
	mkdir -p $@

# "make clean" wipes out build/
clean:
	rm -rf $(BUILD)

# Pull in automatically generated dependencies, if any
-include $(OBJS:.o=.d)