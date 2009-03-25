// .___________________________________________________________.
// ............MODULE PLAYER DEVELOPMENT FACILITY...............
// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#include <stdio.h>

#include "player.h"

// OUTPUT CODES
//int buttle( void *outputBuffer, 
//			void * , 
//			unsigned int frames, 
//			double ,
//			RtAudioStreamStatus ,
//			void * ) {

	//u32 i;
	
	//A2::SNES::EmuAPU( outputBuffer, 0, frames );
	//A2::SNES::Functions::EmuAPU( outputBuffer, 0, frames );
	//s16* output = (s16*)outputBuffer;
	//for( i = 0; i < frames; i++ ) {
	//	*output++ = i*3;
	//}
//	return 0;
//}


// SETUP CODES
//	RtAudio audio;

//	u8 *poops = new u8[66048];
//	FILE *f;
//	f = fopen( "test.spc", "rb" );
//	fread( poops, 66048, 1, f );
//	fclose(f);
//	A2::SNES::LoadSPCFile(poops);
//	delete[] poops; // ?
//	RtAudio::StreamParameters sp;
//	sp.deviceId = audio.getDefaultOutputDevice();
//	sp.nChannels = 2;
//	sp.firstChannel = 0;
//	unsigned int srate = 32000;
//	unsigned int frames=1024;

//	audio.openStream( &sp, NULL, RTAUDIO_SINT16, 
//						srate, &frames, &buttle );
//	audio.startStream();

int main( int argc, char *argv[] ) {

	int a = 5;
	printf( "Hello World!\n" );

	return 0;
}
