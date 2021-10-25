/*
 *			    ACKNOWLEDGEMENTS
 *
 *	ZMDM was derived from rz/sz for Unix  posted by 
 *	Chuck Forsberg (...!tektronix!reed!omen!caf ). We
 *	thank him for his excellent code, and for giving
 *	us permission to use and distribute his code and
 *	documentation.
 *
 *	Atari St version by:
 *	Jwahar Bammi
 * 	bang:   {any internet host}!dsrgsun.ces.CWRU.edu!bammi
 * 	domain: bammi@dsrgsun.ces.CWRU.edu
 *	GEnie:	J.Bammi
 */

#include "config.h"

#define IN_COMMON

/*
 * -rev 08-17-86
 * mode function and most of the rest of the system dependent
 * stuff for rb.c and sb.c   This file is #included so the includer
 * can set parameters such as HOWMANY.  See the main file (rz.c/sz.c)
 * for compile instructions.
 */

#ifdef DLIBS
#include <string.h>
#endif

#include "zmdm.h"
#include "zmodem.h"

#include <stdarg.h>

#ifndef Supexec
		/* Some versions of osbind don't define Supexec */
#define Supexec(X) xbios(38,X)
#endif

#define CRCTABLE

		/* GLOBALS */
int Zmodem;		/* ZMODEM protocol requested */
int Nozmodem;	/* If invoked as "rb" */
int Batch;
int Verbose;
int Quiet;		/* overrides logic that would otherwise set verbose */
int Lleft;		/* number of characters in linbuf */
int Readnum;	/* Number of bytes to ask for in read() from modem */
int Crcflg, Nflag;
int ForceBinary;		/* local binary force override for rz */
int Wcsmask;

FILE *logfile;
FILE *STDERR;
int vdebug;	/* set if RDEBUG or SDEBUG */

char secbuf[TXBSIZE];
char linbuf[KSIZE];
#if (__GNUC__)			/* File i/o buffer */
unsigned char *bufr;	/* In GNU it is lmalloc()'ed in main.c */
#else
#ifndef DYNABUF
unsigned char bufr[BBUFSIZ];
#else
unsigned char *bufr;
#endif /* DYNABUF */
#endif
int fout;
int Lastrx;
int Firstsec;
int Eofseen;		/* indicates cpm eof (^Z) has been received */
int BEofseen;		/* EOF seen on input set by fooseek */
int errors;
long Bytesleft;		/* number of bytes of incoming file left */
long Modtime;		/* Unix style mod time for incoming file */
unsigned int Filemode;	/* Unix style mode for incoming file */
char Pathname[PATHLEN];
char *Progname;		/* the name by which we were called */

int Thisbinary;		/* current file is to be received in bin mode */
int Blklen;		/* record length of received packets */
char Lzmanag;		/* Local file management request */
char zconv;		/* ZMODEM file conversion request */
char zmanag;		/* ZMODEM file management request */
char ztrans;		/* ZMODEM file transport request */

jmp_buf tohere;		/* For the interrupt on RX timeout */
jmp_buf abrtjmp;	/* for force abort */
jmp_buf busjmp;		/* for bus errors */
jmp_buf addrjmp;	/* for address errors */
unsigned long BusErr, AddrErr;	/* saved vector addresses */
int Zctlesc;		/* Encode control characters */
int SendType;		/* Which send line to use	*/
int Modem;		/* Send using Xmodem */
int lsct;
int tryzhdrtype;	/* Header type to send corresponding to Last rx close */
int Txfcs32;		/* TRUE means send binary frames with 32 bit FCS */
long Thisflen;
int Crc32t;		/* Controls 32 bit CRC being sent */
			/* 1 == CRC32,  2 == CRC32 + RLE */
int Crc32r;		/* Indicates/controls 32 bit CRC being received */
			/* 0 == CRC16,  1 == CRC32,  2 == CRC32 + RLE */
int Usevhdrs;		/* Use variable length headers */
int Rxhlen;		/* Length of header received */

	/* Globals used by ZMODEM functions */
int Rxframeind;		/* ZBIN or ZHEX indicates type of frame received */
int Rxtype;		/* Type of header received */
int Rxcount;		/* Count of data bytes received */
int Rxtimeout;		/* Tenths of seconds to wait for something */
int Znulls;		/* Number of nulls to send at beginning of ZDATA hdr */
char Rxhdr[ZMAXHLEN];	/* Received header */
char Txhdr[ZMAXHLEN];	/* Transmitted header */
long Rxpos;		/* Received file position */
long Txpos;		/* Transmitted file position */
char Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */
char *Altcan;		/* Alternate canit string */
int Zrwindow;		/* RX window size (controls garbage count) */

		/* Globals used by Sz */
unsigned Txwindow; /* Control the size of the transmitted window */
unsigned Txwspac;	/* Spacing between zcrcq requests */
unsigned Txwcnt;	/* Counter used to space ack requests */
long Lrxpos;		/* Receiver's last reported offset */
int Lskipnocor;
int Rxclob;	/* Clobber existing file */

char Lzconv;	/* Local ZMODEM file conversion request */
char Lztrans;
char *Cmdstr;		/* Pointer to the command string */
int Optiong;		/* Let it rip no wait for sector ACK's */
int Totsecs;		/* total number of sectors this file */
int Command;		/* Send a command, then exit. */
int Cmdack1;		/* Rx ACKs command, then do it */
int Exitcode;
int Testattn;		/* Force receiver to send Attn, etc with qbf. */
int Wantfcs32;		/* want to send 32 bit FCS */
jmp_buf intrjmp;	/* For the interrupt on RX CAN */

char *qbf;
unsigned int Rxbuflen;	/* Receiver's max buffer length */
int Cmdtries;
int Filcnt;		/* count of number of files opened */
int Lfseen;
int Tframlen;		/* Override for tx frame length */
int blkopt;		/* Override value for zmodem blklen */
int Rxflags;
int Ascii;		/* Add CR's for brain damaged programs */
int Fullname;		/* transmit full pathname */
int Unlinkafter;	/* Unlink file after it is sent */
int Dottoslash;		/* Change foo.bar.baz to foo/bar/baz */
int errcnt;		/* number of files unreadable */
int siggi;		/* line interrupt enable flag     */

/* crctab calculated by Mark G. Mendel, Network Systems Corporation */
unsigned int crctab[256] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

/*
 * Copyright (C) 1986 Gary S. Brown.  You may use this program, or
 * code or tables extracted from it, as desired without restriction.
 *
 *   Need an unsigned type capable of holding 32 bits;
 */
typedef unsigned long int UNS_32_BITS;

/* First, the polynomial itself and its table of feedback terms.  The  */
/* polynomial is                                                       */
/* X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0 */
/* Note that we take it "backwards" and put the highest-order term in  */
/* the lowest-order bit.  The X^32 term is "implied"; the LSB is the   */
/* X^31 term, etc.  The X^0 term (usually shown as "+1") results in    */
/* the MSB being 1.                                                    */

/* Note that the usual hardware shift register implementation, which   */
/* is what we're using (we're merely optimizing it by doing eight-bit  */
/* chunks at a time) shifts bits into the lowest-order term.  In our   */
/* implementation, that means shifting towards the right.  Why do we   */
/* do it this way?  Because the calculated CRC must be transmitted in  */
/* order from highest-order term to lowest-order term.  UARTs transmit */
/* characters in order from LSB to MSB.  By storing the CRC this way,  */
/* we hand it to the UART in the order low-byte to high-byte; the UART */
/* sends each low-bit to hight-bit; and the result is transmission bit */
/* by bit from highest- to lowest-order term without requiring any bit */
/* shuffling on our part.  Reception works similarly.                  */

/* The feedback terms table consists of 256, 32-bit entries.  Notes:   */
/*                                                                     */
/*     The table can be generated at runtime if desired; code to do so */
/*     is shown later.  It might not be obvious, but the feedback      */
/*     terms simply represent the results of eight shift/xor opera-    */
/*     tions for all combinations of data and CRC register values.     */
/*                                                                     */
/*     The values must be right-shifted by eight bits by the "updcrc"  */
/*     logic; the shift must be unsigned (bring in zeroes).  On some   */
/*     hardware you could probably optimize the shift in assembler by  */
/*     using byte-swap instructions.                                   */

unsigned long int crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
	0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L, 
	0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
	0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L, 
	0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 
	0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
	0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L, 
	0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
	0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL, 
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
	0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 
	0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
	0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL, 
	0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 
	0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
	0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L, 
	0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
	0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L, 
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
	0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 
	0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
	0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L, 
	0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 
	0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
	0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L, 
	0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
	0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL, 
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
	0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 
	0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
	0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L, 
	0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 
	0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
	0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L, 
	0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
	0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L, 
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
	0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 
	0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
	0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L, 
	0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 
	0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
	0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL, 
	0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
	0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L, 
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
	0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 
	0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
	0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L, 
	0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 
	0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
	0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL, 
	0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
	0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L, 
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 
	0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL, 
	0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L, 
	0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL 
}; 

#ifndef REMOTE
int *aline_addr;		/* Base addr of aline variables */
#endif

int hlines, ihlines;		/* # of lines on screen		*/
int rez;			/* current resolution		*/
int scolor = 0;			/* current fg/bg screen color toggle */

#ifndef REMOTE
#ifndef OVERSCAN
long *ms_ptr;			/* Pointer to my screen memory, aligned at
				   a 256 bytes boundary */
#else
long *ms_ptr;			/* Pointer to my screen memory, aligned at */
long *ms_log;			/* a 256 bytes boundary */
#endif /* OVERSCAN */

#endif /* REMOTE */

#ifndef CPM
#if (!__GNUC__)
#ifndef REMOTE
long m_screen[8*1024+32];	/* My screen memory
				   32K bytes + 256 Bytes guard for alignement
				   In the worst case when we align we have to
				   go 255 bytes from &m_screen[0], hence the
				   256 Byte guard is required */
#endif /* REMOTE */

#else /*  __GNUC__ */

#ifdef RECURSE
  #ifdef BIGSTACK
    #if __GNUC__
long _stksize = 128L * 1024L;
    #else
long _STKSIZ = 128L * 1024L;
    #endif
  #else
    #if __GNUC__
long _stksize = 16L * 1024L;
    #else
long _STKSIZ = 16L * 1024L;
    #endif
  #endif /* BIGSTACK */
#else
  #if __GNUC__
long _stksize = 16384L;
  #else
long _STKSIZ = 16384L;
  #endif
#endif /* RECURSE */

#ifndef REMOTE
long *m_screen;			/* Presently Mark Willams will not allow
				   > 32K static structures */
#endif /* REMOTE */

#endif /* MWC */
#endif /* CPM */
#ifdef DLIBS
  #ifdef RECURSE
    #ifdef BIGSTACK
long _STKSIZE = 128L * 1024L;
    #else
long _STKSIZE = 16L * 1024L;
    #endif /* BIGSTACK */
  #else
long _STKSIZE = 16384L;
  #endif /* RECURSE */
#endif /* DLIBS */

struct stat statbuf;		  /* Disk Transfer address for Find first etc */
int Baudrate;			  /* Current baud rate			      */
long drv_map;			  /* bit vector of valid drives */

#ifndef	HIBAUD
BAUDS vbauds[] = {
    { "19200", 0, 19200 },
    { "9600" , 1,  9600 },
    { "4800" , 2,  4800 },
    { "4800" , 2,  4800 },
    { "2400" , 4,  2400 },
    { "2400" , 4,  2400 },
    { "2400" , 4,  2400 },
    { "1200" , 7,  1200 },
    { "1200" , 7,  1200 },
    { "300"  , 9,   300 },
#if 0
    { "38400", 4, 3840  },	/* (4,1,8, -1, -1, -1) */
    { "57000", 3, 5700  },	/* (3,1,8, -1, -1, -1) */
#endif
    { (char *)NULL, -1, -1 }
};
#else
BAUDS vbauds[] = {
    { "38400", 0, 38400 },
    { "19200", 1, 19200 },
    { "9600" , 2,  9600 },
    { "4800" , 3,  4800 },
    { "2400" , 4,  2400 },
    { "1200" , 5,  1200 },
    { "600"  , 6,   600 },
    { "300"  , 7,   300 },
    { (char *)NULL, -1, -1 }
};

CBAUD cbauds[] = {
	{ 3, 2 },	/* 38400 : div 16, 2x */
	{ 3, 4 },	/* 19200 */
	{ 3, 8 },	/* 9600	*/
	{ 3, 16 },	/* 4800	*/
	{ 3, 32 },	/* 2400	*/
	{ 3, 64 },	/* 1200	*/
	{ 5, 32 },	/* 600	*/
	{ 5, 64 }	/* 300	*/
};
#endif /* HIBAUD */

#ifdef	FLOW_CTRL
FLOWS vflows[] = {
    { "none",				0},
    { "XON/XOFF",			1},
    { "RTS/CTS",			2},
    { "both XON/XOFF & RTS/CTS",	3}
};
#endif /* FLOW_CTRL */



IOREC save,	/* the original Iorec is saved here for the duration of this
		   process */
      *savep;	/* ptr returned by Iorec() */

char iobuf[IBUFSIZ]; /* My large Rs232 receive buffer */

#ifdef DYNABUF
long BBUFSIZ;
#endif /* DYNABUF */

#ifdef __GNUC__
extern volatile long pr_time;
#else
extern long pr_time;
#endif
extern void rd_time();
 
		/* statics */ 
static char *tsr_ptr = (char *)0x00fffa2dL; /* See St internals */
static long *hz_200 =  (long *)0x000004ba; /* Yes the Hitch Hikers */
					   /* Guide is wrong!! */
 
static long 
     alrm_time = 0L;	  /* Time of next timeout (200 Hz) */ 
 
static char *special[] = { 
".PRG", ".TOS", ".TTP", ".ARC", ".ACC", ".IMG", ".RSC", ".O",
".OBJ", ".NEO", ".ZOO", ".CZ" , ".HZ" , ".GF" , ".PK" , ".LZH",
".PIC", ".PI1", ".PI2", ".PI3", ".PQ1", ".PQ2", ".PQ3", ".BRD",
".ANI", ".STW", ".OLB", ".LZS",
".FNT", ".PRT", ".SNG", ".NEC", ".CNF", ".Z"  , ".DFN", ".GEM",
".EZD", ".LNK", 
".SYM", ".DVI", ".PIX", ".X32", ".OUT", ".A",   ".CCC", ".CL",
".CMD", ".COM", 
".CRL", ".DAT", ".DIR", ".EXE", ".OVL", ".PAG", ".REL", ".SAV",
".SUB", ".SWP", 
".SYS", ".TAR", ".UTL", ".BIN", ".LBR", ".IM",  ".PAK",
".SBM", ".OBM", ".DIC", ".NDX", ".TFM", ".PBM", ".PBP", ".PCC",
".PXL", ".SYM", ".EL",  ".ELC",
"" 
}; 
static char *in();
 
/* 
 * mode(n) 
 *  2: set a cbreak, XON/XOFF control mode if using Pro-YAM's -g
 option 
 *  1: save old tty stat, set raw mode  
 *  0: restore original tty mode 
 */
int mode(n)
int n;
{

	vfile("mode:%d", n);
	switch(n) {

	case 2:	/* Cbreak mode used by sb when -g detected */
		return OK;
	case 1:
		return OK;
	case 0:
		while(Bconstat(1)) Bconin(1);	/* flush input              */

		return OK;
	default:
		return ERROR;
	}
}

/*
 * send a break
 * Modifies Bit 3 in the TSR (reg 23) of the Mfp
 */

void sendbrk()
{
	register long save_ssp;
	register long time;

	save_ssp = Super(0L);	/* Super Mode */

	/* set bit 3 of the TSR */
	*tsr_ptr |= (char)8;

	
	/* wait for 250 ms */
	time = *hz_200 + 50;
	while(*hz_200 < time)
		/* wait */ ;

	/* reset bit 3 of the tsr */
	*tsr_ptr &= (char)~8;

	Super(save_ssp);	/* Back to user Mode */
}

#ifdef TX
/*
 * Put transmit section of the MFP uart off
 *
 */
Txoff()
{
	register long save_ssp;

	save_ssp = Super(0L);	/* Super Mode */
	
	/* clear bit 0 of the tsr */
	*tsr_ptr &= (char)~1;

	Super(save_ssp);	/* Back to user Mode */
}

/*
 * Put transmit section of the MFP uart on
 *
 */
Txon()
{
	register long save_ssp;

	save_ssp = Super(0L);	/* Super Mode */
	
	/* set bit 0 of the tsr */
	*tsr_ptr |= (char)1;

	Super(save_ssp);	/* Back to user Mode */
}
#endif /* TX */


/*
 * read_modem() - read upto count characters from the modem port
 * Check for user abort (^C) and timeout at the same time
 */
int read_modem(buf, count)
register char *buf;
register int count;
{
	register int n;
	extern void rd_time();

	n = 0;

	while(1)
	{
#ifndef REMOTE
		if(Bconstat(2))
		{
			/* Character Hit at the Keyboard - is it ^C */
			if((Bconin(2) & 0x7f) == CTRL('C'))
			{
			    /* It is a Control-C */
				if(SendType)
					bibis(3);
				else
					bibi(3);
			}
			
		}
#endif
		if(Bconstat(1))
		{
			/* Character available at Modem Port */
			n++;
			*buf++ = Bconin(1);

			while((n < count)  && Bconstat(1))
			{
				*buf++ = Bconin(1);
				n++;

			}
			return n;
		}
		
		/* Check for time out if required */
		if(alrm_time != 0)
		{
			/* Alarm Set */
			Supexec(rd_time);
			if(pr_time >= alrm_time)
			    /* timeout */
			    longjmp(tohere, -1);
		}
	}
}


/*
 * alarm() - set the alarm time
 */
void stalarm(n)
unsigned int n;
{
	extern void rd_time();

	if(n > 0)
	{
		Supexec(rd_time);
		/* We really need n * 200 but n * 256 is close enough */
		alrm_time = pr_time + ( n << 8 );
	}
	else
	    alrm_time = 0L;
}

/*
 * write_modem() - send buffer to the modem port
 *
*/
void write_modem(buf,len)
register char *buf;
register int len;
{
	while(len-- > 0)
	   	Bconout(1, *buf++);
}


/*
 * flushinput() - flush any characters in the modem port
 * a future enhancement may be flush any characters that are in there
 * and any that come in over the next 1 second -- Ymodem does that
 *
 */
void flushinput()
{
	while(Bconstat(1))
	    Bconin(1);
}



int isbinary(name)
register char *name;
{
	register char **p;
	register char *ext;

	if((ext = strrchr(name,'.')) == (char *)NULL)
	{
		return 0;
	}
	
	for(p = special; **p != '\0'; p++)
	    if(ustrcmp(ext,*p) == 0)
		return 1;
	return 0;
}

#define upper(X) (islower(X)?toupper(X):X)

/* Strcmp - case insensative */
int ustrcmp(s1,s2)
register char *s1, *s2;
{
	while(*s1 != '\0')
	{
		if(*s2 == '\0')
		    return 1;
		if(upper(*s1) != *s2)
		    return 1;
		s1++;
		s2++;
	}
	if(*s2 != '\0')
	    return 1;
	else
	    return 0;
}


/*
 * Ensure that each subdirectory in the pathname exists.
 * If one doesn'nt, make the rest of the directory
 * Returns ERROR if it has trouble creating a path
 *
 */
int pathensure(name)
char *name;
{
	if(strchr(name, '\\') == (char *)NULL)
		/* nothing to check */
		return OK;

	return pathrest(name, (char *)NULL);
}

/*
 * check rest of the path recursively
 *	If any component of the path name is longer than 8/12 characters then
 *	warn.
 *
 */
int pathrest(name, prev)
char *name, *prev;
{
	char previous[128];
	char component[13];
	register char *p, *q, *r, *s;
	register int warn;

	if((r = strchr(name, '\\')) == (char *)NULL)
		/* nothing more to check */
		return OK;

	/* pick up the next component of the path name, 8/12 chars max for ST */
	warn = 0;
	if((s = in(name, r, '.')) == (char *)NULL)
	{
		/* component does'nt contain a dot */
		if( ((long)r - (long)name) > 8)
		{
			warn++;
			strncpy(component, name, 8);
			q = &component[8];
		}
		else
		{
			register int n;

			n = (int)((long)r - (long)name);
			strncpy(component, name, n);
			component[n] = '\0';
			q = &component[((int)strlen(component))];
		}
	}
	else
	{
		/* component contains a dot */
		if(((long)s - (long)name) > 8)
		{
			warn++;
			strncpy(component, name, 8);
			q = &component[8];
		}
		else
		{
			for(p = name, q = component; p != s; )
				*q++ = *p++;
		}
		*q++ = '.';
		s++;
		if(s != r)
		{
			if(((long)r - (long)s) > 3)
			{
				warn++;
				strncpy(q, s, 3);
				q = &q[3];
			}
			else
			{
				for(p = s; p != r; )
					*q++ = *p++;
			}
		}
	}
	*q = '\0';

#ifndef REMOTE
	if(warn)
	{
		fprintf(stderr,
		"?WARNING - A component of the path is longer than 8/12 chars\n");
		fprintf(stderr,"%s --> %s\n", name, component);
	}
#endif
	if(prev == (char *)NULL)
		strcpy(previous, component);
	else
	{
		strcpy(previous, prev);
		strcat(previous, "\\");
		strcat(previous, component);		
	}

	if(existd(previous))
		/* it exists - go do rest */
		return pathrest(++r, previous);

	/* does not exist, 			    */
	/* make this component and all its children */
	return makesubtree(++r, previous);
}

/*
 * make a subtree
 */
int makesubtree(name, prev)
char *name, *prev;
{
	char previous[128];
	char component[13];
	register char *p, *q, *r, *s;
	register int warn;

	if(Dcreate(prev) != 0)
	{
		log3("Trouble trying to create subtree\n");
		fprintf(STDERR,"%s\n", prev);
		return ERROR;
	}
#ifndef REMOTE
	fprintf(stderr,"Created Directory %s\n", prev);
#endif
	if((r = strchr(name, '\\')) == (char *)NULL)
		/* nothing more to do */
		return OK;

	/* pick up the next component of the path name, 8/12 chars max for ST */
	warn = 0;
	if((s = in(name, r, '.')) == (char *)NULL)
	{
		/* component does'nt contain a dot */
		if( ((long)r - (long)name) > 8)
		{
			warn++;
			strncpy(component, name, 8);
			q = &component[8];
		}
		else
		{
			register int n;

			n = (int)((long)r - (long)name);
			strncpy(component, name, n);
			component[n] = '\0';
			q = &component[((int)strlen(component))];
		}
	}
	else
	{
		/* component contains a dot */
		if(((long)s - (long)name) > 8)
		{
			warn++;
			strncpy(component, name, 8);
			q = &component[8];
		}
		else
		{
			for(p = name, q = component; p != s; )
				*q++ = *p++;
		}
		*q++ = '.';
		s++;
		if(s != r)
		{
			if(((long)r - (long)s) > 3)
			{
				warn++;
				strncpy(q, s, 3);
				q = &q[3];
			}
			else
			{
				for(p = s; p != r; )
					*q++ = *p++;
			}
		}
	}
	*q = '\0';

#ifndef REMOTE
	if(warn)
	{
		fprintf(stderr,
		"?WARNING - A component of the path is longer than 8/12 chars\n");
		fprintf(stderr,"%s --> %s\n", name, component);
	}
#endif
	strcpy(previous, prev);
	strcat(previous, "\\");
	strcat(previous, component);		



	/* go do the rest of them */
	return makesubtree(++r, previous);
}


/*
 * test if a subdirectory exists
 * include special case of 'D:\' that Fsfirst does'nt handle correctly
 */
int existd(name)
register char *name;
{
	/* assuming the DTA buffer is already set up */
	extern long drv_map;
	extern struct stat statbuf;
	register int drive;
	
	if (Fsfirst(name , 0x0021|0x0010) == 0)
	{
		if((statbuf.st_mode & 0x0010) == 0x0010)
			return TRUE;
	}

	/* Gemdos doesn't like d:\ style dirs */
	if((name[3] == '\0') && (name[2] == '\\') && (name[1] == ':'))
	{
		drive = name[0];
		if(isupper(drive))
			drive = tolower(drive);

		drive = drive - 'a';
		if((drv_map & (1L << drive)) == 0)
			return FALSE;
		else
			return TRUE;
	}
	/* Nor does Gemdos understand '.' or '..' */
	/* Hey Atari, don't you guys ever test anything */
	if((strcmp(name,".") == 0) || (strcmp(name,"..") == 0) ||
	   (strcmp(name,".\\") == 0) || (strcmp(name,"..\\") == 0))
		return TRUE;

	return FALSE;
}

/*
 * Does a file exist
 */
int existf(name)
register char *name;
{
	/* assuming the DTA buffer is already set up */
	
	return (Fsfirst(name , 0x0001) == 0);

}

/*
 * stsystem(cmd) - execute a process
 * char *cmd;   command to execute (including redirections etc)
 */
int stsystem(cmd)
char *cmd;
{
        register char *ptr1;             /* general pointers */
        register char save;              /* */
        register int status;            /* return status       */
        register char *args;            /* arguments            */
        register int len;               /* length of args - 2   */
        char nils[2];
#ifdef __GNUC__
        extern void *malloc(size_t);
#else
        extern char *malloc();
#endif


        nils[0] = nils[1] = '\0';
        for(ptr1 = cmd; (!isspace(*ptr1)) && (*ptr1 != '\0'); ptr1++)
                /* skip till end of path name */;
        if(*ptr1 != '\0')
        {
                /* cmd does have a command tail */
                /* save the char at the position and terminate the path */
                save = *ptr1;
                *ptr1++ = '\0'; /* command tail is the rest of it */

#ifdef __GNUC__
                if((args = (char *) malloc((size_t)((len = strlen(ptr1)) + 2)))
#else
                if((args = (char *) malloc((len = strlen(ptr1)) + 2))
#endif
                    == (char *)NULL)
                {
                        /* could not allocate memory */
                        return(-1);
                }
                *args++ = len;
                strcpy(args,ptr1);
                args--;
                /* now do the load and go */
                status = Pexec(0,cmd,args,(char *)NULL);
                /* restore cmd to original state */
                *--ptr1 = save;
                free(args);
        }
        else
        {
                /* command does not have a tail */

                /* now do the load and go */
                status = Pexec(0,cmd,nils,(char *)NULL);
        }

        return(status);
}

void stsleep(n)
int n;
{
	extern void rd_time();

	if(n != 0)
	{
		Supexec(rd_time);
		/* We really need n * 200 but n * 256 if close enough */
		alrm_time = pr_time + ( n << 8 );

		while(alrm_time > pr_time)
		{
			Supexec(rd_time);
		}
		alrm_time = 0L;
	}
}


void initz()
{
	 Zmodem=0;		/* ZMODEM protocol requested */
	 Nozmodem = 0;		/* If invoked as "rb" */
	 Batch=0;
	 Verbose=0;
	 lsct = 1;
	 Quiet=0;		/* overrides logic that would otherwise set verbose */
	 Lleft=0;		/* number of characters in linbuf */

	 logfile = (FILE *)NULL;
	 vdebug = 0;

	 Readnum = KSIZE;	/* Number of bytes to ask for in read() from modem */
	 Crcflg = FALSE;
	 Wcsmask = 0377;
	 Modtime = 0;
	 Nflag = FALSE;
	 fout = (-1);
	 errors = 0;
	 Rxtimeout = 100;	/* Tenths of seconds to wait for something */
	 Modem = 0;
	 Blklen = SECSIZ;	/* record length of received packets */

	qbf="The quick brown fox jumped over the lazy dog's back 1234567890\r\n";
	Cmdtries = 11;
	Filcnt=0;		/* count of number of files opened */
	Lfseen=0;
	Rxbuflen = (unsigned)16384;	/* Receiver's max buffer length */
	Tframlen = 0;		/* Override for tx frame length */
	blkopt=0;		/* Override value for zmodem blklen */
	Rxflags = 0;
	Ascii=0;		/* Add CR's for brain damaged programs */
	Fullname=0;		/* transmit full pathname */
	Unlinkafter=0;		/* Unlink file after it is sent */
	Dottoslash=0;		/* Change foo.bar.baz to foo/bar/baz */
	errcnt=0;		/* number of files unreadable */
	Testattn = FALSE;
	siggi = 0;
	Command = FALSE;
	Wantfcs32 = TRUE;	/* want to send 32 bit FCS */
	Znulls = 0;
	tryzhdrtype=ZRINIT;	/* Header type to send corresponding to Last rx close */
	Txfcs32 = FALSE;	/* TRUE means send binary frames with 32 bit FCS */
	ForceBinary = FALSE;	/* Local force binary override for rz */
	 
	 Txwindow = 0; /* Control the size of the transmitted window */
	 Txwspac= 0;	/* Spacing between zcrcq requests */
	 Txwcnt = 0;	/* Counter used to space ack requests */
	 Lrxpos = 0;		/* Receiver's last reported offset */
	 Lskipnocor = 0;
	 Zrwindow = 1400;	/* RX window size (controls garbage count) */
	 Rxclob=TRUE;	/* Clobber existing file */
	Crc32r = 0;
	Crc32t = 0;
	Zctlesc = 0;
	Optiong = FALSE;
	Usevhdrs = 0;		/* Use variable length headers */
	 Rxhlen = 0;
	 Thisflen = 0L;
	 Lztrans = 0;
	 Lzmanag = 0;
	 Lzconv = 0;
	 Cmdack1 = 0;
	 Eofseen = 0;		/* EOF seen on input set by zfilbuf */
	 BEofseen = 0;		/* EOF seen on input set by fooseek */

}

/* abort current session - due to async fault */
void aexit(n)
int n;
{
	longjmp(abrtjmp, n);
}

void buserr()
{
	longjmp(busjmp, -1);
}

void addrerr()
{
	longjmp(addrjmp, -1);
}

/*
 * Local console output simulation
 */
void bttyout(c)
int c;
{
	if (Verbose)
#ifdef REMOTE
		Bconout(1, c);
#else
		putc(c, stderr);
#endif
}

/*
 *  Send a character to modem.  Small is beautiful.
 */
#ifndef __GNUC__
void sendline(c)
int c;
{
#ifndef REMOTE
	if (Verbose>4)
	{
		fprintf(stderr, "Sendline: %x\n", c);

	}
#endif
	if (SendType)
		Bconout(1, c & 0377);
	else
		Bconout(1, c);

}

void flush_modem()
{
	while(Bcostat(1) == 0);
}
#endif /* __GNUC__ */


/*
 * substr(string, token) searches for token in string s
 * returns pointer to token within string if found, NULL otherwise
 */
char *
substr(s, t)
register char *s;
char *t;
{
	register char *ss,*tt;
	/* search for first char of token */
	for (ss=s; *s; s++)
		if (*s == *t)
			/* compare token with substring */
			for (ss=s,tt=t; ;)
			{
				if (*tt == '\0')
					return s;
				if (*ss++ != *tt++)
					break;
			}

	return ((char *)NULL);
}


/* send cancel string to get the other end to shut up */
void canit()
{
	static char canistr[] = {
	 ZPAD,ZPAD,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0
	};

	write_modem(canistr, (int)strlen(canistr));
	if(!SendType)
		Lleft=0;	/* Do read next time ... */

}


/*
 *  Debugging information output interface routine
 */
/* VARARGS1 */
void vfile(const char *fmt, ...)
{
	if (Verbose > 3)
	{	/* verbose 2 is normal for us */
	    va_list args1;
	    va_start(args1, fmt);
		vfprintf(STDERR, fmt, args1);
		fprintf(STDERR, "\r\n");
		va_end(args1);

		if(vdebug && (logfile != (FILE *)NULL))
		{
		    va_list args2;
		    va_start(args2, fmt);
			vfprintf(logfile, fmt, args2);
			fprintf(logfile, "\n");
			fflush(logfile);
			va_end(args2);
		}
	}
}

static char *in(from, to, c)
register char *from, *to;
register int c;
{
	for(; from < to; from++)
	{
		if(*from == c)
			return from;
	}
	return (char *)NULL;
}

/*
 * SetIoBuf() - Save the systems Rs232 buffer and install my large
 * Rs232 buffer.
 *
 */
void SetIoBuf()
{
	/* Get pointer to Rs232 input record */
	savep = (IOREC *)Iorec(0);
	
	/* Save the info */
	save.ibuf	= savep->ibuf;
	save.ibufsiz	= savep->ibufsiz;
	save.ibufhd	= savep->ibufhd;
	save.ibuftl	= savep->ibuftl;
	save.ibuflow	= savep->ibuflow;
	save.ibufhi	= savep->ibufhi;

	
	/* Install my buffer in its place */
	savep->ibuf		= &iobuf[0];
	savep->ibufsiz	= IBUFSIZ;
	savep->ibuflow	= IBUFSIZ/8;
	savep->ibufhi	= (IBUFSIZ / 4) * 3;
	savep->ibufhd = savep->ibuftl = 0;

	
}

/*
 * ResetIoBuf() - Reset the Rs232 buffer to the saved (system's) buffer
 *
 */
void ResetIoBuf()
{
	savep->ibuf	= save.ibuf;
	savep->ibufsiz	= save.ibufsiz;
	savep->ibuflow	= save.ibuflow;
	savep->ibufhi	= save.ibufhi;
	savep->ibufhd	= save.ibufhd;
	savep->ibuftl	= save.ibuftl;

}


/*
 * Log an error if verbose
 */

/*VARARGS1*/
void log3(const char *s, ...)
{
	if (!Verbose)
		return;
    fprintf(STDERR, "\nerror %d: ", errors);
    va_list args1;
    va_start(args1, s);
	vfprintf(STDERR, s, args1);
        fprintf(STDERR, "\r\n");
	va_end(args1);

#ifdef RDEBUG
	if(Verbose > 3)
	{
		fprintf(logfile, "error %d: ", errors);
        va_list args2;
        va_start(args2, s);
		vfprintf(logfile, s, args2);
		va_end(args2);
	}
#endif

}
/*
 * This version of readline is reasonably well suited for
 * reading many characters.
 * timeout is in tenths of seconds
 */
int readline(timeout)
int timeout;
{
#ifdef __GNUC__
	volatile int n;
#else
	register int n;
#endif
	static char *cdq;	/* pointer for removing chars from linbuf */

	if (--Lleft >= 0)
	{
#ifdef RDEBUG
		if (Verbose > 8)
		{
			fprintf(logfile, "%02x ", *cdq & 0377);
		}
#endif

		return (*cdq++ & 0377);
	}
/*	n = timeout/10; */
	n = timeout >> 3;  /* close enough for rock and roll - see alarm() */
	if (n < 2)
		n = 3;
#ifdef RDEBUG
	if (Verbose > 3)
		fprintf(logfile, "Calling read: n=%d ", n);
#endif

	if (setjmp(tohere))
	{
		Lleft = 0;
#ifdef RDEBUG
		if (Verbose>3)
		{
			fprintf(STDERR, "Readline:TIMEOUT\n");
			fprintf(logfile, "Readline:TIMEOUT\n");

		}
#endif
		return TIMEOUT;
	}
	stalarm(n);
	Lleft=read_modem(cdq=linbuf, Readnum);
	stalarm(0);

#ifdef RDEBUG
	if (Verbose > 3)
	{
		fprintf(logfile, "Read returned %d bytes\n", Lleft);
	}
#endif

	if (Lleft < 1)
		return TIMEOUT;
	--Lleft;

#ifdef RDEBUG
	if (Verbose > 8)
	{
		fprintf(logfile, "%02x ", *cdq & 0377);
	}
#endif

	return (*cdq++ & 0377);
}

void report(sct)
int sct;
{
#ifndef REMOTE
	if (Verbose>1)
		fprintf(STDERR,"%03d%c",sct,sct%10? ' ' : '\r');
#endif
}

void lreport(sct)
long sct;
{
#ifndef REMOTE
	if (Verbose>1)
		fprintf(STDERR,"%06ld%c",sct,lsct%10? ' ' : '\r');
	lsct++;
#endif
}


#ifdef DLIBS
_initargs()
{
}
#endif

void wr_modem(s)
register char *s;
{
	while(*s != '\0')
		Bconout(1, *s++);
}

#ifdef DYNABUF
unsigned char *dalloc()
{
	register long avail;

	if((avail = (long)Malloc(-1L) - LEAVEALONE) < MINACC)
		return (unsigned char *)NULL;

	BBUFSIZ = avail;
	return (unsigned char *)Malloc(BBUFSIZ);
}
#endif /* DYNABUF */

/* *******************************************************
	This code courtesy of katzung@laidbak.UUCP
	 -- thank you very much 
   ******************************************************* */


/* From laidbak!katzung@Sun.COM Tue Apr 26 19:34:41 1988
Return-Path: <laidbak!katzung@Sun.COM>
From: Brian Katzung <laidbak!katzung@Sun.COM>
To: bammi@mandrill.CES.CWRU.Edu
Subject: Re:  UW
*/

#define	MFP	0xFFFFFA01L
#define	TDDR	36
#define	hz_200	((unsigned long *) 0x4BAL)

/* Baud rates 75 (120) and 50 (80) are not currently decoded. */
#ifndef	HIBAUD
static int	timevals[] = {
	1,	2,	4,	5,	8,	10,	11,
	16,	32,	64,	96,	128,	143,	175
};
#else
/* 300 & 600 overlap 1200 & 2400 */
static int	timevals[] = {
	2,	4,	8,	16,	32,	64
};
#endif /* HIBAUD */


/* load rs232 baud rate, returns -1 if not known */
int getbaud ()
{
	register int	i;
	int	tv;			/* Timer value */
	int	maxtv;			/* Maximum timer value */
	unsigned long	endhz;		/* End of polling period */
	long	savessp;		/* Old stack pointer */

	savessp = Super(0L);
	maxtv = 0;
	endhz = *hz_200 + 8;
	while (*hz_200 < endhz)
	{
		tv = *((unsigned char *) (MFP + TDDR));
		if (tv > maxtv)
			maxtv = tv;
	}
	(void) Super(savessp);
	for (i = sizeof(timevals) / sizeof(timevals[0]);
	  --i >= 0 && timevals[i] != maxtv; );
#ifdef	HIBAUD
	if (i >= 0) {
		for (tv = 0;
		 vbauds[tv].sbaud != NULL || vbauds[tv].ibaud != i; tv++);
		if (vbauds[tv].sbaud != NULL)
			return(tv);
	}
#endif /* HIBAUD */
	return(i);
}

/* -eof- */
