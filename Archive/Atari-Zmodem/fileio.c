/*
 * 	File I/O (with large buffers) Module
 *
 *	Jwahar Bammi
 * 	bang:   {any internet host}!dsrgsun.ces.CWRU.edu!bammi
 * 	domain: bammi@dsrgsun.ces.CWRU.edu
 *	GEnie:	J.Bammi
 */

#include "config.h"

#include "zmdm.h"
#include "common.h"

#define	O_RDONLY	1
#define O_WRONLY	2
#define O_APONLY	4
#ifndef DYNABUF
#define MBUFSIZ		(((long)BBUFSIZ)-1L)
#else
static long MBUFSIZ;
#endif /* DYNABUF */

static unsigned char *bptr;

static long flcount = (-1L);
static int bufmode;

int stfopen(name, mode)
char *name, *mode;
{
	register int handl;

	switch(*mode)
	{
	    case 'r':
		if((handl = Fopen(name, 0)) <= 0)
			return -1;
		bufmode = O_RDONLY;
		break;

	    case 'w':
		if((handl = Fcreate(name, 0)) <= 0)
		{
			if((handl = Fopen(name, 1)) <= 0)
				return -1;
		}
		bufmode = O_WRONLY;
		break;

	    case 'a':
		if((handl = Fopen(name, 2)) <= 0)
			return -1;
		Fseek(0L, handl, 2);
		bufmode = O_APONLY;
		break;

	   default:
		return -1;
	}

#ifdef DYNABUF
	MBUFSIZ = BBUFSIZ - 1L;
#endif /* DYNABUF */

	bptr = bufr;
	flcount = (-1L);
	return handl;
}

stfclose(handl)
int handl;
{
	if(bufmode == O_RDONLY)
		return Fclose(handl);
	if(stflush(handl))
	{
		Fclose(handl);
		return -1;
	}
	return Fclose(handl);
}

stputc(c, handl)
unsigned int c;
int handl;
{
	if(flcount >= MBUFSIZ)
	{
		if(Fwrite(handl, (long)BBUFSIZ, bufr) != (long)BBUFSIZ)
			return -1;
		flcount = (-1L);
		bptr  = bufr;
	}
	flcount++;
	*bptr++ = c;
	return 0;
}

stgetc(handl)
int handl;
{
	if(flcount <= 0)
	{
		if((flcount = Fread(handl, (long)BBUFSIZ, bufr)) == 0)
			return EOF;
		bptr = bufr;
	}
	flcount--;
	return(*bptr++);
}

stflush(handl)
int handl;
{
	if(flcount < 0)
		return 0;

	if(Fwrite(handl, (long)(flcount+1L), bufr) != (flcount+1L))
		return -1;
	flcount = (-1L);
	bptr = bufr;
	return 0;
}
		
stfseek(handl, disp, mode)
int handl;
long disp;
int  mode;
{
	if(bufmode != O_RDONLY)
		if(stflush(handl))
			return -1;
	Fseek(disp, handl, mode);
	flcount = (-1L);
	bptr = bufr;
	return 0;
}

long stread(handl, b, bytes)
int handl;
unsigned char *b;
long bytes;
{
    long done;
    
    if(bytes == 0L) return 0L;
    
    if(flcount <= 0)
    {
	if((flcount = Fread(handl, (long)BBUFSIZ, bufr)) == 0)
	    return 0L;
	bptr = bufr;
    }
    if(flcount >= bytes)
    {
	bcopy(bptr, b, bytes);
	flcount -= bytes;
	bptr += bytes;
	return bytes;
    }
    bcopy(bptr, b, flcount);
    done = flcount;
    flcount = 0;
    return done + stread(handl, b+done, bytes-done);
}

#ifndef __GNUC__
void bcopy(s, d, n)
unsigned char *s, *d;
long n;		/* note: long */
{
    while(n--)
	*d++ = *s++;
}
#endif
/* -eof- */
