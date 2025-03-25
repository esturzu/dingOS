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
#include "usb.h"

extern "C" void kernelMain() {
    CRTI::_init();

    heap_init();

    init_message_queue();

    PhysMem::page_init();  // No MMU setup, just placeholder

    init_event_loop();

    printf("DingOS is Booting!\n");

    SMP::bootCores();
    printf("Step 9: SMP bootCores completed on core %d\n", SMP::whichCore());

    run_page_tests();
    printf("Step 10: Page tests completed on core %d\n", SMP::whichCore());

    SD::init();
    printf("Step 11: SD initialized on core %d\n", SMP::whichCore());

    fs_init();
    printf("Step 12: Filesystem initialized on core %d\n", SMP::whichCore());

    // USB Test on core 0 only
    if (SMP::whichCore() == 0) {
      printf("Step 13: Core 0 starting USB init\n");
      UsbController usb;
      usb.init();
      printf("Step 14: Core 0 finished USB init\n");

      uint32_t timeout = 5000000;
      while (timeout > 0) {
          timeout--;        
          if (usb.is_device_connected()) {
              printf("Step 15: USB Test: Device detected\n");
              uint8_t setup_packet[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x08, 0x00};
              usb.send_data(0, setup_packet, 8);

              for (volatile int i = 0; i < 100000; i++);  

              uint8_t buffer[8] = {0};
              uint32_t bytes_received = usb.receive_data(0, buffer, 8);
              if (bytes_received > 0) {
                  printf("Step 16: USB Test: Received (%d bytes): ", bytes_received);
                  for (uint32_t i = 0; i < bytes_received; i++) {
                      printf("%02x ", buffer[i]);
                  }
                  printf("\n");
              }
              break;
          }
          if (timeout % 1000000 == 0) {
              printf("Step 15: USB Test: Polling, timeout remaining: %d\n", timeout);
          }
          for (volatile int i = 0; i < 100; i++);
      }
      if (timeout == 0) {
          printf("Step 15: USB Test: No device detected within timeout\n");
      }
    } else {
      printf("Step 13: Core %d skipping USB init\n", SMP::whichCore());
    }

    setupTests();
    printf("Step 17: Tests setup on core %d\n", SMP::whichCore());

    printf("Step 18: Core %d entering infinite loop\n", SMP::whichCore());
    while (1) {}
}