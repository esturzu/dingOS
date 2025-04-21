/*
 * Citations:
 * - RFC 826: An Ethernet Address Resolution Protocol (https://datatracker.ietf.org/doc/html/rfc826)
 *   Used for ARP packet structure and protocol details.
 * - OSDev Wiki: e1000 (https://wiki.osdev.org/Intel_Ethernet_i217)
 *   Used for e1000 packet transmission details.
 * - "TCP/IP Illustrated, Volume 1" by W. Richard Stevens
 *   Used for understanding ARP and Ethernet frame formats.
 */

 #include "e1000_driver.h"
 #include <stdint.h>
 #include <printf.h>
 
 // Define memset and memcpy since we don't have a standard library
 void* memset(void* ptr, int value, size_t num) {
     uint8_t* p = reinterpret_cast<uint8_t*>(ptr);
     for (size_t i = 0; i < num; i++) {
         p[i] = static_cast<uint8_t>(value);
     }
     return ptr;
 }
 
 void* memcpy(void* dest, const void* src, size_t num) {
     uint8_t* d = reinterpret_cast<uint8_t*>(dest);
     const uint8_t* s = reinterpret_cast<const uint8_t*>(src);
     for (size_t i = 0; i < num; i++) {
         d[i] = s[i];
     }
     return dest;
 }
 
 // Serial output (assumes your OS has this, adapted for UART ttyAMA0)
 
//  }
 
//  void serialPrintHex(uint8_t val) {
//      const char hex[] = "0123456789ABCDEF";
//      serialPrint("0x");
//      char buf[2] = { hex[(val >> 4) & 0xF], hex[val & 0xF] };
//      serialPrint(buf);
//  }
 
 // Network definitions
 constexpr uint16_t ETH_TYPE_ARP = 0x0806;
 constexpr uint16_t ARP_HTYPE_ETHER = 0x0001;
 constexpr uint16_t ARP_PTYPE_IP = 0x0800;
 constexpr uint16_t ARP_OP_REQUEST = 0x0001;
 constexpr size_t MAC_SIZE = 6;
 
 struct EthernetFrame {
     uint8_t dstMac[6];
     uint8_t srcMac[6];
     uint16_t etherType;
     uint8_t payload[];
 } __attribute__((packed));
 
 struct ArpPacket {
     uint16_t hwType;
     uint16_t protoType;
     uint8_t hwLen;
     uint8_t protoLen;
     uint16_t opcode;
     uint8_t srcMac[6];
     uint32_t srcIp;
     uint8_t dstMac[6];
     uint32_t dstIp;
 } __attribute__((packed));
 
 void testNetwork() {
     E1000Driver driver;
     if (!driver.initialize()) {
        printf("Failed to initialize e1000 driver\n");
         return;
     }
 
     const auto& device = driver.getPciDevice();
     printf("Found e1000 at bus %x, device %x, func %x\n", device.bus, device.device, device.func);
 
     const uint8_t* mac = driver.getMacAddress();
     printf("e1000 MAC: ");
    for (size_t i = 0; i < MAC_SIZE; i++) {
        printf("%x", mac[i]);
        if (i < MAC_SIZE - 1) printf(":");
    }
    printf("\n");
 
     // Send ARP request for 10.0.2.2 (QEMU user-mode gateway)
     uint8_t packet[1500];
     EthernetFrame* frame = reinterpret_cast<EthernetFrame*>(packet);
     ArpPacket* arp = reinterpret_cast<ArpPacket*>(frame->payload);
 
     // Ethernet header
     memset(frame->dstMac, 0xFF, 6); // Broadcast
     memcpy(frame->srcMac, mac, 6);
     frame->etherType = ETH_TYPE_ARP;
 
     // ARP packet
     arp->hwType = ARP_HTYPE_ETHER;
     arp->protoType = ARP_PTYPE_IP;
     arp->hwLen = 6;
     arp->protoLen = 4;
     arp->opcode = ARP_OP_REQUEST;
     memcpy(arp->srcMac, mac, 6);
     arp->srcIp = 0x0A00020F; // 10.0.2.15 (QEMU guest IP)
     memset(arp->dstMac, 0, 6);
     arp->dstIp = 0x0A000202; // 10.0.2.2 (QEMU gateway)
 
     printf("Sending ARP request for 10.0.2.2\n");
     driver.sendPacket(packet, sizeof(EthernetFrame) + sizeof(ArpPacket));
     printf("ARP request sent\n");
 }