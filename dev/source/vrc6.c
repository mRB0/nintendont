#include "vrc6.h"
#include "ports.h"

// period LOW byte
static uint8_t periods_l[3];

// period HIGH byte
static uint8_t periods_h[3];

// 0,1: DUTY+VOLUME
// 2: DELTA
static uint8_t volumes[3];

void VRC6_Reset( void ) {
	uint8_t i;
	for (i = 0; i < 3; i++ ) {
		periods_l[i] = 0x0;
		periods_h[i] = 0xFF;
		volumes[i] = 0xFF;
	}
	//todo: reset vrc6 ports?
}

void VRC6_SetPitch( uint8_t channel, uint16_t period ) {
	uint8_t L = period & 0xFF, H = period >> 8;

	if( periods_l[channel] != L ) {
		periods_l[channel] = L;
		ports_vrc6_write( PORT_VRC691 + (channel << 2), L );
	}

	H |= 0x80;
	if( periods_h[channel] != H ) {
		periods_h[channel] = H;
		ports_vrc6_write( PORT_VRC692 + (channel << 2), H );
	}
}

void VRC6_SetVolume( uint8_t channel, uint8_t volume, uint8_t duty ) {
	if( channel < 2 ) {
		// pulse
		uint8_t v = volume >> 2;
		if( v == 0 ) v = volume ? 1 : 0;
		else if( v == 16 ) v = 15;
		v |= duty << 4;
		if( volumes[channel] != v ) {
			volumes[channel] = v;
			ports_vrc6_write( PORT_VRC690 + (channel << 2), v );
		}
	} else {
		// sawtooth
		uint8_t v = (volume * 42) >> 6;
		if( v == 0 ) v = volume ? 1 : 0;
		if( volumes[2] != v ) {
			volumes[2] = v;
			ports_vrc6_write( PORT_VRC6B0, v );
		}
	}
}
