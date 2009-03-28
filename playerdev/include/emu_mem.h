#ifndef __EMU_MEM_H
#define __EMU_MEM_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif



extern uint8_t IBANK[];

void SetExAddr( uint24_t address );

uint8_t ReadEx8n();
uint8_t ReadEx8( uint24_t address );
uint16_t ReadEx16( uint24_t address );

uint8_t *GetEBankPtr();
uint8_t *GetIBankPtr();

#ifdef __cplusplus
}
#endif

#endif
