# This is the makefile for the OS in general.
# There are makefiles in the subdirectories for each component.
# Recursive make is considered harmful. Bite me.

COMPONENTS = kernel
TARGET =

.PHONY: all debug clean qemu $(COMPONENTS)

all: $(COMPONENTS)

components: $(COMPONENTS)

# Invoke make for each component subdir
$(COMPONENTS):
	$(MAKE) -C $@ $(TARGET)

debug: TARGET = debug
debug: components

# Build everything, then invoke qemu
qemu: components
	$(MAKE) -C kernel qemu

# Invoke "clean" in each component subdir
clean:
	for dir in $(COMPONENTS); do \
		$(MAKE) -C $$dir clean; \
	done

# ------------------------------
# EXT2 IMAGE GENERATION SECTION
# ------------------------------
EXT2_IMG := ext2.img
FS_ROOT := fs_root
BLOCK_SIZE := 1024
DISK_SIZE := 32m

fs-image: $(EXT2_IMG)

$(EXT2_IMG): $(wildcard $(FS_ROOT)/** $(FS_ROOT)/*)
	@echo "Creating ext2 image: $(EXT2_IMG)"
	@mkfs.ext2 -q -b $(BLOCK_SIZE) -i $(BLOCK_SIZE) -I 128 -r 0 -t ext2 -d $(FS_ROOT) $(EXT2_IMG) $(DISK_SIZE)
	@echo "Image created successfully."

clean-fs:
	@rm -f $(EXT2_IMG)

EXT3_IMG := ext3.img

fs3-image: $(EXT3_IMG)

$(EXT3_IMG): $(wildcard $(FS_ROOT)/** $(FS_ROOT)/*)
	@echo "Creating ext3 image: $(EXT3_IMG)"
	@mkfs.ext3 -F -b 1024 -O has_journal ext3.img 32M
	@echo "Image created successfully."

clean-fs-3:
	@rm -f $(EXT3_IMG)