#include "emu_timer.h"
#include "player.h"

#include "math.h"

static const double system_clock = 33554432.0;

static bool irq;
static bool bits16;
static int prescaler;
static bool active;

static double counter;
static bool need_update;
static double timerhz;
static double boundary;

extern "C" {

	// 
	// copy behavior of library functions
	//

	void OpenTimer0( uint8_t config ) {
		
		irq = !!(config & TIMER_INT_ON);
		bits16 = !!(config & T0_16BIT);
		prescaler = config & 0xF;
		need_update=false;

		counter=  0;
		boundary = bits16 ? 65536.0 : 256.0;
		timerhz = 33554432.0 / (1<<prescaler);

		active = true;

		
	}

	void CloseTimer0() {
		active = false;
	}

	void WriteTimer0( uint16_t value ) {
		counter = value;
	}

	uint16_t ReadTimer0() {
		return (uint16_t)floor(counter);
	}

}

//
// Get the amount of audio frames (samples) 
// until the next timer overflow
//
// rate = audio sampling rate
//
int TIMEREMU_FramesUntilNextPulse( double rate ) {

	double time = boundary - counter;
	
	return (int)ceil(rate*time / timerhz) + 3;
}

//
// add x frames to timer counter
//
// frames = number of audio frames
// rate = audio sampling rate
//
void TIMEREMU_ElapseFrames( double frames, double rate ) {
	counter += frames * timerhz/rate;
	if( counter > boundary ) {
		counter -= boundary;

		if( irq )
			need_update = true;
	}
}

// 
// update timer interrupt function
// if timer had overflowed
//
void TIMEREMU_TryUpdate() {
	if( need_update ) {
		// update...
		Player_OnTick();
	}
}

// 
// checks if the timer is enabled
// 
bool TIMEREMU_IsActive() {
	return active;
}
