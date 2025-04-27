#ifndef LAN9118_DRIVER_H
#define LAN9118_DRIVER_H

#include <stdint.h>

class LAN9118Driver {
public:
    LAN9118Driver();
    bool init();
    void send_packet(const uint8_t* packet, uint32_t length);
    void get_mac_address(uint8_t* mac);

private:
    volatile uint32_t* regs;
    uint8_t mac_address[6];
};

#endif