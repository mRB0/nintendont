#include "player.h"

VoiceData Voices[11];
PlayerData Player;

/*************************************************************************
 * Player_Reset
 *
 * Stop playback and reset data.
 *************************************************************************/
void Player_Reset() {
	// Reset voices
	uint8_t i;
	uint8_t j;
	Player.Active = 0;
	for( i = 0; i < 11; i++ ) {
		Voices[i].Note = 0;
		Voices[i].Instrument = 0;
		Voices[i].VolumeCommand = 0;
		Voices[i].Command = 0;
		Voices[i].CommandParam = 0;
		Voices[i].Pitch = 0;
		Voices[i].Volume = 0;
		Voices[i].ChannelVolume = 0;

		for( j = 0; j < 16; j++ ) {
			Voices[i].CommandMemory[j] = 0;
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
	Player.Position = NewPosition;

}

/*************************************************************************
 * Player_StartTimer
 *
 * Start playback timer
 *************************************************************************/
void Player_StartTimer() {
	// TODO: start playback timer
}

/*************************************************************************
 * Player_StopTimer
 *
 * Stop playback timer
 *************************************************************************/
void Player_StopTimer() {
	// TODO: stop playback timer
}

/*************************************************************************
 * Player_SetTempo
 *
 * Change playback timer frequency
 *************************************************************************/
void Player_SetTempo( uint8_t Tempo ) {
	Player.Tempo = Tempo;
	
	// TODO: set timer frequency
}

/*************************************************************************
 * Player_Start
 *
 * Start module playback
 *************************************************************************/
void Player_Start( uint8_t ModuleIndex ) {
	Player.Active = 1;
	
	Player_SetTempo( ReadExternalByte( Player.eBank + MH_InitialTempo ) );
	Player.Speed = ReadExternalByte( Player.eBank + MH_InitialSpeed );
}

/*************************************************************************
 * Player_SetBank
 *
 * Setup bank addresses.
 *************************************************************************/
void Player_SetBank( uint24_t ExternalBankAddress, 
					 uint24_t InternalBankAddress ) {
	Player.eBank = ExternalBankAddress;
	Player.iBank = InternalBankAddress;
}
