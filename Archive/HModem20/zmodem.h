/*
 *   Z M O D E M . H     Manifest constants for ZMODEM
 *    application to application file transfer protocol
 *    05-23-87  Chuck Forsberg Omen Technology Inc
 *    12-10-87  Modified by HM for cpm
 */

#ifdef   EXTERN
#undef   EXTERN
#endif

#ifndef ZM
#define  EXTERN    extern
#else
#define  EXTERN
#endif

/***************** line numbers for report function *************************/

#define     PROTOCOL    3
#define     PATHNAME    4
#define     FILESIZE    5
#define     BLKCHECK    6
#define     SENDTIME    7
#define     KBYTES      8
#define     BLOCKS      9
#define     FBLOCKS     10
#define     ERRORS      11
#define     MESSAGE     12

/********************* zmodem defines ***************************************/

#define     ZPAD        '*'	/* 052 Padding character begins frames */
#define     ZDLE        030	/* Ctrl-X Zmodem escape - `ala BISYNC DLE */
#define     ZDLEE       (ZDLE^0100)	/* Escaped ZDLE as transmitted */
#define     ZBIN        'A'	/* Binary frame indicator */
#define     ZHEX        'B'	/* HEX frame indicator */
#define     ZBIN32      'C'	/* Binary frame with 32 bit FCS */

/* Frame types (see array "frametypes" in zm.c) */

#define     ZRQINIT	   0	/* Request receive init */
#define     ZRINIT	   1	/* Receive init */
#define     ZSINIT      2	/* Send init sequence (optional) */
#define     ZACK        3		/* ACK to above */
#define     ZFILE       4		/* File name from sender */
#define     ZSKIP       5		/* To sender: skip this file */
#define     ZNAK        6		/* Last packet was garbled */
#define     ZABORT      7	/* Abort batch transfers */
#define     ZFIN        8		/* Finish session */
#define     ZRPOS       9		/* Resume data trans at this position */
#define     ZDATA       10	/* Data packet(s) follow */
#define     ZEOF        11		/* End of file */
#define     ZFERR       12	/* Fatal Read or Write error Detected */
#define     ZCRC        13		/* Request for file CRC and response */
#define     ZCHALLENGE  14	/* Receiver's Challenge */
#define     ZCOMPL      15	/* Request is complete */
#define     ZCAN        16	  /* Other end canned session with CAN*5 */
#define     ZFREECNT    17	/* Request for free bytes on filesystem */
#define     ZCOMMAND    18	/* Command from sending program */
#define     ZSTDERR     19	/* Output to standard error, data follows */

/* ZDLE sequences */

#define     ZCRCE       'h'	/* CRC next, frame ends, header packet follows */
#define     ZCRCG       'i'	/* CRC next, frame continues nonstop */
#define     ZCRCQ       'j'	/* CRC next, frame continues, ZACK expected */
#define     ZCRCW       'k'	/* CRC next, ZACK expected, end of frame */
#define     ZRUB0       'l'	/* Translate to rubout 0177 */
#define     ZRUB1       'm'	/* Translate to rubout 0377 */

/* zdlread return values (internal) */
/* -1 is general error, -2 is timeout */

#define     GOTOR       0400
#define     GOTCRCE     (ZCRCE|GOTOR)	/* ZDLE-ZCRCE received */
#define     GOTCRCG     (ZCRCG|GOTOR)	/* ZDLE-ZCRCG received */
#define     GOTCRCQ     (ZCRCQ|GOTOR)	/* ZDLE-ZCRCQ received */
#define     GOTCRCW     (ZCRCW|GOTOR)	/* ZDLE-ZCRCW received */
#define     GOTCAN	   (GOTOR|030)	/* CAN*5 seen */

/* Byte positions within header array */
#define     ZF0	      3	/* First flags byte */
#define     ZF1	      2
#define     ZF2	      1
#define     ZF3	      0
#define     ZP0	      0	/* Low order 8 bits of position */
#define     ZP1	      1
#define     ZP2	      2
#define     ZP3	      3	/* High order 8 bits of file position */

/* Bit Masks for ZRINIT flags byte ZF0 */

#define     CANFDX	   01	/* Rx can send and receive true FDX */
#define     CANOVIO	   02	/* Rx can receive data during disk I/O */
#define     CANBRK	   04	/* Rx can send a break signal */
#define     CANCRY	   010	/* Receiver can decrypt */
#define     CANLZW	   020	/* Receiver can uncompress */
#define     CANFC32	   040	/* Receiver can use 32 bit Frame Check */
#define     ESCCTL      0100	/* Receiver expects ctl chars to be escaped */
#define     ESC8        0200	/* Receiver expects 8th bit to be escaped */

/* Parameters for ZSINIT frame */

#define     ZATTNLEN    32	/* Max length of attention string */

/* Bit Masks for ZSINIT flags byte ZF0 */

#define     TESCCTL     0100	/* Transmitter expects ctl chars to be escaped */
#define     TESC8       0200	/* Transmitter expects 8th bit to be escaped */

/* Parameters for ZFILE frame */
/* Conversion options one of these in ZF0 */

#define     ZCBIN	      1	/* Binary transfer - inhibit conversion */
#define     ZCNL	      2	/* Convert NL to local end of line convention */
#define     ZCRESUM	   3	/* Resume interrupted file transfer */

/* Management include options, one of these ored in ZF1 */

#define     ZMSKNOLOC	0200	/* Skip file if not present at rx */

/* Management options, one of these ored in ZF1 */

#define     ZMMASK	   037	/* Mask for the choices below */
#define     ZMNEWL	   1	/* Transfer if source newer or longer */
#define     ZMCRC	      2	/* Transfer if different file CRC or length */
#define     ZMAPND	   3	/* Append contents to existing file (if any) */
#define     ZMCLOB	   4	/* Replace existing file */
#define     ZMNEW	      5	/* Transfer if source newer */

	/* Number 5 is alive ... */

#define     ZMDIFF	   6	/* Transfer if dates or lengths different */
#define     ZMPROT	   7	/* Protect destination file */

/* Transport options, one of these in ZF2 */

#define     ZTLZW	      1	/* Lempel-Ziv compression */
#define     ZTCRYPT	   2	/* Encryption */
#define     ZTRLE	      3	/* Run Length encoding */

/* Extended options for ZF3, bit encoded */

#define     ZXSPARS	   64	/* Encoding for sparse file operations */

/* Parameters for ZCOMMAND frame ZF0 (otherwise 0) */

#define     ZCACK1	   1	/* Acknowledge, then do command */

long rclhdr();

#define     INTRATIME   50    /* intra-packet wait time */

/******************** globals used for zmodem functions *********************/

EXTERN struct stat Fs;
EXTERN long Lrxpos;		/* Receiver's last reported offset */
EXTERN int Errors;
EXTERN int Lastrx;
EXTERN int Firstsec;
EXTERN int Eofseen;		/* indicates cpm eof (^Z) has been received */
EXTERN int Fd;
EXTERN int Wcsmask
#ifdef ZM
=0377
#endif
;
EXTERN int Verbose
#ifdef ZM
=0
#endif
;
EXTERN int Quiet		      /* overrides logic that would otherwise set verbose */
#ifdef ZM
=0
#endif
;
EXTERN int Nflag  		   /* Don't really transfer files */
#ifdef ZM
=0
#endif
;
EXTERN int Rxbinary           /* receive all files in bin mode */
#ifdef ZM
= FALSE
#endif
;
EXTERN int Rxascii            /* receive files in ascii (translate) mode */
#ifdef ZM
= FALSE
#endif
;
EXTERN char *Cpmbuf;             /* buffer bytes for writing to disk */
EXTERN unsigned Cpbufsize;       /* size of Cpmbuf */
EXTERN unsigned Cpindex          /* index for Cpmbuf */
#ifdef ZM
=0
#endif
;
EXTERN char *Secbuf;       /* sector buffer receiving */
EXTERN char Zconv;		   /* ZMODEM file conversion request */
EXTERN char Zmanag;		   /* ZMODEM file management request */
EXTERN char Ztrans;		   /* ZMODEM file transport request */
EXTERN int Zctlesc;		   /* Encode control characters */
EXTERN int Rxtimeout		   /* Tenths of seconds to wait for something */
#ifdef ZM
= 100
#endif
;
EXTERN int Rxframeind;		/* ZBIN ZBIN32, or ZHEX type of frame received */
EXTERN int Rxtype;		/* Type of header received */
EXTERN int Rxcount;		/* Count of data bytes received */
EXTERN char Rxhdr[4];		/* Received header */
EXTERN char Txhdr[4];		/* Transmitted header */
EXTERN long Rxpos;		/* Received file position */
EXTERN long Txpos;		/* Transmitted file position */
EXTERN int Txfcs32;		/* TRUE means send binary frames with 32 bit FCS */
EXTERN int Crc32t;		/* Display flag indicating 32 bit CRC being sent */
EXTERN int Crc32;		/* Display flag indicating 32 bit CRC being received */
EXTERN int Znulls;		/* Number of nulls to send at beginning of ZDATA hdr */
EXTERN char Attn[ZATTNLEN+1];	/* Attention string rx sends to tx on err */
EXTERN int Baudrate;
char *ttime();

#define  FRTYPES     21	   /* Total number of frame types in this array */
		                     /*  not including psuedo negative entries */
#define  FTOFFSET    3
EXTERN char *frametypes[]
#ifdef ZM
 = {
	"Carrier Lost",		/* -3 */
	"TIMEOUT",		/* -2 */
	"ERROR",		/* -1 */
	"ZRQINIT",
	"ZRINIT",
	"ZSINIT",
	"ZACK",
	"ZFILE",
	"ZSKIP",
	"ZNAK",
	"ZABORT",
	"ZFIN",
	"ZRPOS",
	"ZDATA",
	"ZEOF",
	"ZFERR",
	"ZCRC",
	"ZCHALLENGE",
	"ZCOMPL",
	"ZCAN",
	"ZFREECNT",
	"ZCOMMAND",
	"ZSTDERR",
	"xxxxx"
}
#endif
;

/******************* End of ZMODEM.H ****************************************/
