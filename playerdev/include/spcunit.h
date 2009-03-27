#ifndef __SPCUNIT_H
#define __SPCUNIT_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************
 * SPCU_LOAD( LOOP )
 *
 * Start sample transfer
 *
 * LOOP : Loop start position of sample
 *
 * SPCU_TRANSFER( DATA, FINAL )
 *
 * Transfer sample data
 *
 * DATA : Next word of data
 * FINAL : If set, this is the last word,
 *         and the transfer will be terminated.
 **********************************************************/
void SPCU_LOAD( uint16_t );
void SPCU_TRANSFER( uint16_t, uint8_t );

/**********************************************************
 * SPCU_MVOL( LEFT, RIGHT )
 *
 * Set Main Volume
 *
 * LEFT : Left volume level (-128..127)
 * RIGHT: Right volume level (-128..127)
 **********************************************************/
void SPCU_MVOL( int8_t, int8_t );

/**********************************************************
 * SPCU_EVOL( LEFT, RIGHT )
 *
 * Set Echo Volume
 *
 * LEFT : Left volume level (-128..127)
 * RIGHT : Right volume level (-128..127)
 **********************************************************/
void SPCU_EVOL( int8_t, int8_t );

/**********************************************************
 * SPCU_EDL( DELAY )
 *
 * Set Echo Delay
 *
 * EDL : Delay size (0..15)
 * NOTE: Requires 2K*EDL bytes of spc memory.
 **********************************************************/
void SPCU_EDL( uint8_t );

/**********************************************************
 * SPCU_EFB( FEEDBACK )
 *
 * Set Echo Feedback.
 *
 * EFB : Echo feedback level (-128..127)
 **********************************************************/
void SPCU_EFB( int8_t );

/**********************************************************
 * SPCU_COEF( INDEX, VALUE )
 *
 * Set Echo FIR Coefficient
 *
 * INDEX : Index of coefficient (0..7) (8-tap)
 * VALUE : New coefficient value
 **********************************************************/
void SPCU_COEF( uint8_t, int8_t );

/**********************************************************
 * SPCU_EON( CHANNELS )
 *
 * Set Echo Enable Bits
 *
 * CHANNELS : Channel selection bits (low bit = channel 0)
 **********************************************************/
void SPCU_EON( uint8_t );

/**********************************************************
 * SPCU_ECEN( ENABLED )
 *
 * Set echo master enable
 *
 * ENABLED : 1 = enable echo
 **********************************************************/
void SPCU_EON( uint8_t );

/**********************************************************
 * SPCU_RET()
 *
 * Return to processing.
 * (Make sure to call after all messages are transferred)
 **********************************************************/
void SPCU_RET();

/**********************************************************
 * SPCU_RESET()
 *
 * Reset system. (clears memory)
 **********************************************************/
void SPCU_RESET();

/**********************************************************
 * SPCU_KOF( CHANNELS )
 *
 * 'Cut' channels.
 *
 * CHANNELS : Channel selection bits
 **********************************************************/
void SPCU_KOF( uint8_t );

/**********************************************************
 * SPCU_OFS( INDEX, OFFSET )
 *
 * Set Sample Offset. Will be applied next KeyOn.
 *
 * INDEX : Index of channel (0..7)
 * OFFSET : Sample offset (samples / 256)
 **********************************************************/
void SPCU_OFS( uint8_t, uint8_t );

/**********************************************************
 * SPCU_PITCH( INDEX, PITCH )
 *
 * Set channel pitch.
 *
 * INDEX : Index of channel (0..7)
 * PITCH : New pitch value (2.12 rate value)
 **********************************************************/
void SPCU_PITCH( uint8_t, uint16_t );

/**********************************************************
 * SPCU_VOL( INDEX, VOLUME, PANNING )
 *
 * Set channel volume/pitch.
 *
 * INDEX : Index of channel (0..7)
 * VOLUME : New volume value (0..127)
 * PANNING : New panning value (0..127) &128 = ignore!
 **********************************************************/
void SPCU_VOL( uint8_t, uint8_t, uint8_t );

/**********************************************************
 * SPCU_KON( INDEX, VOLUME, SOURCE )
 * 
 * Key on channel.
 *
 * INDEX : Channel index (0..7)
 * VOLUME : New volume level (0..127)
 * SOURCE : Sample Index (0..63)
 **********************************************************/
void SPCU_KON( uint8_t, uint8_t, uint8_t );

#ifdef __cplusplus
}
#endif

#endif
