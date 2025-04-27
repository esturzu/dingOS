#include "keyboard.h"

Keyboard* kbd;

bool keyboard_state[256];

Keyboard::~Keyboard() {}

uint64_t Keyboard::receive_keyboard_input() {
    if (!usb->is_device_connected()) {
        return '\0';
    }

    uint8_t ep_num = (kbd_config_descriptor[29]) & 0x0F;
    uint8_t ep_type = (kbd_config_descriptor[30]) & 0x03;
    uint16_t max_packet_size = (kbd_config_descriptor[31]);

    usb->setup_endpoint(0, ep_num, ep_type, max_packet_size, 1, report_buffer);

    uint64_t timeout = 1000000;  // Arbitrary timeout value
    while (usb->handle_interrupt() == -1 && timeout > 0) {
        timeout--;
        // Optionally add a small delay here to avoid busy-waiting
    }

    if (timeout == 0) {
        // Handle timeout, maybe return an error or retry
        printf("Error: Timeout waiting for transfer completion\n");
        return '\0';  // or some other error handling mechanism
    }

    return *((uint64_t*) report_buffer);
}

void Keyboard::parse_input() {
    uint8_t modifiers = report_buffer[0] & 0xFF;
    for (int i = 2; i < 8; i++) {
        if (report_buffer[i] == 0x00 || report_buffer[i] == 0x01) {
            break;
        }
        
        uint8_t keycode = report_buffer[i];
        if (!keyboard_state[keycode]) {
            keyboard_state[keycode] = true; // Key is pressed
            bool shift = modifiers & (1 << 1) || modifiers & (1 << 5); // LShift or RShift
            char c = shift ? key_mappings[keycode].shifted : key_mappings[keycode].unshifted; // Get actual value
            printf("Key %c has been pressed\n", c);
        }
        else {
            if (prev_keys[0] != 0x00) {
                bool found = false;
                for (int j = 0; j < 6; j++) {
                    if (keycode == prev_keys[j]) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    keyboard_state[keycode] = false;
                    printf("Key %c has been released\n", key_mappings[keycode].unshifted);
                }
            }
        }
    }
    for (int i = 0; i < 6; i++) {
        prev_keys[i] = report_buffer[i + 2];
        report_buffer[i + 2] = 0;
    }
}

void Keyboard::keyboard_loop() {
    for (int i = 0; i < 256; i++) {
        keyboard_state[i] = false;
    }

    while (true) {
        uint64_t report = receive_keyboard_input();
        if (report != 0x00) {
            parse_input();
        }
    }
}

