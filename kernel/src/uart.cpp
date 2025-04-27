// Citations
// https://wiki.osdev.org/Raspberry_Pi_Bare_Bones
// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

#include "uart.h"
#include "definitions.h"
#include "gpio.h"
#include <stdint.h>

// Debug function to write directly to physical UART (before MMU is enabled)
static void debugPutChar(char c) {
    volatile uint32_t* uart = reinterpret_cast<volatile uint32_t*>(0x3F201000); // Physical address
    while (uart[0x18 / 4] & (1 << 5)); // Wait for UART to be ready (FR.TXFF)
    uart[0] = c;
}

static void debugPutString(const char* str) {
    while (*str) {
        debugPutChar(*str++);
    }
}

class UART {
  const uint64_t base_address;

 public:
  inline void transmit_data(char data) {
    volatile uint32_t* data_register =
        (volatile uint32_t*)(base_address + data_register_offset);
    *data_register = data;
  }

  inline void set_integer_braud(uint16_t integer_braud) {
    volatile uint32_t* integer_braud_register =
        (volatile uint32_t*)(base_address + integer_braud_offset);
    *integer_braud_register = integer_braud;
  }

  inline void set_fractional_braud(uint8_t fractional_braud) {
    volatile uint32_t* fractional_braud_register =
        (volatile uint32_t*)(base_address + fractional_braud_offset);
    *fractional_braud_register = fractional_braud;
  }

  inline void set_line_control(uint8_t line_control) {
    volatile uint32_t* line_control_register =
        (volatile uint32_t*)(base_address + line_control_offset);
    *line_control_register = line_control;
  }

  inline void set_control(uint16_t control) {
    volatile uint32_t* control_register =
        (volatile uint32_t*)(base_address + control_offset);
    *control_register = control;
  }

  inline void clear_interrupts() {
    volatile uint32_t* interrupt_clear_register =
        (volatile uint32_t*)(base_address + interrupt_clear_offset);
    *interrupt_clear_register = 0b11111111111;
  }

  inline void set_interrupt_mask(uint16_t mask) {
    volatile uint32_t* interrupt_mask_set_clear_register =
        (volatile uint32_t*)(base_address + interrupt_mask_offset);
    *interrupt_mask_set_clear_register = mask;
  }

  inline bool transmit_fifo_full() {
    volatile uint32_t* flag_register =
        (volatile uint32_t*)(base_address + flag_register_offset);
    return (*flag_register) & 0b100000;
  }

  static constexpr uint64_t data_register_offset = 0x00;
  static constexpr uint64_t flag_register_offset = 0x18;
  static constexpr uint64_t integer_braud_offset = 0x24;
  static constexpr uint64_t fractional_braud_offset = 0x28;
  static constexpr uint64_t line_control_offset = 0x2c;
  static constexpr uint64_t control_offset = 0x30;
  static constexpr uint64_t interrupt_mask_offset = 0x38;
  static constexpr uint64_t interrupt_clear_offset = 0x44;

  UART(uint64_t base_address) : base_address(base_address) {
    debugPutString("UART: Disabling UART\n");
    set_control(0);

    debugPutString("UART: Setting GPIO pull\n");
    GPIO::set_pull_register(GPIO::PUD::OFF);

    debugPutString("UART: Setting GPIO clock 1\n");
    GPIO::set_clock(0, 0b100000000000000);

    debugPutString("UART: Setting GPIO clock 2\n");
    GPIO::set_clock(0, 0);

    debugPutString("UART: Clearing interrupts\n");
    clear_interrupts();

    debugPutString("UART: Setting integer baud\n");
    set_integer_braud(1);

    debugPutString("UART: Setting fractional baud\n");
    set_fractional_braud(40);

    debugPutString("UART: Setting line control\n");
    set_line_control(0b1110000);

    debugPutString("UART: Setting interrupt mask\n");
    set_interrupt_mask(0b11111110010);

    debugPutString("UART: Enabling UART\n");
    set_control(0b1100000001);
  }

  ~UART() {}
};

// Global UART pointer (null until initialized)
static UART* uart0 = nullptr;

void uart_init(uint64_t base_address) {
    debugPutString("UART: Initializing\n");
    uart0 = new UART(base_address);
}

extern "C" void uart_putc(char c) {
    if (!uart0) {
        return;
    }
    while (uart0->transmit_fifo_full()) {
    }
    uart0->transmit_data(c);
}