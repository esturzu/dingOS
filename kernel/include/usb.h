#ifndef USB_H
#define USB_H

#include <stdint.h>      // For fixed-size integer types
#include "interrupts.h"  // For interrupt handling
#include "printf.h"      // For debug output
#include "cores.h"       // For SMP::whichCore()

// Physical base address of the DWC_OTG USB controller on RPi 3B
#define USB_BASE 0x3F980000

// Core USB OTG registers (offsets from USB_BASE)
#define USB_GOTGCTL   (0x000)  // OTG Control and Status
#define USB_GAHBCFG   (0x008)  // AHB Configuration
#define USB_GUSBCFG   (0x00C)  // USB Configuration
#define USB_GRSTCTL   (0x010)  // Reset Control
#define USB_GINTSTS   (0x014)  // Interrupt Status
#define USB_GINTMSK   (0x018)  // Interrupt Mask
#define USB_HCFG      (0x400)  // Host Configuration
#define USB_HPTXFSIZ  (0x100)  // Host Periodic Tx FIFO Size
#define USB_HC0_BASE  (0x500)  // Host Channel 0 base (offset for each channel)

// Host Channel registers (per channel)
#define HCCHAR   (0x00)  // Channel Characteristics
#define HCINT    (0x08)  // Channel Interrupt
#define HCINTMSK (0x0C)  // Channel Interrupt Mask
#define HCTSIZ   (0x10)  // Channel Transfer Size

// USB IRQ number for RPi 3B
#define USB_IRQ 9

class UsbController {
private:
    volatile uint32_t* usb_base;  // USB controller registers

    // Register access helpers
    inline uint32_t read_reg(uint32_t offset) const {
        return *(usb_base + (offset >> 2));
    }
    
    inline void write_reg(uint32_t offset, uint32_t value) {
        *(usb_base + (offset >> 2)) = value;
    }

    // Internal methods
    void reset_controller();
    void configure_host_mode();
    void setup_endpoint(uint8_t channel, uint8_t ep_num, uint8_t ep_type, uint16_t max_packet_size);

public:
    UsbController();
    ~UsbController();

    void init();                    
    bool is_device_connected();    
    void send_data(uint8_t channel, const uint8_t* data, uint32_t length);  
    uint32_t receive_data(uint8_t channel, uint8_t* buffer, uint32_t max_length); 
    void handle_interrupt();        
};

extern UsbController g_usb;

#endif // USB_H