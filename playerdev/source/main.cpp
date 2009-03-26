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
int16_t spc_buffer[65536];// = new int16_t[frames];

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
				//VRC6EMU_Run( updatelen, vrc6_buffer + writepos );
				frames_until_next -= updatelen;
				writepos += updatelen;
			}
		}
	} else {
		//SPCEMU_Run( updatelen, spc_buffer );
		//VRC6EMU_Run( updatelen, vrc6_buffer );
	}
	
	int16_t *output = (int16_t*)outputBuffer;

	// temporary clear until spc emulation is complete
	for(i=0;i<frames;i++)spc_buffer[i]=0;
	
	// mix output
	
	for( i = 0; i < frames; i++ ) {
		*output++ = saturate_i16( (vrc6_buffer[i] * amp_vrc6 + spc_buffer[i] * amp_spc) >> 8 );
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
	audio_rate = 22050;
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

	Player_Init();
	Player_Start(0);
	Player_StartTimer();

	printf( "Press key to terminate\n" );
	getchar();

	audio.stopStream();

	return 0;
}
