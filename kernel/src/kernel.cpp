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
    // printf("Step 1: Entering kernelMain on core %d\n", SMP::whichCore());
    CRTI::_init();
    // printf("Step 2: CRTI initialized on core %d\n", SMP::whichCore());
    // printf("Step 3: CurrentEL %s\n", STRING_EL(get_CurrentEL()));

    heap_init();
    // printf("Step 4: Heap initialized on core %d\n", SMP::whichCore());

    init_message_queue();
    // printf("Step 5: Message queue initialized on core %d\n", SMP::whichCore());

    PhysMem::page_init();
    // printf("Step 6: Physical memory initialized on core %d (no MMU)\n", SMP::whichCore());

    init_event_loop();
    // printf("Step 7: Event loop initialized on core %d\n", SMP::whichCore());

    printf("DingOS is Booting!\n");
    // printf("Step 8: Boot message printed on core %d\n", SMP::whichCore());

    SMP::bootCores();
    printf("Step 9: SMP bootCores completed on core %d\n", SMP::whichCore());

    run_page_tests();
    printf("Step 10: Page tests completed on core %d\n", SMP::whichCore());

    SD::init();
    printf("Step 11: SD initialized on core %d\n", SMP::whichCore());

    fs_init();
    printf("Step 12: Filesystem initialized on core %d\n", SMP::whichCore());

    if (SMP::whichCore() == 0) {
        printf("Step 13: Core 0 starting USB init\n");
        UsbController usb;
        usb.init();
        printf("Step 14: Core 0 finished USB init\n");

        // Update for keyboard testing
        uint32_t timeout = 5000000;
        while (timeout > 0) {
            timeout--;
            if (usb.is_device_connected()) {
                printf("Step 15: USB Test: Device detected\n");

                uint8_t setup_packet[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x08, 0x00};
                usb.send_data(0, setup_packet, 8);
                uint8_t buffer[8] = {0};
                uint32_t bytes_received = usb.receive_data(0, buffer, 8);
                if (bytes_received > 0) {
                    printf("Step 16: USB Test: Received (%d bytes): ", bytes_received);
                    for (uint32_t i = 0; i < bytes_received; i++) {
                        printf("%02x ", buffer[i]);
                    }
                    printf("\n");
                }

                // Poll for HID report 
                printf("Step 17: Starting keyboard polling\n");
                while (1) {  
                    uint8_t hid_report[8] = {0};
                    uint32_t report_received = usb.receive_data(0, hid_report, 8);
                    if (report_received > 0) {
                        printf("Step 18: Keyboard Input: ");
                        for (uint32_t i = 0; i < report_received; i++) {
                            printf("%02x ", hid_report[i]);
                        }
                        printf("\n");
                    }
                    for (volatile int i = 0; i < 10000; i++); 
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
    printf("Step 19: Tests setup on core %d\n", SMP::whichCore());
    printf("Step 20: Core %d entering infinite loop\n", SMP::whichCore());
    while (1) {}
}