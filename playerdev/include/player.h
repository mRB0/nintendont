#include "stdint.h"

// a very elegant code

#ifndef __PLAYER_H
#define __PLAYER_H

//-------------------------------------------------------------------------------
typedef struct t_ChannelData {
//-------------------------------------------------------------------------------
	uint8_t		Note;			// note index
	uint16_t	Pitch;			// ?
	uint8_t		Volume;			// 0..64
	uint8_t		VolumeScale;	// 0..64
	uint8_t		Panning;		// 0..64
	uint8_t		Sample;			// 0..?
	
	uint8_t		p_Note;			// pattern data
	uint8_t		p_Instrument;	//
	uint8_t		p_VolumeCommand;//
	uint8_t		p_Command;		//
	uint8_t		p_Parameter;	//
	uint8_t		p_MaskVar;		//
	
	uint8_t		Flags;			// some flags
	uint8_t		FlagsH;			// some more flags
	
	uint16_t	VE_Y;			// volume envelope
	uint8_t		VE_Node;		//
	uint16_t	PE_Y;			//
	uint8_t		PE_Node;		//
	
	uint16_t	Fadeout;		// a fadeout level
	
} ChannelData;

enum {
	CF_NOTE		=1,
	CF_INSTR	=2,
	CF_VCMD		=4,
	CF_CMD		=8,
	CF_NOTEOFF	=16,
	CF_NOTECUT	=32
};

enum {
	CFH_KEYON	=1,
	CFH_FADE	=2
};

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
