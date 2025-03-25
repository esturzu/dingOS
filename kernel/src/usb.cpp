#include "usb.h"
#include "physmem.h"      
#include "system_timer.h" 

UsbController g_usb;  

UsbController::UsbController() {
    usb_base = reinterpret_cast<volatile uint32_t*>(USB_BASE);
    if (!usb_base || read_reg(USB_GOTGCTL) == 0xFFFFFFFF) {
        printf("USB: Failed to access controller at 0x%X on core %d\n", USB_BASE, SMP::whichCore());
    }
}

UsbController::~UsbController() {}

void UsbController::reset_controller() {
    printf("USB: Resetting controller\n");
    write_reg(USB_GRSTCTL, (1 << 0)); 
    while (read_reg(USB_GRSTCTL) & (1 << 0)) {
        for (volatile int i = 0; i < 1000; i++);
    }
    while (!(read_reg(USB_GRSTCTL) & (1 << 31))) { 
        for (volatile int i = 0; i < 1000; i++);
    }
    printf("USB: Reset complete\n");
}

void UsbController::configure_host_mode() {
    printf("USB: Configuring host mode\n");
    uint32_t usbcfg = read_reg(USB_GUSBCFG);
    usbcfg |= (1 << 29);  // Force Host Mode
    usbcfg &= ~(1 << 30); 
    write_reg(USB_GUSBCFG, usbcfg);

    uint32_t timeout = 1000000;
    uint32_t gotgctl;
    bool negotiated = false;
    while (timeout--) {
        gotgctl = read_reg(USB_GOTGCTL);
        if (gotgctl & (1 << 20)) { 
            negotiated = true;
            break;
        }
        if (timeout % 250000 == 0) {
            printf("USB: Waiting for host mode, GOTGCTL = 0x%X, timeout = %d\n", gotgctl, timeout);
        }
        for (volatile int i = 0; i < 100; i++);  
    }
    if (negotiated) {
        printf("USB: Host mode configured, GOTGCTL = 0x%X\n", gotgctl);
    } else {
        printf("USB: Host mode config timed out, proceeding anyway, GOTGCTL = 0x%X\n", gotgctl);
    }
}

void UsbController::setup_endpoint(uint8_t channel, uint8_t ep_num, uint8_t ep_type, uint16_t max_packet_size) {
    uint32_t hc_base = USB_HC0_BASE + (channel * 0x20);

    uint32_t hcchar = (max_packet_size << 0) |  // Max packet size
                      (ep_num << 11) |          // Endpoint number
                      (ep_type << 18) |         // Endpoint type (0=Control, 2=Bulk)
                      (1 << 15);                // Enable channel
    write_reg(hc_base + HCCHAR, hcchar);

    write_reg(hc_base + HCTSIZ, (1 << 19) | max_packet_size);  
    write_reg(hc_base + HCINTMSK, (1 << 0));  
}

void UsbController::init() {
    printf("USB: Starting init on core %d\n", SMP::whichCore());
    write_reg(USB_GAHBCFG, (1 << 0));
    printf("USB: AHB interrupt enabled\n");
    reset_controller();
    configure_host_mode();
    printf("USB: Post-configure_host_mode\n");

    write_reg(USB_GINTMSK, (1 << 24) | (1 << 3));
    printf("USB: Interrupts masked\n");
    setup_endpoint(0, 0, 0, 8);
    printf("USB: Endpoint 0 setup\n");

    // Interrupts::Enable_IRQ(USB_IRQ);
    printf("USB: Skipping IRQ enable for debugging\n");

    printf("USB: Controller initialized at 0x%X on core %d\n", USB_BASE, SMP::whichCore());
}

bool UsbController::is_device_connected() {
    uint32_t otgctl = read_reg(USB_GOTGCTL);
    uint32_t gintsts = read_reg(USB_GINTSTS);
    printf("USB: GOTGCTL = 0x%X, GINTSTS = 0x%X\n", otgctl, gintsts);
    return (otgctl & (1 << 2)) != 0 || (gintsts & (1 << 24)) != 0; 
}

void UsbController::send_data(uint8_t channel, const uint8_t* data, uint32_t length) {
    uint32_t hc_base = USB_HC0_BASE + (channel * 0x20);

    write_reg(hc_base + HCTSIZ, (1 << 19) | length);

    volatile uint32_t* fifo = usb_base + (0x1000 >> 2) + (channel * 0x1000);
    for (uint32_t i = 0; i < length; i += 4) {
        uint32_t word = 0;
        for (uint8_t j = 0; j < 4 && (i + j) < length; j++) {
            word |= (data[i + j] << (j * 8));
        }
        *fifo = word;
    }

    uint32_t hcchar = read_reg(hc_base + HCCHAR);
    write_reg(hc_base + HCCHAR, hcchar | (1 << 15)); 

    printf("USB: Sent %d bytes on channel %d from core %d\n", length, channel, SMP::whichCore());
}

uint32_t UsbController::receive_data(uint8_t channel, uint8_t* buffer, uint32_t max_length) {
    uint32_t hc_base = USB_HC0_BASE + (channel * 0x20);
    volatile uint32_t* fifo = usb_base + (0x2000 >> 2); 

    // Wait for data, polling
    uint32_t timeout = 1000000;
    while (!(read_reg(hc_base + HCINT) & (1 << 0)) && timeout--) {
        for (volatile int i = 0; i < 100; i++);  // Short delay
    }
    if (timeout == 0) {
        printf("USB: Timeout waiting for data on channel %d\n", channel);
        return 0;
    }

    uint32_t bytes_received = 0;
    while (bytes_received < max_length) {
        uint32_t word = *fifo;
        for (uint8_t i = 0; i < 4 && bytes_received < max_length; i++) {
            buffer[bytes_received++] = (word >> (i * 8)) & 0xFF;
        }
    }

    write_reg(hc_base + HCINT, (1 << 0));  
    printf("USB: Received %d bytes on channel %d\n", bytes_received, channel);
    return bytes_received;
}

void UsbController::handle_interrupt() {
    uint32_t gintsts = read_reg(USB_GINTSTS);

    if (gintsts & (1 << 24)) {  // Port Interrupt
        if (is_device_connected()) {
            printf("USB: Device connected on core %d\n", SMP::whichCore());
        } else {
            printf("USB: Device disconnected on core %d\n", SMP::whichCore());
        }
    }

    if (gintsts & (1 << 3)) { 
        for (uint8_t ch = 0; ch < 16; ch++) {
            uint32_t hc_base = USB_HC0_BASE + (ch * 0x20);
            uint32_t hcint = read_reg(hc_base + HCINT);
            if (hcint & (1 << 0)) {  // Transfer Completed
                printf("USB: Transfer completed on channel %d, core %d\n", ch, SMP::whichCore());
                write_reg(hc_base + HCINT, hcint); 
            }
        }
    }

    write_reg(USB_GINTSTS, gintsts); 
}