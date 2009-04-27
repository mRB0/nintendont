#if 0
#include "player.h"

#ifdef __PLAYERDEV__
/////////////////////////////////////////
#include "emu_timer.h"
#include "emu_mem.h"
/////////////////////////////////////////
#else

#include <timers.h>

#endif

#include "ports.h"
#include "spcunit.h"
#include "vrc6.h"

extern const rom uint16_t spc_ftab[];
extern const rom uint16_t vrc6_ftab[];
extern const rom uint16_t timer_tab[];
extern const rom uint8_t lut_div3[];

const rom int8_t IT_FineSineData[] = {
	  0,  2,  3,  5,  6,  8,  9, 11, 12, 14, 16, 17, 19, 20, 22, 23,
	 24, 26, 27, 29, 30, 32, 33, 34, 36, 37, 38, 39, 41, 42, 43, 44,
	 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59,
	 59, 60, 60, 61, 61, 62, 62, 62, 63, 63, 63, 64, 64, 64, 64, 64,
	 64, 64, 64, 64, 64, 64, 63, 63, 63, 62, 62, 62, 61, 61, 60, 60,
	 59, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46,
	 45, 44, 43, 42, 41, 39, 38, 37, 36, 34, 33, 32, 30, 29, 27, 26,
	 24, 23, 22, 20, 19, 17, 16, 14, 12, 11,  9,  8,  6,  5,  3,  2,
	  0, -2, -3, -5, -6, -8, -9,-11,-12,-14,-16,-17,-19,-20,-22,-23,
	-24,-26,-27,-29,-30,-32,-33,-34,-36,-37,-38,-39,-41,-42,-43,-44,
	-45,-46,-47,-48,-49,-50,-51,-52,-53,-54,-55,-56,-56,-57,-58,-59,
	-59,-60,-60,-61,-61,-62,-62,-62,-63,-63,-63,-64,-64,-64,-64,-64,
	-64,-64,-64,-64,-64,-64,-63,-63,-63,-62,-62,-62,-61,-61,-60,-60,
	-59,-59,-58,-57,-56,-56,-55,-54,-53,-52,-51,-50,-49,-48,-47,-46,
	-45,-44,-43,-42,-41,-39,-38,-37,-36,-34,-33,-32,-30,-29,-27,-26,
	-24,-23,-22,-20,-19,-17,-16,-14,-12,-11, -9, -8, -6, -5, -3, -2
};

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

typedef struct {
	uint8_t y;
	uint8_t duration;
	int16_t delta;
} EnvelopeNode;

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

	IMOD_TABLE_START =6,

	INS_ENVELOPES	=0x0E
};

enum {
	INITIAL_SPC_VOLUME_L = 0x7F,
	INITIAL_SPC_VOLUME_R = 0x7F
};

enum {
	PITCH_MAX = 107<<6
};

extern ChannelData Channels[];

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

static uint8_t PatternJumpIndex;
static uint8_t PatternJumpEnable;

static uint8_t EON_BITS;
static uint8_t EON_DIRTY;
static uint8_t EFB_LEVEL;
static uint8_t EFB_DIRTY;

//
// temporary values for some module channel processing....
//

static uint8_t t_SampleOffset;
static uint8_t t_Volume;
static uint8_t t_Panning;
static uint16_t t_Pitch;

static uint8_t pattern_command_memory[11*8];
static uint8_t pattern_command_memory_len = 11*8;

static void UpdateChannels( void );
static void UpdateChannel( uint8_t, uint8_t );
static void ProcessVolumeCommand( ChannelData *ch );
static void ProcessCommand( uint8_t );
static void ProcessChannelAudio( uint8_t ch_index, uint8_t use_t );
static void StartNewNote( ChannelData *ch );
static void ReadPattern( void );
static void ResetVolume( ChannelData *ch );

/*************************************************************************
 * Player_Init
 *
 * Setup system
 *************************************************************************/
void Player_Init( void ) {
	SPCU_BOOT();
	
	SPCU_MVOL( INITIAL_SPC_VOLUME_L, INITIAL_SPC_VOLUME_R );
}

/*************************************************************************
 * Player_Reset
 *
 * Stop playback and reset data.
 *************************************************************************/
void Player_Reset( void ) {
	// Reset voices
	uint8_t i;//, j;
	
	ModActive = 0;
	for( i = 0; i < 11; i++ ) {
		//Channels[i].Note			= 0;
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
		Channels[i].VE_Tick			= 0;
//		Channels[i].PE_Y			= 0;
//		Channels[i].PE_Node			= 0;

		Channels[i].Fadeout			= 0;
	
	}
	for( i = 0; i < pattern_command_memory_len; i++ ) {
		pattern_command_memory[i] = 0;
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
	ports_flash_open( SequenceAddress + NewPosition );
	
	ModPosition = NewPosition;
	
	while( (patt = ports_flash_read()) == 254 ) {

		// skip +++ patterns
		ModPosition++;
	}

	if( patt == 255 ) {
		ModPosition = 0; //TODO: STOP PLAYBACK!!!!!!!!!??????
		patt = ports_flash_readimm( SequenceAddress + ModPosition );
	}
	
	// get pattern address
	PatternAddress = EModAddress + PatternTable[patt] + 4;
	PatternRows = ports_flash_readimm( PatternAddress - 2 );
	
	PatternJumpEnable = 0;
	ModTick = 0;
	ModRow = 0;

	ports_flash_close();
	
}

/*************************************************************************
 * Player_StartTimer
 *
 * Start playback timer
 *************************************************************************/
void Player_StartTimer( void ) {

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
void Player_StopTimer( void ) {
	
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
	TimerReload = timer_tab[NewTempo-32];

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
	int24_t length;
	uint16_t loop;

	// seek to sample in ebank
	ports_flash_open(EBANK_SAMPLE_TABLE+(index<<1));
	ports_flash_setaddr( ports_flash_read16() << 6 );

	// read length,loop
	length = ports_flash_read16();
	loop = ports_flash_read16();

	ports_flash_close();

	SPCU_LOAD( loop );

	if( !length )
		SPCU_TRANSFER( 0, 1 );

	while( length > 0 ) {
		uint16_t data;
		ports_flash_opencont();
		data = ports_flash_read16();
		ports_flash_close();
		length -= 2;
		SPCU_TRANSFER( data, length <= 0 );
	}
}

/*************************************************************************
 * Player_Start( INDEX )
 *
 * Start module playback
 *************************************************************************/
void Player_Start( uint8_t ModuleIndex ) {
	
	uint8_t i;

	SPCU_EVOL(0,0);
	SPCU_ECEN(0);
	SPCU_RESET();

	ports_flash_opencont();

	Module = (rom IModuleData*)(IBank + ports_flash_readimm16( EBANK_IMOD_TABLE + ModuleIndex*2 ));
	SampleTable = (rom uint16_t*)(ModuleAddr + IMOD_TABLE_START);
	InstrumentTable = SampleTable + Module->SampleCount;

	EModAddress = ports_flash_readimm16( EBANK_EMOD_TABLE + ModuleIndex*2 ) << 6;
	SequenceAddress = EModAddress + EMD_Sequence;
	ModActive = 1;
	
	Player_SetTempo( ports_flash_readimm( EModAddress + EMD_InitialTempo ) );
	ModSpeed = ports_flash_readimm( EModAddress + EMD_InitialSpeed );
	ModGlobalVolume = ports_flash_readimm( EModAddress + EMD_InitialVolume );
	
	for( i = 0; i < 11; i++ ) {
		Channels[i].VolumeScale = ports_flash_readimm( EModAddress + EMD_InitialChannelVolume + i );
	}

	for( i = 0; i < 11; i++ ) {
		Channels[i].Panning = ports_flash_readimm( EModAddress + EMD_InitialChannelPanning + i );
	}
	
	PatternCount = ports_flash_readimm( EModAddress + EMD_NumberOfPatterns );
	
	{ // setup PATTERN table
		uint24_t addr = EModAddress + EMD_Sequence + Module->SequenceLength;
		for( i = 0; i < PatternCount; i++ ) {
			PatternTable[i] = addr - EModAddress;
			addr += ports_flash_readimm16( addr ) + 4;
		}
	}

	ports_flash_close();
	
	{ // load SPC sampeels
		for( i = 9; i < Module->SampleCount; i++ ) {
			rom Sample *s = (rom Sample*)(ModuleAddr + SampleTable[i]);
			Player_LoadSample( s->SampleIndex );
		}
	}

	ports_flash_opencont();
	
	{ // setup SPC echo
		
		uint8_t edl = ports_flash_readimm( EModAddress + EMD_EchoDelay );
		EFB_LEVEL = ports_flash_readimm( EModAddress + EMD_EchoFeedback );
		ports_flash_close();
		SPCU_EDL( edl );
		SPCU_EFB( EFB_LEVEL );
		ports_flash_open( EModAddress + EMD_EchoFirCoefficients );
		ports_flash_close();
		for( i = 0; i < 8; i++ ) {
			uint8_t c;
			ports_flash_opencont();
			c = ports_flash_read();
			ports_flash_close();
			SPCU_COEF( i, c );
		}
		if( edl ) {
			uint8_t  evol, evolr;
			ports_flash_opencont();
			EON_BITS = ports_flash_readimm(EModAddress + EMD_EchoEnableBits);
			evol = ports_flash_readimm(EModAddress + EMD_EchoVolumeLeft);
			evolr = ports_flash_readimm(EModAddress + EMD_EchoVolumeRight);
			ports_flash_close();
			SPCU_EON( EON_BITS );
			SPCU_EVOL( evol, evolr );
			SPCU_ECEN( 1 );
		}
	}
	
	Player_ChangePosition( 0 );

	EON_DIRTY = 0;
	EFB_DIRTY = 0;
	
	SPCU_RET();
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
void Player_OnTick( void ) {

	// reset timer
	WriteTimer0( TimerReload );

	if( ModTick == 0 ) {
		ReadPattern();
	}

	UpdateChannels();

	// (update remaining instruments? or do that in UpdateChannel

	ModTick++;
	if( ModTick >= ModSpeed ) {
		ModTick = 0;

		if( PatternJumpEnable ) {
			Player_ChangePosition( PatternJumpIndex );
		} else {
			ModRow++;
			if( ModRow > PatternRows ) {
				Player_ChangePosition( ModPosition + 1 );
			}
		}
	}

	if( EON_DIRTY || EFB_DIRTY ) {
		if( EON_DIRTY )
			SPCU_EON(EON_BITS);
		if( EFB_DIRTY )
			SPCU_EFB(EFB_LEVEL);
		EON_DIRTY = EFB_LEVEL = 0;
		SPCU_RET();
	}
	
	//SPCU_RET();
}

static void UpdateChannels( void ) {
	uint8_t ch;
	for( ch = 0; ch < 11; ch++ ) {
		UpdateChannel( ch, !!(PatternUpdateFlags & (1<<ch)) );
	}
}

static void UpdateChannel( uint8_t ch_index, uint8_t new_data ) {
	ChannelData *ch = Channels + ch_index;

	if( new_data ) {
		if( ModTick == 0 ) {
			if( ch->Flags & CF_NOTE ) {
				
				if( ch->p_Note == 254 ) {
					// note cut
					ch->Flags &= ~CF_NOTE;
					ch->Volume = 0;
				} else if( ch->p_Note == 255 ) {
					// note off
					
					ch->Flags &= ~CF_NOTE;
					ch->FlagsH &= ~CFH_KEYON;
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
						rom Instrument* ins = (rom Instrument*)(ModuleAddr + InstrumentTable[ch->p_Instrument-1]);
						if( !(ins->SetPan & 128) ) {
							ch->Panning = ins->SetPan;
						}
					}

					if( ch->Sample ) {
						rom Sample *samp = (rom Sample*)(ModuleAddr + SampleTable[ch->Sample-1]);
						ch->Volume = samp->DefaultVolume;
						if( !(samp->SetPan & 128) ) {
							ch->Panning = samp->SetPan;
						}
					}
				}
				
				if( ch->Flags & (CF_NOTE|CF_INSTR) ) {
					ResetVolume( ch );
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
			ProcessCommand( ch_index );
		}
		ProcessChannelAudio( ch_index, 1 );
	} else {
		ProcessChannelAudio( ch_index, 0 );
	}
	
}

static void ResetVolume( ChannelData *ch ) {
	ch->Fadeout = 1024;

	// reset envelopes
	ch->VE_Node = 0;
	ch->VE_Tick = 0;
//	ch->PE_Node = 0;
//	ch->PE_Tick = 0;

	ch->Cmem = 0;

	// set keyon, clear fade
	ch->FlagsH |= CFH_KEYON;
	ch->FlagsH &= ~(CFH_FADE|CFH_DELAY);
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
	} else if( v < 105 ) {				// vol down
		if( ModTick != 0 ) {
			vt -= v - 95;
			ch->Volume = vt < 0 ? 0 : vt;
		}
	} else if( v >= 128 && v <= 192 ) {
		if( ModTick == 0 ) {
			ch->Panning = v - 128;
		}
	}
}

typedef void (*command_routine)(uint8_t);

static void Command_SetSpeed( uint8_t ch_index );
static void Command_PatternJump( uint8_t ch_index );
static void Command_PatternBreak( uint8_t ch_index );
static void Command_VolumeSlide( uint8_t ch_index );
static void Command_PitchSlideDown( uint8_t ch_index );
static void Command_PitchSlideUp( uint8_t ch_index );
static void Command_Glissando( uint8_t ch_index );
static void Command_Vibrato( uint8_t ch_index );
static void Command_Tremor( uint8_t ch_index );
static void Command_Arpeggio( uint8_t ch_index );
static void Command_VibratoVolumeSlide( uint8_t ch_index );
static void Command_GlissandoVolumeSlide( uint8_t ch_index );
static void Command_ChannelVolume( uint8_t ch_index );
static void Command_ChannelVolumeSlide( uint8_t ch_index );
static void Command_SampleOffset( uint8_t ch_index );
static void Command_PanningSlide( uint8_t ch_index );
static void Command_RetriggerNote( uint8_t ch_index );
static void Command_Tremolo( uint8_t ch_index );
static void Command_Extended( uint8_t ch_index );
static void Command_Tempo( uint8_t ch_index );
static void Command_FineVibrato( uint8_t ch_index );
static void Command_GlobalVolume( uint8_t ch_index );
static void Command_GlobalVolumeSlide( uint8_t ch_index );
static void Command_SetPanning( uint8_t ch_index );
static void Command_Panbrello( uint8_t ch_index );
static void Command_Zxx( uint8_t ch_index );


static const rom command_routine command_list[] = {
	Command_SetSpeed,			// Axx
	Command_PatternJump,		// Bxx
	Command_PatternBreak,		// Cxx
	Command_VolumeSlide,		// Dxx
	Command_PitchSlideDown,		// Exx
	Command_PitchSlideUp,		// Fxx
	Command_Glissando,			// Gxx
	Command_Vibrato,			// Hxx
	Command_Tremor,				// Ixx
	Command_Arpeggio,				// Jxx
	Command_VibratoVolumeSlide,		// Kxx
	Command_GlissandoVolumeSlide,	// Lxx
	Command_ChannelVolume,			// Mxx
	Command_ChannelVolumeSlide,		// Nxx
	Command_SampleOffset,			// Oxx
	Command_PanningSlide,		// Pxx
	Command_RetriggerNote,		// Qxx
	Command_Tremolo,			// Rxx
	Command_Extended,			// Sxx
	Command_Tempo,				// Txx
	Command_FineVibrato,		// Uxx
	Command_GlobalVolume,		// Vxx
	Command_GlobalVolumeSlide,	// Wxx
	Command_SetPanning,			// Xxx
	Command_Panbrello,			// Yxx
	Command_Zxx					// Zxx

};

const rom uint8_t command_memory_map[] = {
	   0,  0,  0,  1,  2,  2,  3,  7,  0,
	// A   B   C   D   E   F   G   H   I
	   4,  1,  1,  0,  1,  5,  1,  8,  7,
	// J   K   L   M   N   O   P   Q   R
	   6,  0,  7,  0,  1,  0,  7,  0
	// S   T   U   V   W   X   Y   Z
};

enum {
	PCMDMEM_VOL,
	PCMDMEM_PSLIDE,
	PCMDMEM_GLIS,
	PCMDMEM_ARP,
	PCMDMEM_SOFS,
	PCMDMEM_EX,
	PCMDMEM_VIB,
	PCMDMEM_Q
};

static void ProcessCommandMemory( uint8_t ch_index ) {
	ChannelData *ch = Channels + ch_index;
	uint8_t p = ch->p_Parameter;
	uint8_t mi = command_memory_map[ ch->p_Command - 1 ];
	
	if( mi >= 7 ) {
		
		// double
		if( (p & 0xF) == 0 ) {
			p |= pattern_command_memory[(ch_index<<3) + mi - 1] & 0xF;
		} else {
			pattern_command_memory[(ch_index<<3) + mi - 1] =
				(pattern_command_memory[(ch_index<<3) + mi - 1] & 0xF0) | (p & 0xF);
		}
		
		if( (p & 0xF0) == 0 ) {
			p |= pattern_command_memory[(ch_index<<3) + mi - 1] & 0xF0;
		} else {
			pattern_command_memory[(ch_index<<3) + mi - 1] =
				(pattern_command_memory[(ch_index<<3) + mi - 1] & 0x0F) | (p & 0xF0);
		}
	} else if( mi >= 1 ) {
		// single
		if( p == 0 ) {
			p = pattern_command_memory[(ch_index<<3) + mi - 1];
		} else {
			pattern_command_memory[(ch_index<<3) + mi - 1] = p;
		}
	}
	ch->p_Parameter = p;
}

static void ProcessCommand( uint8_t ch_index ) {
	ChannelData *ch = Channels + ch_index;
	if( ch->p_Command >= 1 ) {

		if( ModTick == 0 )
			ProcessCommandMemory( ch_index );
		command_list[ch->p_Command-1]( ch_index );
	}
}

/*******************************************************************
 * Get new pitch and ...
 *
 *
 *******************************************************************/
static void StartNewNote( ChannelData *ch ) {
	
	// get new pitch
	ch->Pitch = ch->p_Note << 6;
	
	// get sample# (if instrument exists)
	if( ch->p_Instrument ) {
		rom Instrument *ins = (rom Instrument*)(ModuleAddr + InstrumentTable[ch->p_Instrument-1]);
		ch->Sample = ins->SampleIndex + 1;
	}

	ch->FlagsH |= CFH_START;
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
	rom Sample *samp;
	rom Instrument *ins;
	uint8_t VEV;//, PEV; //todo:panning envelope
	uint8_t vol = 0;
	if( ch->Sample ) samp = (rom Sample*)(ModuleAddr + SampleTable[ch->Sample-1]);
	if( ch->p_Instrument ) ins = (rom Instrument*)(ModuleAddr + InstrumentTable[ch->p_Instrument-1]);
	
	if( ch->p_Instrument ) {
		// process envelopes
		if( ins->V_Length ) {
			EnvelopeNode *env = (EnvelopeNode*)(((uint8_t*)ins) + INS_ENVELOPES);
			env += ch->VE_Node;
			// Process volume envelope
			if( ch->VE_Tick == 0 ) {
				// tick0 processing
				ch->VE_Y = env->y << 8;
					
			} else {
				ch->VE_Y += *(((int16_t*)(env))+1);

				// clamp value under zero (unsigned overflow)
				if( ch->VE_Y >= 49152 ) ch->VE_Y = 0;

				// clamp value above 64.0
				if( ch->VE_Y > 16384 ) ch->VE_Y = 16384;
			}

			VEV = ch->VE_Y >> 8;
			
			if( !( ch->VE_Node == ins->V_Sustain && (ch->FlagsH & CFH_KEYON)) ) {
			
				ch->VE_Tick++;
				if( ch->VE_Tick >= (env->duration+1) ) {
					ch->VE_Tick = 0;
					
					
					if( ch->VE_Node == ins->V_LoopEnd ) {
						// loop
						ch->VE_Node = ins->V_LoopStart;
						ch->VE_Tick = 0;

						if( !(ch->FlagsH & CFH_KEYON) ) {
							ch->FlagsH |= CFH_FADE;
						}
					} else if( ch->VE_Node == (ins->V_Length - 1) ) {
						// final node...
						// do something?
						ch->VE_Tick = 0;

						if( !(ch->FlagsH & CFH_KEYON) ) {
							ch->FlagsH |= CFH_FADE;
						}
					} else {
						ch->VE_Node++;
					}
				}
			}
		} else {
			VEV = 64;
		}
	} else {
		VEV = 64;
	}

	if( ch->FlagsH & CFH_FADE ) {
		ch->Fadeout -= ins->Fadeout;
		if( ch->Fadeout > 1024 ) ch->Fadeout = 0;
	}

	if( !(ch->FlagsH & CFH_DELAY) ) {
		
		// set volume:
		if( ch->p_Instrument ) {
			
			uint16_t vol16;
			// volume - r=6bit+
			vol = use_t ? t_Volume : ch->Volume;
			
			// *CV - r=7bit+
			vol = (vol * ch->VolumeScale) >> 5;
			
			// *SV - r=7bit+
			if( ch->Sample ) {
				vol = (vol * samp->GlobalVolume) >> 6;
			}
			
			// *IV - r=7bit+
			if( ch->p_Instrument ) {
				vol = (vol * ins->GlobalVolume) >> 7;
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
			if( ch_index < 3 ) rpitch -= 768; // adjust vrc6 channels
			oct = lut_div3[(rpitch >> 8)];
			f = rpitch - ((uint16_t)(oct*3) << 8);
			if( ch_index >= 3 ) { // SPC
				uint16_t spc_pitch = spc_ftab[f] >> (8-oct);
				SPCU_PITCH( ch_index-3, spc_pitch );
			} else { // VRC6
				uint16_t vrc6_pitch = vrc6_ftab[f] >> oct;
				VRC6_SetPitch( ch_index, vrc6_pitch );
			}
		}

		if( ch->FlagsH & CFH_START ) {
			ch->FlagsH &= ~CFH_START;
			if( ch_index > 2 ) {

				if( t_SampleOffset && use_t )
					SPCU_OFS( ch_index-3, t_SampleOffset );
				SPCU_KON( ch_index-3, vol, ch->Sample - 10 );
			}
		}


		if( ch_index > 2 ) SPCU_RET();
	}
}


/********************************************************************
 * Read pattern data into channels
 * Sets PatternUpdateFlags for whichever channels have data
 ********************************************************************/
static void ReadPattern( void ) {
	uint8_t channelvar;
	
	PatternUpdateFlags = 0;
	
	ports_flash_open(PatternAddress
		+1); // skip hints
	
	while( channelvar = ports_flash_read() ) {
		uint8_t channel = (channelvar-1) & 63;
		uint8_t maskvar;

		
		PatternAddress++;
		PatternUpdateFlags |= 1 << channel;
		
		if( channelvar & 128 ) {
			maskvar = Channels[channel].p_MaskVar = ports_flash_read();
			PatternAddress++;
		} else {
			maskvar = Channels[channel].p_MaskVar;
		}
		
		if( maskvar & 1 ) {
			Channels[channel].p_Note = ports_flash_read();
			PatternAddress++;
		}
		if( maskvar & 2 ) {
			Channels[channel].p_Instrument = ports_flash_read();
			PatternAddress++;
		}
		if( maskvar & 4 ) {
			Channels[channel].p_VolumeCommand = ports_flash_read();
			PatternAddress++;
		}
		if( maskvar & 8 ) {
			Channels[channel].p_Command = ports_flash_read();
			Channels[channel].p_Parameter = ports_flash_read();
			PatternAddress+=2;
		}
		Channels[channel].Flags = maskvar >> 4;
	}

	PatternAddress = ports_flash_tell();
	ports_flash_close();
}

/************************************************************
 *
 * Pattern Commands
 *
 ************************************************************/

static uint8_t DoVolumeSlide( uint8_t base, uint8_t param, uint8_t max ) {
	uint8_t a = param>>4, b = param&0xF;
	if( b == 0 ) { // Dx0
		if( ModTick != 0 || (a == 0xF) ) {
			base += a;
			if( base > max ) base = max;
		}
	} else if( a == 0 ) { // D0x
		if( ModTick != 0 || (b == 0xF) ) {
			base -= b;
			if( base > max ) base = 0;
		}
	} else if( b == 0xF ) { // DxF
		if( ModTick == 0 ) {
			base += a;
			if( base > max ) base = max;
		}
	} else if( a == 0xF ) { // DFx
		if( ModTick == 0 ) {
			base -= b;
			if( base > max ) base = 0;
		}
	}
	return base;
}

static void Command_SetSpeed( uint8_t ch_index ) {
	if( ModTick == 0 ) {
		if( Channels[ch_index].p_Parameter != 0 )
			ModSpeed = Channels[ch_index].p_Parameter;
	}
}

static void Command_PatternJump( uint8_t ch_index ) {
	if( ModTick == 0 ) {
		PatternJumpIndex = Channels[ch_index].p_Parameter;
		PatternJumpEnable = 1;
	}
}

static void Command_PatternBreak( uint8_t ch_index ) {
	if( ModTick == 0 ) {
		PatternJumpIndex = ModPosition + 1;
		PatternJumpEnable = 1;
	}
}

static void Command_VolumeSlide( uint8_t ch_index ) {
	ChannelData *ch = Channels+ch_index;
	ch->Volume = DoVolumeSlide( ch->Volume, ch->p_Parameter, 64 );
	t_Volume = ch->Volume;
}

static void Command_PitchSlideDown( uint8_t ch_index ) {
	ChannelData *ch = Channels+ch_index;
	uint8_t p = ch->p_Parameter;
	if( p >= 0xF0 ) { // fine slide
		if( ModTick == 0 ) {
			ch->Pitch -= (p & 0xF) << 2;
		}
	} else if( p >= 0xE0 ) { // extra fine slide
		if( ModTick != 0 ) {
			ch->Pitch -= (p & 0xF);
		}
	} else {
		if( ModTick != 0 ) {
			ch->Pitch -= p << 2;
		}
	}
	if( ch->Pitch > 16384 ) ch->Pitch = 0;
	t_Pitch = ch->Pitch;
}

static void Command_PitchSlideUp( uint8_t ch_index ) {
	ChannelData *ch = Channels+ch_index;
	uint8_t p = ch->p_Parameter;
	if( p >= 0xF0 ) { // fine slide
		if( ModTick == 0 ) {
			ch->Pitch += (p & 0xF) << 2;
		}
	} else if( p >= 0xE0 ) { // extra fine slide
		if( ModTick != 0 ) {
			ch->Pitch += (p & 0xF);
		}
	} else {
		if( ModTick != 0 ) {
			ch->Pitch += p << 2;
		}
	}
	if( ch->Pitch > (107<<6) ) ch->Pitch = 107<<6;
	t_Pitch = ch->Pitch;
}

static void Command_Glissando( uint8_t ch_index ) {

	if( ModTick != 0 ) {
		ChannelData *ch = Channels+ch_index;
		uint8_t p = ch->p_Parameter;

		uint16_t target_pitch = ch->p_Note << 6;

		if( ch->Pitch < target_pitch ) {
			ch->Pitch += ch->p_Parameter << 2;
			if( ch->Pitch > target_pitch ) {
				ch->Pitch = target_pitch;
			}
		} else if( ch->Pitch > target_pitch ) {
			ch->Pitch -= ch->p_Parameter << 2;
			if( ch->Pitch > 16384 ) ch->Pitch = 0;
			if( ch->Pitch < target_pitch ) ch->Pitch = target_pitch;
		}

		t_Pitch = ch->Pitch;
	}
}

static void Command_Vibrato( uint8_t ch_index ) {
	ChannelData *ch = Channels+ch_index;
	uint8_t p = pattern_command_memory[(ch_index<<3)+PCMDMEM_VIB];
	uint8_t x = p >> 4;
	uint8_t y = p & 0xF;

	ch->Cmem += x * 4;

	t_Pitch += (IT_FineSineData[ch->Cmem] * y) >> 4;
	if( t_Pitch > 16384 ) t_Pitch = 0;
	if( t_Pitch > PITCH_MAX ) t_Pitch = PITCH_MAX;
}

static void Command_Tremor( uint8_t ch_index ) {
	//todo
}

static void Command_Arpeggio( uint8_t ch_index ) {
	ChannelData *ch = Channels + ch_index;
	if( ModTick == 0 ) {
		ch->Cmem = 0;
	} else {
		ch->Cmem++;
		if( ch->Cmem == 3 ) ch->Cmem = 0;
	}
	if( ch->Cmem == 1 ) {
		t_Pitch += (ch->p_Parameter >> 4) << 6;
	} else if( ch->Cmem == 2 ) {
		t_Pitch += (ch->p_Parameter & 0xF) << 6;
	}
}

static void Command_VibratoVolumeSlide( uint8_t ch_index ) {
	ChannelData *ch = Channels+ch_index;
	Command_Vibrato( ch_index );

	ch->Volume = DoVolumeSlide( ch->Volume, ch->p_Parameter, 64 );
	t_Volume = ch->Volume;
}

static void Command_GlissandoVolumeSlide( uint8_t ch_index ) {
	//todo
}

static void Command_ChannelVolume( uint8_t ch_index ) {
	if( ModTick == 0 ) {
		uint8_t p = Channels[ch_index].p_Parameter;
		Channels[ch_index].VolumeScale = p > 64 ? 64 : p;
		
	}
}

static void Command_ChannelVolumeSlide( uint8_t ch_index ) {
	ChannelData *ch = Channels+ch_index;
	ch->VolumeScale = DoVolumeSlide( ch->VolumeScale, ch->p_Parameter, 64 );
}

static void Command_SampleOffset( uint8_t ch_index ) {
	if( ModTick == 0 )
		t_SampleOffset = Channels[ch_index].p_Parameter;
}

static void Command_PanningSlide( uint8_t ch_index ) {
	ChannelData *ch = Channels+ch_index;
	ch->Panning = DoVolumeSlide( ch->Panning, ch->p_Parameter, 64 );
}

static void Command_RetriggerNote( uint8_t ch_index ) {
	ChannelData *ch = Channels + ch_index;
	uint8_t x = ch->p_Parameter >> 4;
	uint8_t y = ch->p_Parameter & 0xF;
	if( x == 0 ) x = 1;
	if( ch->Cmem == 0 ) {
		ch->Cmem = x;
	} else {
		ch->Cmem--;
		if( ch->Cmem == 0 ) {
			// retrigget note...
			ch->Cmem = x;
			switch( y ) {
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				ch->Volume -= 1<<(y-1);
				if( ch->Volume > 64 ) ch->Volume = 0;
				break;
			case 6:
				ch->Volume = (ch->Volume * 170) >> 8;
				break;
			case 7:
				ch->Volume >>= 1;
				break;
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
				ch->Volume += 1<<(y-9);
				if( ch->Volume > 64 ) ch->Volume = 64;
				break;
			case 14:
				ch->Volume = (ch->Volume * 3) >> 1;
				if( ch->Volume > 64 ) ch->Volume = 64;
				break;
			case 15:
				ch->Volume <<= 1;
				if( ch->Volume > 64 ) ch->Volume = 64;
			}
			
			ch->FlagsH |= CFH_START;
		}
	}
}

static void Command_Tremolo( uint8_t ch_index ) {
	//todo
}

static void SCommand_Echo( uint8_t ch_index );
static void SCommand_Panning( uint8_t ch_index );
static void SCommand_NoteCut( uint8_t ch_index );
static void SCommand_NoteDelay( uint8_t ch_index );
static void SCommand_PatternDelay( uint8_t ch_index );
static void SCommand_FUNKREPEAT( uint8_t ch_index );
static void SCommand_Unsupported( uint8_t c ) {}

static const rom command_routine s_command_list[] = {
	SCommand_Echo,			// S0x
	SCommand_Unsupported,	// S1x
	SCommand_Unsupported,	// S2x
	SCommand_Unsupported,	// S3x
	SCommand_Unsupported,	// S4x
	SCommand_Unsupported,	// S5x
	SCommand_Unsupported,	// S6x
	SCommand_Unsupported,	// S7x
	SCommand_Panning,		// S8x
	SCommand_Unsupported,	// S9x
	SCommand_Unsupported,	// SAx
	SCommand_Unsupported,	// SBx
	SCommand_NoteCut,		// SCx
	SCommand_NoteDelay,		// SDx
	SCommand_PatternDelay,	// SEy
	SCommand_FUNKREPEAT,	// SFx
};

static void Command_Extended( uint8_t ch_index ) {
	s_command_list[Channels[ch_index].p_Parameter>>4](ch_index);
}

static void SCommand_Echo( uint8_t ch_index ) {
	if( ModTick == 0 ) {
		if( ch_index >= 3 ) {
			
			uint8_t p = Channels[ch_index].p_Parameter & 0xF;
			ch_index -= 3;
			if( p == 1 ) { // turn off echo per channel
				EON_BITS &= ~(1<<ch_index);
			} else if( p == 2 ) { // turn on echo per channel
				EON_BITS |= 1<<ch_index;
			} else if( p == 3 ) { // turn off echo for all channels
				EON_BITS = 0;
			} else if( p == 4 ) { // turn on echo for all channels
				EON_BITS = 0xFF;
			}
			EON_DIRTY = 1;
		}
	}
}

static void SCommand_Panning( uint8_t ch_index ) {
	if( ModTick == 0 ) {
		uint8_t p = (Channels[ch_index].p_Parameter & 0xF);
		Channels[ch_index].Panning = (p << 2) + (p >> 2);
	}
}

static void SCommand_NoteCut( uint8_t ch_index ) {
	if( ModTick == (Channels[ch_index].p_Parameter & 0xF) ) {
		Channels[ch_index].Volume = t_Volume = 0;
	}
}

static void SCommand_NoteDelay( uint8_t ch_index ) {
	ChannelData *ch = Channels + ch_index;
	uint8_t p = ch->p_Parameter & 0xF;
	if( p != 0 ) {
		if( ModTick < p ) {
			ch->FlagsH |= CFH_DELAY;
		} else {
			ch->FlagsH &= ~CFH_DELAY;
		}
	}
}

static void SCommand_PatternDelay( uint8_t ch_index ) {
	//todo
}

static void SCommand_FUNKREPEAT( uint8_t ch_index ) {
	if( ModTick == 0 ) {
		uint8_t p = Channels[ch_index].p_Parameter & 0xF;
		EFB_LEVEL = (p<<3) + (p>>1);
		EFB_DIRTY = 1;
	}
}

static void Command_Tempo( uint8_t ch_index ) {
	ChannelData *ch = Channels+ch_index;
	uint8_t p = ch->p_Parameter;
	if( p >= 0x20 ) {
		if( ModTick == 0 )
			Player_SetTempo( p );
	} else if( p >= 0x10 ) {
		if( ModTick != 0 ) {
			ModTempo += p & 0xF;
			if( ModTempo < 32 ) ModTempo = 255;
			Player_SetTempo( ModTempo );
		}
	} else {
		if( p == 0 ) return;
		if( ModTick != 0 ) {
			Player_SetTempo( ModTempo - p < 32 ? 32 : ModTempo - p );
		}
	}
}

static void Command_FineVibrato( uint8_t ch_index ) {
	ChannelData *ch = Channels+ch_index;
	uint8_t p = pattern_command_memory[(ch_index<<3)+PCMDMEM_VIB];
	uint8_t x = p >> 4;
	uint8_t y = p & 0xF;
	ch->Cmem += x * 4;
	t_Pitch += (IT_FineSineData[ch->Cmem] * y) >> 6;
	if( t_Pitch > 16384 ) t_Pitch = 0;
	if( t_Pitch > PITCH_MAX ) t_Pitch = PITCH_MAX;
}

static void Command_GlobalVolume( uint8_t ch_index ) {
	if( ModTick == 0 )
		ModGlobalVolume = Channels[ch_index].p_Parameter;
}

static void Command_GlobalVolumeSlide( uint8_t ch_index ) {
	ModGlobalVolume = DoVolumeSlide( ModGlobalVolume, Channels[ch_index].p_Parameter, 128 );
}

static void Command_SetPanning( uint8_t ch_index ) {
	if( ModTick == 0 ) {
		Channels[ch_index].Panning = Channels[ch_index].p_Parameter >> 2;
	}
}

static void Command_Panbrello( uint8_t ch_index ) {
	//todo
}

static void Command_Zxx( uint8_t ch_index ) {
	// do what?
}

#endif
