// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "cores.h"
#include "crti.h"
#include "event_loop.h"
#include "heap.h"
#include "interrupts.h"
#include "machine.h"
#include "physmem.h"
#include "printf.h"
#include "sd.h"
#include "stdint.h"
#include "system_timer.h"
#include "tester.h"
#include "bfs.h"



void tempFileTest() {
    fs_init();  // Initialize BFS (Baby File System)

    const char* test_filename = "testfile";

    // Allocate aligned buffers using new
    uint8_t* test_data = new uint8_t[SD::BLOCKSIZE];

    // Manually copy "Hello from DingOS!" into test_data
    const char* message = "Hello from DingOS!";
    for (size_t i = 0; message[i] != '\0' && i < SD::BLOCKSIZE - 1; i++) {
        test_data[i] = message[i];
    }
    test_data[SD::BLOCKSIZE - 1] = '\0';  // Null-terminate the string

    uint8_t* read_buffer = new uint8_t[SD::BLOCKSIZE]();

    // Create the file
    if (fs_create(test_filename, SD::BLOCKSIZE) == 0) {
        printf("File '%s' created successfully.\n", test_filename);
    } else {
        printf("ERROR: File creation failed.\n");
    }

    // Write to file
    if (fs_write(test_filename, reinterpret_cast<char*>(test_data), SD::BLOCKSIZE) == 0) {
        printf("File '%s' written successfully.\n", test_filename);
    } else {
        printf("ERROR: File write failed.\n");
    }

    // List files
    printf("about to list the files");
    fs_list();

    // Read from file
    printf("DEBUG: Attempting to read file '%s'\n", test_filename);
    int read_response = fs_read(test_filename, reinterpret_cast<char*>(read_buffer));
    printf("DEBUG: After fs_read(): test_filename=%s\n", test_filename);

    printf("read response: %d\n", read_response);
    if (read_response > 0) {
        printf("Read data: %s\n", reinterpret_cast<char*>(read_buffer));
    } else {
        printf("ERROR: File read failed.\n");
    }

    // Cleanup allocated memory
    delete[] test_data;
    delete[] read_buffer;
}

extern "C" void kernelMain() {
  // Handled uart Init
  CRTI::_init();

  debug_printf("CurrentEL %s\n", STRING_EL(get_CurrentEL()));

  heap_init();
  PhysMem::page_init();
  init_event_loop();

  printf("DingOS is Booting!\n");
  debug_printf("Core %d! %s\n", SMP::whichCore(), STRING_EL(get_CurrentEL()));

  SMP::bootCores();

  SD::init();

  tempFileTest();

  setupTests();

  event_loop();

  while (1)
    ;
}
