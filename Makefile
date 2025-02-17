# This is the makefile for the OS in general
# There are makefiles in the subdirectories for each component
# Recursive make is considered harmful. Bite me.

COMPONENTS = kernel
TARGET =
.PHONY: all debug clean qemu $(COMPONENTS) run-test debug-test run-all-tests all-tests

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

# --- Test commands from root ---
run-test:
	$(MAKE) -C kernel run-test TEST_NAME=$(TEST_NAME)

debug-test:
	$(MAKE) -C kernel debug-test TEST_NAME=$(TEST_NAME)

run-all-tests:
	$(MAKE) -C kernel run-all-tests

all-tests:
	$(MAKE) -C kernel all-tests
