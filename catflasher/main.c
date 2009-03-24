#include <p18f4620.h>
#include <delays.h>
#include "stdint.h"

#include "project.h"
#include "interrupts.h"

#include "excitebike.h"

static uint16_t _pgm_calls = 0;

#define FLASHSIZE 262144 // bytes

#define wait10ms() Delay1KTCYx(82)
#define wait10us() Delay10TCYx(9)
#define wait6us() Delay10TCYx(6)
#define flash_putc(c)	do { 							\
							FLASH_DATA_TRIS = 0;		\
							FLASH_DATA_LAT = c;			\
							FLASH__CE_LAT = 0;			\
							FLASH__WE_LAT = 0;			\
							FLASH__WE_LAT = 1;			\
							FLASH__CE_LAT = 1;			\
							FLASH_DATA_TRIS = 0xff;		\
						} while(0)
#define flash_getc(c)	do { 							\
							FLASH_DATA_TRIS = 0xff;		\
							FLASH__CE_LAT = 0;			\
							FLASH__OE_LAT = 0;			\
							c = FLASH_DATA;				\
							FLASH__OE_LAT = 1;			\
							FLASH__CE_LAT = 1;			\
						} while(0)
#define flash_set_addr(addr)	do {																		\
									FLASH_ADDR0_LAT = (uint24_t)addr & (uint24_t)0xff;								\
									FLASH_ADDR1_LAT = (uint8_t) (((uint24_t)addr>>(uint24_t)8) & (uint24_t)0xff);		\
									FLASH_ADDR2_LAT = (uint8_t) (((uint24_t)addr>>(uint24_t)16) & (uint24_t)0x03);	\
								} while(0)
																
#define flash_reset()	do { flash_putc(0xff); flash_putc(0xff); flash_putc(0x00); } while(0)

#define flash_set_readmode()	flash_putc(0x00)

#define FLASH_PGM_PLSCNT_MAX 25
#define FLASH_ERA_PLSCNT_MAX 1000

void system_init(void)
{
	// 8 MHz
	OSCCONbits.IRCF0 = 1;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF2 = 1;
	OSCCONbits.IDLEN = 1;
	
	// x4: 32 MHz
	OSCTUNEbits.PLLEN = 1;
	
	// interrupts on or off?
	ISR_disable();
	
	// I/O ports
	
	ADCON1 = 0x0f;
	
	SWITCH_TRIS = 1;
	if (0 && SWITCH_PORT) // high == program flash, low == don't interfere
	{
		FLASH_ADDR0_TRIS = 0;
		FLASH_ADDR1_TRIS = 0;
		FLASH_ADDR2_TRIS &= ~(0x03); // only 2 bits
		
		FLASH_DATA_TRIS = 0;
		
		ROM__OE_TRIS = 0;
		ROM__OE_LAT = 1;
		
		FLASH__CE_TRIS = 0;
		FLASH__CE_LAT = 1;
		FLASH__WE_TRIS = 0;
		FLASH__WE_LAT = 1;
		FLASH__OE_TRIS = 0;
		FLASH__OE_LAT = 1;
	}
	else
	{
		// let NES drive addr[0..13] and data
		
		FLASH_ADDR0_TRIS = 0xff;
		FLASH_ADDR1_TRIS = 0x3f;
		FLASH_ADDR2_TRIS &= ~(0x03);
		
		flash_set_addr(0x0); // set upper bytes to zero
		
		FLASH_DATA_TRIS = 0xff;
		
		// hold PRG ROM /OE high
		ROM__OE_TRIS = 0;
		ROM__OE_LAT = 1;
		
		// hold /WE high
		FLASH__WE_TRIS = 0;
		FLASH__WE_LAT = 1;
		
		// hold flash /CE low
		FLASH__CE_TRIS = 0;
		FLASH__CE_LAT = 0;
		
		FLASH__OE_TRIS = 1; // let NES drive /OE
		
		for(;;)
		{
			_asm SLEEP _endasm;
		}
	}
}

int8_t flash_pgm_byte(uint24_t addr, uint8_t data)
{
	uint8_t plscnt;
	uint8_t readdata;
	
	_pgm_calls++;
	
	flash_set_addr(addr);
	
	plscnt = 0;
	
	if ((addr % 0x10000) == 0)
	{
		Nop();
		Nop();
	}
	
	do
	{
		flash_putc(0x40);
		flash_putc(data);
		wait10us();
		flash_putc(0xc0);
		wait6us();
		flash_getc(readdata);
	} while((readdata != data) && ((++plscnt) < FLASH_PGM_PLSCNT_MAX));
	
	if (FLASH_PGM_PLSCNT_MAX == plscnt)
	{
		// error, abort
		return -1;
	}
	return 0;
}

int8_t flash_erase(void)
{
	uint16_t plscnt;
	uint24_t addr;
	uint8_t readdata;
	int8_t rc;
	
	
	// first, program all bytes to 0x00
	for(addr = 0; addr < FLASHSIZE; addr++)
	{
		rc = flash_pgm_byte(addr, 0x00);
		if (rc < 0)
		{
			return -1;
		}
	}
	
	
	// next, start erase
	addr = 0;
	flash_set_addr(addr);
	
	plscnt = 0;
	
	do
	{
		flash_putc(0x20); // erase setup
		flash_putc(0x20); // erase GO!
		
		wait10ms();
		
		do
		{
			flash_set_addr(addr);
			flash_putc(0xa0); // erase verify
			wait6us();
			flash_getc(readdata);
			
			if (0xff == readdata)
			{
				addr++;
			}
			else
			{
				plscnt++;
				break;
			}
		} while(addr < FLASHSIZE);
	} while((addr < FLASHSIZE) && ((++plscnt) < FLASH_ERA_PLSCNT_MAX));
	
	if (FLASH_ERA_PLSCNT_MAX == plscnt)
	{
		// error, abort
		return -1;
	}
	
	flash_set_readmode();

	return 0;
}

int8_t flash_pgm_from_rom(uint24_t addr)
{
	uint8_t plscnt;
	uint8_t readdata;
	uint8_t data;
	
	_pgm_calls++;
	
	flash_set_addr(addr);
	
	plscnt = 0;
	
	do
	{		
		// set up write
		flash_putc(0x40);
		
		// get out of the way
		FLASH_DATA_TRIS = 0xff; // data as input, will be provided from ROM directly
		
		ROM__OE_LAT = 0;
		
		FLASH__CE_LAT = 0;
		FLASH__WE_LAT = 0;
		data = FLASH_DATA; // get byte off data line for verification
		FLASH__WE_LAT = 1;
		FLASH__CE_LAT = 1;
		
		ROM__OE_LAT = 1;
	
		wait10us();
		flash_putc(0xc0);
		wait6us();
		flash_getc(readdata);
		Nop();
		Nop();
		
	} while((readdata != data) && ((++plscnt) < FLASH_PGM_PLSCNT_MAX));
	
	if (FLASH_PGM_PLSCNT_MAX == plscnt)
	{
		// error, abort
		return -1;
	}
	return 0;
}


int8_t flash_isempty(void)
{
	uint24_t addr;
	uint8_t data;
	
	flash_set_readmode();
	
	for(addr = 0; addr < FLASHSIZE; addr++)
	{
		flash_set_addr(addr);
		flash_getc(data);
		if (data != 0xff)
		{
			return 0;
		}
	}
	return 1;
}

int8_t write_excitebike(void)
{
	uint24_t addr;
	
	for(addr = 0; addr < excitebike_len; addr++)
	{
		if (-1 == flash_pgm_byte(addr, excitebike[addr]))
		{
			return -1;
		}
	}
	return 0;
}

int8_t copy_excitebike(void)
{
	uint24_t addr;
	
	for(addr=0; addr < FLASHSIZE /* excitebike_len */; addr++)
	{
		if (-1 == flash_pgm_from_rom(addr))
		{
			Nop();
			Nop();
			return -1;
		}
	}
	return 0;
}

void main(void)
{
	uint8_t sig, dev;
	
	system_init();
	
	Delay100TCYx(100);
	
	flash_reset();
	
	Delay100TCYx(100);
	
	
	flash_set_addr(0x000000);
	flash_putc(0x90);
	flash_getc(sig); // should be 0x31  0b00110001
	
	flash_set_addr(0x000001);
	flash_putc(0x90);
	flash_getc(dev); // should be 0xbd  0b10111101
	
	if (sig != 0x31 || dev != 0xbd)
	{
		// error
		for(;;)
		{
			Nop();
			Nop();
		}
	}
	
	flash_set_readmode();
	
	// check if flash needs to be erased
	if (!flash_isempty())
	{
		Nop();
		Nop();
		// erase flash
		if (-1 == flash_erase())
		{
			Nop();
			Nop();
		}
	}
	
	// program excitebike
//	if (-1 == write_excitebike())
	if (-1 == copy_excitebike())
	{
		Nop();
		Nop();
	}
	
	for(;;)
	{
		Nop();
		Nop();
	}
	
}
