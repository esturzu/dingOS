// Citations
// https://medium.com/applied/applied-c-align-array-elements-32af40a768ee

#include "kernel.h"

#include "cores.h"
#include "crti.h"
#include "event_loop.h"
#include "ext2.h"
#include "framebuffer.h"
#include "heap.h"
#include "interrupts.h"
#include "local_timer.h"
#include "machine.h"
#include "physmem.h"
#include "printf.h"
#include "process.h"
#include "sd.h"
#include "stdint.h"
#include "system_timer.h"
#include "tester.h"
#include "vmm.h"

extern "C" void kernelMain() {
  // Handled uart Init
  PhysMem::page_init();

  CRTI::_init();

  VMM::init();

  printf("CurrentEL %s\n", STRING_EL(get_CurrentEL()));

  heap_init();
  init_event_loop();

  printf("DingOS is Booting!\n");
  debug_printf("Core %d! %s\n", SMP::whichCore(), STRING_EL(get_CurrentEL()));

  SMP::bootCores();

  LocalTimer::setup_timer();

  run_page_tests();

  SD::init();

  // // I run this with this command make clean-fs;make fs-image;clear; make
  // clean qemu DEBUG_ENABLED=0 to have the right disk, you also have to mkdir
  // fs_root beforehand
  //   SDAdapter* adapter = new SDAdapter(1024);
  //   Ext2* fs = new Ext2(adapter);
  //   const char* existing_file_name = "hello.txt";
  //   Node* existing_test_file = find_in_directory(fs->root,
  //   existing_file_name); int file_size = existing_test_file->size_in_bytes();
  //       if (existing_test_file) {
  //           char buffer[file_size + 1];
  //           int bytes_read = read_file(existing_test_file, buffer,
  //           file_size); if (bytes_read > 0) {
  //             buffer[bytes_read] = '\0';
  //             printf("File contents: %s\n", buffer);
  //           }
  //           delete existing_test_file;
  //       }
  //   // Create a test file
  //   const char* test_filename = "example.txt";
  //   Node* test_file = create_file(fs->root, test_filename);

  //   if (test_file) {
  //       const char* content = "Hello from DingOS EXT2 filesystem!";
  //       test_file->write_all(0, strlen_ext(content), (char*)content);
  //       printf("Successfully wrote to %s\n", test_filename);
  //       delete test_file;

  //       // Read the file back
  //       Node* reading_test_file = find_in_directory(fs->root, test_filename);
  //       int file_size = reading_test_file->size_in_bytes();
  //       if (reading_test_file) {
  //           char buffer[file_size + 1];
  //           int bytes_read = read_file(reading_test_file, buffer, file_size);
  //           if (bytes_read > 0) {
  //             buffer[bytes_read] = '\0';
  //             printf("File contents: %s\n", buffer);
  //           }
  //           delete reading_test_file;
  //       }
  //   }

  printf("BEFORE TESTS OK WTF PLEASE START PRINTING PLEASE START PRINTING!");
  setupTests();

  // while (true) {
  // }

  //   schedule_event([]{
  //     Process* proc = new Process();
  //     proc->run();
  //   });

  // event_loop();

  // Request a framebuffer at 800x600x32
  debug_printf("HELLO FROM FRAMEBUFFER BEFORE");
  // FrameBufferInfo* fb = framebuffer_init(640, 480, 32);
  // if (!fb) {
  //   debug_printf("Failed to init framebuffer!\n");
  //   while (true) { /* spin */
  //   }
  // }

  // Fill the screen with a color
  // framebuffer_fill(fb, 0xFFFF00);

  while (1);
}
