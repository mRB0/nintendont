#include <stdint.h>

#include "simpleplayer.h"
#include "ports.h"

static int const pinD0 = 30;
static int const pinD1 = 31;
static int const pinD2 = 32;
static int const pinD3 = 33;
static int const pinD4 = 34;
static int const pinD5 = 35;
static int const pinD6 = 36;
static int const pinD7 = 37;

static int const pinA0 = 22;
static int const pinA1 = 23;
static int const pinA12 = 24;
static int const pinA13 = 25;
static int const pinA14 = 26;

static int const pin_WE = 42;
static int const pin_CE = 43;

static int const pinVRC6OSCEN = 27;

void writeAddr(uint16_t addr) {
    digitalWrite(pinA0, (addr >> 0) & 0x01);
    digitalWrite(pinA1, (addr >> 1) & 0x01);
    digitalWrite(pinA12, (addr >> 12) & 0x01);
    digitalWrite(pinA13, (addr >> 13) & 0x01);
    digitalWrite(pinA14, (addr >> 14) & 0x01);
}

void writeData(uint8_t value) {
    digitalWrite(pinD0, (value >> 0) & 0x01);
    digitalWrite(pinD1, (value >> 1) & 0x01);
    digitalWrite(pinD2, (value >> 2) & 0x01);
    digitalWrite(pinD3, (value >> 3) & 0x01);
    digitalWrite(pinD4, (value >> 4) & 0x01);
    digitalWrite(pinD5, (value >> 5) & 0x01);
    digitalWrite(pinD6, (value >> 6) & 0x01);
    digitalWrite(pinD7, (value >> 7) & 0x01);
}

void port_write(uint16_t addr, uint8_t data) {
    writeAddr(addr);
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
    digitalWrite(pinVRC6OSCEN, 0);
    pinMode(pinVRC6OSCEN, OUTPUT);
    digitalWrite(pinVRC6OSCEN, 0);
    
    pinMode(pin_CE, OUTPUT);
    pinMode(pin_WE, OUTPUT);
    digitalWrite(pin_CE, 1);
    digitalWrite(pin_WE, 1);
    
    writeAddr(0);
    writeData(0);
    
    pinMode(pinD0, OUTPUT);
    pinMode(pinD1, OUTPUT);
    pinMode(pinD2, OUTPUT);
    pinMode(pinD3, OUTPUT);
    pinMode(pinD4, OUTPUT);
    pinMode(pinD5, OUTPUT);
    pinMode(pinD6, OUTPUT);
    pinMode(pinD7, OUTPUT);
    pinMode(pinA0, OUTPUT);
    pinMode(pinA1, OUTPUT);
    pinMode(pinA12, OUTPUT);
    pinMode(pinA13, OUTPUT);
    pinMode(pinA14, OUTPUT);
    pinMode(pin_CE, OUTPUT);
    pinMode(pin_WE, OUTPUT);

    // init vrc6
    
    port_write(0x9003, 0x00);

	port_write(0x8000, 0x00);
	port_write(0xc000, 0x00);

	port_write(0xb003, 0x08);
	port_write(0xf000, 0x00);
	port_write(0xf001, 0x00);
	port_write(0xf002, 0x00);

    // init vrc6 sound

	port_write(0x9000, 0x7f);
	port_write(0x9001, 0x00);
	port_write(0x9002, 0x00);

	port_write(0xa000, 0x7f);
	port_write(0xa001, 0x00);
	port_write(0xa002, 0x00);

	port_write(0xb000, 0x3f);
	port_write(0xb001, 0x00);
	port_write(0xb002, 0x00);

    digitalWrite(pinVRC6OSCEN, 1);
}

void funplayer(void);

void loop(void) {
    simpleplayer();
    //funplayer();
}

void funplayer(void) {
    uint8_t duty = 0;
    
    for(;;) {
        //duty = (duty + 1) % 0x8;
        duty = 7;
        
        port_write(0x9000, 0x0f | (duty << 4));
        //port_write(0xb000, 0x0f | (duty << 4));

        // set pitches

        //port_write(0xb001, 0xa7);
        //port_write(0xb002, 0x83);
        port_write(0x9001, 0xe7);
        port_write(0x9002, 0x82);

        delay(100);

        //port_write(0xb001, 0xe7);
        //port_write(0xb002, 0x82);
        port_write(0x9001, 0x72);
        port_write(0x9002, 0x82);

        delay(100);

        //port_write(0xb001, 0x72);
        //port_write(0xb002, 0x82);
        port_write(0x9001, 0xd6);
        port_write(0x9002, 0x81);
        
        delay(100);

    }
}
