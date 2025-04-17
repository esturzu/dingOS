#include "usb.h"
#include "physmem.h"      
#include "system_timer.h" 

UsbController g_usb;  

UsbController::UsbController() {
    usb_base = reinterpret_cast<volatile uint32_t*>(USB_BASE);
    if (!usb_base || read_reg(USB_GOTGCTL) == 0xFFFFFFFF) {
        printf("USB: Failed to access controller at 0x%X on core %d\n", USB_BASE, SMP::whichCore());
    }

    // Initialize fake device descriptor
    uint8_t descriptor[18] = {
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, // bLength, bDescriptorType, bcdUSB, bDeviceClass, etc.
        0x34, 0x12, 0x78, 0x56, 0x00, 0x01, 0x00, 0x00, // idVendor, idProduct, bcdDevice, iManufacturer, etc.
        0x00, 0x01                                              // iSerialNumber, bNumConfigurations
    };
    for (int i = 0; i < 18; i++) {
        fake_device_descriptor[i] = descriptor[i];
    }
    use_fake_descriptor = true;
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

    // Configure GUSBCFG
    uint32_t usbcfg = read_reg(USB_GUSBCFG);
    usbcfg |= (1 << 29);  // Force Host Mode
    usbcfg &= ~(1 << 30); // Clear Force Device Mode
    usbcfg |= (1 << 6);   // PHY Interface: 16-bit (typical for DWC_OTG)
    usbcfg &= ~(1 << 3);  // Ensure ULPI PHY is not selected (use UTMI)
    write_reg(USB_GUSBCFG, usbcfg);

    // Power the port
    uint32_t hprt = read_reg(USB_HPRT);
    hprt |= (1 << 12);  // Set Port Power
    write_reg(USB_HPRT, hprt);

    // Wait for device connection
    uint32_t timeout = 1000000;
    while (timeout--) {
        hprt = read_reg(USB_HPRT);
        if (hprt & (1 << 0)) {  // Port Connect Detected
            printf("USB: Device connected, HPRT = 0x%X\n", hprt);
            break;
        }
        if (timeout % 250000 == 0) {
            printf("USB: Waiting for connection, HPRT = 0x%X, timeout = %d\n", hprt, timeout);
        }
        for (volatile int i = 0; i < 100; i++);
    }

    if (timeout == 0) {
        printf("USB: Host mode failed: No device detected\n");
        return;
    }

    // Force full-speed
    uint32_t hcfg = read_reg(USB_HCFG);
    hcfg &= ~0x3;  // Clear FSLSPCS (bits 1:0)
    hcfg |= 0x1;   // Set FSLSPCS to 1 (full-speed)
    write_reg(USB_HCFG, hcfg);

    printf("USB: Host mode configured successfully\n");
}

void UsbController::setup_endpoint(uint8_t channel, uint8_t ep_num, uint8_t ep_type, uint16_t max_packet_size, uint8_t device_address) {
    uint32_t hc_base = USB_HC0_BASE + (channel * 0x20);
    uint32_t hcchar = (max_packet_size << 0) |  // Max packet size
                      (ep_num << 11) |          // Endpoint number
                      (ep_type << 18) |         // Endpoint type (0=Control)
                      (device_address << 22) |  // Device address (bits 22-28)
                      (1 << 15);                // Enable channel
    write_reg(hc_base + HCCHAR, hcchar);
    write_reg(hc_base + HCTSIZ, (1 << 19) | max_packet_size);
    write_reg(hc_base + HCINTMSK, (1 << 0));
}

void UsbController::init() {
    printf("USB: Starting init on core %d\n", SMP::whichCore());
    write_reg(USB_GAHBCFG, (1 << 0)); // Enable AHB interrupt (no DMA)
    printf("USB: AHB interrupt enabled\n");

    write_reg(USB_GRXFSIZ, 0x200);
    write_reg(USB_GNPTXFSIZ, (0x200 << 16) | 0x200);
    
    reset_controller();
    configure_host_mode();
    printf("USB: Post-configure_host_mode\n");

    if (enumerate_device()) {
        printf("USB: Device enumerated successfully\n");
    } else {
        printf("USB: Device enumeration failed\n");
    }

    write_reg(USB_GINTMSK, (1 << 24) | (1 << 3));
    printf("USB: Interrupts masked\n");
    setup_endpoint(0, 0, 0, 8);
    // printf("USB: Endpoint 0 setup\n");

    printf("USB: Controller initialized at 0x%X on core %d\n", USB_BASE, SMP::whichCore());
}

bool UsbController::is_device_connected() {
    uint32_t hprt = read_reg(USB_HPRT);
    printf("USB: HPRT = 0x%X\n", hprt);
    return (hprt & (1 << 0)) != 0;  // Check Port Connect Detected bit
}

void UsbController::send_data(uint8_t channel, const uint8_t* data, uint32_t length) {
    uint32_t hc_base = USB_HC0_BASE + (channel * 0x20);
    write_reg(hc_base + HCTSIZ, (1 << 19) | length);
    
    for (uint32_t i = 0; i < length; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");

    // Store the setup packet for checking in receive_data
    if (length == 8) {
        for (uint32_t i = 0; i < 8; i++) {
            last_setup_packet[i] = data[i];
        }
    }

    // Skip FIFO write (not implemented in QEMU)
    uint32_t hcchar = read_reg(hc_base + HCCHAR);
    hcchar |= (1 << 15); // Enable channel
    hcchar &= ~(1 << 20); // OUT direction for Setup
    write_reg(hc_base + HCCHAR, hcchar);
    printf("USB: Sent %d bytes on channel %d from core %d (FIFO write skipped)\n", length, channel, SMP::whichCore());
}

uint32_t UsbController::receive_data(uint8_t channel, uint8_t* buffer, uint32_t max_length) {
    // Check if this is a GET_DESCRIPTOR request for a device descriptor
    bool is_get_device_descriptor = (last_setup_packet[0] == 0x80 &&
                                     last_setup_packet[1] == 0x06 &&
                                     last_setup_packet[2] == 0x00 &&
                                     last_setup_packet[3] == 0x01);

    // Return fake descriptor if applicable
    if (use_fake_descriptor && is_get_device_descriptor) {
        uint32_t bytes_to_copy = (max_length <= 18) ? max_length : 18;
        for (uint32_t i = 0; i < bytes_to_copy; i++) {
            buffer[i] = fake_device_descriptor[i];
        }
        printf("USB: Returning fake device descriptor (%d bytes)\n", bytes_to_copy);
        return bytes_to_copy;
    }

    // Otherwise, proceed with the transfer (which will fail in QEMU)
    uint32_t hc_base = USB_HC0_BASE + (channel * 0x20);
    
    write_reg(hc_base + HCTSIZ, (1 << 19) | max_length);
    
    uint32_t hcchar = read_reg(hc_base + HCCHAR);
    hcchar |= (1 << 15); // Enable channel
    hcchar |= (1 << 20); // IN direction for Data
    write_reg(hc_base + HCCHAR, hcchar);

    uint32_t timeout = 1000000;
    while (!(read_reg(hc_base + HCINT) & (1 << 0)) && timeout--) {
        for (volatile int i = 0; i < 100; i++);
    }
    if (timeout == 0) {
        printf("USB: Timeout waiting for data on channel %d\n", channel);
        return 0;
    }

    uint32_t hcint = read_reg(hc_base + HCINT);
    printf("USB: HCINT = 0x%X\n", hcint);
    uint32_t hctsiz = read_reg(hc_base + HCTSIZ);
    uint32_t xfersiz = hctsiz & 0x7FFFF;
    uint32_t pktcnt = (hctsiz >> 19) & 0x3FF;
    printf("USB: HCTSIZ = 0x%X, xfersiz = %d, pktcnt = %d\n", hctsiz, xfersiz, pktcnt);

    if (xfersiz > max_length) xfersiz = max_length;

    // Skip FIFO read (not implemented in QEMU), fill with dummy data
    uint32_t bytes_received = xfersiz;
    for (uint32_t i = 0; i < bytes_received; i++) {
        buffer[i] = 0xFF; // Dummy value to indicate we skipped the FIFO read
    }
    write_reg(hc_base + HCINT, (1 << 0));

    // Status phase: Send zero-length OUT packet
    write_reg(hc_base + HCTSIZ, (1 << 19) | 0);
    hcchar = read_reg(hc_base + HCCHAR);
    hcchar &= ~(1 << 20); // OUT direction for Status
    write_reg(hc_base + HCCHAR, hcchar | (1 << 15));

    timeout = 1000000;
    while (!(read_reg(hc_base + HCINT) & (1 << 0)) && timeout--);
    write_reg(hc_base + HCINT, (1 << 0));

    printf("USB: Received %d bytes on channel %d (FIFO read skipped)\n", bytes_received, channel);
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

bool UsbController::enumerate_device() {
    uint32_t hprt = read_reg(USB_HPRT);
    hprt |= (1 << 8);  // Port Reset
    write_reg(USB_HPRT, hprt);
    for (volatile int i = 0; i < 500000; i++);
    hprt &= ~(1 << 8);
    write_reg(USB_HPRT, hprt);

    hprt = read_reg(USB_HPRT);
    if (!(hprt & (1 << 0))) {
        printf("USB: Device not connected after reset, HPRT = 0x%X\n", hprt);
        return false;
    } else {
        printf("USB: Device connected after reset, HPRT = 0x%X\n", hprt);
    }

    // Request only 8 bytes first
    uint8_t setup_packet[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x08, 0x00};
    send_data(0, setup_packet, 8);

    for (volatile int i = 0; i < 100000; i++);

    uint8_t buffer[8];
    uint32_t bytes = receive_data(0, buffer, 8);
    if (bytes < 8) {
        printf("USB: Enumeration failed: Received %d bytes instead of 8: ", bytes);
        for (uint32_t i = 0; i < bytes; i++) {
            printf("%02x ", buffer[i]);
        }
        printf("\n");
        return false;
    }

    printf("USB: First 8 bytes of device descriptor: ");
    for (uint32_t i = 0; i < bytes; i++) {
        printf("%02x ", buffer[i]);
    }
    printf("\n");

    // Proceed with full 18-byte request
    uint8_t setup_packet_full[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
    send_data(0, setup_packet_full, 8);

    for (volatile int i = 0; i < 100000; i++);

    uint8_t buffer_full[18];
    bytes = receive_data(0, buffer_full, 18);
    if (bytes < 18) {
        printf("USB: Enumeration failed: Received %d bytes instead of 18: ", bytes);
        for (uint32_t i = 0; i < bytes; i++) {
            printf("%02x ", buffer_full[i]);
        }
        printf("\n");
        return false;
    }

    printf("USB: Device descriptor: ");
    for (uint32_t i = 0; i < bytes; i++) {
        printf("%02x ", buffer_full[i]);
    }
    printf("\n");

    uint8_t new_address = 1;
    uint8_t set_addr_packet[] = {0x00, 0x05, new_address, 0x00, 0x00, 0x00, 0x00, 0x00};
    send_data(0, set_addr_packet, 8);
    for (volatile int i = 0; i < 100000; i++);

    setup_endpoint(0, 0, 0, buffer_full[7], new_address);
    printf("USB: Device enumerated successfully, address set to %d\n", new_address);
    return true;
}