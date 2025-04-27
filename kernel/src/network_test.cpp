#include "network_test.h"
#include "lan9118_driver.h"
#include "printf.h"

// ARP packet structure
struct ArpPacket {
    uint16_t htype;    // Hardware type (Ethernet = 1)
    uint16_t ptype;    // Protocol type (IPv4 = 0x0800)
    uint8_t hlen;      // Hardware address length (6 for Ethernet)
    uint8_t plen;      // Protocol address length (4 for IPv4)
    uint16_t oper;     // Operation (1 for request)
    uint8_t sha[6];    // Sender hardware address (MAC)
    uint32_t spa;      // Sender protocol address (IP)
    uint8_t tha[6];    // Target hardware address (MAC, 0 for request)
    uint32_t tpa;      // Target protocol address (IP)
} __attribute__((packed));

// Ethernet frame structure
struct EthernetFrame {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype; // 0x0806 for ARP
    ArpPacket arp;
} __attribute__((packed));

extern "C" void testNetwork() {
    printf("Starting network test\n");

    // Initialize the LAN9118 driver
    LAN9118Driver lan9118;
    if (!lan9118.init()) {
        printf("Failed to initialize LAN9118\n");
        return;
    }

    // Get the MAC address
    uint8_t mac[6];
    lan9118.get_mac_address(mac);
    printf("LAN9118 MAC: 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Construct an ARP request for 192.168.1.100
    EthernetFrame frame;
    for (int i = 0; i < 6; i++) {
        frame.dst_mac[i] = 0xFF; // Broadcast
        frame.src_mac[i] = mac[i];
        frame.arp.sha[i] = mac[i];
        frame.arp.tha[i] = 0x00;
    }
    frame.ethertype = 0x0806; // ARP
    frame.arp.htype = 1;      // Ethernet
    frame.arp.ptype = 0x0800; // IPv4
    frame.arp.hlen = 6;
    frame.arp.plen = 4;
    frame.arp.oper = 1;       // Request
    frame.arp.spa = 0xC0A80001; // 192.168.0.1 (sender IP)
    frame.arp.tpa = 0xC0A80164; // 192.168.1.100 (target IP)

    printf("Sending ARP request for 192.168.1.100\n");
    lan9118.send_packet((uint8_t*)&frame, sizeof(frame));

    printf("ARP request sent\n");

    // Halt the kernel
    printf("Kernel halting\n");
    while (true) {}
}