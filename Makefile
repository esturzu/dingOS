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
