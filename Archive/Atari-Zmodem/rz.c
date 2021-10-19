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
#define RVERSION "rz 3.01 5-25-89"
#define RSTVERSION "$Revision: 1.72 $ $Date: 1991/04/27 22:12:49 $"
#define OS	"Unix V7/BSD"

/* #define RDEBUG */			/* a lot of debugging garb */

/*
 *	ATARI ST series implementation notes:
 *
 *		- the following command line options were removed as they
 *		  were either  not applicable to the ST environment or
 *		  were not deemed reasonable (by me - ofcourse).
 *			1	Not Applicable here as we have a seperate
 *				serial port.
 *			7	In this day and age? Forget it, get another m/c.
 *			a/b	Ascii/Binary - the receive mode (if not
 *				over-ridden by the sender) is automatically
 *				selected depending on the extention given
 *				in the incoming file name. This idea was
 *				present in earlier rz/sz, i wonder why such
 *				a convenient feature was dropped (Chuck ??).
 *				This feature is relevant to ZMODEM only in rz,
 *				as the sender determines the file mode in
 *				XMODEM/YMODEM transfers.
 *		        B	Note that `B' has a special meaning.
 *				Specifying -B will force override to
 *				binary mode for each incoming file. Useful
 * 				when doing St-to-St transfers.
 *			D	There is no /dev/null on the ST's
 *			u	not applicable to TOS. Upper and lower
 *				case file names are the same. All the
 *				applicable routines like uncap() and
 *				IsAnyLower() were zapped.
 *
 *		- The	[-][v]rzCOMMAND style of invocation was dropped
 *		  as there is no good way to do pipes without the 
 *		  microRtx kernal. All references to Pipe and popen()
 *		  were zapped.
 *		- Verbose is always set to 2 by automatically, as we know that
 *		  stdout != stderr. This can be overridden
 *		  by specifying -q to ensure that Verbose = 0
 *		- The idea of a PUBDIR and Restricted paths in the origonal
 *		  code  was dropped totally as it is not applicable
 *		  to the single owner ST environment. 1 man 1 machine.
 *		- CRCTABLE is default always, hey we have plenty of memory!.
 *		- LOGFILE renamed to 'rzlog/szlog' as we don't always have
 *		  a meaningful environment to pick up TMPDIR from (like when
 *		  running from the desktop).
 *	        - When a subdirectory in an incoming path name is not
 *		  present it is created.
 *		- The file mode transmitted is 0S00 where S is derived from
 *		  the Read/Write attribute of the file on the ST
 *		- When a file mode is received, only the owner bits are
 *		  are checked. If it was read only (r--) on the Unix sytem
 *		  then it is given read only attribute on the ST, read-write
 *		  otherwise.
 *		- Of course all the I/O was completely redone on the ST.
 *		- You will find two versions of VARARGS routines like log,
 *		  one that takes int args, and the other that takes long
 *		  (address) args, since sizeof(int) != sizeof(long)
 *		  and sizeof(int) != sizeof(pointer) on the ST.
 *
 *	ST v1.01
 *	 added support for 32 bit CRC's for Zmodem ++jrb
 *
 *	ST v1.2
 *	 added -B ++jrb
 *	 added all the recursive stuff
 *	 added remote
 */
		
/*% cc -DNFGVMIN -DCRCTABLE -K -O % -o rz; size rz
 *
 * rz.c By Chuck Forsberg
 *
 *	cc -O rz.c -o rz		USG (3.0) Unix
 * 	cc -O -DV7  rz.c -o rz		Unix V7, BSD 2.8 - 4.3
 *
 *	ln rz rb			For either system
 *
 *	ln rz /usr/bin/rzrmail		For remote mail.  Make this the
 *					login shell. rzrmail then calls
 *					rmail(1) to deliver mail.
 *
 *		define CRCTABLE to use table driven CRC
 *
 *  Unix is a trademark of Western Electric Company
 *
 * A program for Unix to receive files and commands from computers running
 *  Professional-YAM, PowerCom, YAM, IMP, or programs supporting XMODEM.
 *  rz uses Unix buffered input to reduce wasted CPU time.
 *
 * Iff the program is invoked by rzCOMMAND, output is piped to 
 * "COMMAND filename"
 *
 *  Some systems (Venix, Coherent, Regulus) may not support tty raw mode
 *  read(2) the same way as Unix. ONEREAD must be defined to force one
 *  character reads for these systems. Added 7-01-84 CAF
 *
 *  Alarm signal handling changed to work with 4.2 BSD 7-15-84 CAF 
 *
 *  NFGVMIN Added 1-13-85 CAF for PC-AT Xenix systems where c_cc[VMIN]
 *  doesn't seem to work (even though it compiles without error!).
 *
 *  USG UNIX (3.0) ioctl conventions courtesy  Jeff Martin
 */


#include "zmdm.h"
#include "common.h"
#include "zmodem.h"

static unsigned long SaveIntr;

#ifndef Vsync 			/* Atari forgot these in osbind.h */
#define Vsync()	xbios(37)
#endif

#ifndef Supexec
		/* Some versions of osbind don't define Supexec */
#define Supexec(X) xbios(38,X)
#endif

#if (MWC || MANX || __GNUC__)
extern FILE  *fopen();
#else
extern FILE  *fopen(), *fopenb();
#endif

#ifndef STANDALONE
#define RETURN return
#else
static void RETURN();
void bibis(n) int n; {} /* dummy */
#endif 

static long start_time;

#ifndef STANDALONE
/* called by simulated signal interrupt or terminate to clean things up */
void bibi(n)
int n;
{

	if (Zmodem)
		zmputs(Attn);
	canit(); mode(0);
	fprintf(STDERR, "\r\nrz: caught signal %d; exiting", n);
	if (fout != -1)
	{
		if (stfclose(fout) != 0)
		{
			fprintf(STDERR, "\r\nfile close ERROR\n");
		}
		fout = (-1);

	}

#ifdef RDEBUG
	if (logf != (FILE *)NULL)
		fclose(logf);
#endif
	aexit(128+n);
}
#endif

#ifdef STANDALONE
int main(argc, argv)
#else
int dorz(argc, argv)
#endif /* STANDALONE */
int argc;
char **argv;
{
	register char *cp;
	register int npats;
	char **patts;
	int exitcode;

#ifdef STANDALONE
#if (MWC || __GNUC__)
#ifdef MWC
	extern char *lmalloc();
#else
#define lmalloc malloc
	extern void *lmalloc(unsigned long);
#endif
#endif

	/* Set up Dta */
	Fsetdta(&statbuf);

	/* Get screen rez */
	rez = Getrez();
	drv_map = Drvmap();

#if (MWC || MANX || __GNUC__)
#ifndef DYNABUF
#if (MWC || __GNUC__)
	if((bufr = (unsigned char *)lmalloc((unsigned long)BBUFSIZ))
					 == (unsigned char *)NULL)
#else
	if((bufr = (unsigned char *)Malloc((unsigned long)BBUFSIZ))
					 == (unsigned char *)NULL)
#endif
#else
	if((bufr = dalloc()) == (unsigned char *)NULL)
#endif /* DYNABUF */
	{
#ifdef REMOTE
		Bauxws("Sorry, could not allocate enough memory\r\n");
#else
		Bconws("Sorry, could not allocate enough memory\r\n");
#endif

		Pterm(4);
	}
#else /* MWC || MANX */
#ifdef DYNABUF
	if((bufr = dalloc()) == (unsigned char *)NULL)
	{
#ifdef REMOTE
		Bauxws("Sorry, could not allocate enough memory\r\n");
#else
		Bconws("Sorry, could not allocate enough memory\r\n");
#endif
		Pterm(5);
	}
#endif /* DYNABUF */
#endif /* MWC || MANX */

#ifndef REMOTE
	STDERR = stderr;
#else
#ifndef DLIBS
	if((STDERR = fopen("aux:", "rw")) == (FILE *)NULL)
	{
		Bauxws("Could not Open Aux Stream for Stderr\r\n");
		finish();
	}
	setbuf(STDERR, (char *)NULL);
#else
	STDERR = stdaux;
#endif /* DLIBS */
	
#endif /* REMOTE */

	{
		int speed;
		speed = getbaud();
		Baudrate = BAUD_RATE(speed);
		SetIoBuf();
		Rsconf(speed, 0,-1,-1,-1,-1);
		Vsync(); Vsync();
	}

#endif /* STANDALONE */

	SendType = 0;
	Rxtimeout = 100;
	exitcode = 0;

	initz();

#ifndef STANDALONE
	chkinvok(argv[0]); 	/* if called as  'rb' set flag */
#else
	Progname = "rz";
#endif

	npats = 0;
	SaveIntr = Setexc(0x0102, -1L);
	BusErr   = Setexc(2, -1L);
	AddrErr  = Setexc(3, -1L);
	vdebug = 0;

	while (--argc)
	{
		cp = *++argv;
		if (*cp == '-')
		{
			while( *++cp)
			{
				switch(*cp)
				{
				case '+':
					Lzmanag = ZMAPND; break;
				case 'B':
					ForceBinary=TRUE; break;
				case 'c':
					Crcflg=TRUE; break;
				case 'D':
					Nflag = TRUE; break;
				case 'e':
					Zctlesc = 1; break;
				case 'p':
					Lzmanag = ZMPROT;  break;
				case 'q':
					Quiet=TRUE; Verbose=0; break;
				case 't':
					if (--argc < 1) {
						rusage();
						RETURN(1);
					}
					Rxtimeout = atoi(*++argv);
					if (Rxtimeout<10 || Rxtimeout>1000)
					{
						rusage();
						RETURN(1);
					}
					break;
				case 'w':
					if (--argc < 1) {
						rusage();
						RETURN(1);
					}
					Zrwindow = atoi(*++argv);
					break;
				case 'v':
					++Verbose; break;
				case 'y':
					Rxclob=FALSE; break;
				default:
					rusage();
					RETURN(1);
				}
			}
		}
		else if ( !npats && argc>0)
		{
			if (argv[0][0])
			{
				npats=argc;
				patts=argv;
			}
		}
	}

	if (npats > 1)
	{
		rusage();
		RETURN(1);
	}

#ifdef RDEBUG
	if (Verbose > 2)
	{
		if ((logf = fopen(RLOGFILE, "a"))== (FILE *)NULL)
		{
			fprintf(STDERR, "Can't open log file %s\n",RLOGFILE);
			RETURN(0200);
		}
		fprintf(logf, "Progname=%s\n", Progname);
		vdebug = 1;
	}
#endif

	if ( !Quiet)
	{
		if (Verbose == 0)
			Verbose = 2;
	}

	Setexc(0x0102, bibi);

	Setexc(2, buserr);
	Setexc(3, addrerr);

	if((exitcode = setjmp(abrtjmp)))
	{
		/* on Contrl-C */
		canit();
		Setexc(2, BusErr);
		Setexc(3, AddrErr);
		Setexc(0x0102, SaveIntr);
		RETURN(exitcode);
	}
	
	if(setjmp(busjmp))
	{
		/* On a bus error - instead of 2 bombs */
		fprintf(STDERR,"\r\nFATAL: Bus Error\n\n");
#ifdef RDEBUG
		if(logf != (FILE *)NULL)
			fclose(logf);
#endif
		if(fout != -1)
		{
			if (stfclose(fout) != 0)
			{
				fprintf(STDERR, "\r\nfile close ERROR\n");
			}
			fout = (-1);
		}
		canit();
		Setexc(2, BusErr);
		Setexc(3, AddrErr);
		Setexc(0x0102, SaveIntr);

		RETURN(2);
	}

	if(setjmp(addrjmp))
	{
		/* On address error - instead of 3 bombs */
		fprintf(STDERR,"\r\nFATAL: Address Error\n\n");
#ifdef RDEBUG
		if(logf != (FILE *)NULL)
			fclose(logf);
#endif
		if(fout != -1)
		{
			if (stfclose(fout) != 0)
			{
				fprintf(STDERR, "\r\nfile close ERROR\n");
			}
			fout = (-1);
		}
		canit();
		Setexc(2, BusErr);
		Setexc(3, AddrErr);
		Setexc(0x0102, SaveIntr);

		RETURN(3);
	}

	mode(1);

	if (wcreceive(npats, patts)==ERROR)
	{
		exitcode=0200;
		canit();
	}

	mode(0);
	if (exitcode && !Zmodem)	/* bellow again with all thy might. */
		canit();

#ifdef RDEBUG
	if(logf != (FILE *)NULL)
		fclose(logf);
#endif

	if(fout != -1)
	{
		if (stfclose(fout) != 0)
		{
			fprintf(STDERR, "\r\nfile close ERROR\n");
		}
		fout = (-1);
	}
	Setexc(2, BusErr);
	Setexc(3, AddrErr);
	Setexc(0x0102, SaveIntr);

 	RETURN(exitcode); 
}

#ifdef STANDALONE
static void RETURN(n)
int n;
{
	ResetIoBuf();
#if (MWC || MANX || __GNUC__)
#ifndef DYNABUF
	free(bufr);
#else
	Mfree(bufr);
#endif
#else
#ifdef DYNABUF
	Mfree(bufr);
#endif
#endif

	exit(n);
}
#endif /* STANDALONE */

int rusage()
{
	fprintf(STDERR,
		"%s for %s by ST Enthusiasts at Case Western Reserve University\n",
	  	RSTVERSION, STOS);
	fprintf(STDERR, "\tBased on %s for %s by Chuck Forsberg\n\n",
		RVERSION, OS);

	fprintf(STDERR,"Usage:	rz [-Bepqtvwy]		(ZMODEM Batch)\n");
	fprintf(STDERR,"or	rb [-qtv]		(YMODEM Batch)\n");
	fprintf(STDERR,"or	rz [-cqtv] file	        (XMODEM or XMODEM-1k)\n");
	fprintf(STDERR,"	  -B Force Binary Mode transfers\n");
	fprintf(STDERR,"	  -v Verbose more v's give more info\n");
	fprintf(STDERR,"          -q Quiet suppresses verbosity\n");
	fprintf(STDERR,"	  -t TIM Change timeout to TIM tenths of seconds\n");
	fprintf(STDERR,"	  -c Use 16 bit CRC	(XMODEM)\n");
	fprintf(STDERR,"	  -p Protect existing dest. file by skipping\n");
	fprintf(STDERR,"	     transfer if the dest. file exists (ZMODEM ONLY)\n\n");
	fprintf(STDERR,"	  -e Escape control characters (Z)\n");
	fprintf(STDERR,"	  -w N Window is N bytes (Z)\n");


	if(fout != -1)
	{
		if (stfclose(fout) != 0)
		{
			fprintf(STDERR, "\r\nfile close ERROR\n");
		}
		fout = (-1);
	}

#ifdef RDEBUG
	if(logf != (FILE *)NULL)
		fclose(logf);
#endif

	return(1);
}


/*
 * Let's receive something already.
 */
int wcreceive(argc, argp)
int argc;
char **argp;
{
	register int c;

	if (Batch || argc==0)
	{
	        Crcflg= 1;
		if ( !Quiet)
#ifndef REMOTE
			fprintf(STDERR, "\n%s: ready (CTRL-C to cancel)\n\n",
				Progname);
#else
			fprintf(STDERR, "\n%s: ready\n\n",
				Progname);
#endif
		if (c=tryz())
		{
			if (c == ZCOMPL)
				return OK;
			if (c == ERROR)
				goto fubar;
			c = rzfiles();
			if (c)
				goto fubar;
		}
		else
		{
			for (;;)
			{
				if (wcrxpn(secbuf)== ERROR)
					goto fubar;
				if (secbuf[0]==0)
					return OK;
				if (procheader(secbuf) == ERROR)
					goto fubar;
				if (wcrx()==ERROR)
					goto fubar;
			}
		}
	} 
	else
	{
		Bytesleft = DEFBYTL; Filemode = 0; Modtime = 0L;

		procheader(""); strcpy(Pathname, *argp);
#ifndef REMOTE
		fprintf(STDERR, "\n%s: ready to receive %s (CTRL-C to Cancel)\n\n",
			Progname, Pathname);
#else
		fprintf(STDERR, "\n%s: ready to receive %s\n\n",
			Progname, Pathname);
#endif

#ifdef RDEBUG
		if(logf != (FILE *)NULL)
			fprintf(logf, "\nrz: ready to receive %s ", Pathname);
#endif

		if((fout = stfopen(Pathname,"w")) <= 0)
			return ERROR;
		if (wcrx()==ERROR)
			goto fubar;
	}
	return OK;

fubar:
	canit();
	Modtime = 1;
	if (fout != -1)
	{
		if (stfclose(fout) != 0)
		{
			fprintf(STDERR, "\r\nfile close ERROR\n");
		}
		fout = (-1);
	}

	return ERROR;
}


/*
 * Fetch a pathname from the other end as a C ctyle ASCIZ string.
 * Length is indeterminate as long as less than Blklen
 * A null string represents no more files (YMODEM)
 */
int wcrxpn(rpn)
char *rpn;	/* receive a pathname */
{
	register int c;

	PURGELINE;

et_tu:
	Firstsec=TRUE;  Eofseen=FALSE;
	sendline(Crcflg?WANTCRC:NAK);
	Lleft=0;	/* Do read next time ... */
	while ((c = wcgetsec(rpn, 100)) != 0)
	{
		llog( "Pathname fetch returned %d\n", c);
		if (c == WCEOT)
		{
			sendline(ACK);
			Lleft=0;	/* Do read next time ... */
			readline(1);
			goto et_tu;
		}
		return ERROR;
	}
	sendline(ACK);
	return OK;
}

/*
 * Adapted from CMODEM13.C, written by
 * Jack M. Wierda and Roderick W. Hart
 */

int wcrx()
{
	register int sectnum, sectcurr;
	register char sendchar;
	int cblklen;			/* bytes to dump this block */

	Firstsec=TRUE;sectnum=0; Eofseen=FALSE;
	sendchar=Crcflg?WANTCRC:NAK;

	for (;;)
	{
		sendline(sendchar);	/* send it now, we're ready! */
		Lleft=0;	/* Do read next time ... */
		sectcurr=wcgetsec(secbuf, (sectnum&0177)?50:130);
		report(sectcurr);
		if (sectcurr==(sectnum+1 &Wcsmask))
		{
			sectnum++;
			cblklen = Bytesleft>Blklen ? Blklen:Bytesleft;
			if (putsec(secbuf, cblklen)==ERROR)
				return ERROR;
			if ((Bytesleft-=cblklen) < 0)
				Bytesleft = 0;
			sendchar=ACK;
		}
		else if (sectcurr==(sectnum&Wcsmask))
		{
			log2( "Received dup Sector\n");
			sendchar=ACK;
		}
		else if (sectcurr==WCEOT)
		{
			if (closeit(0L))
				return ERROR;
			sendline(ACK);
			Lleft=0;	/* Do read next time ... */
			return OK;
		}
		else if (sectcurr==ERROR)
			return ERROR;
		else
		{
			log2( "Sync Error\n");
			return ERROR;
		}
	}
}


/*
 * Wcgetsec fetches a Ward Christensen type sector.
 * Returns sector number encountered or ERROR if valid sector not received,
 * or CAN CAN received
 * or WCEOT if eot sector
 * time is timeout for first char, set to 4 seconds thereafter
 ***************** NO ACK IS SENT IF SECTOR IS RECEIVED OK **************
 *    (Caller must do that when he is good and ready to get next sector)
 */
int wcgetsec(rxbuf, maxtime)
char *rxbuf;
int maxtime;
{
	register int checksum, wcj, firstch;
	register unsigned int oldcrc;
	register char *p;
	int sectcurr;

	for (Lastrx=errors=0; errors<RETRYMAX; errors++)
	{
		if ((firstch=readline(maxtime))==STX)
		{
			Blklen=KSIZE; goto get2;
		}
		if (firstch==SOH)
		{
			Blklen=SECSIZ;
get2:
			sectcurr=readline(1);
			if ((sectcurr+(oldcrc=readline(1)))==Wcsmask)
			{
				oldcrc=checksum=0;
				for (p=rxbuf,wcj=Blklen; --wcj>=0; )
				{
					if ((firstch=readline(1)) < 0)
						goto bilge;
					oldcrc=updcrc(firstch, oldcrc);
					checksum += (*p++ = firstch);
				}
				if ((firstch=readline(1)) < 0)
					goto bilge;
				if (Crcflg)
				{
					oldcrc=updcrc(firstch, oldcrc);
					if ((firstch=readline(1)) < 0)
						goto bilge;
					oldcrc=updcrc(firstch, oldcrc);
					if (oldcrc & 0xFFFF)
						llog("CRC=0%o\n", oldcrc);
					else
					{
						Firstsec=FALSE;
						return sectcurr;
					}
				}
				else if (((checksum-firstch)&Wcsmask)==0)
				{
					Firstsec=FALSE;
					return sectcurr;
				}
				else
					log2( "Checksum Error\n");
			}
			else
				log2("Sector number garbled 0%o 0%o\n",
				 sectcurr, oldcrc);
		}
		/* make sure eot really is eot and not just mixmash */

		else if (firstch==EOT && Lleft==0)
			return WCEOT;

		else if (firstch==CAN)
		{
			if (Lastrx==CAN)
			{
				log2( "Sender CANcelled\n");
				return ERROR;
			}
			else
			{
				Lastrx=CAN;
				continue;
			}
		}
		else if (firstch==TIMEOUT)
		{
			if (Firstsec)
				goto humbug;
bilge:
			log2( "Timeout\n");
		}
		else
			llog( "Got 0%o sector header\n", firstch);

humbug:
		Lastrx=0;
		while(readline(1)!=TIMEOUT)
			;
		if (Firstsec)
		{
			sendline(Crcflg?WANTCRC:NAK);
			Lleft=0;	/* Do read next time ... */
		}
		else
		{
			maxtime=40; sendline(NAK);
			Lleft=0;	/* Do read next time ... */
		}
	}
	/* try to stop the bubble machine. */
	canit();
	return ERROR;
}



unsigned int timep[2];

/*
 * Process incoming file information header
 */
int procheader(name)
char *name;
{
	register char  *p;
	register int dot;
	char openmode[4];
#ifdef __GNUC__
	extern size_t strlen();
#else
	extern int strlen();
#endif

	/* convert to ST style path names */
	for( p = name; *p != '\0'; p++)
	{
		if(*p == '/')
			*p = '\\';
	}
	
	/* pick out the last extention in the filename in each part of path */
	while(p != name)
	{
		dot = 0; p-- ;
		while((p != name) && (*p != '\\'))
		{
			if(*p == '.')
			{
				if(dot == 0)
				{
					dot = 1;
				}
				else
				{
				   /* replace all but the last dot with '_' */
					*p = '_';
				}
			}
			p--;
		}
	}
				
	/* set default parameters and overrides */
	strcpy(openmode,"w");

	Thisbinary = isbinary(name);

	if (Lzmanag)
		zmanag = Lzmanag;

	/*
	 *  Process ZMODEM remote file management requests
	 */
	if ( zconv == ZCNL)	/* Remote ASCII override */
		Thisbinary = 0;
	if (zconv == ZCBIN)	/* Remote Binary override */
		++Thisbinary;
	else if (zmanag == ZMAPND)
		strcpy(openmode, "a");

	if (ForceBinary == TRUE )	/* local binary force override */
		++Thisbinary;

	/* ZMPROT check for existing file */
	if (!Rxclob && (zmanag&ZMMASK) != ZMCLOB && existf(name))
	{
		return ERROR;
	}

/* ATARI ST NOTE:
 *	We will not accept rooted paths ie. paths that begin in '\' or '.\'
 *	If the incoming filename is rooted, we skip the beginning
 *	'\'   '.\'  or  '..\'
 */

	if( (name[0] == '\\') || (name[0] == '.')  )
	{
		/* skip over the leading stuff */
		if(name[0] == '\\')
			name = &name[1];
		else
		{
			if(name[1] == '.')
				name = &name[3];  /* Skip the "..\" */
			else
				name = &name[2];  /* Skip the ".\"  */
		}
	}

	/* ST addition, create any dierctories in the path that don't exist */
	if( pathensure(name) == ERROR)
		return ERROR;

	Bytesleft = DEFBYTL; Filemode = 0; Modtime = 0L;

	p = name + 1 + (int)strlen(name);
	if (*p)
	{	/* file coming from Unix or DOS system */
		sscanf(p, "%ld%lo%o", &Bytesleft, &Modtime, &Filemode);
		if(Modtime)
			unix2st(Modtime, &timep[0], &timep[1]);
		else
			timep[0] = timep[1] = 0;
#ifndef REMOTE
		if (Verbose)
		{
			fprintf(STDERR,
			"\nIncoming:\n\tName:\t%s\n\tBytes:\t%ld\n\
\tModTime: %02d/%02d/%04d  %02d:%02d:%02d\n\tMode:\t0%03o\n\tBufSize: %ld\n\n",
			  name, Bytesleft, 
			  (timep[1] >> 5) & 0x0f,
			  timep[1] & 0x1f,
			  ((timep[1] >> 9)  & 0x7f) + 1980,
			  (timep[0] >> 11) & 0x1f,
			  (timep[0] >> 5)  & 0x3f,
			  timep[0] & 0x1f, (Filemode & (unsigned int)0777), (long)BBUFSIZ);

#ifdef RDEBUG
			if(logf != (FILE *)NULL)
				fprintf(logf,  "Incoming: %s %ld %lo %o\n",
				  name, Bytesleft, Modtime, Filemode);
#endif

		}
#endif /* REMOTE */

	}
	else
	{		/* File coming from CP/M system */
		for (p=name; *p; ++p)		/* change / to _ */
			if ( *p == '/')
				*p = '_';

		if ( *--p == '.')		/* zap trailing period */
			*p = 0;
	}
	
	strcpy(Pathname, name);
#ifndef REMOTE
	if (Verbose)
	{
		fprintf(STDERR,  "Receiving %s %s [mode %s]\n\n",
		  name, Thisbinary?"BIN":"ASCII", openmode);

#ifdef RDEBUG
		if(logf != (FILE *)NULL)
			fprintf(logf,  "Receiving %s %s %s\n",
			  name, Thisbinary?"BIN":"ASCII", openmode);
#endif

	}
#endif /* REMOTE */

	if ((fout=stfopen(name, openmode)) <= 0)
		return ERROR;

	return OK;
}

/*
 * Putsec writes the n characters of buf to receive file fout.
 *  If not in binary mode,  all characters
 *  starting with CPMEOF are discarded.
 */
int putsec(buf, n)
unsigned char *buf;
register int n;
{
	register unsigned char *p;

	if(n == 0)
	    return OK;
	
	if (Thisbinary)
	{
		for (p=buf; --n>=0; p++ )
		{
			if(stputc( *p, fout) < 0)
			{
				fprintf(STDERR, "\r\nError while Writing file\n");
				return ERROR;
			}
		}
	}
	else
	{
		if (Eofseen)
			return OK;

		for (p=buf; --n>=0; p++ )
		{
			if (*p == CPMEOF)
			{
				Eofseen=TRUE;
				return OK;
			}
			if(*p == '\n')
			{
				if(stputc('\r' ,fout) < 0)
				{
					fprintf(STDERR, "\r\nError while Writing file\n");
					return ERROR;
				}
			}
			if(stputc(*p ,fout) < 0)
			{
				fprintf(STDERR,"\r\nError while Writing file\n");
				return ERROR;
			}
		}
	}

	return OK;
}

/*
 * Log an error only if high verbose
 */
/*VARARGS1*/
void llog(s,p,u)
char *s;
int p, u;
{
	if (Verbose < 3)
		return;
#ifdef RDEBUG
	fprintf(logf, "error %d: ", errors);
	fprintf(logf, s, p, u);
#endif

	fprintf(STDERR, "\nerror %d: ", errors);
	fprintf(STDERR, s, p, u);
}



#ifndef STANDALONE
/*
 * If called as rb use YMODEM protocol
 */
void chkinvok(s)
char *s;
{
	Progname = s;
	if (s[0]=='r' && s[1]=='b')
		Nozmodem = TRUE;
}
#endif

/*
 * Initialize for Zmodem receive attempt, try to activate Zmodem sender
 *  Handles ZSINIT frame
 *  Return ZFILE if Zmodem filename received, -1 on error,
 *   ZCOMPL if transaction finished,  else 0
 */
int tryz()
{
	register int n, c;
	register int cmdzack1flg;

	if (Nozmodem)		/* Check for "rb" program name */
		return 0;


	for (n=Zmodem?15:5; --n>=0; )
	{
		/* Set buffer length (0) and capability flags */
		stohdr(0L);
#if 0
		Txhdr[ZF0] = CANFC32|CANFDX|CANOVIO|CANBRK;
#else
		Txhdr[ZF0] = CANFC32|CANFDX|CANOVIO;
#endif
		if (Zctlesc)
			Txhdr[ZF0] |= TESCCTL;
		Txhdr[ZF0] |= CANRLE;
		Txhdr[ZF1] = CANVHDR;
		/* tryzhdrtype may == ZRINIT */
		zshhdr(4,tryzhdrtype, Txhdr);
		if (tryzhdrtype == ZSKIP)	/* Don't skip too far */
			tryzhdrtype = ZRINIT;	/* CAF 8-21-87 */
again:
		switch (zgethdr(Rxhdr, 0))
		{
		case ZRQINIT:
			if (Rxhdr[ZF3] & 0x80)
				Usevhdrs = 1;	/* we can var header */
			continue;
		case ZEOF:
			continue;
		case TIMEOUT:
			continue;
		case ZFILE:
			zconv = Rxhdr[ZF0];
			zmanag = Rxhdr[ZF1];
			ztrans = Rxhdr[ZF2];
			if (Rxhdr[ZF3] & ZCANVHDR)
				Usevhdrs = TRUE;
			tryzhdrtype = ZRINIT;
			c = zrdata(secbuf, KSIZE);
/*			mode(3); */
			if (c == GOTCRCW)
				return ZFILE;
			zshhdr(4,ZNAK, Txhdr);
			goto again;
		case ZSINIT:
			Zctlesc = TESCCTL & Rxhdr[ZF0];
			if (zrdata(Attn, ZATTNLEN) == GOTCRCW)
			{
				stohdr(1L);
				zshhdr(4, ZACK, Txhdr);
				goto again;
			}
			zshhdr(4, ZNAK, Txhdr);
			goto again;
		case ZFREECNT:
			stohdr(~0L);
			zshhdr(4, ZACK, Txhdr);
			goto again;
		case ZCOMMAND:
			cmdzack1flg = Rxhdr[ZF0];
			if (zrdata(secbuf, KSIZE) == GOTCRCW)
			{
				if (cmdzack1flg & ZCACK1)
					stohdr(0L);
				else
					stohdr((long)sys2(secbuf));
				PURGELINE;	/* dump impatient questions */
				do {
					zshhdr(4, ZCOMPL, Txhdr);
				}
				while (++errors<20 && zgethdr(Rxhdr,1) != ZFIN);
				ackbibi();
				if (cmdzack1flg & ZCACK1)
					exec2(secbuf);
				return ZCOMPL;
			}
			zshhdr(4, ZNAK, Txhdr); goto again;
		case ZCOMPL:
			goto again;
		default:
			continue;
		case ZFIN:
			ackbibi(); return ZCOMPL;
		case ZCAN:
			return ERROR;
		}
	}
	return 0;
}

/*
 * Receive 1 or more files with ZMODEM protocol
 */
int rzfiles()
{
	register int c;

	for (;;) {
		switch (c = rzfile()) {
		case ZEOF:
		case ZSKIP:
			switch (tryz()) {
			case ZCOMPL:
				return OK;
			default:
				return ERROR;
			case ZFILE:
				break;
			}
			continue;
		default:
			return c;
		case ERROR:
			return ERROR;
		}
	}
}

/*
 * Receive a file with ZMODEM protocol
 *  Assumes file name frame is in secbuf
 */
int rzfile()
{
	register int c, n;
	long rxbytes;
	extern void rd_time();

	Eofseen=FALSE;
	if (procheader(secbuf) == ERROR) {
		return (tryzhdrtype = ZSKIP);
	}

	n = 20; rxbytes = 0L;

	Supexec(rd_time);
	start_time = pr_time;

	for (;;)
	{
		stohdr(rxbytes);
		zshhdr(4, ZRPOS, Txhdr);
nxthdr:
		switch (c = zgethdr(Rxhdr, 0)) {
		default:
			vfile("rzfile: zgethdr returned %d", c);
			return ERROR;
		case ZNAK:
		case TIMEOUT:
			if ( --n < 0)
			{
				vfile("rzfile: zgethdr returned %d", c);
				return ERROR;
			}
		case ZFILE:
			zrdata(secbuf, KSIZE);
			continue;
		case ZEOF:
			/* ++jrb */
			if (rclhdr(Rxhdr) != rxbytes)
			{
				/*
			         * Ignore eof if it's at wrong place - force
			         *  a timeout because the eof might have gone
			         *  out before we sent our zrpos.
			         */

			        errors = 0;  goto nxthdr;
			}
			if (closeit(rxbytes))
			{
				tryzhdrtype = ZFERR;
				vfile("rzfile: closeit returned <> 0");
				return ERROR;
			}
			vfile("rzfile: normal EOF");
			return c;
		case ERROR:	/* Too much garbage in header search error */
			if ( --n < 0)
			{
				vfile("rzfile: zgethdr returned %d", c);
				return ERROR;
			}
			zmputs(Attn);
			continue;
		case ZSKIP:
			Modtime = 1;
			closeit(rxbytes);
			vfile("rzfile: Sender SKIPPED file");
			return c;
		case ZDATA:
			if (rclhdr(Rxhdr) != rxbytes)
			{
			    if(--n < 0)
				return ERROR;
			    
				zmputs(Attn);
				continue;
			}
moredata:
			switch (c = zrdata(secbuf, KSIZE))
			{
			case ZCAN:
				vfile("rzfile: zgethdr returned %d", c);
				return ERROR;
			case ERROR:	/* CRC error */
				if ( --n < 0)
				{
					vfile("rzfile: zgethdr returned %d", c);
					return ERROR;
				}
				zmputs(Attn);
				continue;
			case TIMEOUT:
				if ( --n < 0)
				{
					vfile("rzfile: zgethdr returned %d", c);
					return ERROR;
				}
				continue;
			case GOTCRCW:
				n = 20;
				
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				lreport(rxbytes);
				stohdr(rxbytes);
				zshhdr(4, ZACK, Txhdr);
				sendline(XON);
				goto nxthdr;
			case GOTCRCQ:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				lreport(rxbytes);
				stohdr(rxbytes);
				zshhdr(4, ZACK, Txhdr);
				goto moredata;
			case GOTCRCG:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				lreport(rxbytes);
				goto moredata;
			case GOTCRCE:
				n = 20;
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				lreport(rxbytes);
				goto nxthdr;
			}
		}
	}
}

/*
 * Send a string to the modem, processing for \336 (sleep 1 sec)
 *   and \335 (break signal)
 */
void zmputs(s)
char *s;
{
	register int c;

	while (*s) {
		switch (c = *s++) {
		case '\336':
			stsleep(1); continue;
		case '\335':
			sendbrk(); continue;
		default:
			sendline(c);
		}
	}
}



/*
 * Close the receive dataset, return OK or ERROR
 */
int closeit(rxbytes)
long rxbytes;
{
	long end_time;
	extern void rd_time();

	if (stfclose(fout) != 0) {
		fprintf(STDERR, "\r\nfile close ERROR\n");
		return ERROR;
	}
	fout = (-1);

	Supexec(rd_time);
	end_time = pr_time;

	if (Modtime) {
		touch(Pathname, timep);
	}

	/* if it is read only by owner on remote, then it is set
	 * to read only on the ST, all other file modes are
	 * irrelevant.
	 */
	if (Filemode)
	{
		unsigned int fmode;

		fmode = (unsigned int)(Filemode & 000777);
		if( ((fmode & 0200) == 0) && ((fmode & 0400) != 0) )
		{
			/* it is readonly by owner on the remote, so
			 * make it read only on the ST too
			 */
			Fattrib(Pathname, 1, 0x01);
		}
	}
#ifndef REMOTE
	if(rxbytes != 0L)
	    fprintf(STDERR,"\033K\n\n%s Closed\nTransfer Time %ld secs.\tfor %ld bytes\
\tApprox %ld cps\n\n", Pathname, (end_time - start_time)/200L, rxbytes,
rxbytes/((end_time - start_time)/200L));
	else
		fprintf(STDERR,"\033K\n\n%s Closed\n\n", Pathname);
#endif
	lsct = 1;
	return OK;
}

/*
 * Ack a ZFIN packet, let byegones be byegones
 */
void ackbibi()
{
    register int n;

    vfile("ackbibi:");
    Readnum = 1;
    stohdr(0L);
    for (n=3; --n>=0; )
    {
	    PURGELINE;
	    zshhdr(4, ZFIN, Txhdr);
	    switch (readline(100))
	    {
	      case 'O':
		readline(1);	/* Discard 2nd 'O' */
		return;
	      case RCDO:
		return;
	      case TIMEOUT:
	      default:
		break;
	    }
	}
}


/*
 * Strip leading ! if present, do shell escape. 
 */
int sys2(s)
register char *s;
{
	if (*s == '!')
		++s;
	return stsystem(s);
}
/*
 * Strip leading ! if present, do exec.
 */
void exec2(s)
char *s;
{
/** Are you kidding
	if (*s == '!')
		++s;
	mode(0);
	execl("/bin/sh", "sh", "-c", s); 
**/
}

/*
 * Touch a file
 */
#ifndef MANX
#ifndef __GNUC__
#undef Fdatime		/* There exist brain damaged versions of osbind.h */
#define	Fdatime(a,b,c)	gemdos(0x57,a,b,c)
#endif /* __GNUC__ */
#endif /* MANX has its _Gemdos stuff */

void touch(name, timep)
char *name;
unsigned int *timep;
{
	register int handl;

	if((handl = Fopen(name, 0)) < 0)
	{
#ifndef REMOTE
		fprintf(STDERR,"*WARNING* Could not set file modification time for %s\n",
			name);
#endif
		return;
	}


	Fdatime(timep, handl, 1);
	Fclose(handl);
}

/* -eof- */
