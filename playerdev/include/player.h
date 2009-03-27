#include "stdint.h"

// a very elegant code

#ifndef __PLAYER_H
#define __PLAYER_H

//-------------------------------------------------------------------------------
typedef struct t_ChannelData {
//-------------------------------------------------------------------------------
	uint8_t		Note;
	uint8_t		Instrument;
	uint8_t		VolumeCommand;
	uint8_t		Command;
	uint8_t		CommandParam;
	uint16_t	Pitch;
	uint8_t		Volume;
	uint8_t		VolumeScale;
	uint8_t		Panning;

	uint8_t		CommandMemory[16];
} ChannelData;

#ifdef __cplusplus
extern "C" {
#endif

void Player_Init();
void Player_Reset();
void Player_ChangePosition( uint8_t NewPosition );
void Player_StartTimer();
void Player_StopTimer();
void Player_SetTempo( uint8_t NewTempo );
void Player_Start( uint8_t ModuleIndex );
void Player_SetIBank( rom uint8_t *InternalBankAddress );
void Player_OnTick();

#ifdef __cplusplus
}
#endif

#endif
