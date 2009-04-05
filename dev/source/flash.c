#include <delays.h>
#include <usart.h>

#include "serial.h"
#include "flash.h"
#include "ports.h"

void flash_init(void)
{
	
	ACTIVATE_FL0();
	
	flash_reset();
	
	DEACTIVATE_FL0();
	
	
	flash_test();
}

void flash_test(void)
{
	uint8_t sig_mfr, sig_dev;
	
	ACTIVATE_FL0();
	
	set_addr(0x0);
	port_putc(0x90);
	sig_mfr = port_getc();
	
	port_write(0x0, 0x90);
	sig_dev = port_read(0x1);
	
	Nop();
	Nop();
	
	DEACTIVATE_FL0();
	//for(;;);
}

/*
 * program a byte into flash 
 * flash_chip is which flash chip we'll use (0 or 1)
 */
int8_t flash_pgm_byte(uint8_t flash_chip, uint24_t addr, uint8_t data)
{
	uint8_t plscnt;
	uint8_t readdata;
	
	if (flash_chip == 0)
	{
		ACTIVATE_FL0();
	}
#ifdef LAT_FL1_CE
	else
	{
		ACTIVATE_FL1();
	}
#endif

	//_pgm_calls++;
	
	set_addr(addr);
	
	plscnt = 0;
	
	if ((addr % 0x10000) == 0)
	{
		Nop();
		Nop();
	}
	
	do
	{
		port_putc(0x40);
		port_putc(data);
		wait10us();
		port_putc(0xc0);
		wait6us();
		readdata = port_getc();
	} while((readdata != data) && ((++plscnt) < FLASH_PGM_PLSCNT_MAX));
	
	if (FLASH_PGM_PLSCNT_MAX == plscnt)
	{
		// error, abort
		if (flash_chip == 0)
		{
			DEACTIVATE_FL0();
		}
		#ifdef LAT_FL1_CE
		else
		{
			DEACTIVATE_FL1();
		}
		#endif
		return -1;
	}
	
	if (flash_chip == 0)
	{
		DEACTIVATE_FL0();
	}
	#ifdef LAT_FL1_CE
	else
	{
		DEACTIVATE_FL1();
	}
	#endif
	return 0;
}

/*
 * erase flash 
 * flash_chip is which flash chip we'll erase (0 or 1)
 */
int8_t flash_erase(uint8_t flash_chip)
{
	uint16_t plscnt;
	uint24_t addr;
	uint8_t readdata;
	int8_t rc;
		
	// first, program all bytes to 0x00
	for(addr = 0; addr < FLASHSIZE; addr++)
	{
		rc = flash_pgm_byte(flash_chip, addr, 0x00);
		if (rc < 0)
		{
			if (flash_chip == 0)
			{
				DEACTIVATE_FL0();
			}
			#ifdef LAT_FL1_CE
			else
			{
				DEACTIVATE_FL1();
			}
			#endif
			return -1;
		}
	}
	
	// flash_pgm_byte deactivates fl0 when finished
	if (flash_chip == 0)
	{
		ACTIVATE_FL0();
	}
#ifdef LAT_FL1_CE
	else
	{
		ACTIVATE_FL1();
	}
#endif
	
	// next, start erase
	addr = 0;
	set_addr(addr);
	
	plscnt = 0;
	
	do
	{
		port_putc(0x20); // erase setup
		port_putc(0x20); // erase GO!
		
		wait10ms();
		
		do
		{
			set_addr(addr);
			port_putc(0xa0); // erase verify
			wait6us();
			readdata = port_getc();
			
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
		if (flash_chip == 0)
		{
			DEACTIVATE_FL0();
		}
		#ifdef LAT_FL1_CE
		else
		{
			DEACTIVATE_FL1();
		}
		#endif
		return -1;
	}
	
	flash_set_readmode();

	if (flash_chip == 0)
	{
		DEACTIVATE_FL0();
	}
	#ifdef LAT_FL1_CE
	else
	{
			DEACTIVATE_FL1();
	}
	#endif
	return 0;
}
