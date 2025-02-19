#include "in_memory_fs.h"
#include <cstring>

// Dummy ELF binary images (replace with actual ELF file data)
extern uint8_t init_elf[];
extern uint8_t shell_elf[];
extern size_t init_elf_size;
extern size_t shell_elf_size;

// Static in-memory file table
static File ramdisk_files[] = {
    { "init.elf", init_elf, init_elf_size },
    { "shell.elf", shell_elf, shell_elf_size }
};

// Initializes the filesystem (currently a no-op)
void init_fs() {
    // If needed, perform setup here.
}

// Finds a file in the in-memory filesystem
File* find_file(const char* filename) {
    for (size_t i = 0; i < sizeof(ramdisk_files) / sizeof(File); i++) {
        if (strcmp(ramdisk_files[i].name, filename) == 0) {
            return &ramdisk_files[i];
        }
    }
    return nullptr; // File not found
}
