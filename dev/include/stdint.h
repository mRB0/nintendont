#ifndef __STDINT_H
#define __STDINT_H

#ifndef __PLAYERDEV__

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned short long uint24_t;
typedef unsigned long uint32_t;

typedef signed char int8_t;
typedef signed int int16_t;
typedef signed short long uint24_t;
typedef signed long int32_t;

#else

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint24_t;
typedef unsigned int uint32_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int24_t;
typedef signed int int32_t;

#define rom

#endif // PLAYERDEV

#endif // __STDINT_H
