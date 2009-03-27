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
#include "emu_spc.h"

const int amp_spc = 256;//2048;
const int amp_vrc6 = 256;

static inline int saturate_i16( int value ) {
	if( value < -32768 ) return -32768;
	if( value > 32767 ) return 32767;
	return value;
}

uint32_t audio_rate;

int frames_until_next = 0;

int16_t vrc6_buffer[8192];// = new int16_t[frames];
int16_t spc_buffer[8192*2];// = new int16_t[frames];

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
				SPCEMU_RUN( updatelen, spc_buffer + writepos, audio_rate );
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
	
//	delete[] vrc6_buffer;
//	delete[] spc_buffer;
	UPDATING = false;
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

	SPCEMU_INIT();
	
	audio.startStream();

	{
		int cv = 0;
		
		SPCEMU_WRITEPORT(0,0x00);
		SPCEMU_WRITEPORT(2,0x00);
		SPCEMU_WRITEPORT(3,0x00);
		
		cv = 0x81;
		SPCEMU_WRITEPORT(1,cv);
		
		while( SPCEMU_READPORT(1) != cv ) {}
		
		FILE *f = fopen( "piano.brr", "rb" );

		/// load sample
		uint8_t a;
		while( !feof(f) ) {
			fread( &a, 1, 1, f );
			SPCEMU_WRITEPORT(2, a );

			fread( &a, 1, 1, f );
			SPCEMU_WRITEPORT(3, a );

			if( feof(f) ) {
				cv = 0;
				
			} else {
				cv ^= 0x80;
			}
			SPCEMU_WRITEPORT(1, cv);
			while(SPCEMU_READPORT(1) != cv) {} //(these are useless)
		}
		fclose(f);
		
		// SET M VOLUME
		SPCEMU_WRITEPORT(0, 0x01);
		SPCEMU_WRITEPORT(2, 0x7F);
		SPCEMU_WRITEPORT(3, 0x7F);
		cv = cv ^ 0x80;
		SPCEMU_WRITEPORT(1, cv);
		while(SPCEMU_READPORT(1) != cv) {}
		
		// set pitch
		SPCEMU_WRITEPORT(0, 0x10);
		SPCEMU_WRITEPORT(2, 0x00);
		SPCEMU_WRITEPORT(3, 0x10);
		cv = cv ^ 0x80;
		SPCEMU_WRITEPORT(1, cv);
		while(SPCEMU_READPORT(1) != cv) {}

		// set volume
		SPCEMU_WRITEPORT(0, 0x18);
		SPCEMU_WRITEPORT(2, 0x40);
		SPCEMU_WRITEPORT(3, 0x00);
		cv = cv ^ 0x80;
		SPCEMU_WRITEPORT(1, cv);
		while(SPCEMU_READPORT(1) != cv) {}

		// set keyon
		SPCEMU_WRITEPORT(0, 0x20);
		SPCEMU_WRITEPORT(2, 0x7f);
		SPCEMU_WRITEPORT(3, 0x00);
		cv = cv ^ 0x80;
		SPCEMU_WRITEPORT(1, cv);
		while(SPCEMU_READPORT(1) != cv) {}
	}


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
