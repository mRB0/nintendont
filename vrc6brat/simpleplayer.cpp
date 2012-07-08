#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "Arduino.h"

#include "simpleplayer.h"

#include "fancysong.h"
#include "ports.h"

#include "waves.h"
#include "vrc6brat.h"

const uint16_t player_frequencies[] = { 4095, 3865, 3648, 3443, 3250, 3067, 2895, 2733, 2579, 2434, 2298, 2169, 2047, 1932, 1824, 1721, 1625, 1533, 1447, 1366, 1289, 1217, 1149, 1084, 1023, 966, 912, 860, 812, 766, 723, 683, 644, 608, 574, 542, 511, 483, 456, 430, 406, 383, 361, 341, 322, 304, 287, 271, 255, 241, 228, 215, 203, 191, 180, 170, 161, 152, 143, 135, 127, 120, 114, 107, 101, 95, 90, 85, 80, 76, 71, 67, 63, 60, 57, 53, 50, 47, 45, 42, 40, 38, 35, 33, 31, 30, 28, 26, 25, 23, 22, 21, 20, 19, 17, 16 };
const uint8_t player_freq_len = 96;

volatile uint8_t ledstate = 0;
volatile uint8_t ledscale = 0;

inline void enable_timer_interrupt() {
    TIMSK2 |= _BV(OCIE2A);
}

inline void disable_timer_interrupt() {
    TIMSK2 &= ~_BV(OCIE2A);
}

static int const PinBase = 2;


inline void enable_timer3_interrupt() {
    TIMSK3 |= _BV(OCIE3A);
}

inline void disable_timer3_interrupt() {
    TIMSK3 &= ~_BV(OCIE3A);
}

void setup_timer3() {   
    // for(int i = 0; i < 6; i++) {
    //     pinMode(PinBase + i, OUTPUT);
    //     digitalWrite(PinBase + i, LOW);
    // }
    DDRK = 0x3f;
    PORTK = 0x00;
    
    // WGM33:0 = 0b0100 (TOP=OCR3A)
    disable_timer3_interrupt();
    TCCR3A = 0x00;
    TCCR3A &= ~_BV(WGM30);
    TCCR3A &= ~_BV(WGM31);
    TCCR3B |= _BV(WGM32);
    TCCR3B &= ~_BV(WGM33);
    TCCR3B &= ~_BV(CS32);
    TCCR3B &= ~_BV(CS31);
    TCCR3B |= _BV(CS30);
    OCR3A = 0x800;
  
//  TCCR4A &= ~_BV(WGM30);
//  TCCR4A &= ~_BV(WGM31);
//  TCCR4B |= _BV(WGM32);
//  TCCR4B &= ~_BV(WGM33);
//  TCCR4B &= ~_BV(CS32);
//  TCCR4B &= ~_BV(CS31);
//  TCCR4B |= _BV(CS30);
//  OCR4A = 0x501;
   
  
}


volatile uint8_t ticked = 0;

void simpleplayer() {
    disable_timer_interrupt();
    
    TCCR2A &= ~_BV(WGM20);
    TCCR2A |= _BV(WGM21);
    TCCR2B &= ~_BV(WGM22);

    TCCR2B |= _BV(CS22);
    TCCR2B |= _BV(CS21);
    TCCR2B |= _BV(CS20);
    
    OCR2A = 0xa0;

    pinMode(13, OUTPUT);

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

    setup_timer3();

    enable_timer_interrupt();
    enable_timer3_interrupt();
    ticked = 0;

//    for(;;);
    
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
                    VRC6AddrBase | (((uint8_t)chan_offs+(uint8_t)1) << (uint8_t)VRC6AddrChanShift) | (uint8_t)reg_offs,
                    regs[chan_offs*3 + reg_offs]
					);
            }
        }
			
        //for(i=0; i<5; i++) { Delay100TCYx(255); }
			
        //printf("huk\n\r");
			
        // process channel data
			
			
        enbits = pgm_read_byte(fancysong + (song_offs++));
			
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
                regs[i] = pgm_read_byte(fancysong + song_offs);
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


static volatile uint8_t vol = 6;
static volatile uint16_t ax = 0;
static volatile uint16_t smpoffs = 0;
static volatile uint8_t entrop = 2;
static volatile uint8_t per = 0;

ISR(TIMER3_COMPA_vect) {
  
    uint8_t valb = pgm_read_byte(wavelet + smpoffs);
    smpoffs = (smpoffs + 1) % wavelet_len;
    valb >>= 6 - vol;
    PORTK = valb;
    // for (uint8_t i = 0; i < 6; i++) {
    //     uint8_t writeval;
    //     writeval = 0x01 & (valb >> i);
    //     digitalWrite(PinBase + i, writeval);
    // }
  
    // ax = (ax + 1) % (0x1000 - (OCR3A - 0x400));
    // if (!ax) {
    //     if (vol == 0) {
    //         vol = 6;
    //         // per = (per + 1) % 8;
    //         // OCR3A = 0x400 + (per * 0x200);
    //     } else {
    //         vol--;
    //     }
    // }
};

volatile uint8_t tickScale = 0;
volatile uint8_t tickScaleMask = 0x1;

ISR(TIMER2_COMPA_vect) {
    tickScale = (tickScale + 1) & tickScaleMask;
    if (tickScale == 0) {
        ticked = 1;
    }
        ledscale = (ledscale + 1) & 0x7;
        if (!ledscale) {
            ledstate = !ledstate;
            digitalWrite(13, ledstate);
        }
};


