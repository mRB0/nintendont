#include <stdint.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "Arduino.h"

#include "simpleplayer.h"

#include "fancysong.h"
#include "ports.h"

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


static volatile uint8_t ticked = 0;
static volatile uint8_t _smp_num = 0;
static volatile uint8_t _smp_vol = 6;
static volatile uint16_t _smp_offs = 0;
static volatile uint8_t _smp_stride = 0;

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
	
	uint16_t enbits;
	uint8_t i;
	uint8_t regs[9];

    uint8_t smp_num = 0xfe;
    uint16_t intr_period = 0x1000;
    uint8_t smp_stride = 0;
    uint8_t smp_vol = 0;
	
    delay(200);
	
	for(i=0; i<9; i++)
	{
		regs[i] = 0;
	}

    setup_timer3();

    enable_timer_interrupt();
//    enable_timer3_interrupt();
    ticked = 0;

//    for(;;);
    
	// start player
    for(;;) {
        while(!ticked) {
        }
        ticked = 0;
        
        // update registers from last read
        
        for(chan_offs = 0; chan_offs < 3; chan_offs ++)
        {
            for(reg_offs = 2; reg_offs >= 0 && reg_offs < 3; reg_offs --)
            {
                port_write(
                    VRC6AddrBase | (((uint8_t)chan_offs+(uint8_t)1) << (uint8_t)VRC6AddrChanShift) | (uint8_t)reg_offs,
                    regs[chan_offs*3 + reg_offs]
					);
            }
        }

        // update sample playback from last read

        if (smp_num == 0xff) {
            // no change to existing note
        } else if (smp_num == 0xfe) {
            // stop current sample
            disable_timer3_interrupt();
        } else {
            // play new sample (and reset offset)
            disable_timer3_interrupt();
            _smp_num = smp_num;
            _smp_offs = 0;
            enable_timer3_interrupt();
        }

        _smp_vol = smp_vol;
        _smp_stride = smp_stride;
        OCR3A = intr_period;

        //
        // process channel data
        //
        
        enbits = (((uint16_t)pgm_read_byte(fancysong + song_offs)) << 8) |
            (uint16_t)pgm_read_byte(fancysong + song_offs + 1);
        song_offs += 2;
		
        for(i = 0; i < 9; i++)
        {
            // vrc6 registers
            
            // decompress pattern data
				
            if ((enbits >> (15 - i)) & 0x01)
            {
                regs[i] = pgm_read_byte(fancysong + song_offs);
                song_offs++;
            }
        }
        
        for(; i < 16; i++)
        {
            // sample playback
            if ((enbits >> (15 - i)) & 0x01)
            {
                switch(i) {
                case 9: // sample number
                    smp_num = pgm_read_byte(fancysong + song_offs++);
                    break;
                case 10: // interrupt period
                    intr_period = (((uint16_t)pgm_read_byte(fancysong + song_offs)) << 8) |
                        (uint16_t)pgm_read_byte(fancysong + song_offs + 1);
                    song_offs += 2;
                    break;
                case 11: // sample stride
                    smp_stride = pgm_read_byte(fancysong + song_offs++);
                    break;
                case 12: // sample vol
                    smp_vol = pgm_read_byte(fancysong + song_offs++);
                    break;
                default: // unknown bit set; assume we need to move 1 byte in song offs
                    song_offs++;
                }
            }
        }
			
        if (song_offs == fancysong_len)
        {
            song_offs = 0;
        }

        //delay(15);
    }
}



ISR(TIMER3_COMPA_vect) {
    struct sample_info *smp = &(samples[_smp_num]);
    
    uint8_t *smp_pnt = smp->p_smp;
    uint8_t valb = pgm_read_byte(smp_pnt + _smp_offs);
    valb >>= 6 - _smp_vol;
    PORTK = valb;
    
    _smp_offs = (_smp_offs + _smp_stride);
    if (smp->loop_en) {
        if (_smp_offs >= smp->loop_end) {
            // Push the sample back to the loop start + the amount
            // we've walked off the loop end.  We also mod smp->len
            // here, because we don't want to accidentally walk off
            // the end of the whole sample... this is a little hacky,
            // but you shouldn't have such short loops anyway.
            _smp_offs = ((_smp_offs % smp->loop_end) + smp->loop_start) % smp->len;
        }
    } else {
        if (_smp_offs >= smp->len) {
            _smp_offs = 0;
            disable_timer3_interrupt();
        }
    }
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


