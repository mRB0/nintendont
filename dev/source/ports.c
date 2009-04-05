
#ifndef __PLAYERDEV__
#include <p18f4620.h>
#include <delays.h>
#endif

#include "ports.h"
#include "flash.h"

#ifdef __PLAYERDEV__

#include "emu_spc.h"
#include "emu_vrc6.h"
#include "emu_mem.h"
#include "stdio.h"

uint8_t SPC_OPEN;
uint8_t FLASH_OPEN;
uint24_t FLASH_ADDR;


void ports_spc_open( void ) {
	if( FLASH_OPEN )
		printf( "warning: opening spc when flash is open\n" );
	SPC_OPEN = 1;
}

void ports_spc_close( void ) {
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

void ports_flash_opencont( void ) {
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

void ports_flash_close( void ) {
	FLASH_OPEN = 0;
}

uint8_t ports_flash_read( void ) {
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

uint16_t ports_flash_read16( void ) {
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

uint24_t ports_flash_tell( void ) {
	return FLASH_ADDR;
}

#else

// [real codes]

uint24_t _flash_addr;

void ports_spc_open( void ) {
	ACTIVATE_SPC();
}

void ports_spc_close( void ) {
	DEACTIVATE_SPC();
}

uint8_t ports_spc_read(uint8_t port) {
	LATA = port;
	return port_getc();
}

void ports_spc_write(uint8_t port, uint8_t data) {
	LATA = port;
	port_putc(data);
}



void ports_vrc6_write(uint8_t port, uint8_t data) {
	//insert codes, port is 0,1,2,4,5,6,8,9,10
	LATA = port & 0x3;
	LATCbits.LATC0 = (port >> 2) & 0x1;
	LATCbits.LATC1 = (port >> 3) & 0x1;
	LATCbits.LATC2 = 0;
	
	port_putc(data);
}



void ports_flash_open( uint24_t addr ) {
	ACTIVATE_FL0();
	_flash_addr = addr;
	set_addr(addr);
}

void ports_flash_opencont() {
	ACTIVATE_FL0();
}

void ports_flash_setaddr( uint24_t addr ) {
	_flash_addr = addr;
}

void ports_flash_close( void ) {
	DEACTIVATE_FL0();
}

uint8_t ports_flash_read( void ) {
	set_addr(_flash_addr);
	_flash_addr++;
	return port_getc();
}

uint8_t ports_flash_readimm( uint24_t addr ) {
	set_addr(addr);
	_flash_addr = addr+1;
	return port_getc();
}

uint16_t ports_flash_readimm16( uint24_t addr ) {
	uint16_t data;
	
	data = ((uint16_t)port_read(addr+1) << 8) | ((uint16_t)port_read(addr));
	
	_flash_addr = addr+2;
	return data;
}

uint24_t ports_flash_tell( void ) {
	return _flash_addr;
}

uint16_t ports_flash_read16( void ) {
	uint16_t data;
	
	data = ((uint16_t)port_read(_flash_addr+1) << 8) | ((uint16_t)port_read(_flash_addr));
	
	_flash_addr += 2;
	return data;
}

/* begin mrb */

void set_addr(uint24_t addr)
{
	LATA = addr & 0xff;
	
	LATB = ((addr >> (uint16_t)8) & 0x0f) | (LATB & 0xf0);
	LATC = ((addr >> (uint16_t)12) & 0x0f) | (LATC & 0xf0);
	
	LATCbits.LATC5 = (addr >> 16) & 0x1;
	
	// A17
	LATBbits.LATB4 = (addr >> 17) & 0x1;
	
	//Nop();
	//Nop();
	
}

uint8_t port_getc(void)
{
	uint8_t data;
	
	TRIS_DATA = TRIS_INPUT;
	
	LAT_OE = 0;
	//Delay10TCYx(100);
	
	data = PORT_DATA;
	LAT_OE = 1;
	
	return data;
}

uint8_t port_read(uint24_t addr)
{
	set_addr(addr);
	
	return port_getc();
}
	
void port_putc(uint8_t data)
{
	TRIS_DATA = TRIS_OUTPUT;
	LAT_DATA = data;
	LAT_WE = 0;
	
	// these delays are required for vrc6 when
	// Fosc = 32 MHz and vrc6 = 2 MHz :(
	Nop();
	Nop();
	Nop();
	
	//Delay10TCYx(1);
	
	LAT_WE = 1;
}

void port_write(uint24_t addr, uint8_t data)
{
	set_addr(addr);
	port_putc(data);
}

/* end mrb */


#endif
