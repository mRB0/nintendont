/* lol */

#include <p18f4620.h>

#include <delays.h>
#include <usart.h>
#include <stdio.h>
#include <string.h>

#include "player.h"


#include "circbuf.h"
#include "interrupts.h"
#include "ports.h"
#include "serial.h"
#include "flash.h"

#define BOTRDY 0x51
#define BOTRDY_I 0x61
#define INJRDY 0x52
#define INJRDY_I 0x62
#define INJABT 0x53
#define INJABT_I 0x63
#define ACK 0x53
#define BLKSIZE 64

#define INTERNAL_MEM_BASE 0xa000

void mem_load(uint8_t internal)
{
	uint24_t len;
	uint8_t i;
	uint24_t addr;
	uint8_t blk[BLKSIZE];
	
	uint8_t botrdy=BOTRDY, injrdy=INJRDY, injabt=INJABT;
	
	if (internal)
	{
		botrdy = BOTRDY_I;
		injrdy = INJRDY_I;
		injabt = INJABT_I;
	}
	
	ISR_disable();
	
	// drain
	while(DataRdyUSART())
	{
		ReadUSART();
	}
	
	// wait for injector
	do
	{
		while(BusyUSART()) {}
		WriteUSART(botrdy);
		Delay100TCYx(250);
		if (DataRdyUSART())
		{
			blk[0] = ReadUSART();
			if (blk[0] == injabt)
			{
				// don't program
				ISR_enable();
				return;
			}
			if (blk[0] == injrdy)
			{
				break;
			}
		}
		
	} while(1);
	
	while(BusyUSART()) {}
	WriteUSART(ACK);
	
	// wait for length
	len = 0;
	
	while(!DataRdyUSART()) {}
	blk[0] = ReadUSART();
	while(!DataRdyUSART()) {}
	blk[1] = ReadUSART();
	while(!DataRdyUSART()) {}
	blk[2] = ReadUSART();
	len = ((uint24_t)(blk[0])<<16) | (((uint24_t)blk[1])<<8) | (uint24_t)blk[2];
		
	// receive all data in BLKSIZE chunks
	
	addr = 0;
	if (!internal)
	{
		ACTIVATE_FL0();
	}
	
	while(BusyUSART()) {}
	WriteUSART(ACK);
	
	do
	{
		for(i=0; i<BLKSIZE; i++)
		{
			while(!DataRdyUSART()) {}
			blk[i] = ReadUSART();
		}
		
		// write block to memory
		if (internal)
		{
			memmoveram2flash(INTERNAL_MEM_BASE+addr, blk);
		}
		else
		{
			for(i=0; i<BLKSIZE; i++)
			{
				port_write(addr+i, blk[i]);
			}
		}
		
		addr += BLKSIZE;
		
		while(BusyUSART()) {}
		WriteUSART(ACK);
	
	} while (addr < len);
	
	if (!internal)
	{
		DEACTIVATE_FL0();
	}
	
	ISR_enable();
}

void mem_test(void)
{
	uint24_t addr;
	uint8_t data, data2;
	
	ACTIVATE_FL0();
	
	for(data=0, addr=0; addr<0x40000; addr++, data=(data+1)%254)
	{
		if (addr%0x4000 == 0)
		{
			printf("writing 0x%02hhx to addr 0x%05Hx\r\n", data, addr);
		}
		port_write(addr, data);
	}
	
	DEACTIVATE_FL0();
	
	ports_flash_open(0);
	
	for(data2=0, addr=0; addr<0x40000; addr++, data2=(data2+1)%254)
	{
		if (addr%0x4000 == 0)
		{
			printf("read from addr 0x%05Hx\r\n", addr);
		}
		//data = port_read(addr);
		data = ports_flash_read();
		if (data != data2)
		{
			printf("0x%05Hx: got 0x%02hhx, expected 0x%02hhx\r\n", addr, data, data2);
		}
	}
	
	ports_flash_close();
}	

/*
 * The SPC module is located at 0x00 .. 0x03, with LAT_SPC_CE low.
 * On the SNES it is located at 0x2140 .. 0x2143, which are the same ports.
 *
 * So to communicate to SPC:
 *   
 */
void spc_init(void)
{
	//LAT_SPC_RESET = 0; // hold in reset
	DEACTIVATE_SPC();
	
	//TRIS_SPC_RESET = 0;
	TRIS_SPC_CE = 0;
	
	// reset (A7 is SPC_RESET, not inverted, active whcn /CE)
	set_addr(0x80);
	ACTIVATE_SPC();
	
	Delay1KTCYx(100);
	
	DEACTIVATE_SPC();
}

void vrc6_init(void)
{
	uint16_t i;
	
	// delay some time to allow oscillator to settle
	/*for(i=0; i<10; i++)
	{
		Delay100TCYx(255);
		Delay100TCYx(255);
		Delay100TCYx(255);
	}*/
	
	TRIS_VRC6_CE = 0;
	ACTIVATE_VRC6();
	
	// init vrc6 mapping stuff
	
	port_write(0x9003, 0x00);
	
	port_write(0x8000, 0x00);
	port_write(0xc000, 0x00);
	
	port_write(0xb003, 0x08);
	port_write(0xf000, 0x00);
	port_write(0xf001, 0x00);
	port_write(0xf002, 0x00);

	//DEACTIVATE_VRC6();
	
	// init vrc6 sound
	
	//ACTIVATE_VRC6();
	
	port_write(0x9000, 0x7f);
	port_write(0x9001, 0x00);
	port_write(0x9002, 0x00);
	
	port_write(0xa000, 0x7f);
	port_write(0xa001, 0x00);
	port_write(0xa002, 0x00);

	port_write(0xb000, 0x3f);
	port_write(0xb001, 0x00);
	port_write(0xb002, 0x00);
	
	DEACTIVATE_VRC6();
	
}

void system_init(void)
{
	// 8 MHz: IRCF2:0 = 0b111
	OSCCONbits.IRCF0 = 1;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF2 = 1;
	OSCCONbits.IDLEN = 1;
	
	// x4: 32 MHz
	OSCTUNEbits.PLLEN = 1;
	
	// interrupts on or off?
	ISR_disable();
	
	// I/O ports
	
	ADCON1 = 0x0f; // digital I/O
	
	
	TRISA = 0;
	//TRISBbits.TRISB4 = 0; // this is address line A6, because RA6 is used for clock output.
	TRISB = 0;
	TRISC = 0;
	
	TRIS_DATA = TRIS_INPUT;
	
	DEACTIVATE_FL0();
	DEACTIVATE_VRC6();
	DEACTIVATE_SPC();
	
	TRIS_FL0_CE = 0;
	
	LAT_WE = 1;
	LAT_OE = 1;
	TRIS_WE = 0;
	TRIS_OE = 0;
	
	spc_init();
	vrc6_init();
	
	serial_init();
	
	ISR_enable();
}

void tester(void)
{
	int a = 5;
	tester();
}

void spc_test(void)
{
	uint8_t data;
	
	//LAT_SPC_RESET = 1;
	
	// wait for 0xaa
	
	ACTIVATE_SPC();
	
	do
	{
		data = port_read(0x2140);
		
		Nop();
		Nop();
	} while(data != 0xaa);
	
	// wait for 0xbb
	
	do
	{
		data = port_read(0x2141);
		
		Nop();
		Nop();
	} while(data != 0xbb);
	
	// write any non-zero value to 0x2141
	port_write(0x2141, 0x55);
	
	// dest addr
	port_write(0x2142, 0x00);
	port_write(0x2143, 0x00);
	// write 0xcc and wait for 0xcc
	
	port_write(0x2140, 0xcc);
	
	do
	{
		data = port_read(0x2140);
		
		Nop();
		Nop();
	
	
	} while(data != 0xcc);
	DEACTIVATE_SPC();
	
}

void play_fun_tones()
{
	uint8_t duty = 0x0;
	uint8_t c;
	
	for(;;)
	{
		
		ISR_disable();
		while (_interrupts.rx)
		{
			_interrupts.rx = 0;
			ISR_enable();
			
			while(!CIRCBUF_EMPTY(_rxbuf))
			{
				CIRCBUF_POPCHAR_INLINE(_rxbuf, c);
				Nop();
				Nop();
				if ('a' == c)
				{
					duty = (duty + 1) % 0x8;
				}
			}
			
			ISR_disable();
		}
		ISR_enable();
	
		putcUSART(duty + 'A');
		
		
		// set duty
		
		ACTIVATE_VRC6();
		
		port_write(0x9000, 0x0f | (duty << 4));
		port_write(0xa000, 0x0f | (duty << 4));
		
		// set pitches
		
		port_write(0xa001, 0xa7);
		port_write(0xa002, 0x83);
		port_write(0x9001, 0xe7);
		port_write(0x9002, 0x82);
		
		Delay10KTCYx(75);
		
		port_write(0xa001, 0xe7);
		port_write(0xa002, 0x82);
		port_write(0x9001, 0x72);
		port_write(0x9002, 0x82);
		
		Delay10KTCYx(75);
		
		port_write(0xa001, 0x72);
		port_write(0xa002, 0x82);
		port_write(0x9001, 0xd6);
		port_write(0x9002, 0x81);

		DEACTIVATE_VRC6();
		
		Delay10KTCYx(50);
		
		
	}
}

const rom uint16_t frequencies[] = { 1024, 966, 913, 862, 814, 768, 725, 685, 647, 611, 577, 545, 514, 486, 458, 433, 409, 386, 364, 344, 325, 307, 290, 273, 258, 244, 230, 217, 205, 194 };
const rom uint8_t keys[] = { 'z', 's', 'x', 'd', 'c', 'v','g','b','h','n','j','m','q','2','w','3','e','r','5','t','6','y','7','u','i','9','o','0','p' };

void fun_keyboard(void)
{
	int i;
	uint8_t c;
	uint8_t duty = 5;
	
	ACTIVATE_VRC6();
	
	port_write(0xa000, 0x0f | ((duty-1)<<4));
	
	for(;;)
	{
		
		ISR_disable();
		while (_interrupts.rx)
		{
			_interrupts.rx = 0;
			ISR_enable();
			
			while(!CIRCBUF_EMPTY(_rxbuf))
			{
				CIRCBUF_POPCHAR_INLINE(_rxbuf, c);
				Nop();
				Nop();
				
				for(i=0; i<29; i++)
				{
					if (c == keys[i])
					{
						port_write(0xa001, 0x00 | frequencies[i]);
						port_write(0xa002, 0x80 | (frequencies[i] >> 8));
					}
				}
				
				switch(c)
				{
					case ' ':
						port_write(0xa002, 0x00);
						break;
					case ',':
						duty = duty - 1;
						if (duty == 0)
						{
							duty = 8;
						}
						port_write(0xa000, 0x0f | ((duty-1)<<4));
						break;
					case '.':
						duty = duty + 1;
						if (duty == 9)
						{
							duty = 1;
						}
						port_write(0xa000, 0x0f | ((duty-1)<<4));
						break;
				}
			}
			
			ISR_disable();
		}
		ISR_enable();
	}
}

void main(void)
{
	int i, k;
	uint24_t j=1;
	
	int8_t rc1=0, rc2=-1;
	
	system_init();
	
	//spc_test();
	
	//flash_test();
	
	// play fun tones
	
	fun_keyboard();
	
	//play_fun_tones();
	
	mem_load(0); // external
	mem_load(1); // internal
	
	//for(;;);
	
	Player_SetIBank(INTERNAL_MEM_BASE);
	//Player_Init();
	Player_Reset();
	Player_Start(0);
	
}
