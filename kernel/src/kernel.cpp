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

    run_page_tests();

    SD::init();


  // // I run this with this command make clean-fs;make fs-image;clear; make clean qemu DEBUG_ENABLED=0 to have the right disk, you also have to mkdir fs_root beforehand
//   SDAdapter* adapter = new SDAdapter(1024);
//   Ext2* fs = new Ext2(adapter);
//   const char* existing_file_name = "hello.txt";
//   Node* existing_test_file = find_in_directory(fs->root, existing_file_name);
//   int file_size = existing_test_file->size_in_bytes();
//       if (existing_test_file) {
//           char buffer[file_size + 1];
//           int bytes_read = read_file(existing_test_file, buffer, file_size);
//           if (bytes_read > 0) {
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

  g_usb.init();
  printf("USB TEST: Core %d finished USB init\n", SMP::whichCore());

  // Step 15: Test USB by requesting the device descriptor
  uint8_t setup_packet[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
  g_usb.send_data(0, setup_packet, 8);
  for (volatile int i = 0; i < 100000; i++);
  uint8_t buffer[18];
  uint32_t bytes = g_usb.receive_data(0, buffer, 18);
  printf("USB TEST: USB Test: Received (%d bytes): ", bytes);
  for (uint32_t i = 0; i < bytes; i++) {
      printf("%02x ", buffer[i]);
  }
  printf("\n");
  if (bytes >= 2 && buffer[0] == 0x12 && buffer[1] == 0x01) {
      printf("USB TEST: USB Test: Valid device descriptor\n");
  } else {
      printf("USB TEST: USB Test: Invalid device descriptor\n");
  }



  g_usb.init();
  printf("USB TEST: Core %d finished USB init\n", SMP::whichCore());

  // Step 15: Test USB by requesting the device descriptor
  uint8_t setup_packet[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
  g_usb.send_data(0, setup_packet, 8);
  for (volatile int i = 0; i < 100000; i++);
  uint8_t buffer[18];
  uint32_t bytes = g_usb.receive_data(0, buffer, 18);
  printf("USB TEST: USB Test: Received (%d bytes): ", bytes);
  for (uint32_t i = 0; i < bytes; i++) {
      printf("%02x ", buffer[i]);
  }
  printf("\n");
  if (bytes >= 2 && buffer[0] == 0x12 && buffer[1] == 0x01) {
      printf("USB TEST: USB Test: Valid device descriptor\n");
  } else {
      printf("USB TEST: USB Test: Invalid device descriptor\n");
  }


  
 

  setupTests();

  while (true) {}

//   schedule_event([]{
//     Process* proc = new Process();
//     proc->run();
//   });

  // event_loop();

  while (1);
}