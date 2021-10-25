/*
 * 	Common include file
 *
 *	Jwahar Bammi
 * 	bang:   {any internet host}!dsrgsun.ces.CWRU.edu!bammi
 * 	domain: bammi@dsrgsun.ces.CWRU.edu
 *	GEnie:	J.Bammi
 */

extern int Zmodem;		/* ZMODEM protocol requested */
extern int Nozmodem;		/* If invoked as "rb" */
extern int Batch;
extern int Verbose;
extern int Quiet;		/* overrides logic that would otherwise set verbose */
extern int Lleft;		/* number of characters in linbuf */
extern int Readnum;		/* Number of bytes to ask for in read() from modem */
extern int Crcflg, Nflag;
extern int ForceBinary;		/* local binary force override for rz */
extern char secbuf[];
extern char linbuf[];
extern unsigned char *bufr;
extern int fout;
extern int Lastrx;
extern int Firstsec;
extern int Eofseen;		/* indicates cpm eof (^Z) has been received */
extern int BEofseen;		/* EOF seen on input set by fooseek */
extern int errors;
extern long Bytesleft;		/* number of bytes of incoming file left */
extern long Modtime;		/* Unix style mod time for incoming file */
extern unsigned int Filemode;	/* Unix style mode for incoming file */
extern char Pathname[];
extern char *Progname;		/* the name by which we were called */
extern int Zctlesc;		/* Encode control characters */
extern int SendType;		/* Which send line to use	*/
extern int Wcsmask;

extern int Thisbinary;		/* current file is to be received in bin mode */
extern int Blklen;		/* record length of received packets */
extern char Lzmanag;		/* Local file management request */
extern char zconv;		/* ZMODEM file conversion request */
extern char zmanag;		/* ZMODEM file management request */
extern char ztrans;		/* ZMODEM file transport request */
extern jmp_buf tohere;		/* For the interrupt on RX timeout */
extern jmp_buf busjmp;		/* for bus errors */
extern jmp_buf addrjmp;		/* for address errors */
extern void (*BusErr)(void); /* saved vector addresses */
extern void (*AddrErr)(void);
extern void buserr();		/* Bus error handler */
extern void addrerr();		/* address error handler */
extern int Modem;		/* Send using Xmodem */
extern FILE *logfile;
extern FILE *STDERR;
extern int vdebug;
extern int lsct;
extern int tryzhdrtype;		/* Header type to send corresponding to Last rx close */
extern int Txfcs32;		/* TRUE means send binary frames with 32 bit FCS */

 	/* Globals used by ZMODEM functions */
extern int Rxframeind;		/* ZBIN or ZHEX indicates type of frame received */
extern int Rxtype;		/* Type of header received */
extern int Rxcount;		/* Count of data bytes received */
extern int Rxtimeout;		/* Tenths of seconds to wait for something */
extern char Rxhdr[];		/* Received header */
extern char Txhdr[];		/* Transmitted header */
extern long Rxpos;		/* Received file position */
extern long Txpos;		/* Transmitted file position */
extern char Attn[];		/* Attention string rx sends to tx on err */

extern int Zrwindow;		/* RX window size (controls garbage count) */

	/* Globals specific to Sz */
extern unsigned Txwindow; /* Control the size of the transmitted window */
extern unsigned Txwspac;	/* Spacing between zcrcq requests */
extern unsigned Txwcnt;	/* Counter used to space ack requests */
extern long Lrxpos;		/* Receiver's last reported offset */
extern int Lskipnocor;
extern int Rxclob;	/* Clobber existing file */
extern long Thisflen;
extern int Crc32t;	/* Controls 32 bit CRC being sent */
			/* 1 == CRC32,  2 == CRC32 + RLE */
extern int Crc32r;	/* Indicates/controls 32 bit CRC being received */
			/* 0 == CRC16,  1 == CRC32,  2 == CRC32 + RLE */
extern int Usevhdrs;	/* Use variable length headers */
extern int Rxhlen;	/* Length of header received */

extern long Baudrate;
extern int Ascii;		/* Add CR's for brain damaged programs */
extern int Fullname;		/* transmit full pathname */
extern int Unlinkafter;		/* Unlink file after it is sent */
extern int Dottoslash;		/* Change foo.bar.baz to foo/bar/baz */
extern int errcnt;		/* number of files unreadable */
extern int Optiong;		/* Let it rip no wait for sector ACK's */
extern int Totsecs;		/* total number of sectors this file */
extern int Filcnt;		/* count of number of files opened */
extern int Lfseen;
extern unsigned int Rxbuflen;	/* Receiver's max buffer length */
extern int Tframlen;		/* Override for tx frame length */
extern int blkopt;		/* Override value for zmodem blklen */
extern int Rxflags;
extern char Lzconv;		/* Local ZMODEM file conversion request */
extern char Lztrans;
extern int Command;		/* Send a command, then exit. */
extern char *Cmdstr;		/* Pointer to the command string */
extern int Cmdtries;
extern int Cmdack1;		/* Rx ACKs command, then do it */
extern int Exitcode;
extern int Testattn;		/* Force receiver to send Attn, etc with qbf. */
extern char *qbf;
extern jmp_buf intrjmp;		/* For the interrupt on RX CAN */
extern jmp_buf abrtjmp;		/* for force abort */
extern int siggi;		/* Line interrupt enable flag */
extern int Wantfcs32;		/* want to send 32 bit FCS */
extern int Znulls;		/* Number of nulls to send at beginning of ZDATA hdr */

#ifndef REMOTE
extern int *aline_addr;		/* Base addr of aline variables */
#endif

extern int hlines, ihlines;	/* # of lines on screen		*/
extern int rez;			/* current resolution		*/
extern int scolor;		/* current fg/bg screen color toggle */

#ifndef REMOTE
extern long *ms_ptr;			/* Pointer to my screen memory */
#if __GNUC__
extern long *m_screen;
#else
extern long m_screen[];
#endif /* __GNUC__ */
#endif /* REMOTE */

#if (__GNUC__)
extern long _stksize;
#endif /* __GNUC__ */

#ifdef DLIBS
extern long _STKSIZE;
#endif /* DLIBS */

extern struct stat statbuf;		  /* Disk Transfer address for Find first etc */
extern long Baudrate;			  /* Current baud rate			      */
extern long drv_map;			  /* bit vector of valid drives */

#ifdef	FLOW_CTRL
extern FLOWS vflows[];
#endif

#ifdef	HIBAUD
extern CBAUD cbauds[];
#endif

extern BAUDS vbauds[];
extern IOREC save,	/* the original Iorec is saved here for the duration
			   of this process */
 	*savep;		/* ptr returned by Iorec() */

extern char iobuf[]; /* My large Rs232 receive buffer */
extern char oobuf[]; /* My large Rs232 send    buffer */

extern char *substr();

#ifdef DYNABUF
extern long BBUFSIZ;
extern unsigned char *dalloc();
#endif /* DYNABUF */

#ifdef __GNUC__
extern volatile long pr_time;
#else
extern long pr_time;
#endif
extern void rd_time();

/* -eof- */
