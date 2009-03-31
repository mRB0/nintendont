// .___________________________________________________________.
// ............MODULE PLAYER DEVELOPMENT FACILITY...............
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// (vrc6bot emulator)

const char *USAGE = "\n"

"USAGE: vrc6bot [...OPTIONS...]\n\n"

"Required Options:\n"
"-b <input.ibank> <input.ebank> Specify sound banks.\n"
"-i <song_index> Specify which song to play\n\n"

"Optional Options:\n"
"-s <audio rate> Specify audio sampling rate [default=44100]\n";

#include <stdio.h>

#include "stdint.h"
#include "player.h"
#include "RtAudio.h"

#include "player.h"
#include "emu_mem.h"
#include "emu_timer.h"
#include "emu_vrc6.h"
#include "emu_spc.h"
#include "spcunit.h"

const int amp_spc = 128;
const int amp_vrc6 = 256;

static inline int saturate_i16( int value ) {
	if( value < -32768 ) return -32768;
	if( value > 32767 ) return 32767;
	return value;
}

uint32_t audio_rate = 44100;

int frames_until_next = 0;

int16_t vrc6_buffer[8192];
int16_t spc_buffer[8192*2];

int SL1=0, SL2=0;
int SR1=0, SR2=0;

enum {
	c1 = 256,
	c2 = 0,
	c3 = 0
};

bool UPDATING=false;

int AudioCallback( void *outputBuffer, void *,
				  unsigned int frames, double,
				  RtAudioStreamStatus, void * ) {

	if( UPDATING ) return frames;

	UPDATING = true;
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
				SPCEMU_RUN( updatelen, spc_buffer + writepos*2, audio_rate );
				VRC6EMU_RUN( updatelen, vrc6_buffer + writepos, audio_rate );
				frames_until_next -= updatelen;
				writepos += updatelen;
			}
		}
	} else {
		SPCEMU_RUN( frames, spc_buffer, audio_rate );
		VRC6EMU_RUN( frames, vrc6_buffer, audio_rate );
	}
	
	int16_t *output = (int16_t*)outputBuffer;

	// mix output

	int S;
	
	for( i = 0; i < frames; i++ ) {

		S = (saturate_i16( (vrc6_buffer[i] * amp_vrc6 + spc_buffer[i*2  ] * amp_spc) >> 8 ));
		S = (S * c1 + SL1 * c2 + SL2 * c3) >> 8;
		if( S < -32768 ) S = -32768;
		if( S > 32767 ) S = 32767;
		// left
		*output++ = S;
		SL2 = SL1;
		SL1 = S;

		S = (saturate_i16( (vrc6_buffer[i] * amp_vrc6 + spc_buffer[i*2+1] * amp_spc) >> 8 ));
		S = (S * c1 + SR1 * c2 + SR2 * c3) >> 8;
		if( S < -32768 ) S = -32768;
		if( S > 32767 ) S = 32767;
		// right
		*output++ = S;
		SR2 = SR1;
		SR1 = S;
	}
	
	UPDATING = false;
	return 0;
}

int main( int argc, char *argv[] ) {
	
	int a = 5;
	if( argc < 3 ) {
		printf( USAGE );
		return 0;
	}

	char *bank_i, *bank_e;
	int song_index;

	for( int arg = 1, m=0; arg < argc; arg++ ) {
		if( m==0 ) {
			if( argv[arg][0] == '-' ) {
				switch( argv[arg][1] ) {
					case 'b':
						m=1;
						break;
					case 'i':
						m=3;
						break;
					case 's':
						m=4;
						break;
				}
			}
		}else if( m == 1 ) {
			bank_i = argv[arg];
			m=2;
		}else if( m == 2 ){
			bank_e = argv[arg];
			m=0;
		}else if( m == 3 ) {
			song_index = atoi(argv[arg]);
			m=0;
		}else if( m == 4 ) {
			audio_rate = atoi(argv[arg]);
		}
	}
	
	printf( "flashing memory...!\n" );
	
	// flash rom banks
	FlashIbank( bank_i );
	FlashEbank( bank_e );
		
	printf( "Hello, world of vrc6bot!\n" );
	
	RtAudio audio;
	RtAudio::StreamParameters sp;
	sp.deviceId = audio.getDefaultOutputDevice();
	sp.nChannels = 2;
	sp.firstChannel = 0;
	uint32_t frames = 1024;
	
	audio.openStream(
		&sp, 
		NULL, 
		RTAUDIO_SINT16,
		audio_rate, 
		&frames, 
		AudioCallback
	);

	SPCEMU_INIT();
	
	audio.startStream();

	Player_Init();
	Player_SetIBank( GetIBankPtr() );
	
	
	
	Player_Start(song_index);
	Player_StartTimer();

	printf( "Press ENTER key to terminate\n" );
	getchar();

	audio.stopStream();

	printf( "heh\n" );
	getchar();

	return 0;
}
