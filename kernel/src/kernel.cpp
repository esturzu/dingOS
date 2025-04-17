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

    run_page_tests();

    SD::init();

    fs_init();

    // USB Test on core 0 only
    if (SMP::whichCore() == 0) {
        g_usb.init();    
        uint32_t timeout = 5000000;
        while (timeout > 0) {
            timeout--;
            if (g_usb.is_device_connected()) {
                printf("USB Test: Device detected\n");
                
                // Update endpoint with enumerated address (assuming 1)
                g_usb.setup_endpoint(0, 0, 0, 8, 1); // Use address 1
                uint8_t setup_packet[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
                g_usb.send_data(0, setup_packet, 8);
    
                for (volatile int i = 0; i < 100000; i++);
    
                uint8_t buffer[18] = {0};
                uint32_t bytes_received = g_usb.receive_data(0, buffer, 18);
                if (bytes_received == 18) {
                    printf("USB Test: Received (%d bytes): ", bytes_received);
                    for (uint32_t i = 0; i < bytes_received; i++) {
                        printf("%02x ", buffer[i]);
                    }
                    printf("\n");
                    if (buffer[0] != 0x12 || buffer[1] != 0x01) {
                        printf("USB Test: Invalid device descriptor\n");
                    }
                } 
                else {
                    printf("USB Test: Failed to receive descriptor, got %d bytes: ", bytes_received);
                    for (uint32_t i = 0; i < bytes_received; i++) {
                        printf("%02x ", buffer[i]);
                    }
                    printf("\n");
                }
                break;
            }
            if (timeout % 1000000 == 0) {
                printf("USB Test: Polling, timeout remaining: %d iterations\n", timeout);
            }
            for (volatile int i = 0; i < 100; i++);
        }
        if (timeout == 0) {
            printf("USB Test: No device detected within timeout\n");
        }
    }

    setupTests();
    while (1) {}
}