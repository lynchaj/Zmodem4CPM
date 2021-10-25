/*
 * 	Utilities Module
 *
 *		Jwahar Bammi
 *			usenet: mandrill!bammi@{decvax,sun}.UUCP
 *			csnet:  bammi@mandrill.ces.CWRU.edu
 *			arpa:   bammi@mandrill.ces.CWRU.edu
 *			CompuServe: 71515,155
 */


#include "config.h"
#include "zmdm.h"

#ifndef Vsync 			/* Atari forgot these in osbind.h */
#define Vsync()	xbios(37)
#endif

#define GOTO(r,c)	EscSeq('Y');Bconout(2, r+040);Bconout(2, c+040)

 	/* External Variables */
#ifdef __GNUC__
volatile long     pr_time   = 0L;  /* Present time	  (200 Hz) */ 
#else
long     pr_time   = 0L;	  /* Present time	  (200 Hz) */ 
#endif

#ifndef STANDALONE
#ifndef REMOTE
extern int  *aline_addr; /* Ptr to base of Aline variables */
extern long *ms_ptr;	/* pointer to my screen memory aligned to a 256
			   byte boundary */

#ifndef OVERSCAN
 	/* Globals */
static char *hs_ptr;   /* pointer to his screen memory */
#else
extern long *ms_log;
  
   	/* Globals */
static char *hs_phys;   /* pointer to his screen memory */
static char *hs_log;
#endif /* OVERSCAN */
static int  x_saved, y_saved;	/* Saved cursor position */
#endif
#endif

static long *hz_200 =  (long *)0x000004ba; /* Yes the Hitch Hikers */
					   /* Guide is wrong!! */

/*
 * rd_time() - read the value of the systems 200 Hz counter
 * must be called in Super Mode else you get
 * what you deserve - MUSHROOMS!!
 */
void rd_time()
{
	pr_time = *hz_200;
}

#ifndef STANDALONE
/*
 * my_screen() - Start using my screen memory
 * for all output.
 *
 */
void my_screen()
{
#ifndef REMOTE
	/* The cursor position has been saved prior to calling this routine */
	x_saved = aline_addr[-14];
	y_saved = aline_addr[-13];

#ifndef OVERSCAN
	/* save his screen memory pointer */
	hs_ptr = (char *)Logbase();
	
	/* switch to my display memory */
	Setscreen(ms_ptr, ms_ptr, -1);
#else
	/* save his screen memory pointer */
	hs_phys= (char *)Physbase();
	hs_log = (char *)Logbase();
	
	/* switch to my display memory */
	Setscreen(ms_log, ms_ptr, -1);
#endif /* OVERSCAN */
	Vsync(); Vsync();
#else
	wr_modem("\r\n\n");
#endif
}


void his_screen()
{
#ifndef REMOTE
	/* switch to his Screen memory, wait for a Vblank, so
	 * that the Logical Loc == Physical Loc, then pop saved
	 * cursor */
#ifndef OVERSCAN
	Setscreen(hs_ptr, hs_ptr, -1);
#else
	Setscreen(hs_log, hs_phys, -1);
#endif
	Vsync();
	Vsync();	/* starts happening at the last one */
	GOTO(y_saved, x_saved);
#else
	wr_modem("\r\n\n");
#endif
	
}

/*
 * hit_key() - wait for the user to hit a key
 * 
 */
void hit_key()
{
#ifndef REMOTE
	Bconws("\r\n\033pHit Any Key To Continue .....\033q");
	Bconin(2);
	Bconws("\r\n");
#else
	wr_modem("\r\nHit Any Key To Continue .....");
	Bconin(1);
	wr_modem("\r\n");
#endif
}
#endif /* STANDALONE */

/*
 * find the size of a file given its name
 */
long filesize(name)
register char *name;
{
	extern struct stat statbuf;
	
	if(Fsfirst(name,(int)(0x01 | 0x020)) != 0)
	{
		return 0L;
		
	}

	return statbuf.st_size;
}


/*
 * Write a string to console
 */
void Bconws(s)
register char *s;
{
	while(*s)
		Bconout(2, *s++);
}

#ifndef STANDALONE
#ifndef REMOTE
#ifndef __GNUC__
#ifndef MANX
#ifndef MWC

/* **WARNING** **WARNING** **WARNING** **WARNING** **WARNING** **WARNING** */
/*									   */
/*  ALCYON C dependant, this routine must return the base address of	   */
/*	of the linea variable block					   */
/*									   */
/* **WARNING** **WARNING** **WARNING** **WARNING** **WARNING** **WARNING** */

/*
 * return the base address of the line A variables 
 */
int *aaddress()
{
	asm("dc.w $A000");	/* Line A trap - 0000 is init aline  */
				/* d0 and a0 now contain the address */
				/* so we can just return and the result
				 * will be valid
				 */
}


/*
 *  Make hi rez screen bios handle 50 lines of 8x8 characters
 *
 * Adapted to C use from origonal asm posting
 *
 */
void hi50()
{
	/* Switch to 50 lines display */

	asm(".dc.w    $a000");		/* get the important pointers */

	asm("move.l  04(a1),a1");	/* a1 -> 8x8 font header */

	asm("move.l  72(a1),-$0A(a0)");	/* v_off_ad <- 8x8 offset table addr */
	asm("move.l  76(a1),-$16(a0)");	/* v_fnt_ad <- 8x8 font data addr */

	asm("move.w  #008,-$2E(a0)");	/* v_cel_ht <- 8    8x8 cell height */
	asm("move.w  #049,-$2A(a0)");	/* v_cel_my <- 49   maximum cell "Y" */
	asm("move.w  #640,-$28(a0)"); /* v_cel_wr <- 640  offset to cell Y+1 */

}
   
/*
 * Make hi rez screen bios handle 25 lines of 8x16 characters
 *
 */
void hi25()
{
	
	/* Switch to 25 lines display */


	asm(".dc.w    $a000");		/* get the important pointers */
	
	asm("move.l  08(a1),a1");	/* a1 -> 8x16 font header */

	asm("move.l  72(a1),-$0A(a0)");	/* v_off_ad <- 8x16 offset table addr */
	asm("move.l  76(a1),-$16(a0)");	/* v_fnt_ad <- 8x16 font data addr */

	asm("move.w  #0016,-$2E(a0)");	/* v_cel_ht <- 16    8x16 cell height */
	asm("move.w  #0024,-$2A(a0)");	/* v_cel_my <- 24    maximum cell "Y" */
	asm("move.w  #1280,-$28(a0)");	/* v_cel_wr <- 1280  vertical byte offset */
}

#else

/* **WARNING** **WARNING** **WARNING** **WARNING** **WARNING** **WARNING** */
/*									   */
/*     MWC  dependant, this routine must return the base address of	   */
/*	of the linea variable block					   */
/*									   */
/* **WARNING** **WARNING** **WARNING** **WARNING** **WARNING** **WARNING** */
#include <linea.h>
/*
 * return the base address of the line A variables 
 */

int *aaddress()
{
	linea0();	/* Init LineA - dumps returned values into la_init */
	return (int *)(la_init.li_a0); /* Return address of Parameter Block */
}

		/* Mark Williams C 	   */
		/* See file hi5025.s       */
		/* for functions hi50()    */
		/* and hi25()		   */
#endif /* MWC */
#endif /* MANX */
#endif /* __GNUC__ */	/* see ghi5025 for Gnu version */
#endif /* REMOTE */
#endif /* STANDALONE */

/* -eof- */
