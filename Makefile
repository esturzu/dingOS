# This is the makefile for the OS in general
# There are makefiles in the subdirectories for each component
# Recursive make is considered harmful. Bite me.

COMPONENTS = kernel
TARGET =
.PHONY: all debug clean qemu $(COMPONENTS)

all: $(COMPONENTS)

components: $(COMPONENTS)

$(COMPONENTS):
	$(MAKE) -C $@ $(TARGET)

debug: TARGET = debug
debug: components

qemu: components
	$(MAKE) -C kernel qemu

clean:
	for dir in $(COMPONENTS); do \
		$(MAKE) -C $$dir clean; \
	done