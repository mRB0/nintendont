#include "player.h"

#define PLAYERDEV

#ifdef PLAYERDEV
/////////////////////////////////////////
#include "emu_timer.h"
#include "emu_mem.h"
/////////////////////////////////////////
#endif

#include "spcunit.h"
#include "vrc6.h"

extern const rom uint16_t spc_ftab[];
extern const rom uint16_t vrc6_ftab[];
extern const rom uint16_t timer_tab[];
extern const rom uint8_t lut_div3[];
/*
// very precise timer setting...
static const rom uint16_t timer_tab[] = {
    24576, 25818, 26986, 28087, 29128, 30112, 31044, 31928, 32768, 33568, 34329, 35055, 35747, 36409, 37043, 37649, 38230, 38787, 39322, 39836, 40330, 40806, 41264, 41705, 42131, 42541, 42938, 43321, 43691, 44049, 44396, 44731, 
    45056, 45372, 45677, 45974, 46261, 46541, 46812, 47076, 47332, 47581, 47824, 48060, 48290, 48514, 48732, 48945, 49152, 49355, 49552, 49745, 49933, 50116, 50296, 50471, 50642, 50809, 50973, 51133, 51290, 51443, 51593, 51739, 
    51883, 52024, 52162, 52297, 52429, 52559, 52686, 52811, 52933, 53053, 53171, 53287, 53400, 53512, 53621, 53728, 53834, 53937, 54039, 54139, 54237, 54334, 54429, 54522, 54614, 54704, 54793, 54880, 54966, 55051, 55134, 55216, 
    55296, 55376, 55454, 55531, 55607, 55681, 55755, 55827, 55899, 55969, 56039, 56107, 56174, 56241, 56306, 56371, 56434, 56497, 56559, 56620, 56680, 56740, 56798, 56856, 56913, 56970, 57025, 57080, 57134, 57188, 57241, 57293, 
    57344, 57395, 57446, 57495, 57544, 57593, 57641, 57688, 57735, 57781, 57826, 57871, 57916, 57960, 58004, 58047, 58089, 58131, 58173, 58214, 58255, 58295, 58335, 58374, 58413, 58452, 58490, 58527, 58565, 58601, 58638, 58674, 
    58710, 58745, 58780, 58815, 58849, 58883, 58917, 58950, 58983, 59016, 59048, 59080, 59111, 59143, 59174, 59205, 59235, 59265, 59295, 59325, 59354, 59383, 59412, 59440, 59468, 59496, 59524, 59551, 59579, 59606, 59632, 59659, 
    59685, 59711, 59737, 59762, 59788, 59813, 59838, 59862, 59887, 59911, 59935, 59959, 59983, 60006, 60029, 60052, 60075, 60098, 60120, 60143, 60165, 60187, 60208, 60230, 60251, 60273, 60294, 60315, 60335, 60356, 60376, 60396, 
};
*/
typedef struct {
	uint8_t SampleCount;
	uint8_t r1;
	uint8_t InstrumentCount;
	uint8_t r2;
	uint8_t	SequenceLength;
} IModuleData;

typedef struct {
	uint8_t DefaultVolume;
	uint8_t GlobalVolume;
	int16_t	PitchBase;
	uint16_t SampleIndex;
	uint8_t SetPan;
} Sample;

typedef struct {
	uint16_t Fadeout;
	uint8_t SampleIndex;
	uint8_t r1;
	uint8_t GlobalVolume;
	uint8_t SetPan;
	uint8_t V_Length;
	uint8_t V_Sustain;
	uint8_t V_LoopStart;
	uint8_t V_LoopEnd;
	uint8_t P_Length;
	uint8_t P_Sustain;
	uint8_t P_LoopStart;
	uint8_t P_LoopEnd;
} Instrument;

//---------------------------------------------------------------------
enum {
//---------------------------------------------------------------------
	EMD_InitialVolume			=0,
	EMD_InitialTempo			=1,
	EMD_InitialSpeed			=2,
	EMD_InitialChannelVolume	=3,
	EMD_InitialChannelPanning	=0xE,
	EMD_EchoVolumeLeft			=0x19,
	EMD_EchoVolumeRight			=0x1A,
	EMD_EchoDelay				=0x1B,
	EMD_EchoFeedback			=0x1C,
	EMD_EchoFirCoefficients		=0x1D,
	EMD_EchoEnableBits			=0x25,
	EMD_NumberOfPatterns		=0x26,
	EMD_Sequence				=0x27
//---------------------------------------------------------------------
} ExternalModuleDataStructure;
//---------------------------------------------------------------------

enum {
	EBANK_IMOD_TABLE =0,
	EBANK_EMOD_TABLE =0x200,
	EBANK_SAMPLE_TABLE =0x400,

	IMOD_TABLE_START =6
};

enum {
	INITIAL_SPC_VOLUME_L = 0x7F,
	INITIAL_SPC_VOLUME_R = 0x7F
};

ChannelData Channels[11];

static uint8_t ModTick;
static uint8_t ModRow;
static uint8_t ModPosition;

static uint8_t ModTempo;
static uint8_t ModSpeed;
static uint8_t ModActive;

static uint8_t ModGlobalVolume;

static uint8_t TimerActive;
static uint16_t TimerReload;

static rom uint8_t *IBank;

static rom IModuleData *Module;

#define ModuleAddr ((rom uint8_t*)Module)

static rom uint16_t *SampleTable;
static rom uint16_t *InstrumentTable;

static uint24_t	EModAddress;
static uint24_t SequenceAddress;

static uint8_t PatternCount;

static uint24_t PatternAddress;
static uint8_t PatternRows;

static uint16_t PatternUpdateFlags;

extern uint16_t PatternTable[];

//
// temporary values for some module channel processing....
//

static uint8_t t_SampleOffset;
static uint8_t t_Volume;
static uint8_t t_Panning;
static uint16_t t_Pitch;

static void UpdateChannels();
static void UpdateChannel( uint8_t );
static void ProcessVolumeCommand( ChannelData *ch );
static void ProcessCommand( ChannelData *ch );
static void ProcessChannelAudio( uint8_t ch_index, uint8_t use_t );
static void StartNewNote( ChannelData *ch );
static void ReadPattern();
static void ResetVolume( ChannelData *ch );

/*************************************************************************
 * Player_Init
 *
 * Setup system
 *************************************************************************/
void Player_Init() {
	SPCU_BOOT();
	
	SPCU_MVOL( INITIAL_SPC_VOLUME_L, INITIAL_SPC_VOLUME_R );
}

/*************************************************************************
 * Player_Reset
 *
 * Stop playback and reset data.
 *************************************************************************/
void Player_Reset() {
	// Reset voices
	uint8_t i;//, j;
	
	ModActive = 0;
	for( i = 0; i < 11; i++ ) {
		Channels[i].Note			= 0;
		Channels[i].Pitch			= 0;
		Channels[i].Volume			= 0;
		Channels[i].VolumeScale		= 0;
		Channels[i].Panning			= 0;
		Channels[i].Sample			= 0;

		Channels[i].p_Note			= 0;
		Channels[i].p_Instrument	= 0;
		Channels[i].p_VolumeCommand	= 0;
		Channels[i].p_Command		= 0;
		Channels[i].p_Parameter		= 0;
		Channels[i].p_MaskVar		= 0;
		Channels[i].Flags			= 0;
		Channels[i].FlagsH			= 0;


		Channels[i].VE_Y			= 0;
		Channels[i].VE_Node			= 0;
		Channels[i].PE_Y			= 0;
		Channels[i].PE_Node			= 0;

		Channels[i].Fadeout			= 0;
		
//		for( j = 0; j < 16; j++ ) {
//			Channels[i].CommandMemory[j] = 0;
//		}
	}
}

/*************************************************************************
 * Player_ChangePosition
 *
 * Change the position in the sequence and start the pattern.
 * Handles +++ pattern skipping
 *************************************************************************/
void Player_ChangePosition( uint8_t NewPosition ) {

	uint8_t patt;
	ModPosition = NewPosition;
	
	while( (patt = ReadEx8( SequenceAddress + ModPosition )) == 254 ) {

		// skip +++ patterns
		ModPosition++;
	}

	if( patt == 255 ) {
		// STOP PLAYBACK!!!!!!!!!
	}
	
	// get pattern address
	PatternAddress = EModAddress + PatternTable[patt] + 4;
	PatternRows = ReadEx8( PatternAddress - 2 );
	
	ModTick = 0;
	ModRow = 0;
	
}

/*************************************************************************
 * Player_StartTimer
 *
 * Start playback timer
 *************************************************************************/
void Player_StartTimer() {

	CloseTimer0();

	WriteTimer0( TimerReload );
	OpenTimer0( TIMER_INT_ON | T0_16BIT | T0_PS_1_64 );

	TimerActive = 1;
}

/*************************************************************************
 * Player_StopTimer
 *
 * Stop playback timer
 *************************************************************************/
void Player_StopTimer() {
	
	CloseTimer0();
	TimerActive = 0;
}

/*************************************************************************
 * Player_SetTempo
 *
 * Change playback timer frequency
 *************************************************************************/
void Player_SetTempo( uint8_t NewTempo ) {
	ModTempo = NewTempo;
	TimerReload = timer_tab[NewTempo];

	if( TimerActive ) {

		// TODO: adjust by time already elapsed
		WriteTimer0( TimerReload );

	}
}

//
// giev sample to spc
//
//
void Player_LoadSample( uint16_t index ) {
	uint24_t address;
	uint16_t length;
	uint16_t loop;
	address = ReadEx16(EBANK_SAMPLE_TABLE + (index << 1)) << 6;
	
	length = ReadEx16( address );
	address += 2;
	loop = ReadEx16( address );
	address += 2;

	SPCU_LOAD( loop );

	while( length > 0 ) {
		uint16_t data = ReadEx16( address );
		address += 2;
		length -= 2;
		SPCU_TRANSFER( data, length > 0 );
	}
}

/*************************************************************************
 * Player_Start( INDEX )
 *
 * Start module playback
 *************************************************************************/
void Player_Start( uint8_t ModuleIndex ) {
	
	uint8_t i;

	SPCU_MVOL(0,0);
	SPCU_ECEN(0);
	SPCU_RESET();

	Module = (rom IModuleData*)ReadEx16( EBANK_IMOD_TABLE + ModuleIndex*2 );
	SampleTable = (uint16_t*)(ModuleAddr + IMOD_TABLE_START);
	InstrumentTable = SampleTable + Module->SampleCount;

	EModAddress = ReadEx16( EBANK_EMOD_TABLE + ModuleIndex*2 ) << 6;
	SequenceAddress = EModAddress + EMD_Sequence;
	ModActive = 1;
	
	Player_SetTempo( ReadEx8( EModAddress + EMD_InitialTempo ) );
	ModSpeed = ReadEx8( EModAddress + EMD_InitialSpeed );
	ModGlobalVolume = ReadEx8( EModAddress + EMD_InitialVolume );
	
	Player_ChangePosition( 0 );
	
	for( i = 0; i < 11; i++ ) {
		Channels[i].Volume = ReadEx8( EModAddress + EMD_InitialChannelVolume + i );
	}

	for( i = 0; i < 11; i++ ) {
		Channels[i].Panning = ReadEx8( EModAddress + EMD_InitialChannelPanning + i );
	}
	
	PatternCount = ReadEx8( EModAddress + EMD_NumberOfPatterns );
	
	{ // setup PATTERN table
		uint24_t addr = EModAddress + EMD_Sequence + Module->SequenceLength;
		for( i = 0; i < PatternCount; i++ ) {
			PatternTable[i] = addr - EModAddress;
			addr += ReadEx16( addr ) + 4;
		}
	}
	
	{ // load SPC sampeels
		for( i = 9; i < Module->SampleCount; i++ ) {
			rom Sample *s = (rom Sample*)SampleTable[i];
			Player_LoadSample( s->SampleIndex );
		}
	}
	
	{ // setup SPC echo
		uint8_t edl = ReadEx8( EModAddress + EMD_EchoDelay );
		SPCU_EDL( edl );
		SPCU_EFB( ReadEx8( EModAddress + EMD_EchoFeedback ) );
		for( i = 0; i < 8; i++ ) {
			SPCU_COEF( i, EMD_EchoFirCoefficients + i );
		}
		if( edl ) {
			SPCU_EVOL(	ReadEx8( EModAddress + EMD_EchoVolumeLeft ),
						ReadEx8( EModAddress + EMD_EchoVolumeRight ) );
			SPCU_ECEN( 1 );
		}
	}
}

/*************************************************************************
 * Player_SetIBank
 *
 * Setup bank addresses.
 *************************************************************************/
void Player_SetIBank( rom uint8_t *InternalBankAddress ) {
	IBank = InternalBankAddress;
}

/*************************************************************************
 * Player_OnTick
 *
 * Update player routine (call every tick)
 *************************************************************************/
void Player_OnTick() {

	if( ModTick == 0 ) {
		ReadPattern();
	}

	UpdateChannels();

}

static void UpdateChannels() {
	uint8_t ch;
	for( ch = 0; ch < 11; ch++ ) {
		if( PatternUpdateFlags & (1<<ch) ) {
			UpdateChannel( ch );
		}
	}
}

static void UpdateChannel( uint8_t ch_index ) {
	ChannelData *ch = Channels + ch_index;
	if( ModTick == 0 ) {
		if( ch->Flags & CF_NOTE ) {
			
			if( ch->p_Note == 254 ) {
				// note cut
			} else if( ch->p_Note == 255 ) {
				// note off
			} else {
				uint8_t glissando = 0;
				if( ch->Flags & CF_CMD ) {
					if( ch->p_Command == 7 ) {
						// glissando, cancel note
						glissando = 1;
					}
				}
				
				if( glissando ) {
					
				} else {
					StartNewNote( ch );
				}
			}
			
			if( ch->Flags & CF_INSTR ) {
				if( ch->p_Instrument ) {
					rom Instrument* ins = (rom Instrument*)(IBank + InstrumentTable[ch->p_Instrument-1]);
					if( !(ins->SetPan & 128) ) {
						ch->Panning = ins->SetPan;
					}
				}

				if( ch->Sample ) {
					rom Sample *samp = (rom Sample*)(IBank + SampleTable[ch->Sample-1]);
					ch->Volume = samp->DefaultVolume;
					if( !(samp->SetPan & 128) ) {
						ch->Panning = samp->SetPan;
					}
				}
			}
			
			if( ch->Flags & (CF_NOTE|CF_INSTR) ) {
				ResetVolume( ch );
			}
			
			if( ch->Flags & (CF_NOTE) ) {
				if( ch->p_Note == 255 ) {
					ch->FlagsH &= ~CFH_KEYON;
				} else if( ch->p_Note == 254 ) {
					ch->Volume = 0;
				}
			}
			ch->Flags &= ~CF_NOTE;
		}
	}
	
	if( ch->Flags & CF_VCMD ) {
		ProcessVolumeCommand( ch );
	}
	
	// process commands.....
	t_Pitch = ch->Pitch;
	t_SampleOffset = 0;
	t_Volume = ch->Volume;
	t_Panning = ch->Panning;
	
	if( ch->Flags & CF_CMD ) {
		ProcessCommand( ch );
	}
	
	ProcessChannelAudio( ch_index, 1 );
}

static void ResetVolume( ChannelData *ch ) {
	ch->Fadeout = 1024;

	//TODO: reset envelopes

	// set keyon, clear fade
	ch->FlagsH |= CFH_KEYON;
	ch->FlagsH &= ~CFH_FADE;
}

static void ProcessVolumeCommand( ChannelData *ch ) {
	uint8_t v = ch->p_VolumeCommand;
	int8_t vt = ch->Volume;
	if( v < 65 ) {			// set volume
		ch->Volume = v;
	} else if( v < 75 ) {	// fine vol up
		if( ModTick == 0 ) {
			vt += v - 65;
			ch->Volume = vt > 64 ? 64 : vt;
		}
	} else if( v < 85 ) {	// fine vol down
		if( ModTick == 0 ) {
			vt -= v - 75;
			ch->Volume = vt < 0 ? 0 : vt;
		}
	} else if( v < 95 ) {	// vol up
		if( ModTick != 0 ) {
			vt += v - 85;
			ch->Volume = vt > 64 ? 64 : vt;
		}
	} else {				// vol down
		if( ModTick != 0 ) {
			vt -= v - 95;
			ch->Volume = vt < 0 ? 0 : vt;
		}
	}
}

static void ProcessCommand( ChannelData *ch ) {
	// ...............
}

/*******************************************************************
 * Get new pitch and 
 *
 *
 *******************************************************************/
static void StartNewNote( ChannelData *ch ) {
	
	// get new pitch
	ch->Pitch = ch->p_Note << 6;
	
	// get sample# (if instrument exists)
	if( ch->p_Instrument ) {
		rom Instrument *ins = (rom Instrument*)(ModuleAddr + InstrumentTable[ch->p_Instrument-1]);
		ch->Sample = ins->SampleIndex;
	}
}

/********************************************************************
 * Process all tick based stuff
 *
 * and update audio!
 *
 * use_t = replace channel data with 't' values
 ********************************************************************/
static void ProcessChannelAudio( uint8_t ch_index, uint8_t use_t ) {

	ChannelData *ch = Channels + ch_index;
	Sample *samp;
	Instrument *ins;
	uint8_t VEV, PEV;
	if( ch->Sample ) samp = (rom Sample*)(ModuleAddr + SampleTable[ch->Sample-1]);
	if( ch->p_Instrument ) ins = (rom Instrument*)(ModuleAddr + InstrumentTable[ch->p_Instrument-1]);
	
	VEV = 64;
	//TODO: process envelopes
	
	// set volume:
	if( ch->p_Instrument ) {
		
		// volume - r=6bit+
		uint8_t vol = use_t ? t_Volume : ch->Volume;
		uint16_t vol16;
		
		// *CV - r=7bit+
		vol = (vol * ch->VolumeScale) >> 5;
		
		// *SV - r=7bit+
		if( ch->Sample ) {
			vol = (vol * samp->GlobalVolume) >> 6;
		}
		
		// *IV - r=7bit+
		if( ch->p_Instrument ) {
			vol = (vol * samp->GlobalVolume) >> 6;
		}
		
		// *GV - r=14bit+
		vol16 = vol * ModGlobalVolume;
		
		// *VEV - r = 14bit+
		vol16 = (vol16 * VEV) >> 6;
		
		// *NFC - r = 7bit+
		vol = (vol16 * ch->Fadeout) >> (10+7);
		
		if( ch_index < 3 ) {
			VRC6_SetVolume( ch_index, vol >> 1, ch->Sample-1 );
		} else {

			// calculate panning
			uint8_t p = ch->Panning << 1;
			if( p == 128 ) p = 127;
			if( vol == 128 ) vol = 127;
			SPCU_VOL( ch_index-3, vol, p );
		}
	} else {
		// zero volume
		if( ch_index < 3 ) {
			VRC6_SetVolume( ch_index, 0, 0 );
		} else {
			SPCU_VOL( ch_index-3, 0, 128 );
		}
	}
	
	// set pitch:
	if( ch->Sample ) {
		int16_t rpitch, f;
		uint8_t oct;
		rpitch = (use_t ? t_Pitch : ch->Pitch) + samp->PitchBase;
		oct = lut_div3[(rpitch >> 8)];
		f = (uint16_t)(oct*30) << 8;
		if( ch_index >= 3 ) { // SPC
			uint16_t spc_pitch = spc_ftab[f] >> (8-oct);
			SPCU_PITCH( ch_index-3, spc_pitch );
		} else { // VRC6
			uint16_t vrc6_pitch = vrc6_ftab[f] >> oct;
			VRC6_SetPitch( ch_index, vrc6_pitch );
		}
	}
}


/********************************************************************
 * Read pattern data into channels
 * Sets PatternUpdateFlags for whichever channels have data
 ********************************************************************/
static void ReadPattern() {
	uint8_t channelvar;
	
	PatternUpdateFlags = 0;
	
	while( channelvar = ReadEx8n() ) {
		uint8_t channel = (channelvar-1) & 63;
		uint8_t maskvar;
		PatternUpdateFlags |= 1 << channel;
		
		if( channel & 128 ) {
			maskvar = Channels[channel].p_MaskVar = ReadEx8n();
		} else {
			maskvar = Channels[channel].p_MaskVar;
		}
		
		if( maskvar & 1 ) {
			Channels[channel].p_Note = ReadEx8n();
		}
		if( maskvar & 2 ) {
			Channels[channel].p_Instrument = ReadEx8n();
		}
		if( maskvar & 4 ) {
			Channels[channel].p_VolumeCommand = ReadEx8n();
		}
		if( maskvar & 8 ) {
			Channels[channel].p_Command = ReadEx8n();
			Channels[channel].p_Parameter = ReadEx8n();
		}
		Channels[channel].Flags = maskvar >> 4;
	}
}
