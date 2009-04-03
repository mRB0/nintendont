#include "circbuf.h"

/*
 * Circular buffer routines.
 */

/*
 * Pushes a character onto the buffer.
 *
 * Returns -1 in case of error (ie buffer is full), 0 for success.
 */
signed char circbuf_pushchar(volatile CIRCBUF *circle, unsigned char data, unsigned char checkfull)
{
	int writeidx;
	
	if (checkfull && CIRCBUF_FULL(*circle))
	{
		return -1;
	}
	
	//writeidx = (circle->start + circle->queued) % circle->bufsize;
	
	circle->bufdata[circle->writepos] = data;
	
	//circle->writepos = (circle->writepos + 1) % circle->bufsize;
	circle->writepos = (circle->writepos + 1) & circle->szmask;
	
	return 0;
}

/*
 * Pops the next character out of the circular buffer.
 * In case of error, returns '\0'; you should check !CIRCBUF_EMPTY first.
 */
unsigned char circbuf_popchar(volatile CIRCBUF *circle)
{
	char popped;
	
	if (CIRCBUF_EMPTY(*circle))
	{
		return '\0';
	}
	
	popped = circle->bufdata[circle->start];
	
	//circle->start = (circle->start + 1) % circle->bufsize;
	circle->start = (circle->start + 1) & circle->szmask;
	
	return popped;
}

/*
 * Returns the next character out of the circular buffer.
 * Doesn't pop it though.
 * In case of error, returns '\0'; you should check !CIRCBUF_EMPTY first.
 */
unsigned char circbuf_peekchar(volatile CIRCBUF *circle)
{
	char peeked;
	
	if (CIRCBUF_EMPTY(*circle))
	{
		return '\0';
	}
	
	peeked = circle->bufdata[circle->start];
	
	return peeked;
}

/*
 * Pushes an array of characters onto the buffer.
 *
 * Returns -1 in case of error (ie buffer is full), 0 for success.
 * (In case of error, some data will be pushed, but not all)
 */
signed char circbuf_pushchars(volatile CIRCBUF *circle, unsigned char *data, int len)
{
	int rc, i;
	
	for(i = 0; i < len; i++)
	{
		rc = circbuf_pushchar(circle, data[i], 1);
		if (rc)
		{
			return rc;
		}
	}
	
	return 0;
}

unsigned int circbuf_queued(volatile CIRCBUF *circle)
{
	unsigned int virt_writepos = circle->writepos;
	
	if (circle->writepos < circle->start)
	{
		virt_writepos += circle->bufsize;
	}
	
	return virt_writepos - circle->start;
}


