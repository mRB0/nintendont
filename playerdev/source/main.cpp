// .___________________________________________________________.
// ............MODULE PLAYER DEVELOPMENT FACILITY...............
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// (vrc6bot emulator)

#include <stdio.h>

#include "stdint.h"
#include "player.h"
#include "RtAudio.h"

#include "player.h"
#include "emu_mem.h"
#include "emu_timer.h"
#include "emu_vrc6.h"

const int amp_spc = 256;
const int amp_vrc6 = 256;

static inline int saturate_i16( int value ) {
	if( value < -32768 ) return -32768;
	if( value > 32767 ) return 32767;
	return value;
}

uint32_t audio_rate;

int frames_until_next = 0;

int16_t vrc6_buffer[65536];// = new int16_t[frames];
int16_t spc_buffer[65536*2];// = new int16_t[frames];

int SL1=0, SL2=0;
int SR1=0, SR2=0;

enum {
	c1 = 192,
	c2 = 48,
	c3 = 16
};

int AudioCallback( void *outputBuffer, void *,
				  unsigned int frames, double,
				  RtAudioStreamStatus, void * ) {
	uint32_t i;

	uint32_t updatelen;
	
	int32_t writepos = 0;
	
	
	if( TIMEREMU_IsActive() ) {
		while( writepos < (int)frames ) {
			if( frames_until_next == 0 ) {
				TIMEREMU_TryUpdate();
				frames_until_next = TIMEREMU_FramesUntilNextPulse( audio_rate );
				TIMEREMU_ElapseFrames( frames_until_next, audio_rate );
			}

			if( frames_until_next < ((int)frames-writepos) ) {
				updatelen = frames_until_next;
			} else {
				updatelen = (frames-writepos);
			}

			if( updatelen ) {
				//SPCEMU_Run( updatelen, spc_buffer + writepos );
				VRC6EMU_RUN( updatelen, vrc6_buffer + writepos, audio_rate );
				frames_until_next -= updatelen;
				writepos += updatelen;
			}
		}
	} else {
		//SPCEMU_Run( frames, spc_buffer );
		VRC6EMU_RUN( frames, vrc6_buffer, audio_rate );
	}
	
	int16_t *output = (int16_t*)outputBuffer;

	// temporary clear until spc emulation is complete
	for(i=0;i<frames*2;i++)spc_buffer[i]=0;
	
	// mix output

	int S;
	
	for( i = 0; i < frames; i++ ) {

		S = (saturate_i16( (vrc6_buffer[i] * amp_vrc6 + spc_buffer[i*2  ] * amp_spc) >> 8 )) >> 4;
		S = (S * c1 + SL1 * c2 + SL2 * c3) >> 8;
		// left
		*output++ = S;
		SL2 = SL1;
		SL1 = S;

		S = (saturate_i16( (vrc6_buffer[i] * amp_vrc6 + spc_buffer[i*2+1] * amp_spc) >> 8 )) >> 4;
		S = (S * c1 + SR1 * c2 + SR2 * c3) >> 8;
		// right
		*output++ = S;
		SR2 = SR1;
		SR1 = S;
	}
	
//	delete[] vrc6_buffer;
//	delete[] spc_buffer;
	return 0;
}

int main( int argc, char *argv[] ) {
	
	int a = 5;
	if( argc < 3 )
		printf( "USAGE: VRC6BOT INPUT.IBANK INPUT.EBANK\n" );

	printf( "flashing memory...!\n" );

	// flash memory banks
	{
		FILE *f = fopen( argv[1], "rb" );
		fseek( f, 0, SEEK_END );
		int fsize = ftell(f);
		fseek( f, 0, SEEK_SET );
		fread( GetIBankPtr(), 1, fsize, f );
		fclose(f);
		
		f = fopen( argv[2], "rb" );
		fseek( f, 0, SEEK_END );
		fsize = ftell(f);
		fseek( f, 0, SEEK_SET );
		fread( GetEBankPtr(), 1, fsize, f );
		fclose(f);
	}
	
	printf( "Hello, world of vrc6bot!\n" );
	
	RtAudio audio;
	RtAudio::StreamParameters sp;
	sp.deviceId = audio.getDefaultOutputDevice();
	sp.nChannels = 2;
	sp.firstChannel = 0;
	audio_rate = 32000;
	uint32_t frames = 1024;
	
	audio.openStream(
		&sp, 
		NULL, 
		RTAUDIO_SINT16,
		audio_rate, 
		&frames, 
		AudioCallback
	);

	audio.startStream();

//	Player_Init();
//	Player_Start(0);
//	Player_StartTimer();

	printf( "Press key to terminate\n" );
	getchar();

	audio.stopStream();

	printf( "heh\n" );
	getchar();

	return 0;
}
