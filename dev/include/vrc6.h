#ifndef __VRC6_H
#define __VRC6_H

#include "stdint.h"

//
// Reset VRC6
//
void VRC6_Reset();

// 
// Set VRC6 channel pitch
// channel: 0..2
// pitch: 12-bit period value
//
void VRC6_SetPitch( uint8_t channel, uint16_t period );

//
// Set VRC6 channel volume
// channel: 0..2
// volume: 0->64
// duty: 0->7 (channels 0,1 only)
//
void VRC6_SetVolume( uint8_t channel, uint8_t volume, uint8_t duty );

#endif
