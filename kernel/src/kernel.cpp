#include "kernel.h"

#include "local_timer.h"
#include "cores.h"
#include "crti.h"
#include "event_loop.h"
#include "heap.h"
// #include "interrupts.h"
// #include "interrupts.h"
#include "machine.h"
#include "physmem.h"
#include "printf.h"
#include "process.h"
#include "sd.h"
#include "stdint.h"
#include "system_timer.h"
#include "tester.h"
#include "ext2.h"
#include "vmm.h"
#include "usb.h"

extern "C" void kernelMain() {
  // Handled uart Init
  PhysMem::page_init();

  CRTI::_init();

  VMM::init();

  printf("CurrentEL %s\n", STRING_EL(get_CurrentEL()));

  heap_init();
  init_event_loop();

    printf("DingOS is Booting!\n");

  SMP::bootCores();

  LocalTimer::setup_timer();

  // run_page_tests();

    SD::init();

  // To have the right disk, you have to 'mkdir fs_root'
  // Then, run with command:
  // make clean-fs ; make fs-image ; clear ; make clean qemu DEBUG_ENABLED=0
  SDAdapter* adapter = new SDAdapter(1024);
  Ext2* fs = new Ext2(adapter);
  const char* existing_file_name = "hello.txt";
  printf("about to find %s\n", existing_file_name);
  Node* existing_test_file = find_in_directory(fs->root, existing_file_name);
  printf("found%s\n", existing_file_name);
  
  int file_size = existing_test_file->size_in_bytes();
  if (existing_test_file) {
    char buffer[file_size + 1];
    int bytes_read = read_file(existing_test_file, buffer, file_size);
    if (bytes_read > 0) {
      buffer[bytes_read] = '\0';
      printf("File contents: %s\n", buffer);
    }
    delete existing_test_file;
  }
  printf("\n");
  printf("\n");
  printf("\n");
  printf("_________________________________________________________________________________________\n");
  printf("about to find the file, again, should be faster.....bc of cache %s\n", existing_file_name);
  printf("\n");
  printf("\n");
  printf("\n");
  printf("_________________________________________________________________________________________\n");
  Node* existing_test_file_2 = find_in_directory(fs->root, existing_file_name);
  printf("found%s\n", existing_file_name);
  
  int file_size_2 = existing_test_file_2->size_in_bytes();
  if (existing_test_file_2) {
    char buffer[file_size + 1];
    int bytes_read = read_file(existing_test_file_2, buffer, file_size_2);
    if (bytes_read > 0) {
      buffer[bytes_read] = '\0';
      printf("File contents: %s\n", buffer);
    }
    delete existing_test_file_2;
  }
  
  // Create a test file
  const char* test_filename = "example.txt";
  printf("about to create %s\n", test_filename);
  Node* test_file = create_file(fs->root, test_filename);
  printf("Successfully created %s\n", test_filename);

  if (test_file) {
    const char* content = "Hello from DingOS EXT2 filesystem!";
    printf("about to write to %s\n", test_filename);
    test_file->write_all(0, strlen_ext(content), (char*)content);
    printf("Successfully wrote to %s\n", test_filename);
    delete test_file;

    // Read the file back
    Node* reading_test_file = find_in_directory(fs->root, test_filename);
    int file_size = reading_test_file->size_in_bytes();
    if (reading_test_file) {
      char buffer[file_size + 1];
      int bytes_read = read_file(reading_test_file, buffer, file_size);
      if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("File contents: %s\n", buffer);
      }
      delete reading_test_file;
    }
  }

  // setupTests();


  schedule_event([] {
    Process* proc = new Process();
    proc->run();
  });

  event_loop();

  // // Request a framebuffer at 640x480x32
  // FrameBufferInfo* fb = framebuffer_init(640, 480, 32);
  // if (!fb) {
  //   debug_printf("Failed to init framebuffer!\n");
  //   while (true) { /* spin */
  //   }
  // }

  // // Fill the screen with a color
  // framebuffer_fill(fb, 0xFFFF00);

  while (1);
}