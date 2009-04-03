#ifndef __CIRCBUF_H
#define __CIRCBUF_H

/*
 * This is the standard exchange format for circular buffers.
 * You should have a char array of size bufsize, and point
 * bufdata to it.  Initialize start and writepos to the same val.
 *
 * Then you pass this struct around to circular buffer routines.
 *
 * bufdata: pointer to char array containing the circular buffer.
 * bufsize: length of the buffer (character array).
 * szmask: The bitmask against which the start pointer should be
 *         compared (saves us from doing a % check); that is,
 *         we want to say start = (start + 1) % bufsize which
 *         is equivalent to start = (start + 1) & szmask when
 *         (eg) bufsize = 32 and szmask = 0b00011111
 * start: index in bufdata containing the next character to be
 *        read.
 * writepos: position for the next character.
 *         If writepos==start, no bytes may be read.
 *         New bytes are added to writepos;
 *         it is an overflow error to add a byte when
 *         (writepos==bufsize).
 */
typedef struct circbuf
{
	unsigned char *bufdata;
	unsigned int bufsize;
	unsigned int szmask;
	
	unsigned int start, writepos;
	
} CIRCBUF;


signed char circbuf_pushchar(volatile CIRCBUF *circle, unsigned char data, unsigned char checkfull);
unsigned char circbuf_popchar(volatile CIRCBUF *circle);
unsigned char circbuf_peekchar(volatile CIRCBUF *circle);
signed char circbuf_pushchars(volatile CIRCBUF *circle, unsigned char *data, int len);
unsigned int circbuf_queued(volatile CIRCBUF *circle);

// prototype: int CIRCBUF_EMPTY(CIRCBUF circbuf);
#define CIRCBUF_EMPTY(circbuf)	((circbuf).writepos == (circbuf).start)
// prototype: int CIRCBUF_FULL(CIRCBUF circbuf);
/*
 * In order to prevent the conditions of CIRCBUF_FULL and CIRCBUF_EMPTY from meaning
 * the same thing, I'll make the circbuf one character shorter than the total size.
 */
#define CIRCBUF_FULL(circbuf)	((((circbuf).writepos+1) & (circbuf).szmask) == (circbuf).start)

#define CIRCBUF_POPCHAR_INLINE(circbuf, outdata)                                     \
	(outdata) = (circbuf).bufdata[(circbuf).start];                                  \
	(circbuf).start = ((circbuf).start + 1) & (circbuf).szmask

#define CIRCBUF_PUSHCHAR_INLINE(circbuf, indata)                                     \
	(circbuf).bufdata[(circbuf).writepos] = indata;                                  \
	(circbuf).writepos = ((circbuf).writepos + 1) & (circbuf).szmask

#endif
