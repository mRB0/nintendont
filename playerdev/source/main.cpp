// .___________________________________________________________.
// ............MODULE PLAYER DEVELOPMENT FACILITY...............
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

// (vrc6bot emulator)

#include <stdio.h>

#include "stdint.h"
#include "player.h"
#include "RtAudio.h"

int AudioCallback( void *outputBuffer, void *,
				  unsigned int frames, double,
				  RtAudioStreamStatus, void * ) {
	uint32_t i;

	int16_t *output = (int16_t*)outputBuffer;
	for( i = 0; i < frames; i++ ) {
		*output++ = rand();
		*output++ = rand();
	}
	return 0;
}

int main( int argc, char *argv[] ) {

	int a = 5;
	printf( "Hello, world of vrc6bot!\n" );

	RtAudio audio;
	RtAudio::StreamParameters sp;
	sp.deviceId = audio.getDefaultOutputDevice();
	sp.nChannels = 2;
	sp.firstChannel = 0;
	uint32_t sampling_rate = 22050;
	uint32_t frames = 1024;

	audio.openStream(	
		&sp, 
		NULL, 
		RTAUDIO_SINT16,
		sampling_rate, 
		&frames, 
		AudioCallback
	);

	audio.startStream();

	printf( "Press key to terminate\n" );
	getchar();

	return 0;
}
