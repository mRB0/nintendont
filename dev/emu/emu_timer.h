#ifndef __EMU_TIMER_H
#define __EMU_TIMER_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

// TIMER EMULATION FUNCTIONS

enum {
	TIMER_INT_ON	=128,
	TIMER_INT_OFF	=0,
	T0_8BIT			=0,
	T0_16BIT		=64,
	T0_PS_1_1		=0,
	T0_PS_1_2		=1,
	T0_PS_1_4		=2,
	T0_PS_1_8		=3,
	T0_PS_1_16		=4,
	T0_PS_1_32		=5,
	T0_PS_1_64		=6,
	T0_PS_1_128		=7,
	T0_PS_1_256		=8
};

void OpenTimer0( uint8_t config );
void CloseTimer0();

void WriteTimer0( uint16_t value );
uint16_t ReadTimer0();

#ifdef __cplusplus
}

int TIMEREMU_FramesUntilNextPulse( double rate );
void TIMEREMU_ElapseFrames( double frames, double rate );
void TIMEREMU_TryUpdate();
bool TIMEREMU_IsActive();

#endif //c++ interface

#endif
