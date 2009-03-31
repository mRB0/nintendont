#include "ports.h"

#ifdef __PLAYERDEV__

#include "emu_spc.h"
#include "emu_vrc6.h"
#include "emu_mem.h"
#include "stdio.h"

uint8_t SPC_OPEN;
uint8_t FLASH_OPEN;
uint24_t FLASH_ADDR;

/*
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
*/

void ports_spc_open() {
	if( FLASH_OPEN )
		printf( "warning: opening spc when flash is open\n" );
	SPC_OPEN = 1;
}

void ports_spc_close() {
	SPC_OPEN = 0;
}

uint8_t ports_spc_read( uint8_t port ) {
	if( FLASH_OPEN ) {
		printf( "warning: read from spc when flash is open\n" );
	}
	if( SPC_OPEN ) {
		return SPCEMU_READPORT(port);
	} else {
		printf( "warning: read from closed port (spc)\n" );
		return 0;
	}
}

void ports_spc_write( uint8_t port, uint8_t data ) {
	if( FLASH_OPEN ) {
		printf( "warning: write to spc when flash is open\n" );
	}
	if( SPC_OPEN ) {
		SPCEMU_WRITEPORT( port, data );
	} else {
		printf( "warning: write to closed port (spc)\n" );
	}
}

void ports_vrc6_write( uint8_t port, uint8_t data ) {
	if( SPC_OPEN || FLASH_OPEN ) {
		printf( "warning: write to vrc6 when flash or spc is open\n" );
	}
	VRC6EMU_WRITEREG( port, data );
}

void ports_flash_open( uint24_t addr ) {
	if( SPC_OPEN )
		printf( "warning: opening flash when spc is opened.\n" );
	FLASH_OPEN = 1;
	FLASH_ADDR = addr;
}

void ports_flash_opencont() {
	if( SPC_OPEN )
		printf( "warning: opening flash when spc is opened.\n" );
	FLASH_OPEN = 1;
	// continue with previous address
}

void ports_flash_setaddr( uint24_t addr ) {
	if( FLASH_OPEN ) {
		FLASH_ADDR = addr;
	} else {
		printf("warning: attempt to set flash address when not open.\n" );
	}
}

void ports_flash_close() {
	FLASH_OPEN = 0;
}

uint8_t ports_flash_read() {
	if( SPC_OPEN )
		printf( "warning: reading from flash when spc is open.\n" );
	if( FLASH_OPEN ){
		FLASH_ADDR++;
		return GetEBankByte( FLASH_ADDR-1 );
	} else {
		printf( "warning: reading from closed port (flash)\n" );
		return 0;
	}
}

uint8_t ports_flash_readimm( uint24_t addr ) {
	if( SPC_OPEN ) {
		printf( "warning: reading from flash with spc open.\n" );
	}
	if( FLASH_OPEN ) {
		FLASH_ADDR = addr+1;
		return GetEBankByte( FLASH_ADDR-1 );
	} else {
		printf( "warning: reading from closed port (flash)\n" );
		return 0;
	}
}

uint16_t ports_flash_readimm16( uint24_t addr ) {
	if( SPC_OPEN ) {
		printf( "warning: reading from flash with spc open.\n" );
	}
	if( FLASH_OPEN ) {
		FLASH_ADDR = addr+2;
		return GetEBankByte( FLASH_ADDR-2 ) | (GetEBankByte( FLASH_ADDR-1 ) << 8);
	} else {
		printf( "warning: reading 16 from closed port (flash)\n" );
		return 0;
	}
}

uint16_t ports_flash_read16() {
	if( SPC_OPEN ) {
		printf( "warning: reading 16 from flash when spc is open." );
	}
	if( FLASH_OPEN ) {
		FLASH_ADDR += 2;
		return GetEBankByte( FLASH_ADDR-2 ) | (GetEBankByte( FLASH_ADDR-1 ) << 8);
	} else {
		printf( "warning: reading from closed port (flash)\n" );
		return 0;
	}
}

uint24_t ports_flash_tell() {
	return FLASH_ADDR;
}

#else

// [real codes]

//void ports_spc_open(); macro'd
//void ports_spc_close(); macro'd

uint8_t ports_spc_read(uint8_t port) {
	//insert codes
}

void ports_spc_write(uint8_t port, uint8_t data) {
	//insert codes
}



void ports_vrc6_write(uint8_t port, uint8_t data) {
	//insert codes, port is 0,1,2,4,5,6,8,9,10
}


void ports_flash_open( uint24_t addr ) {
	//insert codes
}

void ports_flash_close() {
	//insert codes
}

uint8_t ports_flash_read() {
	//insert codes
}

void ports_flash_readimm( uint24_t addr ) {
	//insert codes
}

#endif