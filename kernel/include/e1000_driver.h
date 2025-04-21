/*
 * Citations:
 * - Intel 82540EM Gigabit Ethernet Controller Datasheet (https://www.intel.com/content/dam/www/public/us/en/documents/datasheets/82540em-gigabit-ethernet-controller-datasheet.pdf)
 *   Used for register definitions (e.g., E1000_CTRL, E1000_TDBAL) and initialization steps.
 * - OSDev Wiki: PCI (https://wiki.osdev.org/PCI)
 *   Used for PCI configuration space access details (CONFIG_ADDRESS, CONFIG_DATA).
 * - OSDev Wiki: e1000 (https://wiki.osdev.org/Intel_Ethernet_i217)
 *   Used for e1000 initialization steps and descriptor ring setup.
 * - "PCI System Architecture" by Tom Shanley and Don Anderson (4th Edition)
 *   Used for understanding PCI bus enumeration and configuration.
 */

 #pragma once

 #include <stdint.h>
 
 // I/O port access (implemented in e1000_driver.cpp)
 extern void outl(uint32_t port, uint32_t value);
 extern uint32_t inl(uint32_t port);
 
 // Memory allocation (maps to your malloc)
 extern void* kmalloc(size_t size);
 
 class E1000Driver {
 public:
     struct PciDevice {
         uint8_t bus;
         uint8_t device;
         uint8_t func;
         uint16_t vendor_id;
         uint16_t device_id;
         uint32_t bar0; // MMIO base address
     };
 
     struct TxDesc {
         uint64_t addr;   // Buffer address
         uint16_t length; // Length of data
         uint8_t cso;
         uint8_t cmd;
         uint8_t status;
         uint8_t css;
         uint16_t special;
     };
 
     E1000Driver();
     ~E1000Driver() = default;
 
     // Find and initialize the e1000 NIC
     bool initialize();
 
     // Get the PCI device information
     const PciDevice& getPciDevice() const { return device_; }
 
     // Get the MAC address
     const uint8_t* getMacAddress() const { return mac_; }
 
     // Send a packet
     void sendPacket(const uint8_t* data, uint16_t length);
 
 private:
     static constexpr uint32_t PCI_CONFIG_ADDRESS = 0xCF8;
     static constexpr uint32_t PCI_CONFIG_DATA = 0xCFC;
 
     // e1000 Registers
     static constexpr uint32_t E1000_CTRL = 0x0000;  // Control Register
     static constexpr uint32_t E1000_STATUS = 0x0008; // Status Register
     static constexpr uint32_t E1000_EERD = 0x0014;  // EEPROM Read
     static constexpr uint32_t E1000_TCTL = 0x0400;  // Transmit Control
     static constexpr uint32_t E1000_TDBAL = 0x3800; // Transmit Descriptor Base Address Low
     static constexpr uint32_t E1000_TDLEN = 0x3808; // Transmit Descriptor Length
     static constexpr uint32_t E1000_TDH = 0x3810;   // Transmit Descriptor Head
     static constexpr uint32_t E1000_TDT = 0x3818;   // Transmit Descriptor Tail
 
     static constexpr size_t NUM_TX_DESCS = 8;
     static constexpr size_t TX_BUFFER_SIZE = 2048;
     static constexpr size_t MAC_SIZE = 6;
 
     bool findPciDevice();
     void readMacAddress();
     void initializeTransmit();
 
     PciDevice device_;
     volatile uint32_t* mmio_;
     uint8_t mac_[MAC_SIZE];
     TxDesc* txRing_;
     int txIndex_;
     uint8_t* txBuffers_;
 };