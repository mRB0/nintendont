#ifndef PORTS_H_
#define PORTS_H_

#define TRIS_VRC6_CE	TRISEbits.TRISE2
#define LAT_VRC6_CE		LATEbits.LATE2

#define TRIS_SPC_CE	TRISCbits.TRISC4
#define LAT_SPC_CE	LATCbits.LATC4

#define TRIS_SPC_RESET	TRISCbits.TRISC5
#define LAT_SPC_RESET	LATCbits.LATC5

#define TRIS_WE	TRISEbits.TRISE1
#define LAT_WE	LATEbits.LATE1

#define TRIS_OE	TRISEbits.TRISE0
#define LAT_OE	LATEbits.LATE0

#define TRIS_DATA	TRISD
#define LAT_DATA	LATD
#define PORT_DATA	PORTD

#define TRIS_INPUT (0x00)
#define TRIS_OUTPUT (0xff)

#endif
