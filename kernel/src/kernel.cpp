// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "atomics.h"
#include "cores.h"
#include "crti.h"
#include "definitions.h"
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
#include "uart.h"
#include "bfs.h"

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

  setupTests();
  uint8_t temp_buffer[512] = {0};
  int res = SD::read(0, 1, temp_buffer);
  debug_printf("Manual SD Read: %d bytes, first: 0x%02x 0x%02x\n", 
            res, temp_buffer[0], temp_buffer[1]);

  
  fs_init();  // Initialize BFS (Baby File System)

  // --- Simple FS Test ---
  const char* test_filename = "testfile";
  const char* test_data = "Hello from DingOS!";
  char read_buffer[64] = {0};  // Buffer for reading back

  // Create the file
  if (fs_create(test_filename, 32) == 0) {
      printf("File '%s' created successfully.\n", test_filename);
  } else {
      printf("ERROR: File creation failed.\n");
  }

  // Write to file
  if (fs_write(test_filename, test_data, 18) == 0) {
      printf("File '%s' written successfully.\n", test_filename);
  } else {
      printf("ERROR: File write failed.\n");
  }

  printf("DEBUG: Attempting to read file '%s'\n", test_filename);
  // Read from file
  printf("DEBUG: Before fs_read(): test_filename=%s\n", test_filename);
  int read_response = fs_read(test_filename, read_buffer);
  printf("DEBUG: After fs_read(): test_filename=%s\n", test_filename);

  printf("read respone %s", read_response);
  if (read_response > 0) {
      printf("Read from %s\n", read_buffer);
  } else {
      printf("ERROR: File read failed.\n");
  }
  // List files
  printf("about to list the files, though idk if it will work bc read eats the file name");
  fs_list();

  event_loop();

  while (1);
}