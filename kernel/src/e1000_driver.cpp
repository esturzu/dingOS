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
 * - BCM2837 ARM Peripherals Datasheet (https://datasheets.raspberrypi.org/bcm2837/bcm2837-peripherals.pdf)
 *   Used for understanding PCIe configuration space access on ARM64.
 * - QEMU Source Code (hw/arm/raspi.c)
 *   Used for understanding how QEMU emulates PCI I/O ports on ARM.
 */

 #include "e1000_driver.h"
 #include "vmm.h"
 #include "heap.h"
 
 // Map kmalloc to your malloc (from heap.h)
 extern void* malloc(size_t size, size_t alignment);
 void* kmalloc(size_t size) {
     return malloc(size, 8); // 8-byte alignment for e1000 descriptors
 }
 
 // PCI Configuration Space Access for ARM64
 // In QEMU raspi3b, PCI config space is emulated via I/O ports (CF8/CFC) like x86.
 // On real hardware (Raspberry Pi 3B), PCI config space is accessed via MMIO (ECAM).
 // We define both implementations and use a macro to switch based on environment.
 
 #ifdef QEMU_EMULATION
 // QEMU: Use I/O port emulation (mimics x86 behavior)
 void outl(uint32_t port, uint32_t value) {
     // QEMU emulates I/O ports via MMIO at a specific address for raspi3b
     // From QEMU source (hw/arm/raspi.c), I/O ports are mapped to MMIO at 0x3F000000
     volatile uint32_t* io_base = reinterpret_cast<volatile uint32_t*>(0x3F000000);
     io_base[port / 4] = value;
 }
 
 uint32_t inl(uint32_t port) {
     volatile uint32_t* io_base = reinterpret_cast<volatile uint32_t*>(0x3F000000);
     return io_base[port / 4];
 }
 #else
 // Real Hardware: Use ECAM (Enhanced Configuration Access Mechanism) for PCIe
 // ECAM base address on Raspberry Pi 3B is typically at 0x3F000000 (BCM2837 PCIe)
 void outl(uint32_t port, uint32_t value) {
     // Port is the PCI config address (bus, device, function, register)
     // Map to ECAM: Address = ECAM_BASE + (bus << 20) + (device << 15) + (func << 12) + reg
     volatile uint32_t* ecam_base = reinterpret_cast<volatile uint32_t*>(0x3F000000);
     uint32_t bus = (port >> 16) & 0xFF;
     uint32_t dev = (port >> 11) & 0x1F;
     uint32_t func = (port >> 8) & 0x7;
     uint32_t reg = port & 0xFC;
     uint32_t offset = (bus << 20) | (dev << 15) | (func << 12) | reg;
     ecam_base[offset / 4] = value;
 }
 
 uint32_t inl(uint32_t port) {
     volatile uint32_t* ecam_base = reinterpret_cast<volatile uint32_t*>(0x3F000000);
     uint32_t bus = (port >> 16) & 0xFF;
     uint32_t dev = (port >> 11) & 0x1F;
     uint32_t func = (port >> 8) & 0x7;
     uint32_t reg = port & 0xFC;
     uint32_t offset = (bus << 20) | (dev << 15) | (func << 12) | reg;
     return ecam_base[offset / 4];
 }
 #endif
 
 E1000Driver::E1000Driver()
     : mmio_(nullptr), txRing_(nullptr), txIndex_(0), txBuffers_(nullptr) {
     device_ = {};
     for (size_t i = 0; i < MAC_SIZE; i++) {
         mac_[i] = 0;
     }
 }
 
 bool E1000Driver::initialize() {
     if (!findPciDevice()) {
         return false;
     }
 
     mmio_ = reinterpret_cast<volatile uint32_t*>(device_.bar0);
 
     // Reset the device
     mmio_[E1000_CTRL / 4] = 0x04000000; // Set RST bit
     for (volatile int i = 0; i < 10000; i++); // Delay
 
     // Read MAC address
     readMacAddress();
 
     // Initialize transmit descriptors
     initializeTransmit();
 
     return true;
 }
 
 bool E1000Driver::findPciDevice() {
     for (uint8_t bus = 0; bus < 256; bus++) {
         for (uint8_t dev = 0; dev < 32; dev++) {
             for (uint8_t func = 0; func < 8; func++) {
                 uint32_t address = (1 << 31) | (bus << 16) | (dev << 11) | (func << 8);
                 outl(PCI_CONFIG_ADDRESS, address);
                 uint32_t id = inl(PCI_CONFIG_DATA);
                 uint16_t vendor_id = id & 0xFFFF;
                 uint16_t device_id = (id >> 16) & 0xFFFF;
                 if (vendor_id == 0x8086 && device_id == 0x100E) { // e1000: Intel, 82540EM
                     device_.bus = bus;
                     device_.device = dev;
                     device_.func = func;
                     device_.vendor_id = vendor_id;
                     device_.device_id = device_id;
                     outl(PCI_CONFIG_ADDRESS, address | 0x10); // BAR0
                     device_.bar0 = inl(PCI_CONFIG_DATA) & ~0xF;
                     return true;
                 }
             }
         }
     }
     return false;
 }
 
 void E1000Driver::readMacAddress() {
     for (int i = 0; i < 3; i++) {
         mmio_[E1000_EERD / 4] = (1 << 0) | (i << 8); // Start read, address i
         while (!(mmio_[E1000_EERD / 4] & (1 << 4))); // Wait for done
         uint16_t data = mmio_[E1000_EERD / 4] >> 16;
         mac_[i * 2] = data & 0xFF;
         mac_[i * 2 + 1] = (data >> 8) & 0xFF;
     }
 }
 
 void E1000Driver::initializeTransmit() {
     // Allocate transmit descriptor ring
     txRing_ = reinterpret_cast<TxDesc*>(kmalloc(sizeof(TxDesc) * NUM_TX_DESCS));
     txBuffers_ = reinterpret_cast<uint8_t*>(kmalloc(NUM_TX_DESCS * TX_BUFFER_SIZE));
 
     if (txRing_ == nullptr || txBuffers_ == nullptr) {
         // Handle allocation failure (e.g., log error and halt)
         return;
     }
 
     for (size_t i = 0; i < NUM_TX_DESCS; i++) {
         txRing_[i].addr = reinterpret_cast<uint64_t>(txBuffers_ + i * TX_BUFFER_SIZE);
         txRing_[i].status = 0x01; // Descriptor Done
     }
 
     mmio_[E1000_TDBAL / 4] = static_cast<uint32_t>(reinterpret_cast<uint64_t>(txRing_));
     mmio_[E1000_TDLEN / 4] = NUM_TX_DESCS * sizeof(TxDesc);
     mmio_[E1000_TDH / 4] = 0;
     mmio_[E1000_TDT / 4] = 0;
     mmio_[E1000_TCTL / 4] = 0x0004000A; // Enable transmitter
 }
 
 void E1000Driver::sendPacket(const uint8_t* data, uint16_t length) {
     txRing_[txIndex_].addr = reinterpret_cast<uint64_t>(data);
     txRing_[txIndex_].length = length;
     txRing_[txIndex_].cmd = 0x09; // EOP | RS
     txRing_[txIndex_].status = 0;
     mmio_[E1000_TDT / 4] = (txIndex_ + 1) % NUM_TX_DESCS;
     txIndex_ = (txIndex_ + 1) % NUM_TX_DESCS;
 
     // Poll for completion (in a real driver, use interrupts)
     while (!(txRing_[txIndex_].status & 0x01));
 }