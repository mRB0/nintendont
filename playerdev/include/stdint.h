#ifndef __STDINT_H
#define __STDINT_H
/*
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned short long uint24_t;
typedef unsigned long uint32_t;

typedef signed char int8_t;
typedef signed int int16_t;
typedef signed short long uint24_t;
typedef signed long int32_t;
*/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint24_t;
typedef unsigned int uint32_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int24_t;
typedef signed int int32_t;

#define rom

#define D0 (1<<0)
#define D1 (1<<1)
#define D2 (1<<2)
#define D3 (1<<3)
#define D4 (1<<4)
#define D5 (1<<5)
#define D6 (1<<6)
#define D7 (1<<7)

#endif
