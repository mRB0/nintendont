#ifndef __EMU_SPC_H
#define __EMU_SPC_H

#include "stdint.h"

void SPCEMU_WRITEPORT( int index, uint8_t value );
uint8_t SPCEMU_READPORT( int index );

void SPCEMU_INIT();
uint8_t SPCEMU_READPORT( int index );
void SPCEMU_RUN( int frames, int16_t *buffer, double framerate );

#endif
