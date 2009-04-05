#ifndef FLASH_H__
#define FLASH_H__

#include "ports.h"
#include "stdint.h"

#define FLASH_PGM_PLSCNT_MAX 25
#define FLASH_ERA_PLSCNT_MAX 1000
#define FLASHSIZE 262144 // bytes


#define wait10ms() Delay1KTCYx(82)
#define wait10us() Delay10TCYx(9)
#define wait6us() Delay10TCYx(6)
																
#define ACTIVATE_FL0()			LAT_FL0_CE = 0
#define DEACTIVATE_FL0()		LAT_FL0_CE = 1

#define ACTIVATE_FL1()			LAT_FL1_CE = 0
#define DEACTIVATE_FL1()		LAT_FL1_CE = 1


#define flash_reset()	do { port_putc(0xff); port_putc(0xff); port_putc(0x00); } while(0)

#define flash_set_readmode()	port_putc(0x00)

int8_t flash_pgm_byte(uint8_t which_chip, uint24_t addr, uint8_t data);
int8_t flash_erase(uint8_t flash_chip);

int8_t flash_init(void);
int8_t flash_test(void);

#endif
