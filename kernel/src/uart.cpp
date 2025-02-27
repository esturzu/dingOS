// Citations
// https://wiki.osdev.org/Raspberry_Pi_Bare_Bones
// https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

#include "uart.h"

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

  inline bool receive_fifo_full() {
    volatile uint32_t* flag_register =
        (volatile uint32_t*)(base_address + flag_register_offset);
    return (*flag_register) & 1 << 6;
  }

  inline bool receive_fifo_empty() {
    volatile uint32_t* flag_register =
        (volatile uint32_t*)(base_address + flag_register_offset);
    return (*flag_register) & 1 << 4;
  }

  inline char receive_data() {
    volatile uint32_t* data_register =
        (volatile uint32_t*)(base_address + data_register_offset);
    return *data_register;
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
    set_control(0);

    // Enable GPIO-14 (UART 0 Transmit)
    GPIO::set_pull_register(GPIO::PUD::OFF);

    GPIO::set_clock(0, 0b100000000000000); //  1 << 14

    GPIO::set_clock(0, 0);

    // Enable GPIO-15 (UART 0 Receive)
    GPIO::set_pull_register(GPIO::PUD::OFF);
    GPIO::set_clock(0, 1 << 15); // 1 << 15
    GPIO::set_clock(0, 0);

    clear_interrupts();

    set_integer_braud(1);

    set_fractional_braud(40);

    set_line_control(0b1110000);

    set_interrupt_mask(0b11111110010);

    set_control(0b1100000001);
  };

  ~UART() {}
};

UART uart0{UART0_BASE};

extern "C" void uart_putc(char c) {
  while (uart0.transmit_fifo_full()) {
  }
  uart0.transmit_data(c);
}

extern "C" char uart_getc(){
  char c = 0;
  if(!uart0.receive_fifo_empty()) {
    c = uart0.receive_data();
  }
  return c;
}


extern bool uart_hasInput() {
  return !uart0.receive_fifo_empty();
}