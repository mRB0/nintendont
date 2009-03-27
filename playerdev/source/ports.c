#include "ports.h"

#define PLAYERDEV

#ifdef PLAYERDEV

#include "emu_spc.h"
#include "emu_vrc6.h"

uint8_t port_read(uint16_t addr) {
	switch( addr ) {
	case PORT_SPC0:
		return SPCEMU_READPORT(0);
	case PORT_SPC1:
		return SPCEMU_READPORT(1);
	case PORT_SPC2:
		return SPCEMU_READPORT(2);
	case PORT_SPC3:
		return SPCEMU_READPORT(3);
	}
	return 0;
}

void port_write(uint16_t addr, uint8_t data) {
	switch( addr ) {
	case PORT_SPC0:
		SPCEMU_WRITEPORT( 0, data ); break;
	case PORT_SPC1:
		SPCEMU_WRITEPORT( 1, data ); break;
	case PORT_SPC2:
		SPCEMU_WRITEPORT( 2, data ); break;
	case PORT_SPC3:
		SPCEMU_WRITEPORT( 3, data ); break;
	case PORT_VRC690:
		VRC6EMU_WRITEREG( 0, data ); break;
	case PORT_VRC691:
		VRC6EMU_WRITEREG( 1, data ); break;
	case PORT_VRC692:
		VRC6EMU_WRITEREG( 2, data ); break;
	case PORT_VRC6A0:
		VRC6EMU_WRITEREG( 3, data ); break;
	case PORT_VRC6A1:
		VRC6EMU_WRITEREG( 4, data ); break;
	case PORT_VRC6A2:
		VRC6EMU_WRITEREG( 5, data ); break;
	case PORT_VRC6B0:
		VRC6EMU_WRITEREG( 6, data ); break;
	case PORT_VRC6B1:
		VRC6EMU_WRITEREG( 7, data ); break;
	case PORT_VRC6B2:
		VRC6EMU_WRITEREG( 8, data ); break;
	}
}

#else

// [real codes]

#endif
