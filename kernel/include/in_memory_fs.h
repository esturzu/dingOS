#ifndef IN_MEMORY_FS_H
#define IN_MEMORY_FS_H

#include <cstddef>
#include <cstdint>

struct File {
    const char* name;
    uint8_t* data;
    size_t size;
};

// Initializes the in-memory filesystem
void init_fs();

// Finds a file by name
File* find_file(const char* filename);

#endif // IN_MEMORY_FS_H
