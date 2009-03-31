#ifndef PORTS_H_
#define PORTS_H_

#include "stdint.h"

#ifndef __PLAYERDEV__

#define TRIS_VRC6_CE	TRISEbits.TRISE2
#define LAT_VRC6_CE		LATEbits.LATE2

#define TRIS_SPC_CE	TRISCbits.TRISC4
#define LAT_SPC_CE	LATCbits.LATC4

// xxx temporary port
#define TRIS_FL0_CE	TRISCbits.TRISC7
#define LAT_FL0_CE	LATCbits.LATC7

// /SPC_RESET moved to A7 (active high)
//#define TRIS_SPC_RESET	TRISCbits.TRISC5
//#define LAT_SPC_RESET	LATCbits.LATC5

#define TRIS_WE	TRISEbits.TRISE1
#define LAT_WE	LATEbits.LATE1

#define TRIS_OE	TRISEbits.TRISE0
#define LAT_OE	LATEbits.LATE0

#define TRIS_DATA	TRISD
#define LAT_DATA	LATD
#define PORT_DATA	PORTD

#define TRIS_INPUT (0xff)
#define TRIS_OUTPUT (0x00)

// macros for CE

#define ACTIVATE_VRC6()			LAT_VRC6_CE = 0
#define DEACTIVATE_VRC6()		LAT_VRC6_CE = 1

#define ACTIVATE_SPC()			LAT_SPC_CE = 0
#define DEACTIVATE_SPC()		LAT_SPC_CE = 1

#define ACTIVATE_FL0()			LAT_FL0_CE = 0
#define DEACTIVATE_FL0()		LAT_FL0_CE = 1

#define ACTIVATE_FL1()			LAT_FL1_CE = 0
#define DEACTIVATE_FL1()		LAT_FL1_CE = 1

#endif

enum {
	PORT_SPC0		=0x00,
	PORT_SPC1		=0x01,
	PORT_SPC2		=0x02,
	PORT_SPC3		=0x03,

	PORT_VRC690		=0x00,
	PORT_VRC691		=0x01,
	PORT_VRC692		=0x02,
	PORT_VRC6A0		=0x04,
	PORT_VRC6A1		=0x05,
	PORT_VRC6A2		=0x06,
	PORT_VRC6B0		=0x08,
	PORT_VRC6B1		=0x09,
	PORT_VRC6B2		=0x0A
};

// begin spc access
void ports_spc_open();

// end spc access
void ports_spc_close();

// read from spc port
uint8_t ports_spc_read(uint8_t);

// write to spc port
void ports_spc_write(uint8_t,uint8_t);

// write to vrc6 port
void ports_vrc6_write(uint8_t,uint8_t);

// begin external flash access
void ports_flash_open(uint24_t);
void ports_flash_opencont();

// set access address
void ports_flash_setaddr(uint24_t);

// end external flash access
void ports_flash_close();

// read a byte & increment pointer
uint8_t ports_flash_read();

// set address and read a byte & increment pointer
uint8_t ports_flash_readimm(uint24_t);

// set address and read a byte & increment pointer
uint16_t ports_flash_readimm16(uint24_t);

// get read address
uint24_t ports_flash_tell();

// read a word & increment pointer
uint16_t ports_flash_read16();

#endif
