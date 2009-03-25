#include "stdint.h"

// a very elegant code

#ifndef __PLAYER_H
#define __PLAYER_H

typedef struct t_VoiceData {
	uint8_t		Note;
	uint8_t		Instrument;
	uint8_t		VolumeCommand;
	uint8_t		Command;
	uint8_t		CommandParam;
	uint16_t	Pitch;
	uint8_t		Volume;
	uint8_t		ChannelVolume;

	uint8_t		CommandMemory[16];
} VoiceData;

typedef struct t_PlayerData {
	uint8_t		Tick;
	uint8_t		Row;
	uint8_t		Position;
	uint8_t		Tempo;
	uint8_t		Speed;
	uint8_t		Active;

	uint24_t	eBank;
	uint24_t	iBank;
} PlayerData;

enum {
	MH_InitialTempo	=0,
	MH_InitialSpeed	=1,
	MH_Sequence
	
} ModuleHeaderDefinitions;

#endif
