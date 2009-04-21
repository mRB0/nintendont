#include <p18f4620.h>
#include <usart.h>
#include <stdio.h>
#include <delays.h>

#define USE_TIMER

#include "simpleplayer.h"
#include "interrupts.h"
#include "fancysong.h"
#include "ports.h"

const rom uint16_t player_frequencies[] = { 4095, 3865, 3648, 3443, 3250, 3067, 2895, 2733, 2579, 2434, 2298, 2169, 2047, 1932, 1824, 1721, 1625, 1533, 1447, 1366, 1289, 1217, 1149, 1084, 1023, 966, 912, 860, 812, 766, 723, 683, 644, 608, 574, 542, 511, 483, 456, 430, 406, 383, 361, 341, 322, 304, 287, 271, 255, 241, 228, 215, 203, 191, 180, 170, 161, 152, 143, 135, 127, 120, 114, 107, 101, 95, 90, 85, 80, 76, 71, 67, 63, 60, 57, 53, 50, 47, 45, 42, 40, 38, 35, 33, 31, 30, 28, 26, 25, 23, 22, 21, 20, 19, 17, 16 };
const rom uint8_t player_freq_len = 96;

void simpleplayer(void)
{
	uint32_t song_offs = 0;
	uint8_t chan_offs = 0;
	uint8_t reg_offs;
	
	uint8_t enbits;
	uint8_t i;
	uint8_t regs[9];
	
	//putrsUSART("\n\r\n\r\n\rbegin\n\r");
	
	for(i=0; i<255; i++)
	{
		Delay100TCYx(255);
		Delay100TCYx(255);
		Delay100TCYx(255);
	}
	
	
	IPR1bits.TMR1IP = 1;
	
#ifdef USE_TIMER
	
	// start timer
	T1CONbits.RD16 = 0;
	T1CONbits.T1RUN = 0;
	T1CONbits.T1CKPS0 = 1;
	T1CONbits.T1CKPS1 = 1;
	T1CONbits.TMR1CS = 0;
	T1CONbits.T1OSCEN = 0;
	T1CONbits.TMR1ON = 1;
	
	PIE1bits.TMR1IE = 1;
#endif
	/*
	return;
	*/
	
	for(i=0; i<9; i++)
	{
		regs[i] = 0;
	}
	
	// start player
	for(;;)
	{
		ISR_disable();
#ifdef USE_TIMER
		while (_interrupts.timer1)
#else
		for(;;)
#endif
		{
			_interrupts.timer1 = 0;
			ISR_enable();
			
			ACTIVATE_VRC6();
			
			for(chan_offs = 0; chan_offs < 3; chan_offs ++)
			{
/*
				for(reg_offs = 2; reg_offs >= 0 && reg_offs < 3; reg_offs --, i ++)
				{
					port_write(
						0x8000 | (((uint24_t)chan_offs+(uint24_t)1) << (uint24_t)12) | (uint24_t)(reg_offs),
						regs[chan_offs*3 + reg_offs]
					);
				}
*/
				for(reg_offs = 2; reg_offs >= 0 && reg_offs < 3; reg_offs --)
				{
					port_write(
						0x8000 | (((uint24_t)chan_offs+(uint24_t)1) << (uint24_t)12) | (uint24_t)(reg_offs),
						regs[chan_offs*3 + reg_offs]
					);
				}
			}
			
			//for(i=0; i<5; i++) { Delay100TCYx(255); }
			
			//printf("huk\n\r");
			
			// process channel data
			
			
			enbits = fancysong[song_offs++];
			
			for(i=0; i<9; i++)
			{
				uint8_t regoffs;
				regoffs = i;
				//ISR_disable();
				// decompress pattern data
				// 8th and 9th byte share the 8th bit (MSB)
				if (regoffs == 8) { regoffs = 7; }
				
				
				if ((enbits>>regoffs) & 0x01)
				{
					regs[i] = fancysong[song_offs];
					//if (chan_offs == 0) {
						/*
						port_write(
							0x8000 | (((uint24_t)chan_offs+(uint24_t)1) << (uint24_t)12) | (uint24_t)reg_offs,
							fancysong[song_offs]
						);
						*/
					//}
					song_offs++;
				}
			}
			//ISR_enable();
			
			DEACTIVATE_VRC6();
			
			if (song_offs == fancysong_len)
			{
				song_offs = 0;
			}
			
			ISR_disable();
		}
		ISR_enable();
		
	}
}

#pragma tmpdata isrhigh_tmp
void timer1_isr(void)
{
	_interrupts.timer1 = 1;
	TMR1H = 0xb0;
	PIR1bits.TMR1IF = 0;
}
