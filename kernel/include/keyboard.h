#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "atomics.h"
#include "usb.h"
#include "stdint.h"

#define HID_SET_PROTOCOL 0x0B
#define HID_BOOT_PROTOCOL 0x00

#define LEFT_CTRL 0
#define LEFT_SHIFT 1
#define LEFT_ALT 2
#define LEFT_GUI 3
#define RIGHT_CTRL 4
#define RIGHT_SHIFT 5
#define RIGHT_ALT 6
#define RIGHT_GUI 7



const uint8_t kbd_device_descriptor[18] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (Device)
    0x00, 0x02, // bcdUSB 2.00
    0x00,       // bDeviceClass (Defined in interface)
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol
    0x08,       // bMaxPacketSize0
    0x34, 0x12, // idVendor (0x1234 - dummy vendor)
    0x78, 0x56, // idProduct (0x5678 - dummy product)
    0x00, 0x01, // bcdDevice 1.00
    0x01,       // iManufacturer
    0x02,       // iProduct
    0x00,       // iSerialNumber
    0x01        // bNumConfigurations
};

const uint8_t kbd_config_descriptor[34] = {
    // --- Configuration Descriptor ---
    0x09,       // bLength
    0x02,       // bDescriptorType (Configuration)
    0x22, 0x00, // wTotalLength (34 bytes)
    0x01,       // bNumInterfaces
    0x01,       // bConfigurationValue
    0x00,       // iConfiguration
    0xA0,       // bmAttributes (Bus-powered, Remote Wakeup)
    0x32,       // bMaxPower (100mA)

    // --- Interface Descriptor ---
    0x09,       // bLength
    0x04,       // bDescriptorType (Interface)
    0x00,       // bInterfaceNumber
    0x00,       // bAlternateSetting
    0x01,       // bNumEndpoints
    0x03,       // bInterfaceClass (HID)
    0x01,       // bInterfaceSubClass (Boot)
    0x01,       // bInterfaceProtocol (Keyboard)
    0x00,       // iInterface

    // --- HID Descriptor ---
    0x09,       // bLength
    0x21,       // bDescriptorType (HID)
    0x11, 0x01, // bcdHID 1.11
    0x00,       // bCountryCode
    0x01,       // bNumDescriptors
    0x22,       // bDescriptorType (Report)
    0x3F, 0x00, // wDescriptorLength (63 bytes)

    // --- Endpoint Descriptor ---
    0x07,       // bLength
    0x05,       // bDescriptorType (Endpoint)
    0x81,       // bEndpointAddress (IN endpoint 1)
    0x03,       // bmAttributes (Interrupt)
    0x08, 0x00, // wMaxPacketSize
    0x0A        // bInterval (10ms)
};

typedef struct {
    uint8_t keycode;
    char unshifted;
    char shifted;
} KeyMapping;

// Only map the keycodes you care about
static const KeyMapping key_mappings[] = {
    {0x00, 'e', 'e'},       // Added several dummy values to avoid indexing issues.
    {0x01, 'e', 'e'},       // 0x00 and 0x01 are used for None and Phantom Key respectively
    {0x02, 'e', 'e'},
    {0x03, 'e', 'e'},
    {0x04, 'a', 'A'},
    {0x05, 'b', 'B'},
    {0x06, 'c', 'C'},
    {0x07, 'd', 'D'},
    {0x08, 'e', 'E'},
    {0x09, 'f', 'F'},
    {0x0A, 'g', 'G'},
    {0x0B, 'h', 'H'},
    {0x0C, 'i', 'I'},
    {0x0D, 'j', 'J'},
    {0x0E, 'k', 'K'},
    {0x0F, 'l', 'L'},
    {0x10, 'm', 'M'},
    {0x11, 'n', 'N'},
    {0x12, 'o', 'O'},
    {0x13, 'p', 'P'},
    {0x14, 'q', 'Q'},
    {0x15, 'r', 'R'},
    {0x16, 's', 'S'},
    {0x17, 't', 'T'},
    {0x18, 'u', 'U'},
    {0x19, 'v', 'V'},
    {0x1A, 'w', 'W'},
    {0x1B, 'x', 'X'},
    {0x1C, 'y', 'Y'},
    {0x1D, 'z', 'Z'},
    {0x1E, '1', '!'},
    {0x1F, '2', '@'},
    {0x20, '3', '#'},
    {0x21, '4', '$'},
    {0x22, '5', '%'},
    {0x23, '6', '^'},
    {0x24, '7', '&'},
    {0x25, '8', '*'},
    {0x26, '9', '('},
    {0x27, '0', ')'},
    {0x28, '\n', '\n'}, // Enter
    {0x29,  27,  27},
    {0x2A, '\b', '\b'},
    {0x2B, '\t', '\t'},
    {0x2C, ' ', ' '},   // Space
    {0x2D, '-', '_'},
    {0x2E, '=', '+'},
    {0x2F, '[', '{'},
    {0x30, ']', '}'},
    {0x31, '\\', '|'},
    {0x00, 'e', 'e'},
    {0x33, ';', ':'},
    {0x34, '\'', '"'},
    {0x35, '`', '~'},
    {0x36, ',', '<'},
    {0x37, '.', '>'},
    {0x38, '/', '?'},
    // Add more mappings if you want...
};

struct keyboard_report {
    uint8_t modifier_keys;
    uint8_t reserved_byte;
    uint8_t keys[6];
};

class Keyboard {
private:
    UsbController* usb;
    uint8_t device_descriptor[18];
    uint8_t config_descriptor[34];
    uint8_t report_buffer[8];
    uint8_t prev_keys[6];
public:

    Keyboard() {
        for (int i = 0; i < 18; i++) {
            device_descriptor[i] = kbd_device_descriptor[i];
        }
        for (int i = 0; i < 34; i++) {
            config_descriptor[i] = kbd_config_descriptor[i];
        }
        usb = new UsbController(device_descriptor, config_descriptor);
        usb->init(report_buffer);

        uint8_t ep_num = (kbd_config_descriptor[29]) & 0x0F;
        uint8_t ep_type = (kbd_config_descriptor[30]) & 0x03;
        uint16_t max_packet_size = (kbd_config_descriptor[31]);
    
        usb->setup_endpoint(0, ep_num, ep_type, max_packet_size, 1, report_buffer);
    }

    ~Keyboard();
    uint64_t receive_keyboard_input();
    void parse_input();
    void keyboard_loop();
};

extern bool keyboard_state[256];

extern Keyboard* kbd;

#endif