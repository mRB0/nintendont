/* lol */

#include <p18f4620.h>

#include <delays.h>
#include <usart.h>
#include <stdio.h>


#include "circbuf.h"
#include "interrupts.h"
#include "ports.h"
#include "serial.h"
#include "flash.h"
	

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
	// xxx moved to main // flash_init();
	
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

const rom uint16_t _frequencies[] = { 626, 590, 557, 526, 496, 468, 442, 417, 394, 372, 351, 331, 313 };

void main(void)
{
	int i, k;
	uint24_t j=1;
	
	uint16_t freq = 0;
	uint8_t duty = 0x0;
	unsigned char c;
	
	int8_t rc1=0, rc2=-1;
	
	system_init();
	
	//spc_test();
	//flash_test();
	
	/*
	set_addr(0x00001 << 9);
	set_addr(0x08000);
	for(;;);
	*/
	
	TRIS_DATA = 0x0;
	//LAT_DATA = 0x01 << 3;
	
	putrsUSART("ok go\n\r");
	/*
	
	for(i=0; ; i=(i+1)%8)
	{
		unsigned char c=0;
		
		//printf("addr=%u, ", j);
		printf("addr=0x%05Hx, ", j);
		printf("data=0x%02hhx ", (0x01<<i));
		printf("(%d)\n\r", i);
		
		LAT_DATA = 0x01 << i;
		set_addr(j);
		
		if (j & 0x00001)
		{
			ACTIVATE_FL0();
		}
		if (j & 0x00002)
		{
			DEACTIVATE_FL0();
		}
		if (j & 0x00004)
		{
			LAT_WE = 0;
		}
		if (j & 0x00008)
		{
			LAT_WE = 1;
		}
		if (j & 0x00010)
		{
			LAT_OE = 0;
		}
		if (j & 0x00020)
		{
			LAT_OE = 1;
		}
		
		j = j << 1;
		
		if (j & 0x40000)
		{
			j = 1;
		}

		do
		{
			ISR_disable();
			while (_interrupts.rx)
			{
				_interrupts.rx = 0;
				ISR_enable();
				
				while(!CIRCBUF_EMPTY(_rxbuf))
				{
					CIRCBUF_POPCHAR_INLINE(_rxbuf, c);
				}
				//c=1;
				ISR_disable();
			}
			ISR_enable();
		
		} while(c==0);
		
	}		
	*/
	
	printf("\r\r\n\n************\n\r\n\r");
	
	flash_reset();
	
	rc1 = flash_init();
	
	printf("init done, rc = 0x%02hhx\n\r", rc1);

	if (rc1 != 0)
	{
		printf("BAILING because flash init failed\n\r");
		for(;;);
	}
	
	
	printf("waiting for input\r\n");
	c = 0;
	
	do
	{
		ISR_disable();
		while (_interrupts.rx)
		{
			_interrupts.rx = 0;
			ISR_enable();
			
			while(!CIRCBUF_EMPTY(_rxbuf))
			{
				CIRCBUF_POPCHAR_INLINE(_rxbuf, c);
			}
			//c=1;
			ISR_disable();
		}
		ISR_enable();
	
	} while(c==0);
		
	printf("\r\nflash beginning\n\r");
	
	//printf("rc = %d\r\n", flash_pgm_byte(0x0, 0x0, 0xff));
	//printf("rc = %d\r\n", flash_pgm_byte(0x0, 0x0, 0xff));
	
	rc1 = flash_pgm_byte(0x0, 0x0, 0xde);
	
	printf("pgmbyte done, rc = 0x%02hhx\n\r", rc1);
	
	if (rc1 == 0x00)
	{
		rc2 = flash_erase(0);
		
		printf("erase done, rc = 0x%02hhx\n\r", rc2);
	}
	
	Nop();
	Nop();
	
	putrsUSART("done recording\n\r");
	//putrsUSART("
	
	for(;;);
	
	// keyboard
	for(;;)
	{
		ISR_disable();
		while (_interrupts.rx)
		{
			_interrupts.rx = 0;
			ISR_enable();
			
			ACTIVATE_VRC6();
			
			while(!CIRCBUF_EMPTY(_rxbuf))
			{
				CIRCBUF_POPCHAR_INLINE(_rxbuf, c);
				switch(c)
				{
					case '`':
						duty = (duty + 1) % 0x8;
						port_write(0x9000, 0x0f | (duty << 4));
						break;
					case 'q':
						freq = _frequencies[0];
						break;
					case '2':
						freq = _frequencies[1];
						break;
					case 'w':
						freq = _frequencies[2];
						break;
					case '3':
						freq = _frequencies[3];
						break;
					case 'e':
						freq = _frequencies[4];
						break;
					case 'r':
						freq = _frequencies[5];
						break;
					case '5':
						freq = _frequencies[6];
						break;
					case 't':
						freq = _frequencies[7];
						break;
					case '6':
						freq = _frequencies[8];
						break;
					case 'y':
						freq = _frequencies[9];
						break;
					case '7':
						freq = _frequencies[10];
						break;
					case 'u':
						freq = _frequencies[11];
						break;
					case 'i':
						freq = _frequencies[12];
						break;
					case ' ':
						freq = 0;
						break;
				}
				if (freq == 0)
				{
					port_write(0x9002, 0x00);
				}
				else
				{
					port_write(0x9001, freq);
					port_write(0x9002, 0x80 | (0xf & (freq >> 8)));
				}
				//port_write(0x9001, 0xa7);
				//port_write(0x9002, 0x83);
				
			}
			
			DEACTIVATE_VRC6();
			
			ISR_disable();
		}
		ISR_enable();
	}
	
	
	// play fun tones
	
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
