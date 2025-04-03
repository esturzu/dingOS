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

  // Initialize filesystem with the working approach
  SDAdapter* adapter = new SDAdapter(1024);
  Ext2* fs = new Ext2(adapter);

  // Now you can access the root directory
  printf("Listing root directory contents:\n");
  list_directory(fs->root);

  // // Find and read hello.txt
  // const char* filename = "hello.txt";
  // printf("Looking for file: %s\n", filename);
  
  // Node* file = find_in_directory(fs->root, filename);
  // if (file) {
  //   printf("Found file '%s' (inode %u, size %u bytes)\n",
  //          filename, file->number, file->node->size_of_iNode);
    
  //   // Read the file content
  //   char buffer[1024];
  //   int bytes_read = read_file(file, buffer, sizeof(buffer) - 1);
    
  //   if (bytes_read > 0) {
  //     printf("File content (%d bytes):\n%s\n", bytes_read, buffer);
      
  //     // Now try to modify the file (as a test for write functionality)
  //     printf("Attempting to modify file...\n");
  //     const char* new_content = "Modified by DingOS!";
  //     int64_t bytes_written = file->write_all(0, strlen_ext(new_content), (char*)new_content);
      
  //     if (bytes_written > 0) {
  //       printf("Successfully modified file - wrote %lld bytes\n", bytes_written);
        
  //       // Read it back to verify
  //       bytes_read = read_file(file, buffer, sizeof(buffer) - 1);
  //       if (bytes_read > 0) {
  //         printf("Updated content: %s\n", buffer);
  //       }
  //     } else {
  //       printf("Failed to modify file\n");
  //     }
  //   } else {
  //     printf("Error reading file or empty file\n");
  //   }
    
  //   delete file;
  // } else {
  //   printf("File '%s' not found\n", filename);
  // }
  
  // Now try creating a new file
  printf("\nTesting file creation...\n");
  const char* test_filename = "test_file.txt";
  printf("Creating file: %s\n", test_filename);
  
  Node* test_file = create_file(fs->root, test_filename);
  if (test_file) {
    printf("File created successfully, writing content...\n");
    
    const char* test_data = "This is a test file created by DingOS.";
    int64_t write_result = test_file->write_all(0, strlen_ext(test_data), (char*)test_data);
    
    if (write_result > 0) {
      printf("Content written successfully (%lld bytes)\n", write_result);
    } else {
      printf("Failed to write content\n");
    }
    
    delete test_file;
    
    // Verify by listing directory again
    printf("\nUpdated directory contents:\n");
    list_directory(fs->root);
  } else {
    printf("Failed to create test file\n");
  }

  while (1);
}