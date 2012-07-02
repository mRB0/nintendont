#include <stdint.h>

#include "simpleplayer.h"
#include "ports.h"

#include "vrc6brat.h"

#define ADDR_PORT PORTA
#define ADDR_PORT_DDR DDRA

#define DATA_PORT PORTC
#define DATA_PORT_DDR DDRC

static uint8_t const pin_WE = 44;
static uint8_t const pin_CE = 38;

uint8_t const VRC6AddrBase = 0x0; // 0x8000 on NES
uint8_t const VRC6AddrChanShift = 2;
uint8_t const VRC6Pulse1Base = VRC6AddrBase | (1 << VRC6AddrChanShift); // 0x9000 on NES
uint8_t const VRC6Pulse2Base = VRC6AddrBase | (2 << VRC6AddrChanShift); // 0xa000 on NES
uint8_t const VRC6SawBase = VRC6AddrBase | (3 << VRC6AddrChanShift); // 0xb000 on NES

//static int const pinVRC6OSCEN = 27;

void writeAddr(uint8_t addr) {
    ADDR_PORT = addr;
}

void writeData(uint8_t value) {
    DATA_PORT = value;
}

void port_write(uint8_t addr, uint8_t data) {
    writeAddr(addr);
    DATA_PORT_DDR = 0xff;
    writeData(data);
    digitalWrite(pin_WE, 1);
    digitalWrite(pin_CE, 0);
    digitalWrite(pin_WE, 0);

    // delay
    delayMicroseconds(250);
    
    digitalWrite(pin_WE, 1);
    digitalWrite(pin_CE, 1);
}

void setup() {
//    digitalWrite(pinVRC6OSCEN, 0);
//    pinMode(pinVRC6OSCEN, OUTPUT);
//    digitalWrite(pinVRC6OSCEN, 0);
    
    pinMode(pin_CE, OUTPUT);
    pinMode(pin_WE, OUTPUT);
    digitalWrite(pin_CE, 1);
    digitalWrite(pin_WE, 1);

    ADDR_PORT_DDR = 0xff;
    writeAddr(0);
    writeData(0);
    
    // init vrc6
    
    port_write(VRC6AddrBase | (0x1 << VRC6AddrChanShift) | 0x03, 0x00); // 0x9003

	port_write(VRC6AddrBase, 0x00); // 0x8000
	port_write(VRC6AddrBase | (0x4 << VRC6AddrChanShift), 0x00); // 0xc000

	port_write(VRC6SawBase | 0x3, 0x08); // 0xa003
	port_write(VRC6AddrBase | (0x7 << VRC6AddrChanShift) | 0x00, 0x00); // 0xf000
	port_write(VRC6AddrBase | (0x7 << VRC6AddrChanShift) | 0x01, 0x00); // 0xf001
	port_write(VRC6AddrBase | (0x7 << VRC6AddrChanShift) | 0x02, 0x00); // 0xf002

    // init vrc6 sound

	port_write(VRC6Pulse1Base | 0x0, 0x7f);
	port_write(VRC6Pulse1Base | 0x1, 0x00);
	port_write(VRC6Pulse1Base | 0x2, 0x00);

	port_write(VRC6Pulse2Base | 0x0, 0x7f);
	port_write(VRC6Pulse2Base | 0x1, 0x00);
	port_write(VRC6Pulse2Base | 0x2, 0x00);

	port_write(VRC6SawBase | 0x0, 0x3f);
	port_write(VRC6SawBase | 0x1, 0x00);
	port_write(VRC6SawBase | 0x2, 0x00);

//    digitalWrite(pinVRC6OSCEN, 1);
}

void loop(void) {
    simpleplayer();
    //funplayer();
}
