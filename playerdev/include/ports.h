#ifndef __PORTS_H
#define __PORTS_H

#include "stdint.h"

enum {
	PORT_SPC0		=0x00,
	PORT_SPC1		=0x01,
	PORT_SPC2		=0x02,
	PORT_SPC3		=0x03,
	PORT_SPC_RESET	=0x80,
	PORT_VRC690		=0x9000,
	PORT_VRC691		=0x9001,
	PORT_VRC692		=0x9002,
	PORT_VRC6A0		=0xA000,
	PORT_VRC6A1		=0xA001,
	PORT_VRC6A2		=0xA002,
	PORT_VRC6B0		=0xB000,
	PORT_VRC6B1		=0xB001,
	PORT_VRC6B2		=0xB002
};

uint8_t port_read(uint16_t addr);
void port_write(uint16_t addr, uint8_t data);

#endif
