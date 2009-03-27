#include "emu_mem.h"

uint8_t IBANK[65536];
uint8_t EBANK[65536];

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

uint8_t *GetEBankPtr() {
	return EBANK;
}

uint8_t *GetIBankPtr() {
	return IBANK;
}
