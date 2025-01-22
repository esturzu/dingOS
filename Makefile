# This is the makefile for the OS in general
# There are makefiles in the subdirectories for each component
# Recursive make is considered harmful. Bite me.

COMPONENTS = kernel
.PHONY: all debug clean $(COMPONENTS)

all: $(COMPONENTS)

clean:
	for dir in $(COMPONENTS); do \
		$(MAKE) -C $$dir clean; \
	done

debug:
	$(MAKE) -C $@ debug

$(COMPONENTS):
	$(MAKE) -C $@

