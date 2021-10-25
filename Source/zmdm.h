/*
 * 	Common include file
 *
 *
 * bang:   uunet!cadence!bammi			jwahar r. bammi
 * domain: bammi@cadence.com
 * GEnie:	J.Bammi
 * CIS:    71515,155
 */

#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include <osbind.h>
#include "decl.h"

	/* See configurable parameters towards the end of the file */
	/* Leave the rest alone					  */


	
 	/* Common defines */
#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif
#define	FALSE 		0
#define TRUE 		1


#define EscSeq(x)	Bconout(2,(int)'\033'); Bconout(2,(int)x) /* Send Esc.
							Seq. to	 console */
#define CTRL(X)		(X & 037) /* CTRL-anything */

#define STOS	  	"GEMDOS/TOS"
#define ZMDMVERSION	"$Revision: 1.76 $ $Date: 1991/04/27 22:12:49 $"

/* Ward Christensen / CP/M parameters - Don't change these! */
#define ENQ 005
#define CAN ('X'&037)
#define XOFF ('s'&037)
#define XON ('q'&037)
#define SOH 1
#define STX 2
#define EOT 4
#define ACK 6
#define NAK 025
#define CPMEOF 032
#define WANTCRC 0103	/* send C not NAK to get crc not checksum */
#define WANTG 0107	/* Send G not NAK to get nonstop batch xmsn */
#define TIMEOUT (-2)
#define RCDO (-3)
#define GCOUNT (-4)
/* #define ERRORMAX 5 */
#define RETRYMAX 10	/* for dogs like compuserve */
#define WCEOT (-10)
#define SECSIZ 128	/* cp/m's Magic Number record size */
#define PATHLEN 128	/* ready for 4.2 bsd ? noooooo */
#define KSIZE 1024	/* record size with k option */
#define UNIXFILE 0x8000	/* happens to the the S_IFREG file mask bit for stat */
#define DEFBYTL 2000000000L	/* default rx file size */
#define WANTG 0107	/* Send G not NAK to get nonstop batch xmsn */
#define TXBSIZE 16384

#define PURGELINE	while(Bconstat(1))Bconin(1);Lleft = 0

#define Bauxws(X)	 wr_modem(X)

#define RLOGFILE "rzlog"
#define zperr vfile
#define zperr2 vfile2

#define OK 0

#ifdef ERROR
#undef ERROR
#endif
#define ERROR (-1)

#define HOWMANY 133
/* Parameters for ZSINIT frame */
#define ZATTNLEN 32	/* Max length of attention string */

 	
 	/* Types */

#ifdef LONG
#undef LONG	/* Get rid of stupid portab.h definition */
#endif

#ifdef WORD
#undef WORD
#endif

#ifdef UWORD
#undef UWORD
#endif

struct	stat
{
	char	st_sp1[21];    /* Junk 	   */
	char	st_mode;       /* File attributes */
	short	st_time;       /* Mod Time	  */
	short   st_date;       /* Mod date	  */
	long	st_size;       /* File size	   */
	char	st_name[14];   /* File name	   */
};

/* The structure returned by Iorec(), really ptr to this type */
typedef struct {
	char  *ibuf;	/* input */
	short ibufsiz;
	short ibufhd;
	short ibuftl;
	short ibuflow;
	short ibufhi;

} IOREC;

typedef  struct {
	char *sbaud;
	int  ibaud;
	int  jbaud;
} BAUDS;

#ifdef	FLOW_CTRL
typedef	struct {
	char *sflow;
	int  iflow;
} FLOWS;
#endif

#ifdef	HIBAUD
typedef	struct {
	char tdc;
	char tdd;
} CBAUD;
#endif


/*
 * updcrc macro derived from article Copyright (C) 1986 Stephen Satchell. 
 *  NOTE: First srgument must be in range 0 to 255.
 *        Second argument is referenced twice.
 * 
 * Programmers may incorporate any or all code into their programs, 
 * giving proper credit within the source. Publication of the 
 * source routines is permitted so long as proper credit is given 
 * to Stephen Satchell, Satchell Evaluations and Chuck Forsberg, 
 * Omen Technology.
 */
#ifndef IN_COMMON
extern unsigned short crctab[]; /* see definition in common.c */
extern unsigned long int crc_32_tab[]; /* see definition in common.c */
#endif

#define updcrc(cp, crc) \
( crctab[(((unsigned short)crc >> 8) & 255)] ^ ((unsigned short)crc << 8) ^ (unsigned short)cp)

#define UPDC32(octet, crc) \
(crc_32_tab[(((unsigned long)crc) ^ ((unsigned long)octet)) & 0xff] ^ (((unsigned long)crc) >> 8))

#ifdef	FLOW_CTRL
#define	FLOW_STRING(s)	((s < 0) ? "none" : vflows[s].sflow)
#endif

#define BAUD_STRING(s)	((s < 0) ? "**UNKNOWN**" : vbauds[s].sbaud)
#define BAUD_RATE(s)	((s < 0) ? 9600 : vbauds[s].jbaud)

#ifdef __GNUC__

extern int SendType;
extern int Wcsmask;

inline static void sendline(c)
int c;
{
	if (SendType)
		Bconout(1, c & Wcsmask);
	else
		Bconout(1, c);
}

inline static void flush_modem()
{
	while(Bcostat(1) == 0);
}
#endif /* __GNUC__ */

/* -eof- */
