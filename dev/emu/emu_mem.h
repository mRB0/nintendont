#ifndef __EMU_MEM_H
#define __EMU_MEM_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t GetEBankByte( uint24_t addr );
uint8_t *GetEBankPtr();
uint8_t *GetIBankPtr();

void FlashIbank( const char *filename );
void FlashEbank( const char *filename );

#ifdef __cplusplus
}
#endif

#endif
