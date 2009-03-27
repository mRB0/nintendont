#ifndef __EMU_VRC6_H
#define __EMU_VRC6_H

#include "stdint.h"

void VRC6EMU_WRITEREG( int index, uint8_t data );
void VRC6EMU_RUN( int frames, int16_t *buffer, double framerate );

#endif
