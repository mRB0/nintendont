#include "emu_mem.h"
#include "stdio.h"

uint8_t IBANK[65536];
uint8_t EBANK[256*1024*2];

void FlashIbank( const char *filename ) {
	int fsize;
	FILE *f = fopen( filename, "rb" );
	fseek( f, 0, SEEK_END );
	fsize = ftell(f);
	fseek( f, 0, SEEK_SET );
	fread( IBANK, 1, fsize, f );
	fclose(f);
}

void FlashEbank( const char *filename ) {
	int fsize;
	FILE *f = fopen( filename, "rb" );
	fseek( f, 0, SEEK_END );
	fsize = ftell(f);
	fseek( f, 0, SEEK_SET );
	fread( EBANK, 1, fsize, f );
	fclose(f);
}

uint8_t GetEBankByte( uint24_t addr ) {
	if( addr > sizeof( EBANK ) ) {
		printf( "WORNING: reading from invalid EBANK address.\n" );
		return 0;
	} else {
		return EBANK[addr];
	}
}

uint8_t *GetEBankPtr() {
	return EBANK;
}

uint8_t *GetIBankPtr() {
	return IBANK;
}
/*
uint24_t ExNext;

uint8_t ReadEx8n() {
	return EBANK[ExNext++];
}

uint8_t ReadEx8( uint24_t address ) {
	ExNext = address;
	return ReadEx8n();
}

uint16_t ReadEx16( uint24_t address ) {
	uint8_t a;
	ExNext = address;
	a = ReadEx8n();
	return a | (ReadEx8n() << 8);
}

void SetExAddr( uint24_t address ) {
	ExNext = address;
}
*/

