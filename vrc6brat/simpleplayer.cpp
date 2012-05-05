#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "Arduino.h"

#include "simpleplayer.h"

#include "fancysong.h"
#include "ports.h"

const uint16_t player_frequencies[] = { 4095, 3865, 3648, 3443, 3250, 3067, 2895, 2733, 2579, 2434, 2298, 2169, 2047, 1932, 1824, 1721, 1625, 1533, 1447, 1366, 1289, 1217, 1149, 1084, 1023, 966, 912, 860, 812, 766, 723, 683, 644, 608, 574, 542, 511, 483, 456, 430, 406, 383, 361, 341, 322, 304, 287, 271, 255, 241, 228, 215, 203, 191, 180, 170, 161, 152, 143, 135, 127, 120, 114, 107, 101, 95, 90, 85, 80, 76, 71, 67, 63, 60, 57, 53, 50, 47, 45, 42, 40, 38, 35, 33, 31, 30, 28, 26, 25, 23, 22, 21, 20, 19, 17, 16 };
const uint8_t player_freq_len = 96;

inline void enable_timer_interrupt() {
    TIMSK2 |= _BV(OCIE2A);
}

inline void disable_timer_interrupt() {
    TIMSK2 &= ~_BV(OCIE2A);
}

volatile uint8_t ticked = 0;

void simpleplayer() {
    pinMode(13, OUTPUT);

    disable_timer_interrupt();
    
    TCCR2A &= ~_BV(WGM20);
    TCCR2A |= _BV(WGM21);
    TCCR2B &= ~_BV(WGM22);

    TCCR2B |= _BV(CS22);
    TCCR2B |= _BV(CS21);
    TCCR2B |= _BV(CS20);
    
    OCR2A = 0xa0;

    
	uint32_t song_offs = 0;
	uint8_t chan_offs = 0;
	uint8_t reg_offs;
	
	uint8_t enbits;
	uint8_t i;
	uint8_t regs[9];
	
    delay(200);
	
	for(i=0; i<9; i++)
	{
		regs[i] = 0;
	}

    enable_timer_interrupt();
    ticked = 0;

	// start player
    for(;;) {
        while(!ticked) {
        }
        ticked = 0;
        
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
                    0x8000 | (((uint16_t)chan_offs+(uint16_t)1) << (uint16_t)12) | (uint16_t)(reg_offs),
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
			
        if (song_offs == fancysong_len)
        {
            song_offs = 0;
        }

        //delay(15);
    }
}

volatile uint8_t ledstate = 0;
volatile uint8_t tickScale = 0;
volatile uint8_t tickScaleMask = 0x1;

ISR(TIMER2_COMPA_vect) {
    ledstate = !ledstate;
    digitalWrite(13, ledstate);
    tickScale = (tickScale + 1) & tickScaleMask;
    if (tickScale == 0) {
        ticked = 1;
    }
};

