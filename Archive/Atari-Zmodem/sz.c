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
#define SVERSION "sz 3.03 5-09-89"
#define SSTVERSION "$Revision: 1.77 $ $Date: 1991/04/27 22:12:49 $"
#define OS	"Unix V7/BSD"

#ifndef STANDALONE
#define RETURN return
#else
static void RETURN();
void bibi(n) int n; {} /* dummy */
#endif 

/* #define SDEBUG */

#include "zmdm.h"
#include "common.h"
#include "zmodem.h"

#ifndef Vsync 			/* Atari forgot these in osbind.h */
#define Vsync()	xbios(37)
#endif

#ifndef Supexec
		/* Some versions of osbind don't define Supexec */
#define Supexec(X) xbios(38,X)
#endif

#define SLOGFILE "szlog"

#define purgeline()	while(Bconstat(1)) Bconin(1)
#define S_IFDIR 0x0010

/*
 * Attention string to be executed by receiver to interrupt streaming data
 *  when an error is detected.  A pause (0336) may be needed before the
 *  ^C (03) or after it.
 */
#ifdef READCHECK
char Myattn[] = { 0 };
#else
#ifdef USG
char Myattn[] = { 03, 0336, 0 };
#else
char Myattn[] = { 0 };
#endif
#endif

#define TXBMASK (TXBSIZE-1)
static char *txbuf = secbuf;		/* Pointer to current file segment */

#if (MWC || MANX || __GNUC__)
FILE *fopen();
#else
FILE *fopen(), *fopenb();
#endif
static long bytcnt, Lastsync;
static unsigned long SaveIntr;
static int Resuming, ForceBin;
static int in;
static int Filesleft;
static long Totalleft;
static long vpos;
static void countem();

#ifndef __GNUC__
extern long stread();
#else
long stread(int, unsigned char *, long);
#endif

/* called by signal interrupt or terminate to clean things up */
#ifndef STANDALONE
void bibis(n)
int n;
{
	canit(); flush_modem(); mode(0);
	fprintf(STDERR, "\r\nsz: caught signal %d; exiting\n", n);

	aexit(128+n);
}
#endif

/* Called when Zmodem gets an interrupt (^X) */
#ifdef ONINTR
void onintr()
{
	siggi = 0;
	longjmp(intrjmp, -1);
}
#endif

#define ZKER

#ifdef STANDALONE
int main(argc, argv)
#else
int dosz(argc, argv)
#endif
int argc;
char **argv;
{
	register char *cp;
	register int npats;
	int agcnt; char **agcv;
	char **patts;

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

	SendType = 1;
	Rxtimeout = 600;
	npats=0;
	Filesleft = 0;
	Totalleft = 0L;
	txbuf = secbuf;
	vpos = 0;
	bytcnt = 0;
	
	if (argc<2)
	{
		susage();
		RETURN(1);
	}

	initz();
#ifndef STANDALONE
	schkinvok(argv[0]);
#else
	Progname = "sz";
#endif

	SaveIntr = Setexc(0x0102, -1L);
	BusErr   = Setexc(2, -1L);
	AddrErr  = Setexc(3, -1L);

	Verbose = 0;
	Resuming = FALSE;
	ForceBin = FALSE;
	in = (-1);
	vdebug = 0;

#ifdef SDEBUG
	logf = (FILE *)NULL;
#endif
	while (--argc) {
		cp = *++argv;
		if (*cp++ == '-' && *cp) {
			while ( *cp) {
				switch(*cp++) {
				case '+':
					Lzmanag = ZMAPND; break;
#ifdef CSTOPB
				case '2':
					Twostop = TRUE; break;
#endif
				case '7':
					Wcsmask=0177; break;

/*
	On the St we look up the ext and decide. For Xmodem
	transfers, the file is always sent in binary mode
	and it is the responsibility of the receiver to
	strip CR if so desired.
				case 'a':
					Lzconv = ZCNL;
					Ascii = TRUE; break;
				case 'b':
					Lzconv = ZCBIN; break;
*/

/*  ST extention, force binary, useful to back up every thing
 *  in image mode, see -B option of rz too +jrb
 */
				case 'B':
					ForceBin = TRUE;
					Lzconv = ZCBIN;
					break;
				case 'C':
					if (--argc < 1) {
						susage();
						RETURN(1);
					}
					Cmdtries = atoi(*++argv);
					break;
				case 'i':
					Cmdack1 = ZCACK1;
					/* **** FALL THROUGH TO **** */
				case 'c':
					if (--argc != 1) {
						susage();
						RETURN(1);
					}
					Command = TRUE;
					Cmdstr = *++argv;
					break;
				case 'd':
					++Dottoslash;
					/* **** FALL THROUGH TO **** */
				case 'f':
					Fullname=TRUE; break;
				case 'e':
					Zctlesc = 1; break;
				case 'k':
					Blklen=KSIZE; break;
				case 'L':
					if (--argc < 1) {
						susage();
						RETURN(1);
					}
					blkopt = atoi(*++argv);
					if (blkopt<24 || blkopt>1024)
					{
						susage();
						RETURN(1);
					}
					break;
				case 'l':
					if (--argc < 1) {
						susage();
						RETURN(1);
					}
					Tframlen = atoi(*++argv);
					if (Tframlen<32 || Tframlen>1024)
					{
						susage();
						RETURN(1);
					}
					break;
				case 'N':
					Lzmanag = ZMNEWL;  break;
				case 'n':
					Lzmanag = ZMNEW;  break;
				case 'o':
					Wantfcs32 = FALSE; break;
				case 'p':
					Lzmanag = ZMPROT;  break;
				case 'r':
					if (Lzconv == ZCRESUM)
						Lzmanag = (Lzmanag & ZMMASK) |
						    ZMCRC; 
					Lzconv = ZCRESUM; Resuming = TRUE;
					break; 
				case 'q':
					Quiet=TRUE; Verbose=0; break;
				case 't':
					if (--argc < 1) {
						susage();
						RETURN(1);
					}
					Rxtimeout = atoi(*++argv);
					if (Rxtimeout<10 || Rxtimeout>1000)
					{
						susage();
						RETURN(1);
					}
					break;
				case 'u':
					++Unlinkafter; break;
				case 'v':
					++Verbose; break;
				case 'w':
					if (--argc < 1) {
						susage();
						RETURN(1);
					}
					Txwindow = atoi(*++argv);
					if (Txwindow < 256)
						Txwindow = 256;
					Txwindow = (Txwindow/64) * 64;
					Txwspac = Txwindow/4;
					if (blkopt > Txwspac
					 || (!blkopt && Txwspac < 1024))
						blkopt = Txwspac;
					break;
				case 'X':
					++Modem; break;
				case 'Y':
					Lskipnocor = TRUE;
					/* **** FALLL THROUGH TO **** */
				case 'y':
					Lzmanag = ZMCLOB; break;
				case 'Z':
				case 'z':
					Lztrans = ZTRLE;  break;
				default:
					susage();
					RETURN(1);
				}
			}
		}
		else if ( !npats && argc>0) {
			if (argv[0][0]) {
				npats=argc;
				patts=argv;
			}
		}
	}
	if (npats < 1 && !Command) 
	{
		susage();
		RETURN(1);
	}

#ifdef SDEBUG
	if (Verbose > 2)
	{
		if ((logf = fopen(SLOGFILE, "a"))== (FILE *)NULL)
		{
			fprintf(STDERR, "Can't open log file %s\n",SLOGFILE);
			RETURN(0200);
		}
		fprintf(logf, "Progname=%s\n", Progname);
		vdebug = 1;
		fflush(logf);
	}
#endif

	if ( !Quiet)
	{
		if (Verbose < 2)
			Verbose = 2;
	}


	Setexc(0x0102, bibis);
	Setexc(2, buserr);
	Setexc(3, addrerr);

	if(setjmp(busjmp))
	{
		/* On a bus error - instead of 2 bombs */
		fprintf(STDERR,"\r\nFATAL: Bus Error\n\n");
#ifdef SDEBUG
		if(logf != (FILE *)NULL)
			fclose(logf);
#endif
		if(in != -1)
		{
			stfclose(in);
			in = (-1);
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
#ifdef SDEBUG
		if(logf != (FILE *)NULL)
			fclose(logf);
#endif
		if(in != -1)
		{
			stfclose(in);
			in = (-1);
		}
		canit();
		Setexc(2, BusErr);
		Setexc(3, AddrErr);
		Setexc(0x0102, SaveIntr);
		RETURN(3);
	}

	if((Exitcode = setjmp(abrtjmp)))
	{
		fprintf(STDERR,"\nTransfer ABORT\n\n");
#ifdef SDEBUG
	if(logf != (FILE *)NULL)
		fclose(logf);
#endif
		if(in != -1)
		{
			stfclose(in);
			in = (-1);
		}
		Setexc(2, BusErr);
		Setexc(3, AddrErr);
		Setexc(0x0102, SaveIntr);
		RETURN(Exitcode);
	}

	mode(1);


	if ( !Modem) {
		if (!Command && !Quiet && Verbose != 1)
		{
			fprintf(STDERR, "sz: %d file%s requested:\n",
				npats, npats>1?"s":"");
			for ( agcnt=npats, agcv=patts; --agcnt>=0; )
			{
				fprintf(STDERR, "%s ", *agcv++);
			}
			fprintf(STDERR, "\n\n");

#ifdef SDEBUG
			if(Verbose > 2)
			{
				fprintf(logf, "sz: %d file%s requested:\n",
					npats, npats>1?"s":"");
				for ( agcnt=npats, agcv=patts; --agcnt>=0; )
				{
					fprintf(logf, "%s ", *agcv++);
				}
				fprintf(logf, "\n");
				fflush(logf);
			}
#endif
		}

		countem(npats, patts);
		if (!Nozmodem) {
			stohdr(0L);
			if (Command)
				Txhdr[ZF0] = ZCOMMAND;
			zshhdr(4, ZRQINIT, Txhdr);
		}
	}
	flush_modem();

	if (Command) {
		if (getzrxinit()) {
			Exitcode=0200; canit();
		}
		else if (zsendcmd(Cmdstr, (int)(1+strlen(Cmdstr)))) {
			Exitcode=0200; canit();
		}
	} else if (wcsend(npats, patts)==ERROR) {
		Exitcode=0200;
		canit();
	}
	flush_modem();
	mode(0);

#ifdef SDEBUG
	if(logf != (FILE *)NULL)
		fclose(logf);
#endif
	if(in != -1)
	{
		stfclose(in);
		in = (-1);
	}

	putc('\n', STDERR);
	Setexc(2, BusErr);
	Setexc(3, AddrErr);
	Setexc(0x0102, SaveIntr);
	RETURN((errcnt != 0) | Exitcode);

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

int wcsend(argc, argp)
int argc;
char *argp[];
{
	register int n;

	Crcflg=FALSE;
	Firstsec=TRUE;
	bytcnt = -1;
	
	for (n=0; n<argc; ++n) {
		Totsecs = 0;
		if (wcs(argp[n])==ERROR)
			return ERROR;
	}
	Totsecs = 0;
	if (Filcnt==0) {	/* bitch if we couldn't open ANY files */
		if (!Modem && !Nozmodem) {
			Command = TRUE;
			Cmdstr = "echo \"sz: Can't open any requested files\"";
			if (getnak()) {
				Exitcode=0200; canit();
			}
			if (!Zmodem)
				canit();
			else if (zsendcmd(Cmdstr, (int)(1+strlen(Cmdstr)))) {
				Exitcode=0200; canit();
			}
			Exitcode = 1; return OK;
		}
		canit();
		fprintf(STDERR,"\n\nCan't open any requested files.\n\n");
		return ERROR;
	}
	if (Zmodem)
		saybibi();
	else if(!Modem)
		wctxpn("");
	return OK;
}

int wcs(oname)
char *oname;
{
	extern struct stat statbuf;
	char name[PATHLEN];

	strcpy(name, oname);

	/* Check for directory or block special files */
	if(Fsfirst(name,(int)(0x01 | 0x010 | 0x020)) != 0)
	{
		++errcnt;
		return OK;	/* may be others */
	}

	if (statbuf.st_mode & S_IFDIR ) {
		return OK;
	}

	if((in = stfopen(oname,"r")) <= 0){
		++errcnt;
		return OK;	/* pass over it, there may be others */
	}
	BEofseen = Eofseen = 0;  vpos = 0;

	++Filcnt;
	switch (wctxpn(name)) {
	case ERROR:
		return ERROR;
	case ZSKIP:
		return OK;
	}
	if (!Zmodem && wctx(statbuf.st_size)==ERROR)
		return ERROR;
	if (Unlinkafter)
		unlink(oname);
	return 0;
}

#define ISDRIVE(X) ( (((X >= 'a') && (X <= 'n'))) || ((X >= 'A') && (X <= 'N')))
/*
 * generate and transmit pathname block consisting of
 *  pathname (null terminated),
 *  file length, mode time and file mode in octal
 *  as provided by the St's Fsfirst() call.
 *  N.B.: modifies the passed name, may extend it!
 */
static long start_time;

int wctxpn(name)
char *name;
{
	register char *p, *q;
	char name2[PATHLEN];
	unsigned long unixtime;
	extern struct stat statbuf;
	extern unsigned long st2unix();	/* Convert St's date and time to unix
				   time (seconds since Jan 1 1970 00:00:00) */
	extern void rd_time();

	if(*name)
		if(Fsfirst(name,(int)(0x01 | 0x020)) != 0)
			return ERROR;

	if (Modem) {
#ifndef REMOTE
		if (*name) {
			fprintf(STDERR,
			"Outgoing:\n\t Name: %s\n\t Size: %ld Bytes\n\
\tBlocks: %ld\n\tBufSize: %ld\n\n",
			  name, statbuf.st_size, statbuf.st_size>>7, (long)BBUFSIZ);
		}
#endif /* REMOTE */
		return OK;
	}

	vfile2("\r\nAwaiting pathname nak for %s\r\n", *name?name:"<END>");

	if ( !Zmodem)
		if (getnak())
			return ERROR;

	/* convert to Unix style path names */
	/* skip any device identifier */
	if(ISDRIVE(name[0]) && (name[1] == ':'))
		name = &name[2];

	for(p = name; *p != '\0'; p++)
	{
		if(*p == '\\')
			*p = '/';
	}

	if(!Resuming)
	{
		if(ForceBin)
		{
			Lzconv = ZCBIN;
			Ascii = FALSE;
		}
		else
		{
			if(!isbinary(name))
			{
				/* We indicate to the other side */
				Lzconv = ZCNL;
				Ascii = TRUE;
			}
			else
			{
				Lzconv = ZCBIN;
				Ascii = FALSE;
			}
		}
	}

	q = (char *) 0;
	if (Dottoslash) {		/* change . to . */
		for (p=name; *p; ++p) {
			if (*p == '/')
				q = p;
			else if (*p == '.')
				*(q=p) = '/';
		}
		if (q && (int)strlen(++q) > 8) {	/* If name>8 chars */
			q += 8;			/*   make it .ext */
			strcpy(name2, q);	/* save excess of name */
			*q = '.';
			strcpy(++q, name2);	/* add it back */
		}
	}

	for (p=name, q=txbuf ; *p; )
		if ((*q++ = *p++) == '/' && !Fullname)
			q = txbuf;
	*q++ = 0;
	p=q;
	while (q < (txbuf + KSIZE))
		*q++ = 0;
	if (*name)
	{
		unixtime = st2unix(statbuf.st_time, statbuf.st_date);
		sprintf(p, "%lu %lo %o %d %ld", statbuf.st_size,
			unixtime,
			 ((statbuf.st_mode & 0x01)?0444:0644),
			Filesleft, Totalleft);
		Totalleft -= statbuf.st_size;
	}

	if(Zmodem)
	{
	    Supexec(rd_time);
	    start_time = pr_time;
	}

	if(Verbose)
#ifndef REMOTE
		fprintf(STDERR,
"Outgoing: [Hit CTRL-C to Cancel]\n\tName: %s\n\tSize: %ld Bytes\n\tBufSize:\
 %ld\n",
			name, statbuf.st_size, (long)BBUFSIZ);
	if(!Resuming)
	{
		fprintf(STDERR,"\tMode: %s\n\n", (Ascii)?"ASCII":"BINARY");
	}
	else
	{
		fprintf(STDERR,"\tMode: Resume Transfer Mode\n\n");
	}
#endif

#ifdef SDEBUG
	if(Verbose > 2)
	{
		fprintf(STDERR,"File: %s (%s)\n", name, p);
		fprintf(logf,"File: %s (%s)\n", name, p);
		fflush(logf);
	}
#endif

	if (--Filesleft <= 0)
		Totalleft = 0;
	if (Totalleft < 0)
		Totalleft = 0;

	/* force 1k blocks if name won't fit in 128 byte block */
	if (txbuf[125])
		Blklen=KSIZE;
	else {		/* A little goodie for IMP/KMD */
		txbuf[127] = (statbuf.st_size + 127) >>7;
		txbuf[126] = (statbuf.st_size + 127) >>15;
	}

	if (Zmodem)
		return zsendfile(txbuf, (int)(1+strlen(p)+ 
				(int)((long)p-(long)txbuf)), statbuf.st_size);
	if (wcputsec(txbuf, 0, SECSIZ)==ERROR)
		return ERROR;
	return OK;
}

int getnak()
{
	register int firstch;

	Lastrx = 0;
	for (;;) {
		switch (firstch = readock(800)) {
		case ZPAD:
			if (getzrxinit())
				return ERROR;
			Ascii = 0;
			return FALSE;
		case TIMEOUT:
			vfile("Timeout on pathname\n");
			return TRUE;
		case WANTG:
#ifdef USG
			mode(2);	/* Set cbreak, XON/XOFF, etc. */
#endif
			Optiong = TRUE;
			Blklen=KSIZE;
		case WANTCRC:
			Crcflg = TRUE;
		case NAK:
			return FALSE;
		case CAN:
			if ((firstch = readock(20)) == CAN && Lastrx == CAN)
				return TRUE;
		default:
			break;
		}
		Lastrx = firstch;
	}
}


int wctx(flen)
long flen;
{
        int thisblklen;
	register int sectnum, attempts, firstch;
	long charssent;

	charssent = 0;
	Firstsec=TRUE;
	thisblklen = Blklen;
	
	while ((firstch=readock(Rxtimeout))!=NAK && firstch != WANTCRC
	  && firstch != WANTG && firstch!=TIMEOUT && firstch!=CAN)
		;
	if (firstch==CAN) {
		fprintf(STDERR, "\r\nReceiver CANcelled\n");
		return ERROR;
	}
	if (firstch==WANTCRC)
		Crcflg=TRUE;
	if (firstch==WANTG)
		Crcflg=TRUE;
	sectnum=0;
	for (;;) {
		if (flen <= (charssent + 896L))
			thisblklen = 128;
		if ( !filbuf(txbuf, thisblklen))
			break;
		if (wcputsec(txbuf, ++sectnum, thisblklen)==ERROR)
			return ERROR;
		charssent += thisblklen;
	}

#ifndef REMOTE
	if (Verbose>1)
	{
		fprintf(STDERR, "\nClosing\n\n");
#ifdef SDEBUG
		if(Verbose > 2)
		{
			fprintf(logf, " Closing\n");
			fflush(logf);
		}
#endif
	}
#endif /* REMOTE */

	stfclose(in);
	in = (-1);

	attempts=0;
	do {
		vfile(" EOT ");
		purgeline();
		sendline(EOT);
		flush_modem();
		++attempts;
	}
		while ((firstch=(readock(Rxtimeout)) != ACK) && attempts < RETRYMAX);
	if (attempts == RETRYMAX) {
		fprintf(STDERR, "\r\nNo ACK on EOT\n");
		return ERROR;
	}
	else
		return OK;
}

int wcputsec(buf, sectnum, cseclen)
char *buf;
int sectnum;
int cseclen;	/* data length of this sector to send */
{
	register int checksum, wcj;
	register char *cp;
	unsigned int oldcrc;
	int firstch;
	int attempts;

	firstch=0;	/* part of logic to detect CAN CAN */

#ifndef REMOTE
	if (Verbose>1)
	{
		fprintf(STDERR, "\rBlock %06d  %04dK ", Totsecs, (Totsecs>>3));
#ifdef SDEBUG
		if(Verbose > 2)
		{
			fprintf(logf, "\rBlock %d %dK ", Totsecs, (Totsecs>>3) );
			fflush(logf);
		}
#endif
	}
#endif /* REMOTE */

	for (attempts=0; attempts <= RETRYMAX; attempts++) {
		Lastrx= firstch;
		sendline(cseclen==KSIZE?STX:SOH);
		sendline(sectnum);
		sendline(-sectnum -1);
		oldcrc=checksum=0;
		for (wcj=cseclen,cp=buf; --wcj>=0; ) {
			sendline(*cp);
			oldcrc=updcrc((0377& *cp), oldcrc);
			checksum += *cp++;
		}
		if (Crcflg) {
			oldcrc=updcrc(0,updcrc(0,oldcrc));
			sendline((int)oldcrc>>8);
			sendline((int)oldcrc);
		}
		else
			sendline(checksum);
		flush_modem();
		
		if (Optiong) {
			Firstsec = FALSE; return OK;
		}
		firstch = readock(Rxtimeout);
gotnak:
		switch (firstch) {
		case CAN:
			if(Lastrx == CAN) {
cancan:
				fprintf(STDERR, "\r\nCancelled\n");  return ERROR;
			}
			break;
		case TIMEOUT:
			vfile("Timeout on sector ACK\n"); continue;
		case WANTCRC:
			if (Firstsec)
				Crcflg = TRUE;
		case NAK:
			vfile("NAK on sector\n"); continue;
		case ACK: 
			Firstsec=FALSE;
			Totsecs += (cseclen>>7);
			return OK;
		case ERROR:
			vfile("Got burst for sector ACK\n"); break;
		default:
			vfile("Got %02x for sector ACK\n", firstch); break;
		}
		for (;;) {
			Lastrx = firstch;
			if ((firstch = readock(Rxtimeout)) == TIMEOUT)
				break;
			if (firstch == NAK || firstch == WANTCRC)
				goto gotnak;
			if (firstch == CAN && Lastrx == CAN)
				goto cancan;
		}
	}
	fprintf(STDERR, "\r\nRetry Count Exceeded\n");
	return ERROR;
}

/* fill buf with count chars padding with ^Z for CPM */
int filbuf(buf, count)
register unsigned char *buf;
register int count;
{
	register int m, c;

	if(!Ascii)
	{
	    m = (int)stread(in, buf, (long)count);
	    if(m <= 0)
		return 0;
	    while (m < count)
		buf[m++] = 032;
	    return count;
	}
	m=count;
	if (Lfseen) {
		*buf++ = 012; --m; Lfseen = 0;
	}
	while ((c=stgetc(in))!=EOF) {
		if (c == 012) {
			*buf++ = 015;
			if (--m == 0) {
				Lfseen = TRUE; break;
			}
		}
		*buf++ =c;
		if (--m == 0)
			break;
	}
	if (m==count)
		return 0;
	else
		while (--m>=0)
			*buf++ = CPMEOF;
	return count;
}

/* fill buf with count chars */
int zfilbuf()
{
	int n, c;

#ifdef SDEBUG
	vfile("zfilbuf: bytcnt =%lu vpos=%lu blklen=%d", bytcnt, vpos, Blklen);
#endif
	/* We assume request is within buffer, or just beyond */
	txbuf = secbuf + (bytcnt & TXBMASK);
	if (vpos <= bytcnt) {
	    for (n=0; n<Blklen; n++)
		if ((c = stgetc(in)) == EOF)
		    break;
		else
		    txbuf[n] = c;
	    vpos += n;
	    if (n < Blklen)
		Eofseen = 1;
#ifdef SDEBUG
	    vfile("zfilbuf: n=%d vpos=%lu Eofseen=%d", n, vpos, Eofseen);
#endif
	    return n;
	}
	if (vpos >= (bytcnt+Blklen))
		return Blklen;
	/* May be a short block if crash recovery etc. */
	Eofseen = BEofseen;
	return (vpos - bytcnt);
}

int fooseek(fptr, pos, whence)
int fptr;
long pos;
int whence;
{
	long m, n;

#ifdef SDEBUG
	vfile("fooseek: pos =%lu vpos=%lu Canseek=1", pos, vpos);
#endif
	/* Seek offset < current buffer */
	if (pos < (vpos -TXBSIZE +1024)) {
		BEofseen = 0;
		if (1) {
			vpos = pos & ~TXBMASK;
			if (vpos >= pos)
				vpos -= TXBSIZE;
			if (stfseek(fptr, vpos, 0))
				return 1;
		}
		while (vpos < pos) {
		    n = (int)stread(fptr, secbuf, (long)TXBSIZE);
		    vpos += n;
#ifdef SDEBUG
		    vfile("n=%d vpos=%ld", n, vpos);
#endif
		    if (n < TXBSIZE) {
			BEofseen = 1;
			break;
		    }
		}
		return 0;
	}
	/* Seek offset > current buffer (Crash Recovery, etc.) */
	if (pos > vpos) {
		if (1)
			if (stfseek(fptr, vpos = (pos & ~TXBMASK), 0))
				return 1;
		while (vpos <= pos) {
			txbuf = secbuf + (vpos & TXBMASK);
			m = TXBSIZE - (vpos & TXBMASK);
			n = (int)stread(fptr, txbuf, (long)m);
			vpos += n;
			if (n < m) {
				BEofseen = 1;
				break;
			}
		}
		return 0;
	}
	/* Seek offset is within current buffer */
	return 0;
}
#define fseek fooseek

/*
 * readock(timeout) reads character(s) from file descriptor 0
 * timeout is in tenths of seconds
 */
readock(timeout)
int timeout;
{
	register int c;
	static char byt;

	if (setjmp(tohere)) {
		vfile("TIMEOUT\n");
		return TIMEOUT;
	}
	c = timeout >> 3;
	if (c<2)
		c=2;
#ifdef SDEBUG
	if (Verbose>3) {
		fprintf(STDERR, "Timeout=%d Calling alarm(%d) ", timeout, c);
		byt[1] = 0;
		fprintf(logf, "Timeout=%d Calling alarm(%d) ", timeout, c);
		fflush(logf);

	}
#endif
	stalarm(c);
	c=read_modem(&byt, 1);

	stalarm(0);
#ifdef SDEBUG
	if (Verbose>5)
	{
		fprintf(STDERR, "ret cnt=%d %x\n", c, byt);
		fprintf(logf, "ret cnt=%d %x\n", c, byt);
		fflush(logf);
	}
#endif

	if (c<1)
		return TIMEOUT;
	return (byt&0377);
}

int susage()
{
	fprintf(STDERR,"\nSend file(s) with ZMODEM/YMODEM/XMODEM Protocol\n");
	fprintf(STDERR,"	(Y) = Option applies to YMODEM only\n");
	fprintf(STDERR,"	(Z) = Option applies to ZMODEM only\n");
	fprintf(STDERR,
	"%s for %s by ST Enthusiasts at Case Western Reserve University\n",
	   SSTVERSION, STOS);
	fprintf(STDERR,"\tBased on %s for %s by Chuck Forsberg\n\n", SVERSION, OS);
/*	fprintf(STDERR,"Usage:	sz [-12+adefknquvXy] [-] file ...\n"); */
	fprintf(STDERR,"Usage:	sz [-+defknquvXyBwZ] file ...\n");
/*	fprintf(STDERR,"	sz [-1eqv] -c COMMAND\n"); */
	fprintf(STDERR,"	sz [-eqv] -c COMMAND\n");
	fprintf(STDERR,"	sb [-dfkquv] [-] file ...\n");
	fprintf(STDERR,"	sx [-kquv] [-] file\n");

/*	fprintf(STDERR,"	1 Use stdout for modem input\n"); */
#ifdef CSTOPB
	fprintf(STDERR,"	2 Use 2 stop bits\n");
#endif
	fprintf(STDERR,"	+ Append to existing destination file (Z)\n");
/*	fprintf(STDERR,"	a (ASCII) change NL to CR/LF\n"); */
	fprintf(STDERR,"	c send COMMAND (Z)\n");
	fprintf(STDERR,"	d Change '.' to '/' in pathnames (Y/Z)\n");
	fprintf(STDERR,"	e Escape control characters (Z)\n");
	fprintf(STDERR,"	f send Full pathname (Y/Z)\n");
	fprintf(STDERR,"	i send COMMAND, ack Immediately (Z)\n");
	fprintf(STDERR,"	k Send 1024 byte packets (Y)\n");
	fprintf(STDERR,"	L N Limit packet length to N bytes (Z)\n");
	fprintf(STDERR,"	l N Limit frame length to N bytes (l>=L) (Z)\n");
	fprintf(STDERR,"	n send file if Newer (Z)\n");
	fprintf(STDERR,"	N send file if Newer or Longer (Z)\n");
	fprintf(STDERR, "	o Use 16 bit CRC instead of 32 bit CRC (Z)\n");
	fprintf(STDERR,"	p Protect existing destination file (Z)\n");
	fprintf(STDERR,"	r Resume/Recover interrupted file transfer (Z)\n");
	fprintf(STDERR,"	q Quiet (no progress reports)\n");
	fprintf(STDERR,"	u Unlink file after transmission\n");
	fprintf(STDERR,"	v Verbose - debugging information\n");
	fprintf(STDERR,"	X XMODEM protocol - send no pathnames\n");
	fprintf(STDERR,"	y Yes, overwrite existing file (Z)\n");
	fprintf(STDERR,"	B Force Binary mode transfers (Z)\n");
	fprintf(STDERR,"	w N Window is N bytes (Z)\n");
	fprintf(STDERR,"	Z   Activate ZMODEM compression(Z)\n");
/*	fprintf(STDERR,
"- as pathname sends standard input as sPID.sz or environment ONAME\n"); */
	return(1);
}

/*
 * Get the receiver's init parameters
 */
int getzrxinit()
{
	register int n;

	for (n=10; --n>=0; ) {
		
		switch (zgethdr(Rxhdr, 1)) {
		case ZCHALLENGE:	/* Echo receiver's challenge numbr */
			stohdr(Rxpos);
			zshhdr(4, ZACK, Txhdr);
			continue;
		case ZCOMMAND:		/* They didn't see out ZRQINIT */
			stohdr(0L);
			zshhdr(4, ZRQINIT, Txhdr);
			continue;
		case ZRINIT:
			Rxflags = 0377 & Rxhdr[ZF0];
			Usevhdrs = Rxhdr[ZF1] & CANVHDR;
 			Txfcs32 = (Wantfcs32 && (Rxflags & CANFC32));

			Zctlesc |= Rxflags & TESCCTL;
			Rxbuflen = 
			    ((unsigned)0337 & (unsigned)Rxhdr[ZP0])+
				(((unsigned)0377 & (unsigned)Rxhdr[ZP1])<<8); 
			if ( !(Rxflags & CANFDX))
				Txwindow = 0;
			vfile("Rxbuflen=%d Tframlen=%d", Rxbuflen, Tframlen);
			siggi = 0;
#ifndef READCHECK
#ifdef USG
			mode(2);	/* Set cbreak, XON/XOFF, etc. */
#else
#if 0
			/* Use 1024 byte frames if no sample/interrupt */
			if (Rxbuflen < 32 || Rxbuflen > 1024) {
				Rxbuflen = 1024;
				vfile("Rxbuflen=%d", Rxbuflen);
			}
#endif
#endif
#endif
			/* Override to force shorter frame length */
			if (Rxbuflen && (Rxbuflen>Tframlen) && (Tframlen>=32))
				Rxbuflen = Tframlen;
			if ( !Rxbuflen && (Tframlen>=32) && (Tframlen<=1024))
				Rxbuflen = Tframlen;
			vfile("Rxbuflen=%d", Rxbuflen);
#if 0
			/*
			 * If input is not a regular file, force ACK's each 1024
			 *  (A smarter strategey could be used here ...)
			 */
			if (Rxbuflen && (Rxbuflen > 1024))
				Rxbuflen = 1024;
			vfile("Rxbuflen=%d", Rxbuflen);
#endif
			/* Set initial subpacket length */
			if (Blklen < 1024) {	/* Command line override? */
				if (Baudrate > 110)
					Blklen = 256;
				if (Baudrate > 1200)
					Blklen = 1024;
			}
			if (Rxbuflen && Blklen>Rxbuflen)
				Blklen = Rxbuflen;
			if (blkopt && Blklen > blkopt)
				Blklen = blkopt;

			if (Lztrans == ZTRLE && (Rxflags & CANRLE))
				Txfcs32 = 2;
			else
				Lztrans = 0;

			return (sendzsinit());
		case ZCAN:
		case TIMEOUT:
			return ERROR;
		case ZRQINIT:
			if (Rxhdr[ZF0] == ZCOMMAND)
				continue;
		default:
			zshhdr(4, ZNAK, Txhdr);
			continue;
		}
	}
	return ERROR;
}

/* Send send-init information */
int sendzsinit()
{
	register int c;
	register int errors;
	
	if (Myattn[0] == '\0' && (!Zctlesc || (Rxflags & TESCCTL)))
		return OK;

	errors = 0;
	for (;;) {
		stohdr(0L);
#ifdef ALTCANOFF
		Txhdr[ALTCOFF] = ALTCANOFF;
#endif
		if (Zctlesc) {
			Txhdr[ZF0] |= TESCCTL; zshhdr(4, ZSINIT, Txhdr);
		}
		else
			zsbhdr(4, ZSINIT, Txhdr);
		zsdata(Myattn, (int)(1+strlen(Myattn)), ZCRCW);
		c = zgethdr(Rxhdr, 1);
		switch (c) {
		case ZCAN:
			return ERROR;
		case ZACK:
			return OK;
		default:
			if (++errors > 19)
				return ERROR;
			continue;
		}
	}
}

/* Send file name and related info */
int zsendfile(buf, blen, szbytes)
char *buf;
int blen;
long szbytes;
{
	register int c, szstat;
	long end_time;
	register unsigned long crc;
	extern void rd_time();
	long lastcrcrq = -1L;

	Supexec(rd_time);
	start_time = pr_time;
	
	for (;;) {
		Txhdr[ZF0] = Lzconv;	/* file conversion request */
		Txhdr[ZF1] = Lzmanag;	/* file management request */
		if (Lskipnocor)
			Txhdr[ZF1] |= ZMSKNOLOC;
		Txhdr[ZF2] = Lztrans;	/* file transport request */
		Txhdr[ZF3] = 0;
		zsbhdr(4, ZFILE, Txhdr);
		zsdata(buf, blen, ZCRCW);
again:
		c = zgethdr(Rxhdr, 1);
		switch (c) {
		case ZRINIT:
			while ((c = readock(50)) > 0)
				if (c == ZPAD) {
					goto again;
				}
			/* **** FALL THRU TO **** */
		default:
			continue;

		case ZCAN:
		case TIMEOUT:
		case ZABORT:
		case ZFIN:
			return ERROR;
		case ZCRC:
			if (Rxpos != lastcrcrq) {
				lastcrcrq = Rxpos;
				crc = 0xFFFFFFFFL;
				if (1) {
					fseek(in, 0L, 0);
					while (((c = stgetc(in)) != EOF) && --lastcrcrq)
						crc = UPDC32(c, crc);
					crc = ~crc;
/*					clearerr(in);	/* Clear possible EOF */
					lastcrcrq = Rxpos;
				}
			}
			stohdr(crc);
			zsbhdr(4, ZCRC, Txhdr);
			goto again;
		case ZSKIP:
#ifndef REMOTE
			fprintf(STDERR,"\n\n");
#endif
			stfclose(in); in = (-1); return c;
		case ZRPOS:
			if(fseek(in, Rxpos, 0))
			{
				fprintf(STDERR,"\r\nError While Seeking file\n");
				return ERROR;
			}
			Lastsync = (bytcnt = Txpos = Lrxpos = Rxpos) -1;
			if((szstat =  zsendfdata()) == OK)
			{
#ifndef REMOTE
				Supexec(rd_time);
				end_time = pr_time;
				fprintf(STDERR,"%ld Bytes Sent\t\
Transfer Time %ld secs.\tApprox %ld cps\n\n", szbytes, (end_time - start_time)/200L,
szbytes/((end_time - start_time)/200L));
#endif
			}
			return szstat;
		}
	}
}

static int Beenhereb4;

/* Send the data in the file */
int zsendfdata()
{
	register int c1, e;
	register long newcnt;
#ifdef __GNUC__
	volatile long tcount = 0;
	volatile int n;
#else
	register int n;
	register long tcount = 0;
#endif
	long ttcount = 0;
	static int tleft = 6;	/* Counter for test mode */
	int junkcount;
	
	Lrxpos = 0;
	junkcount = 0;
	Beenhereb4 = FALSE;
	
somemore:
	if (setjmp(intrjmp)) {
waitack:
	    junkcount = 0;
	    c1 = getinsync(0);
gotack:	    
		switch (c1) {
		default:
		case ZCAN:
			fprintf(STDERR,"\r\nReceiver Cancelled Transfer\n\n");
			stfclose(in);
			in = (-1);
			return ERROR;
		case ZSKIP:
#ifndef REMOTE
			fprintf(STDERR,"\r\nReceiver forced SKIP Transfer(1)\n\n");
#endif
			stfclose(in);
			in = (-1);
			return c1;
		case ZACK:
			/* fall thru */
		case ZRPOS:
			break;
		case ZRINIT:
			return OK;
		}
	}

	siggi = 1;
	newcnt = Rxbuflen;
	Txwcnt = 0;
	stohdr(Txpos);
	zsbhdr(4, ZDATA, Txhdr);

	do {
		n = zfilbuf();
		tcount += n;
		lreport(tcount);
		if (Eofseen)
			e = ZCRCE;
		else if (junkcount > 3)
			e = ZCRCW;
		else if (bytcnt == Lastsync)
			e = ZCRCW;
		else if (Rxbuflen && (newcnt -= n) <= 0)
		        e = ZCRCW;
		else if (Txwindow && (Txwcnt += n) >= Txwspac) {
			Txwcnt = 0;  e = ZCRCQ;
		} else
			e = ZCRCG;
		zsdata(txbuf, n, e);
		bytcnt = (Txpos += n);

		if (e == ZCRCW)
			goto waitack;
		if (Txwindow) {
			while ((ttcount = (Txpos - Lrxpos)) >= Txwindow) {
				vfile("%ld window >= %u", ttcount, Txwindow);
				if (e != ZCRCQ)
					zsdata(txbuf, 0, e = ZCRCQ);
				c1 = getinsync(1);
				if (c1 != ZACK) {
					zsdata(txbuf, 0, ZCRCE);
					goto gotack;
				}
			}
/*			vfile("window = %ld", ttcount); */
		}
	} while (!Eofseen);

	siggi = 0;
	lsct = 1;

	for (;;) {
		stohdr(Txpos);
		zsbhdr(4, ZEOF, Txhdr);
		switch (getinsync(0)) {
		case ZACK:
			continue;
		case ZRPOS:
			goto somemore;
		case ZRINIT:
			return OK;
		case ZSKIP:
#ifndef REMOTE
			fprintf(STDERR,"\r\nReceiver forced SKIP Transfer(2)\n\n");
#endif
			stfclose(in);
			in = (-1);
			return c1;
		default:
			fprintf(STDERR,"\r\nErrors while Send Data\n\n");
			stfclose(in);
			in = (-1);
			return ERROR;
		}
	}
}

/*
 * Respond to receiver's complaint, get back in sync with receiver
 */
int getinsync(flag)
int flag;
{
	register int c;

	for (;;) {
#ifdef TESTATTN
		if (Testattn) {
			wr_modem("\r\n\n\n***** Signal Caught *****\r\n");
			Rxpos = 0; c = ZRPOS;
		} else
#endif
			c = zgethdr(Rxhdr, 0);
		switch (c) {
		case ZCAN:
		case ZABORT:
		case ZFIN:
		case TIMEOUT:
			return ERROR;
		case ZRPOS:
			if (fseek(in, Rxpos, 0))
				return ERROR;
			Eofseen = 0;
			bytcnt = Lrxpos = Txpos = Rxpos;
			if (Lastsync == Rxpos) {
				if (++Beenhereb4 > 4)
					if (Blklen > 32)
						Blklen /= 2;
			}
			Lastsync = Rxpos;
			return c;
		case ZACK:
			Lrxpos = Rxpos;
			if (flag || Txpos == Rxpos)
				return ZACK;
			continue;
		case ZRINIT:
		case ZSKIP:
#ifndef REMOTE
			fprintf(STDERR,"\033K\n\n");
#endif
			stfclose(in);
			in = (-1);
			return c;
		case ERROR:
		default:
			zsbhdr(4, ZNAK, Txhdr);
			continue;
		}
	}
}
/* Say "bibi" to the receiver, try to do it cleanly */
void  saybibi()
{
	for (;;) {
		stohdr(0L);
		zsbhdr(4, ZFIN, Txhdr);
		switch (zgethdr(Rxhdr, 0)) {
		case ZFIN:
			sendline('O'); sendline('O'); flush_modem();
		case ZCAN:
		case TIMEOUT:
			return;
		}
	}
}

/* Send command and related info */
int zsendcmd(buf, blen)
char *buf;
int blen;
{
	register int c, errors;
	long cmdnum;

/*	cmdnum = getpid(); */
	cmdnum = 1;	/* A random # */
	errors = 0;
	for (;;) {
		stohdr(cmdnum);
		Txhdr[ZF0] = Cmdack1;
		zsbhdr(4, ZCOMMAND, Txhdr);
		zsdata(buf, blen, ZCRCW);
listen:
		Rxtimeout = 100;		/* Ten second wait for resp. */
		Usevhdrs = 0;		/* Allow rx to send fixed len headers */
		c = zgethdr(Rxhdr, 1);

		switch (c) {
		case ZRINIT:
		    goto listen;
		case ERROR:
		case GCOUNT:
		case TIMEOUT:
			if (++errors > Cmdtries)
				return ERROR;
			continue;
		case ZCAN:
		case ZABORT:
		case ZFIN:
		case ZSKIP:
		case ZRPOS:
			return ERROR;
		default:
			if (++errors > 20)
				return ERROR;
			continue;
		case ZCOMPL:
			Exitcode = Rxpos;
			saybibi();
			return OK;
		case ZRQINIT:
			vfile("******** RZ *******");
/*			stsystem("rz"); */
			vfile("******** SZ *******");
			goto listen;
		}
	}
}


#ifndef STANDALONE
/*
 * If called as sb use YMODEM protocol
 */
void schkinvok(s)
char *s;
{
	if (s[0]=='s' && s[1]=='b') {
		Nozmodem = TRUE; Blklen=KSIZE;
	}
	if (s[0]=='s' && s[1]=='x') {
		Modem = TRUE;
	}
}
#endif

static void countem(argc, argv)
int argc;
char **argv;
{
    register c;
    extern struct stat statbuf;
    
    for (Totalleft = 0, Filesleft = 0; --argc >=0; ++argv)
    {
	/* Check for directory or block special files */
	if(Fsfirst(*argv,(int)(0x01 | 0x010 | 0x020)) != 0)
	    continue;
	
	if (statbuf.st_mode & S_IFDIR )
	    continue;
	
	++Filesleft;  Totalleft += statbuf.st_size;
    }
}

/* -eof- */
