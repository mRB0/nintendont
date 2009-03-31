#include "spcunit.h"
#include "ports.h"

extern rom uint8_t SPCUNIT_BINARY[];

enum {
	CMD_LOAD	=0,
	CMD_MVOL	=1,
	CMD_EVOL	=2,
	CMD_EDL		=3,
	CMD_EFB		=4,
	CMD_COEF	=5,
	CMD_EON		=6,
	CMD_ECEN	=7,
	CMD_RET		=8,
	CMD_RESET	=9,
	CMD_KOF		=0xA,
	CMD_OFS		=0xB,
	CMD_PITCH	=0x10,
	CMD_VOL		=0x18,
	CMD_KON		=0x20
};

// boot to 0x300
#define BOOT_ADDRESS_L 0x00
#define BOOT_ADDRESS_H 0x03

static uint8_t SPC_V;

static uint16_t CURRENT_PITCH[8];
static uint8_t CURRENT_PAN[8];

#ifndef __PLAYERDEV__

static void UploadDriver( void ) {
	uint8_t data;
	uint8_t PT0;
	uint16_t BINARY_READ=0;
	ports_spc_open();

	// Confirm AA, BB
	while( ports_spc_read(0) != 0xAA ) {}
	while( ports_spc_read(1) != 0xBB ) {}

	// PORT1 = NOT 0
	// PORT2 = ADDRESS LOW
	// PORT3 = ADDRESS HIGH
	// PORT0 = PT0
	ports_spc_write(1, 1);
	ports_spc_write(2, BOOT_ADDRESS_L );
	ports_spc_write(3, BOOT_ADDRESS_H );
	PT0 = 0xCC;
	ports_spc_write(0, PT0);
	while( ports_spc_read(0) != PT0 );

	PT0 = 0;
	
	// --DATA TRANSFER START--
	for( BINARY_READ = 0; BINARY_READ < sizeof(SPCUNIT_BINARY); BINARY_READ++ ) {
		ports_spc_write( 1, SPCUNIT_BINARY[BINARY_READ] );
		ports_spc_write( 0, PT0 );
		while( ports_spc_read(0) != PT0 ) {}
		PT0++;
	}

	PT0++;
	
	ports_spc_write( 1, 0 );
	ports_spc_write( 2, BOOT_ADDRESS_L );
	ports_spc_write( 3, BOOT_ADDRESS_H );
	ports_spc_write( 0, PT0 );

	// driver is uploaded...
	// and will send V=0 when ready
	SPC_V = 0;

	ports_spc_close();
}

#endif

void SPCU_BOOT( void ) {
#ifdef __PLAYERDEV__
	//emulated
	SPC_V = 0x00;
#else

	UploadDriver();

#endif
	
	{
		uint8_t i;
		for( i = 0; i < 8; i++ ) {
			CURRENT_PITCH[i] = 0xFFFF;
			CURRENT_PAN[i] = 0xFF;
		}
	}
}

static void SPC_COMMAND( uint8_t cmd, uint8_t param1, uint8_t param2 ) {

	ports_spc_open();
		while( ports_spc_read(1) != SPC_V ) {}
		ports_spc_write( 0, cmd );
		ports_spc_write( 2, param1 );
		ports_spc_write( 3, param2 );
	
		SPC_V ^= 0x80;
		ports_spc_write( 1, SPC_V );
	ports_spc_close();
}

void SPCU_LOAD( uint16_t LOOP ) {

	ports_spc_open();
		while( ports_spc_read( 1 ) != SPC_V ) {}

		ports_spc_write( 0, CMD_LOAD );
		ports_spc_write( 2, LOOP & 0xFF );
		ports_spc_write( 3, LOOP >> 8 );

		SPC_V ^= 0x80;
		SPC_V |= 1;
		ports_spc_write( 1, SPC_V );
	ports_spc_close();
}

void SPCU_TRANSFER( uint16_t DATA, uint8_t FINAL ) {
	ports_spc_open();
		while( ports_spc_read( 1 ) != SPC_V ) {}
		ports_spc_write( 2, DATA & 0xFF );
		ports_spc_write( 3, DATA >> 8 );
		if( FINAL )
			SPC_V = 0;
		else
			SPC_V ^= 0x80;
		ports_spc_write( 1, SPC_V );
	ports_spc_close();
}

void SPCU_MVOL( int8_t LEFT, int8_t RIGHT ) {
	SPC_COMMAND( CMD_MVOL, (uint8_t)LEFT, (uint8_t)RIGHT );
}

void SPCU_EVOL( int8_t LEFT, int8_t RIGHT ) {
	SPC_COMMAND( CMD_EVOL, (uint8_t)LEFT, (uint8_t)RIGHT );
}

void SPCU_EDL( uint8_t DELAY ) {
	SPC_COMMAND( CMD_EDL, 0, DELAY );
}

void SPCU_EFB( int8_t FEEDBACK ) {
	SPC_COMMAND( CMD_EFB, 0, (uint8_t)FEEDBACK );
}

void SPCU_COEF( uint8_t INDEX, int8_t VALUE ) {
	SPC_COMMAND( CMD_COEF, INDEX, VALUE );
}

void SPCU_EON( uint8_t CHANNELS ) {
	SPC_COMMAND( CMD_EON, 0, CHANNELS );
}

void SPCU_ECEN( uint8_t ENABLED ) {
	SPC_COMMAND( CMD_ECEN, 0, ENABLED );
}

void SPCU_RET( void ) {
	SPC_COMMAND( CMD_RET, 0, 0 );
}

void SPCU_RESET( void ) {
	SPC_COMMAND( CMD_RESET, 0, 0 );
}

void SPCU_KOF( uint8_t CHANNELS ) {
	SPC_COMMAND( CMD_KOF, 0, CHANNELS );
}

void SPCU_OFS( uint8_t INDEX, uint8_t OFFSET ) {
	SPC_COMMAND( CMD_OFS, INDEX, OFFSET );
}

void SPCU_PITCH( uint8_t INDEX, uint16_t PITCH ) {

	if( CURRENT_PITCH[INDEX] != PITCH ) {
		SPC_COMMAND( CMD_PITCH + INDEX, PITCH & 0xFF, PITCH >> 8 );
		CURRENT_PITCH[INDEX] = PITCH;
	}
}

void SPCU_VOL( uint8_t INDEX, uint8_t VOLUME, uint8_t PANNING ) {

	if( PANNING & 128 )
		SPC_COMMAND( CMD_VOL + INDEX, PANNING, VOLUME );
	else {
		if( PANNING == CURRENT_PAN[INDEX] ) {
			SPC_COMMAND( CMD_VOL + INDEX, 128, VOLUME );
		} else {
			SPC_COMMAND( CMD_VOL + INDEX, PANNING, VOLUME );
			CURRENT_PAN[INDEX] = PANNING;
		}
	}
}

void SPCU_KON( uint8_t INDEX, uint8_t VOLUME, uint8_t SOURCE ) {
	SPC_COMMAND( CMD_KON + INDEX, VOLUME, SOURCE );
}
