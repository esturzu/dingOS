#include "lan9118_driver.h"
#include "definitions.h"
#include "printf.h"

// LAN9118 Register Offsets (simplified subset)
#define LAN9118_ID_REV      0x50   // ID and Revision
#define LAN9118_IRQ_CFG     0x54   // Interrupt Configuration
#define LAN9118_INT_STS     0x58   // Interrupt Status
#define LAN9118_INT_EN      0x5C   // Interrupt Enable
#define LAN9118_MAC_CSR_CMD 0xA4   // MAC CSR Command
#define LAN9118_MAC_CSR_DATA 0xA8  // MAC CSR Data
#define LAN9118_TX_DATA     0x20   // TX Data FIFO
#define LAN9118_TX_STATUS   0x30   // TX Status FIFO
#define LAN9118_HW_CFG      0x74   // Hardware Configuration

// MAC CSR Command Bits
#define MAC_CSR_CMD_BUSY    (1U << 31)
#define MAC_CSR_CMD_READ    (1U << 30)

// MAC CSR Registers
#define MAC_CSR_MAC_ADDR    0x01   // MAC Address Low (bytes 0-3)
#define MAC_CSR_MAC_ADDRH   0x02   // MAC Address High (bytes 4-5)

// Interrupt Bits
#define INT_STS_TX          (1U << 9)  // TX Complete Interrupt

LAN9118Driver::LAN9118Driver() : regs((volatile uint32_t*)LAN9118_BASE)
{
    // Hardcode a MAC address for simplicity (same as e1000 driver)
    mac_address[0] = 0x52;
    mac_address[1] = 0x54;
    mac_address[2] = 0x00;
    mac_address[3] = 0x12;
    mac_address[4] = 0x34;
    mac_address[5] = 0x56;
}

bool LAN9118Driver::init()
{
    // Check the ID and Revision to confirm the device is present
    uint32_t id_rev = regs[LAN9118_ID_REV / 4];
    if ((id_rev & 0xFFFF) != 0x0118) {  // LAN9118 ID
        printf("LAN9118: Invalid ID/Revision 0x%x\n", id_rev);
        return false;
    }
    printf("Found LAN9118, ID/Revision: 0x%x\n", id_rev);

    // Reset the device (soft reset via HW_CFG)
    regs[LAN9118_HW_CFG / 4] = 0x00050000;  // SRST bit
    for (volatile int i = 0; i < 100000; i++);  // Wait for reset

    // Clear any pending interrupts
    regs[LAN9118_INT_STS / 4] = 0xFFFFFFFF;

    // Enable TX interrupts
    regs[LAN9118_INT_EN / 4] = INT_STS_TX;
    regs[LAN9118_IRQ_CFG / 4] = 0x00000100;  // Enable IRQ

    // Set the MAC address
    uint32_t mac_low = (mac_address[0] << 0) | (mac_address[1] << 8) |
                       (mac_address[2] << 16) | (mac_address[3] << 24);
    uint32_t mac_high = (mac_address[4] << 0) | (mac_address[5] << 8);

    // Write MAC address to MAC CSR registers
    while (regs[LAN9118_MAC_CSR_CMD / 4] & MAC_CSR_CMD_BUSY);
    regs[LAN9118_MAC_CSR_DATA / 4] = mac_low;
    regs[LAN9118_MAC_CSR_CMD / 4] = MAC_CSR_CMD_BUSY | (MAC_CSR_MAC_ADDR << 8);

    while (regs[LAN9118_MAC_CSR_CMD / 4] & MAC_CSR_CMD_BUSY);
    regs[LAN9118_MAC_CSR_DATA / 4] = mac_high;
    regs[LAN9118_MAC_CSR_CMD / 4] = MAC_CSR_CMD_BUSY | (MAC_CSR_MAC_ADDRH << 8);

    return true;
}

void LAN9118Driver::send_packet(const uint8_t* packet, uint32_t length)
{
    // Write packet to TX Data FIFO
    uint32_t* packet_words = (uint32_t*)packet;
    uint32_t word_count = (length + 3) / 4;  // Round up to nearest word

    // TX Command A: Packet length and tag
    regs[LAN9118_TX_DATA / 4] = (length << 16) | (0x0001);  // Tag 0x0001
    // TX Command B: Start packet
    regs[LAN9118_TX_DATA / 4] = 0x00000000;

    // Write packet data
    for (uint32_t i = 0; i < word_count; i++) {
        regs[LAN9118_TX_DATA / 4] = packet_words[i];
    }

    // Wait for transmission to complete (or timeout)
    uint64_t timeout = 1000000;
    while (timeout--) {
        uint32_t int_status = regs[LAN9118_INT_STS / 4];
        if (int_status & INT_STS_TX) {
            regs[LAN9118_INT_STS / 4] = INT_STS_TX;  // Clear interrupt
            uint32_t tx_status = regs[LAN9118_TX_STATUS / 4];  // Read status
            if (tx_status & 0x8000) {  // Error bit
                printf("LAN9118: TX error, status 0x%x\n", tx_status);
            }
            return;
        }
    }

    // Timeout occurred (expected with -net none)
    printf("Warning: Packet transmission timed out (no network backend?)\n");
}

void LAN9118Driver::get_mac_address(uint8_t* mac)
{
    for (int i = 0; i < 6; i++) {
        mac[i] = mac_address[i];
    }
}