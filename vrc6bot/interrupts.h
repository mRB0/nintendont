#ifndef INTERRUPTS_H_
#define INTERRUPTS_H_

#include <stddef.h>

#include "stdint.h"

typedef union interrupts
{
	struct
	{
		uint8_t unused  :8;
	};
	uint8_t all;
} INTERRUPTS;

extern volatile INTERRUPTS _interrupts;


void ISR_init(void);

#define ISR_enable() 	INTCONbits.GIEH = 1; INTCONbits.GIEL = 1;
#define ISR_disable() 	INTCONbits.GIEH = 0; INTCONbits.GIEL = 0;

#endif