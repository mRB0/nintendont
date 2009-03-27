#include "emu_spc.h"

static const double sclk = 32000;

static uint8_t PORTS_IN[4] = {0};
static uint8_t PORTS_OUT[4] = {0};

static uint8_t comms_v = 0;

static uint8_t memory[65536];

typedef struct {
	uint16_t origin;
	uint16_t loop;
} Sample;

static Sample SampleDirectory[64];
static int NextSample = 0;
static uint16_t TransferAddress = 0x600;

static int8_t MVOL;
static int8_t MVOLR;
static int8_t EVOL;
static int8_t EVOLR;
static uint16_t EDL;
static uint16_t ESA;
static int8_t EFB;
static int8_t COEF[8];
static uint8_t EON;
static bool EnableEcho;

static bool RELEASED;

static uint8_t KOF_BITS;
static uint8_t PAN_BITS;
static uint8_t PITCH_BITS;
static uint8_t UPDATE_DSP;


static uint8_t VOICE_VOLUME[8];
static uint8_t VOICE_KEYON[8];
static uint8_t VOICE_PANNING[8];
static uint16_t VOICE_PITCH[8];
static uint8_t VOICE_OFFSET[8];

static int mode;

enum {
	MODE_IDLE,
	MODE_TRANSFER
};

class DSPVOICE {

private:
	int8_t VOLUME_LEFT; // -128..127
	int8_t VOLUME_RIGHT; // -128..127
	uint8_t GAIN; // 0..127
	
	uint16_t PITCH; // 4.10 fixed

	int16_t sample_buffer[16];
	int32_t source;
	int32_t sourceloop;

	int32_t sampler; // 2.12 fixed

	int16_t a, b;

	bool lastblock;
	bool mute;

	bool LOOPING;
	bool ENDING;

	void DECOMPRESS_BLOCK();

public:
	void KEYON( int, int, int, int );
	void SET_VOLUME( int, int );
	void SET_PITCH( int );
	void SET_GAIN( int );

	void MIX( int32_t*, int );
	void INIT() {
		mute = true;
	}
};

static DSPVOICE VOICES[8];

static const int BUFFER_SIZE = 1024;
int16_t BUFFER_L[BUFFER_SIZE];
int16_t	BUFFER_R[BUFFER_SIZE];
int BUFFER_WRITE;
int BUFFER_READ;
int BUFFER_REMAINING;
int BUFFER_SAMPLER; // 16.16 fixed
int32_t MIXING_BUFFER[BUFFER_SIZE*2];

void TransferSampleStart();
void ContinueSampleTransfer();

void SPCEMU_WRITEPORT( int index, uint8_t value ) {
	PORTS_IN[index] = value;
	
	// received message?
	if( PORTS_IN[1] != comms_v ) {

		RELEASED = false;

		comms_v = PORTS_IN[1];
		
		switch( mode ) {
		case MODE_IDLE:
			
			switch( PORTS_IN[0] ) {

			case 0x00:	// LOAD

				TransferSampleStart();
				break;

			case 0x01:	// MVOL
				
				MVOL = PORTS_IN[2];
				MVOLR = PORTS_IN[3];
				break;
			case 0x02:	// EVOL
				
				EVOL = PORTS_IN[2];
				EVOLR = PORTS_IN[3];
				break;
			case 0x03:	// EDL
				
				EDL = PORTS_IN[3];
				ESA = (65536 - 0x100 * EDL) & 0xFFFF;
				if( ESA == 0 ) ESA = 65536 - 0x100;
				break;
			case 0x04:	// EFB

				EFB = PORTS_IN[3];
				break;
			case 0x05:	// COEF

				COEF[PORTS_IN[2]] = PORTS_IN[3];
				break;
			case 0x06:	// EON
				EON = PORTS_IN[3];
				break;
			case 0x07:	// ECEN
				EnableEcho = !!PORTS_IN[3];
				break;
			case 0x08:	// RET
				RELEASED = true;
				break;
			case 0x09:	// RESET
				// ???
				break;
			case 0x0A:	// KOF
				KOF_BITS = 0;
				break;
			case 0x0B:	// OFS
				VOICE_OFFSET[PORTS_IN[2]] = PORTS_IN[3];
				break;
			case 0x10:	// PITCH
			case 0x11:	// PITCH
			case 0x12:	// PITCH
			case 0x13:	// PITCH
			case 0x14:	// PITCH
			case 0x15:	// PITCH
			case 0x16:	// PITCH
			case 0x17:	// PITCH

				PITCH_BITS |= 1 << (PORTS_IN[0]&0xF);
				VOICE_PITCH[PORTS_IN[0]&0xF] = PORTS_IN[2] | (PORTS_IN[3]<<8);
				UPDATE_DSP = 1;
				break;
			case 0x18:	// VOL
			case 0x19:	// VOL
			case 0x1A:	// VOL
			case 0x1B:	// VOL
			case 0x1C:	// VOL
			case 0x1D:	// VOL
			case 0x1E:	// VOL
			case 0x1F:	// VOL

				if( !(PORTS_IN[2] & 128) ) {
					VOICE_PANNING[(PORTS_IN[0]&0xF)-8] = PORTS_IN[2];
					PAN_BITS |= 1 << ((PORTS_IN[0]&0xF)-8);
					UPDATE_DSP = 1;
				}
				VOICE_VOLUME[(PORTS_IN[0]&0xF)-8] = PORTS_IN[3];
				break;
			case 0x20:	// KON
			case 0x21:	// KON
			case 0x22:	// KON
			case 0x23:	// KON
			case 0x24:	// KON
			case 0x25:	// KON
			case 0x26:	// KON
			case 0x27:	// KON

				VOICE_VOLUME[(PORTS_IN[0]&0xF)] = PORTS_IN[2];
				VOICE_KEYON[(PORTS_IN[0]&0xF)] = PORTS_IN[3] + 1;
				UPDATE_DSP = 1;
				break;
			}
			break;
		case MODE_TRANSFER:
			
			ContinueSampleTransfer();
			break;
		}
		
		PORTS_OUT[1] = comms_v;
	}
}

void TransferSampleStart() {
	SampleDirectory[NextSample].origin = TransferAddress;
	SampleDirectory[NextSample].loop = PORTS_IN[2] | (PORTS_IN[3] << 8);

	mode = MODE_TRANSFER;
}

void ContinueSampleTransfer() {
	memory[TransferAddress++] = PORTS_IN[2];
	memory[TransferAddress++] = PORTS_IN[3];
	if( !comms_v ) {
		mode = MODE_IDLE;
	}
}

void SPCEMU_INIT() {
	MVOL = 80;
	MVOLR = 80;
	EVOL = 0;
	EVOLR=  0;
	EDL = 0;
	ESA = 0;
	COEF[0] = 0;
	COEF[1] = 0;
	COEF[2] = 0;
	COEF[3] = 0;
	COEF[4] = 0;
	COEF[5] = 0;
	COEF[6] = 0;
	COEF[7] = 0;
	EON = 0;
	EnableEcho = false;
	RELEASED = true;

	KOF_BITS = 0;

	BUFFER_READ = 0;
	BUFFER_WRITE = 0;
	BUFFER_SAMPLER = 0;
	BUFFER_REMAINING = 0;

	for(int i = 0; i < 8; i++ ) {
		VOICES[i].INIT();
	}
}

uint8_t SPCEMU_READPORT( int index ) {
	return PORTS_OUT[index];
}

void PROCESS_KEYOFF() {
	if( !KOF_BITS ) return;

	for( int i = 0; i < 8; i++ ) {
		if( KOF_BITS & (1<<i) ) {
			VOICE_VOLUME[i] = 0;
		}
	}

	KOF_BITS = 0;
}

void DSPVOICE::DECOMPRESS_BLOCK() {
	uint8_t head = memory[source++];
	int range = head >> 4;
	int filter = (head >> 2) & 3;
	LOOPING = !!(head & 2);
	ENDING = !!(head & 1);
	
	switch( filter ) {
	case 0:
		for( int i = 0; i < 8; i++ ) {
			int D = ((memory[source] >> 4) ^ 8) - 8;
			D = (D << range) >> 1;
			D = int16_t(D<<1)>>1;
			sample_buffer[i<<1] = D;
			
			D = ((memory[source++] & 0xF) ^ 8) - 8;
			D = (D << range) >> 1;
			D = int16_t(D<<1)>>1;
			sample_buffer[(i<<1)+1] = D;
		}
		a = sample_buffer[15];
		b = sample_buffer[14];
		break;
	case 1:

		for( int i = 0; i < 8; i++ ) {
			int D = ((memory[source] >> 4) ^ 8) - 8;
			D = (D << range) >> 1;
			D = D + a + ((-a) >> 4);
			D = int16_t(D<<1)>>1;
			sample_buffer[i<<1] = D;
			b = a;
			a = D;
			
			D = ((memory[source++] & 0xF) ^ 8) - 8;
			D = (D << range) >> 1;
			D = D + a + ((-a) >> 4);
			D = int16_t(D<<1)>>1;
			sample_buffer[(i<<1)+1] = D;
			b = a;
			a = D;
		}
		break;
	case 2:

		for( int i = 0; i < 8; i++ ) {
			int D = ((memory[source] >> 4) ^ 8) - 8;
			D = (D << range) >> 1;
			D = D + (a<<1) + ((-((a<<1)+a))>>5) - b + (b>>4);
			D = D < -32768 ? -32768 : D;
			D = D > 32767 ? 32767 : D;
			D = int16_t(D<<1)>>1;
			sample_buffer[i<<1] = D;
			b = a;
			a = D;
			
			D = ((memory[source++] & 0xF) ^ 8) - 8;
			D = (D << range) >> 1;
			D = D + (a<<1) + ((-((a<<1)+a))>>5) - b + (b>>4);
			D = D < -32768 ? -32768 : D;
			D = D > 32767 ? 32767 : D;
			D = int16_t(D<<1)>>1;
			sample_buffer[(i<<1)+1] = D;
			b = a;
			a = D;
		}
		break;
	case 3:

		for( int i = 0; i < 8; i++ ) {
			int D = ((memory[source] >> 4) ^ 8) - 8;
			D = (D << range) >> 1;
			D = D + (a<<1) + ((-(a+(a<<2)+(a<<3)))>>6) - b + (((b<<1) + b)>>4);
			D = D < -32768 ? -32768 : D;
			D = D > 32767 ? 32767 : D;
			D = int16_t(D<<1)>>1;
			sample_buffer[i<<1] = D;
			b = a;
			a = D;
			
			D = ((memory[source++] & 0xF) ^ 8) - 8;
			D = (D << range) >> 1;
			D = D + (a<<1) + ((-(a+(a<<2)+(a<<3)))>>6) - b + (((b<<1) + b)>>4);
			D = D < -32768 ? -32768 : D;
			D = D > 32767 ? 32767 : D;
			D = int16_t(D<<1)>>1;
			sample_buffer[(i<<1)+1] = D;
			b = a;
			a = D;
		}
		break;
	}
	if( ENDING ) {
		source = sourceloop;
		if( !LOOPING ) {
			lastblock = true;
		}
	}
}

void DSPVOICE::KEYON( int gain, int src, int sloop, int soff ) {
	GAIN = gain;
	source = src;
	sourceloop = sloop;
	source += soff;
	sampler = 0;
	mute = false;
	lastblock = false;
	a = b = 0;
	
	DECOMPRESS_BLOCK();
}

void DSPVOICE::SET_VOLUME( int left, int right ) {
	VOLUME_LEFT = left;
	VOLUME_RIGHT = right;
}

void DSPVOICE::SET_PITCH( int pitch ) {
	PITCH = pitch;
}

void DSPVOICE::SET_GAIN( int gain ) {
	GAIN = gain;
}

void DSPVOICE::MIX( int32_t *target, int length ) {
	if( mute )
		return;
	for( int i = 0; i < length; i++ ) {
		
		int S = sample_buffer[sampler >> 12];
		
		int SL = S * VOLUME_LEFT >> 6;
		int SR = S * VOLUME_RIGHT >> 6;
		SL = (SL * GAIN) >> 7;
		SR = (SR * GAIN) >> 7;
		
		sampler += PITCH;
		
		target[i*2] += SL;
		target[i*2+1] += SR;
		
		if( sampler >= (16 << 12) ) {
			if( lastblock ) {
				mute = true;
				break;
			}
			sampler -= (16 << 12);
			DECOMPRESS_BLOCK();
		}
	}
}

void PROCESS_DSP_UPDATE() {
	if( !UPDATE_DSP ) return;
	
	for( int i = 0; i < 8; i++ ) {

		if( VOICE_KEYON[i] ) {

			// reset brr

			VOICES[i].KEYON(
				VOICE_VOLUME[i],
				SampleDirectory[VOICE_KEYON[i]-1].origin, 
				SampleDirectory[VOICE_KEYON[i]-1].loop,
				VOICE_OFFSET[i] * 144
			);
		}

		if( PAN_BITS & (1<<i) ) {
			VOICES[i].SET_VOLUME( 127 - VOICE_PANNING[i], VOICE_PANNING[i] );
		}

		if( PITCH_BITS & (1<<i) ) {
			VOICES[i].SET_PITCH( VOICE_PITCH[i] );
		}
		
	}
	
	PAN_BITS = 0;
	PITCH_BITS = 0;
	UPDATE_DSP = 0;
}

void PROCESS_VOLUME() {
	for( int i = 0; i < 8; i++ ) {
		
		VOICES[i].SET_GAIN( VOICE_VOLUME[i] );
	}
}


void PROCESS_SOUND( int samples ) {

	// clear mixing buffer
	for( int i = 0; i < samples*2; i++ ) {
		MIXING_BUFFER[i] = 0;
	}
	
	for( int i = 0; i < 8; i++ ) {
		VOICES[i].MIX( MIXING_BUFFER, samples );
	}

	for( int i = 0; i < samples; i++ ) {
		int S = MIXING_BUFFER[i*2];
		S = (S * MVOL) >> 7;
		S = S < -32768 ? -32768 : S;
		S = S >  32767 ?  32767 : S;
		
		BUFFER_L[BUFFER_WRITE] = (int16_t)S;
		
		S = MIXING_BUFFER[i*2+1];
		S = (S * MVOLR) >> 7;
		S = S < -32768 ? -32768 : S;
		S = S >  32767 ?  32767 : S;
		
		BUFFER_R[BUFFER_WRITE++] = (int16_t)S;
		BUFFER_WRITE &= BUFFER_SIZE-1;
	}
}

static int space_remaining_in_buffer() {
	return ( BUFFER_WRITE <= BUFFER_READ ) ?
		(BUFFER_WRITE - BUFFER_READ + BUFFER_SIZE) :
		(BUFFER_WRITE - BUFFER_READ);
}

static int samples_remaining_in_buffer() {
	int a = BUFFER_WRITE - BUFFER_READ;
	if( a < 0 ) a += BUFFER_SIZE;
	return a;
}

static inline bool sampler_within_buffer( ) {
	int bf = BUFFER_SAMPLER >> 16;
	return ( BUFFER_WRITE < BUFFER_READ ) ?
		((bf >= BUFFER_READ) || (bf < BUFFER_WRITE)) :
		((bf >= BUFFER_READ) && (bf < BUFFER_WRITE));
}

static void refill_buffer( int desired ) {

	BUFFER_READ = BUFFER_WRITE;
	//int not_satisfied = 0;
	int srib = space_remaining_in_buffer();
	int amount_filled = desired;
	if( amount_filled > srib )
		amount_filled = srib;
	//	not_satisfied = samples - srib;
	//} else {
//		not_satisfied = 0;
//	}
	
	BUFFER_REMAINING += amount_filled << 16;
	PROCESS_SOUND( amount_filled );
//	return not_satisfied;
}

void SPCEMU_RUN( int frames, int16_t *buffer, double framerate ) {
	if( !RELEASED ) {
		// assert!
		
	}
	
	PROCESS_KEYOFF();
	PROCESS_DSP_UPDATE();
	PROCESS_VOLUME();
	
	//int samples_needed = (int)(sclk * frames / framerate) + 4;
	//samples_needed -= samples_remaining_in_buffer();
	//samples_needed = refill_buffer(samples_needed);
	
	int fstep = (int)((sclk*65536)/framerate);
	
	for( int frame = 0; frame < frames; frame++ ) {
		/*
		if( sampler_within_buffer() ) {
			buffer[frame*2] = BUFFER_L[BUFFER_SAMPLER >> 16];
			buffer[frame*2+1] = BUFFER_R[BUFFER_SAMPLER >> 16];
			BUFFER_SAMPLER += fstep;
			BUFFER_SAMPLER &= (BUFFER_SIZE<<16) - 1;
		} else {
			refill_buffer( (int)((sclk * (frames - frame)) / framerate) + 4);
		}
		*/
		if( BUFFER_REMAINING <= 0 ) {
			refill_buffer( (int)((sclk * (frames - frame)) / framerate) + 4);
		}
		
		buffer[frame*2] = BUFFER_L[BUFFER_SAMPLER >> 16];
		buffer[frame*2+1] = BUFFER_R[BUFFER_SAMPLER >> 16];
		BUFFER_SAMPLER += fstep;
		BUFFER_REMAINING -= fstep;
		BUFFER_SAMPLER &= (BUFFER_SIZE<<16) - 1;
	}
}
