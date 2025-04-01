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
#include "ext2.h"


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

  run_page_tests();

  SD::init();

  // fs_init_bfs();

  SDAdapter* adapter = new SDAdapter(1024);

  // Initialize filesystem
  Ext2* fs = new Ext2(adapter);

  // Now you can access the root directory
  Node* root_dir = fs->root;
  printf("Listing root directory contents:\n");
  list_directory(fs->root);
  // Find and read hello.txt
  const char* filename = "hello.txt";
  printf("Looking for file: %s\n", filename);
  
  Node* file = find_in_directory(fs->root, filename);
  if (file) {
    printf("Found file '%s' (inode %u, size %u bytes)\n", 
            filename, file->number, file->node->size_of_iNode);
    
    // Read the file content
    char buffer[1024];
    int bytes_read = read_file(file, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
      printf("File content (%d bytes):\n%s\n", bytes_read, buffer);
    } else {
      printf("Error reading file or empty file\n");
    }
    
    delete file;
  } else {
    printf("File '%s' not found\n", filename);
  }
  


  // setupTests();

  // event_loop();

  while (1)
    ;
}
