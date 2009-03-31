// a basic vrc6 emulator

#include "emu_vrc6.h"

static const double clock = 2048000.0;//1789772.7272;
static const int volume = 32767 * 100/100;

// pulse output: 4bit
// saw output: 5bit
// final output: 6bit

// VRC6 Registers

static uint8_t r_9000; // GDDDVVVV
static uint8_t r_9001; // FFFFFFFF
static uint8_t r_9002; // X---FFFF

static uint8_t r_A000; // GDDDVVVV
static uint8_t r_A001; // FFFFFFFF
static uint8_t r_A002; // X---FFFF

// digital mode is not emulated

static uint8_t r_B000; // --PPPPPP
static uint8_t r_B001; // FFFFFFFF
static uint8_t r_B002; // X---FFFF

#define maskdivider( r1, r2 ) ((double)((r1) + (((r2)&0xF)<<8)))

static uint8_t *register_map[] = { 
	&r_9000, &r_9001, &r_9002, 0,
	&r_A000, &r_A001, &r_A002, 0,
	&r_B000, &r_B001, &r_B002, 0
};

static int phase0; // PULSE1 .16 fixed
static int phase1; // PULSE2 .16 fixed
static int phase2; // SAWTOOTH .16 fixed

int dc_adjust = 0;

extern "C" {

	void VRC6EMU_WRITEREG( int index, uint8_t data ) {
		*(register_map[index]) = data;
	}

}
int a = 0;
void VRC6EMU_RUN( int frames, int16_t *buffer, double framerate ) {

//	r_A000 = 31; // test saw
//	r_A001 =a++ ;
//	r_A002 = 0 | 0x80;

	int duty0 = 0x1000 * (((r_9000 >> 4)&7)+1);
	int duty1 = 0x1000 * (((r_A000 >> 4)&7)+1);

	int v0 = r_9000 & 0xF;
	int v1 = r_A000 & 0xF;

	// frequencies
	int f0, f1, f2;
	f0 = (int)(65536.0 * (clock/framerate/maskdivider(r_9001,r_9002)/16.0)+0.5);
	f1 = (int)(65536.0 * (clock/framerate/maskdivider(r_A001,r_A002)/16.0)+0.5);
	f2 = (int)(65536.0 * (clock/framerate/maskdivider(r_B001,r_B002)/14.0)+0.5);

	int sawP = r_B000 & 63;
	
	bool x0, x1, x2;
	x0 = !!(r_9002 & 0x80);
	x1 = !!(r_A002 & 0x80);
	x2 = !!(r_B002 & 0x80);
	
	// samples...
	int16_t s1, s2, s3;
	
	int16_t *output = (int16_t*)buffer;
	
	int dca = 
	-(	(x0 ? ((v0 * (65536-duty0))>>16) : 0) + 
		(x1 ? ((v1 * (65536-duty1))>>16) : 0) +
		(x2 ? ((sawP*6)>>3) : 0)
	) / 2;

	dca=0;
	
	int i;
	for( i = 0; i < frames; i++ ) {
		s1 = x0 ? ((phase0 >= duty0) ? v0:0) : 0;
		s2 = x1 ? ((phase1 >= duty1) ? v1:0) : 0;
		s3 = x2 ? ( (((((phase2*7)>>16)*sawP)&0xFF)>>3) ) : 0;
		
		phase0 = (phase0 + f0) & 0xFFFF;
		phase1 = (phase1 + f1) & 0xFFFF;
		phase2 = (phase2 + f2) & 0xFFFF;
		
		if( dc_adjust < dca ) dc_adjust++;
		if( dc_adjust > dca ) dc_adjust--;
		
		int a = (((s1 + s2 + s3 + dc_adjust) * volume) >> 6);
		if( a > 32767 ) a = 32767;
		if( a < -32768 ) a = -32768;
		*output++ = a;
	}
}
