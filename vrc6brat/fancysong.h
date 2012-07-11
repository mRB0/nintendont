#ifndef FANCYSONG_H_
#define FANCYSONG_H_

#include <stdint.h>
#include <avr/pgmspace.h>

extern prog_uint8_t fancysong[] PROGMEM;
extern prog_uint32_t fancysong_len;

extern prog_uint8_t *samples[];
extern prog_uint16_t sample_lens[] PROGMEM;


#endif
