#include "player.h"

/////////////////////////////////////////
#include "emu_timer.h"
#include "emu_mem.h"
/////////////////////////////////////////


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
	EBANK_SAMPLE_TABLE =0x400
};

ChannelData Channels[11];

static uint8_t ModTick;
static uint8_t ModRow;
static uint8_t ModPosition;

static uint8_t ModTempo;
static uint8_t ModSpeed;
static uint8_t ModActive;

static uint8_t TimerActive;
static uint16_t TimerReload;

static rom uint8_t *IBank;

static uint24_t	EModAddress;

/*************************************************************************
 * Player_Init
 *
 * Setup system
 *************************************************************************/
void Player_Init() {
	
}

/*************************************************************************
 * Player_Reset
 *
 * Stop playback and reset data.
 *************************************************************************/
void Player_Reset() {
	// Reset voices
	uint8_t i, j;
	
	ModActive = 0;
	for( i = 0; i < 11; i++ ) {
		Channels[i].Note			= 0;
		Channels[i].Instrument		= 0;
		Channels[i].VolumeCommand	= 0;
		Channels[i].Command			= 0;
		Channels[i].CommandParam	= 0;
		Channels[i].Pitch			= 0;
		Channels[i].Volume			= 0;
		Channels[i].VolumeScale		= 0;
		
		for( j = 0; j < 16; j++ ) {
			Channels[i].CommandMemory[j] = 0;
		}
	}
}

/*************************************************************************
 * Player_ChangePosition
 *
 * Change the position in the sequence and start the pattern.
 * Handles +++ pattern skipping
 *************************************************************************/
void Player_ChangePosition( uint8_t NewPosition ) {
	ModPosition = NewPosition;
	
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

/*************************************************************************
 * Player_Start
 *
 * Start module playback
 *************************************************************************/
void Player_Start( uint8_t ModuleIndex ) {
	EModAddress = ReadEx16( EBANK_EMOD_TABLE + ModuleIndex*2 ) << 6;
	ModActive = 1;
	
	Player_SetTempo( ReadEx8( EModAddress + EMD_InitialTempo ) );
	ModSpeed = ReadEx8( EModAddress + EMD_InitialSpeed );
	
	
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
	
}
