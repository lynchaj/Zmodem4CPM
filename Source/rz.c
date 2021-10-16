#define VERSION "1.03 05-18-86"
#define PUBDIR "/usr/spool/uucppublic"

/*% cc -DNFGVMIN -K -O % -o rz; size rz
 *
 * rz.c By Chuck Forsberg
 *
 *	cc -O rz.c -o rz		USG (3.0) Unix
 * 	cc -O -DV7  rz.c -o rz		Unix V7, BSD 2.8 - 4.3
 *
 *	ln rz rb			For either system
 *
 *	ln rz /usr/bin/rbrmail		For remote mail.  Make this the
 *					login shell. rbrmail then calls
 *					rmail(1) to deliver mail.
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
#define LOGFILE "/tmp/rzlog"
#define zperr vfile

#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
FILE *popen();

#define OK 0
#define FALSE 0
#define TRUE 1
#define ERROR (-1)

#define HOWMANY 133
int Zmodem=0;		/* ZMODEM protocol requested */
int Nozmodem = 0;	/* If invoked as "rb" */
unsigned Baudrate;
#include "rbsb.c"	/* most of the system dependent stuff here */

char *substr();
FILE *fout;

/*
 * Routine to calculate the free bytes on the current file system
 *  ~0 means many free bytes (unknown)
 */
long getfree()
{
	return(~0L);	/* many free bytes ... */
}

char *Extensions[] = {
".A",
".ARC",
".CCC",
".CL",
".CMD",
".COM",
".CRL",
".DAT",
".DIR",
".EXE",
".O",
".OBJ",
".OVL",
".PAG",
".REL",
".SAV",
".SUB",
".SWP",
".SYS",
".TAR",
".UTL",
".a",
".arc",
".com",
".dat",
".o",
".obj",
".ovl",
".sys",
".tar",
""
};

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
#define TIMEOUT (-2)
#define ERRORMAX 5
#define RETRYMAX 5
#define WCEOT (-10)
#define SECSIZ 128	/* cp/m's Magic Number record size */
#define PATHLEN 257	/* ready for 4.2 bsd ? */
#define KSIZE 1024	/* record size with k option */
#define UNIXFILE 0x8000	/* happens to the the S_IFREG file mask bit for stat */

int Lastrx;
int Crcflg;
int Firstsec;
int Eofseen;		/* indicates cpm eof (^Z) has been received */
int errors;
int Restricted=0;	/* restricted; no /.. or ../ in filenames */
int Badclose = 0;	/* Error on last close */
#ifdef ONEREAD
/* Sorry, Regulus and some others don't work right in raw mode! */
int Readnum = 1;	/* Number of bytes to ask for in read() from modem */
#else
int Readnum = KSIZE;	/* Number of bytes to ask for in read() from modem */
#endif

#define DEFBYTL 2000000000L	/* default rx file size */
long Bytesleft;		/* number of bytes of incoming file left */
long Modtime;		/* Unix style mod time for incoming file */
short Filemode;		/* Unix style mode for incoming file */
char Pathname[PATHLEN];
char *Progname;		/* the name by which we were called */

int Batch=0;
int Wcsmask=0377;
int Topipe=0;
int MakeLCPathname=TRUE;	/* make received pathname lower case */
int Verbose=0;
int Quiet=0;		/* overrides logic that would otherwise set verbose */
int Nflag = 0;		/* Don't really transfer files */
int Rxbinary=FALSE;	/* receive all files in bin mode */
int Thisbinary;		/* current file is to be received in bin mode */
int Blklen;		/* record length of received packets */
char secbuf[KSIZE];
char linbuf[KSIZE];
int Lleft=0;		/* number of characters in linbuf */
time_t timep[2];
char zconv;		/* ZMODEM file conversion request */
char zmanag;		/* ZMODEM file management request */
char ztrans;		/* ZMODEM file transport request */

jmp_buf tohere;		/* For the interrupt on RX timeout */

unsigned short updcrc();

#include "zm.c"


alrm()
{
	longjmp(tohere, -1);
}

/* called by signal interrupt or terminate to clean things up */
bibi(n)
{
	if (Zmodem)
		zmputs(Attn);
	canit(); mode(0);
	fprintf(stderr, "sb: caught signal %d; exiting", n);
	exit(128+n);
}

main(argc, argv)
char *argv[];
{
	register char *cp;
	register npats;
	char *virgin, **patts;
	char *getenv();
	int exitcode;

	setbuf(stderr, NULL);
	if ((cp=getenv("SHELL")) && (substr(cp, "rsh") || substr(cp, "rsh")))
		Restricted=TRUE;

	chkinvok(virgin=argv[0]);	/* if called as [-]rzCOMMAND set flag */
	npats = 0;
	while (--argc) {
		cp = *++argv;
		if (*cp == '-') {
			while( *++cp) {
				switch(*cp) {
				case '1':
					iofd = 1; break;
				case '7':
					Wcsmask = 0177;
				case 'b':
					Rxbinary=TRUE; break;
				case 'k':
				case 'c':
					Crcflg=TRUE; break;
				case 'D':
					Nflag = TRUE; break;
				case 'q':
					Quiet=TRUE; Verbose=0; break;
				case 'u':
					MakeLCPathname=FALSE; break;
				case 'v':
					++Verbose; break;
				default:
					usage();
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
	if (npats > 1)
		usage();
	if (Verbose) {
		if (freopen(LOGFILE, "a", stderr)==NULL) {
			printf("Can't open log file %s\n",LOGFILE);
			exit(0200);
		}
		setbuf(stderr, NULL);
		fprintf(stderr, "argv[0]=%s Progname=%s\n", virgin, Progname);
	}
	if (fromcu() && !Quiet) {
		if (Verbose == 0)
			Verbose = 2;
	}
	mode(1);
	if (signal(SIGINT, bibi) == SIG_IGN) {
		signal(SIGINT, SIG_IGN); signal(SIGKILL, SIG_IGN);
	}
	else {
		signal(SIGINT, bibi); signal(SIGKILL, bibi);
	}
	if (wcreceive(npats, patts)==ERROR) {
		exitcode=0200;
		canit();
	}
	mode(0);
	if (exitcode && !Zmodem)	/* bellow again with all thy might. */
		canit();
	exit(exitcode);
}


usage()
{
	fprintf(stderr,"%s %s for %s by Chuck Forsberg\n",
	  Progname, VERSION, OS);
	fprintf(stderr,"Usage:	rz [-1buv]		(ZMODEM Batch)\n");
	fprintf(stderr,"or	rb [-1buv]		(YMODEM Batch)\n");
	fprintf(stderr,"or	rz [-1bcv] file		(XMODEM)\n");
	fprintf(stderr,"	  -1 For cu(1): Use fd 1 for input\n");
	fprintf(stderr,"	  -b Binary transfer for all files\n");
	fprintf(stderr,"	  -u Allow all UPPER CASE names\n");
	fprintf(stderr,"	  -v Verbose more v's give more info\n");
	fprintf(stderr,"	  -c Use 16 bit CRC	(XMODEM)\n");
	exit(1);
}
/*
 *  Debugging information output interface routine
 */
/* VARARGS1 */
vfile(f, a, b, c)
register char *f;
{
	if (Verbose > 1) {
		fprintf(stderr, f, a, b, c);
		fprintf(stderr, "\n");
	}
}

/*
 * Let's receive something already.
 */
wcreceive(argc, argp)
char **argp;
{
	register c;

	if (Batch || argc==0) {
		Crcflg=(Wcsmask==0377);
		if ( !Quiet)
			fprintf(stderr, "rz: ready ");
		if (c=tryz()) {
			if (c == ZCOMPL)
				return OK;
			if (c == ERROR)
				goto fubar;
			c = rzfiles();
			if (c)
				goto fubar;
		} else {
			for (;;) {
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
	} else {
		Bytesleft = DEFBYTL; Filemode = 0; Modtime = 0L;

		strcpy(Pathname, *argp);
		checkpath(Pathname);
		fprintf(stderr, "\nrz: ready to receive %s ", Pathname);
		if ((fout=fopen(Pathname, "w")) == NULL)
			return ERROR;
		if (wcrx()==ERROR)
			goto fubar;
	}
	return OK;
fubar:
	canit();
	if (Topipe && fout) {
		pclose(fout);  return ERROR;
	}
	if (fout)
		fclose(fout);
	if (Restricted) {
		unlink(Pathname);
		fprintf(stderr, "\r\nrz: %s removed.\r\n", Pathname);
	}
	return ERROR;
}


/*
 * Fetch a pathname from the other end as a C ctyle ASCIZ string.
 * Length is indeterminate as long as less than Blklen
 * A null string represents no more files (YMODEM)
 */
wcrxpn(rpn)
char *rpn;	/* receive a pathname */
{
	register c;

#ifdef NFGVMIN
	readline(1);
#else
	purgeline();
#endif

et_tu:
	Firstsec=TRUE;  Eofseen=FALSE;
	sendline(Crcflg?WANTCRC:NAK);
	Lleft=0;	/* Do read next time ... */
	while ((c = wcgetsec(rpn, 100)) != 0) {
		log( "Pathname fetch returned %d\n", c);
		if (c == WCEOT) {
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

wcrx()
{
	register int sectnum, sectcurr;
	register char sendchar;
	register char *p;
	int cblklen;			/* bytes to dump this block */

	Firstsec=TRUE;sectnum=0; Eofseen=FALSE;
	sendchar=Crcflg?WANTCRC:NAK;

	for (;;) {
		sendline(sendchar);	/* send it now, we're ready! */
		Lleft=0;	/* Do read next time ... */
		sectcurr=wcgetsec(secbuf, (sectnum&0177)?50:130);
		report(sectcurr);
		if (sectcurr==(sectnum+1 &Wcsmask)) {

			if (sectnum==0 && !Thisbinary) {
				p=secbuf;  sectcurr=Blklen;
				if (*p == 032)	/* A hack for .arc files */
					goto binbin;
				for (; *p != 032 && --sectcurr>=0; ++p)
					if (*p < 07 || (*p & 0200)) {
binbin:
						Thisbinary++;
						if (Verbose)
							fprintf(stderr, "Changed to BIN\n");
						break;
					}
			}
			sectnum++;
			cblklen = Bytesleft>Blklen ? Blklen:Bytesleft;
			if (putsec(secbuf, cblklen)==ERROR)
				return ERROR;
			if ((Bytesleft-=cblklen) < 0)
				Bytesleft = 0;
			sendchar=ACK;
		}
		else if (sectcurr==(sectnum&Wcsmask)) {
			log( "Received dup Sector\n");
			sendchar=ACK;
		}
		else if (sectcurr==WCEOT) {
			if (closeit())
				return ERROR;
			sendline(ACK);
			Lleft=0;	/* Do read next time ... */
			return OK;
		}
		else if (sectcurr==ERROR)
			return ERROR;
		else {
			log( "Sync Error\n");
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

wcgetsec(rxbuf, maxtime)
char *rxbuf;
int maxtime;
{
	register checksum, wcj, firstch;
	register unsigned short oldcrc;
	register char *p;
	int sectcurr;

	for (Lastrx=errors=0; errors<RETRYMAX; errors++) {

		if ((firstch=readline(maxtime))==STX) {
			Blklen=KSIZE; goto get2;
		}
		if (firstch==SOH) {
			Blklen=SECSIZ;
get2:
			sectcurr=readline(1);
			if ((sectcurr+(oldcrc=readline(1)))==Wcsmask) {
				oldcrc=checksum=0;
				for (p=rxbuf,wcj=Blklen; --wcj>=0; ) {
					if ((firstch=readline(1)) < 0)
						goto bilge;
					oldcrc=updcrc(firstch, oldcrc);
					checksum += (*p++ = firstch);
				}
				if ((firstch=readline(1)) < 0)
					goto bilge;
				if (Crcflg) {
					oldcrc=updcrc(firstch, oldcrc);
					if ((firstch=readline(1)) < 0)
						goto bilge;
					oldcrc=updcrc(firstch, oldcrc);
					if (oldcrc & 0xFFFF)
						log("CRC=0%o\n", oldcrc);
					else {
						Firstsec=FALSE;
						return sectcurr;
					}
				}
				else if (((checksum-firstch)&Wcsmask)==0) {
					Firstsec=FALSE;
					return sectcurr;
				}
				else
					log( "Checksum Error\n");
			}
			else
				log("Sector number garbled 0%o 0%o\n",
				 sectcurr, oldcrc);
		}
		/* make sure eot really is eot and not just mixmash */
#ifdef NFGVMIN
		else if (firstch==EOT && readline(1)==TIMEOUT)
			return WCEOT;
#else
		else if (firstch==EOT && Lleft==0)
			return WCEOT;
#endif
		else if (firstch==CAN) {
			if (Lastrx==CAN) {
				log( "Sender CANcelled\n");
				return ERROR;
			} else {
				Lastrx=CAN;
				continue;
			}
		}
		else if (firstch==TIMEOUT) {
			if (Firstsec)
				goto humbug;
bilge:
			log( "Timeout\n");
		}
		else
			log( "Got 0%o sector header\n", firstch);

humbug:
		Lastrx=0;
		while(readline(1)!=TIMEOUT)
			;
		if (Firstsec) {
			sendline(Crcflg?WANTCRC:NAK);
			Lleft=0;	/* Do read next time ... */
		} else {
			maxtime=40; sendline(NAK);
			Lleft=0;	/* Do read next time ... */
		}
	}
	/* try to stop the bubble machine. */
	canit();
	return ERROR;
}

/*
 * This version of readline is reasoably well suited for
 * reading many characters.
 *  (except, currently, for the Regulus version!)
 *
 * timeout is in tenths of seconds
 */
readline(timeout)
int timeout;
{
	register n;
	static char *cdq;	/* pointer for removing chars from linbuf */

	if (--Lleft >= 0) {
		if (Verbose > 8) {
			fprintf(stderr, "%02x ", *cdq&0377);
		}
		return (*cdq++ & Wcsmask);
	}
	n = timeout/10;
	if (n < 2)
		n = 3;
	if (Verbose > 3)
		fprintf(stderr, "Calling read: n=%d ", n);
	if (setjmp(tohere)) {
#ifdef TIOCFLUSH
/*		ioctl(iofd, TIOCFLUSH, 0); */
#endif
		Lleft = 0;
		if (Verbose>1)
			fprintf(stderr, "Readline:TIMEOUT\n");
		return TIMEOUT;
	}
	signal(SIGALRM, alrm); alarm(n);
	Lleft=read(iofd, cdq=linbuf, Readnum);
	alarm(0);
	if (Verbose > 3) {
		fprintf(stderr, "Read returned %d bytes\n", Lleft);
	}
	if (Lleft < 1)
		return TIMEOUT;
	--Lleft;
	if (Verbose > 8) {
		fprintf(stderr, "%02x ", *cdq&0377);
	}
	return (*cdq++ & Wcsmask);
}



/*
 * Purge the modem input queue of all characters
 */
purgeline()
{
	Lleft = 0;
#ifdef USG
	ioctl(iofd, TCFLSH, 0);
#else
	lseek(iofd, 0L, 2);
#endif
}

/*
 * Update CRC CRC-16 used by XMODEM/CRC, YMODEM, and ZMODEM.
 *  Note: Final result must be masked with 0xFFFF before testing
 *  More efficient table driven routines exist.
 */
unsigned short
updcrc(c, crc)
register c;
register unsigned crc;
{
	register count;

	for (count=8; --count>=0;) {
		if (crc & 0x8000) {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
			crc ^= 0x1021;
		}
		else {
			crc <<= 1;
			crc += (((c<<=1) & 0400)  !=  0);
		}
	}
	return crc;	
}

/*
 * Process incoming file information header
 */
procheader(name)
char *name;
{
	register char *openmode, *p, **pp;

	/* set default parameters */
	openmode = "w"; Thisbinary=Rxbinary;

	/*
	 *  Process ZMODEM remote file management requests
	 */
	if (zconv == ZCNL)	/* Remote ASCII override */
		Thisbinary = 0;
	if (zconv == ZCBIN)	/* Remote Binary override */
		++Thisbinary;
	else if (zmanag == ZMAPND)
		openmode = "a";

	Bytesleft = DEFBYTL; Filemode = 0; Modtime = 0L;

	p = name + 1 + strlen(name);
	if (*p) {	/* file coming from Unix or DOS system */
		sscanf(p, "%ld%lo%o", &Bytesleft, &Modtime, &Filemode);
		if (Filemode & UNIXFILE)
			++Thisbinary;
		if (Verbose) {
			fprintf(stderr,  "Incoming: %s %ld %lo %o\n",
			  name, Bytesleft, Modtime, Filemode);
		}
	}
	else {		/* File coming from CP/M system */
		for (p=name; *p; ++p)		/* change / to _ */
			if ( *p == '/')
				*p = '_';

		if ( *--p == '.')		/* zap trailing period */
			*p = 0;
	}

	/* scan for extensions that signify a binary file */
	if (p=substr(name, "."))
		for (pp=Extensions; **pp; ++pp)
			if (strcmp(p, *pp)==0) {
				Thisbinary=TRUE; break;
			}

	/* scan for files which should be appended */
	if ( !Thisbinary
	  && (substr(name, ".TXT")
	  || substr(name, ".txt")
	  || substr(name, ".MSG")))
		openmode = "a";
	if (MakeLCPathname && !IsAnyLower(name))
		uncaps(name);
	if (Topipe) {
		sprintf(Pathname, "%s %s", Progname+2, name);
		if (Verbose)
			fprintf(stderr,  "Topipe: %s %s\n",
			  Pathname, Thisbinary?"BIN":"ASCII");
		if ((fout=popen(Pathname, "w")) == NULL)
			return ERROR;
	} else {
		strcpy(Pathname, name);
		if (Verbose) {
			fprintf(stderr,  "Receiving %s %s %s\n",
			  name, Thisbinary?"BIN":"ASCII", openmode);
		}
		checkpath(name);
		if (Nflag)
			name = "/dev/null";
		if ((fout=fopen(name, openmode)) == NULL)
			return ERROR;
	}
	return OK;
}

/* Make string s lower case */
uncaps(s)
register char *s;
{
	for ( ; *s; ++s)
		if (isupper(*s))
			*s = tolower(*s);
}


/*
 * IsAnyLower returns TRUE if string s has lower case letters.
 */
IsAnyLower(s)
register char *s;
{
	for ( ; *s; ++s)
		if (islower(*s))
			return TRUE;
	return FALSE;
}
/*
 * Putsec writes the n characters of buf to receive file fout.
 *  If not in binary mode, carriage returns, and all characters
 *  starting with CPMEOF are discarded.
 */
putsec(buf, n)
char *buf;
register n;
{
	register char *p;

	if (Thisbinary) {
		for (p=buf; --n>=0; )
			putc( *p++, fout);
	}
	else {
		if (Eofseen)
			return OK;
		for (p=buf; --n>=0; ++p ) {
			if ( *p == '\r')
				continue;
			if (*p == CPMEOF) {
				Eofseen=TRUE; return OK;
			}
			putc(*p ,fout);
		}
	}
	return OK;
}

/*
 *  Send a character to modem.  Small is beautiful.
 */
sendline(c)
{
	char d;

	d = c;
	if (Verbose>4)
		fprintf(stderr, "Sendline: %x\n", c);
	write(1, &d, 1);
}

xsendline(c)
{
	sendline(c);
}

flushmo() {}




/*
 * substr(string, token) searches for token in string s
 * returns pointer to token within string if found, NULL otherwise
 */
char *
substr(s, t)
register char *s,*t;
{
	register char *ss,*tt;
	/* search for first char of token */
	for (ss=s; *s; s++)
		if (*s == *t)
			/* compare token with substring */
			for (ss=s,tt=t; ;) {
				if (*tt == 0)
					return s;
				if (*ss++ != *tt++)
					break;
			}
	return NULL;
}

/*
 * Log an error
 */
/*VARARGS1*/
log(s,p,u)
char *s, *p, *u;
{
	if (!Verbose)
		return;
	fprintf(stderr, "error %d: ", errors);
	fprintf(stderr, s, p, u);
}

/* send cancel string to get the other end to shut up */
canit()
{
	static char canistr[] = {
	 ZPAD,ZPAD,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0
	};

	printf(canistr);
	Lleft=0;	/* Do read next time ... */
	fflush(stdout);
}


/*
 * Return 1 iff stdout and stderr are different devices
 *  indicating this program operating with a modem on a
 *  different line
 */
fromcu()
{
	struct stat a, b;
	fstat(1, &a); fstat(2, &b);
	return (a.st_rdev != b.st_rdev);
}

report(sct)
int sct;
{
	if (Verbose>1)
		fprintf(stderr,"%03d%c",sct,sct%10? ' ' : '\r');
}

/*
 * If called as [-][dir/../]vrzCOMMAND set Verbose to 1
 * If called as [-][dir/../]rzCOMMAND set the pipe flag
 * If called as rb use YMODEM protocol
 */
chkinvok(s)
char *s;
{
	register char *p;

	p = s;
	while (*p == '-')
		s = ++p;
	while (*p)
		if (*p++ == '/')
			s = p;
	if (*s == 'v') {
		Verbose=1; ++s;
	}
	Progname = s;
	if (s[0]=='r' && s[1]=='b')
		Nozmodem = TRUE;
	if (s[2] && s[0]=='r' && s[1]=='b')
		Topipe=TRUE;
	if (s[2] && s[0]=='r' && s[1]=='z')
		Topipe=TRUE;
}

/*
 * Totalitarian Communist pathname processing
 */
checkpath(name)
char *name;
{
	if (Restricted) {
		if (fopen(name, "r") != NULL) {
			canit();
			fprintf(stderr, "\r\nrz: %s exists\n", name);
			bibi();
		}
		/* restrict pathnames to current tree or uucppublic */
		if ( substr(name, "../")
		 || (name[0]== '/' && strncmp(name, PUBDIR, strlen(PUBDIR))) ) {
			canit();
			fprintf(stderr,"\r\nrz:\tSecurity Violation\r\n");
			bibi();
		}
	}
}

/*
 * Initialize for Zmodem receive attempt, try to activate Zmodem sender
 *  Handles ZSINIT frame
 *  Return ZFILE if Zmodem filename received, -1 on error,
 *   ZCOMPL if transaction finished,  else 0
 */
tryz()
{
	register c, n;
	register cmdzack1flg;

	if (Nozmodem)		/* Check for "rb" program name */
		return 0;
	Rxtimeout = 100;


	for (n=5; --n>=0; ) {
		/* Set buffer length (0) and capability flags */
		stohdr(0L);
		Txhdr[ZF0] = CANFDX|CANOVIO|CANBRK;
		zshhdr(Badclose?ZFERR:ZRINIT, Txhdr);
again:
		switch (zgethdr(Rxhdr, 0)) {
		case ZRQINIT:
			continue;
		case ZEOF:
			continue;
		case TIMEOUT:
			continue;
		case ZFILE:
			zconv = Rxhdr[ZF0];
			zmanag = Rxhdr[ZF1];
			ztrans = Rxhdr[ZF2];
			Badclose = FALSE;
			if (zrbdata(secbuf, KSIZE) == GOTCRCW)
				return ZFILE;
			zshhdr(ZNAK, Txhdr);
		case ZSINIT:
			if (zrbdata(Attn, ZATTNLEN) == GOTCRCW) {
				zshhdr(ZACK, Txhdr);
				goto again;
			}
			zshhdr(ZNAK, Txhdr);
			continue;
		case ZFREECNT:
			stohdr(getfree());
			zshhdr(ZACK, Txhdr);
			goto again;
		case ZCOMMAND:
			cmdzack1flg = Rxhdr[ZF0];
			if (zrbdata(secbuf, KSIZE) == GOTCRCW) {
				if (cmdzack1flg & ZCACK1)
					stohdr(0L);
				else
					stohdr((long)sys2(secbuf));
				purgeline();	/* dump impatient questions */
				do {
					zshhdr(ZCOMPL, Txhdr);
				}
				while (++errors<10 && zgethdr(Rxhdr,1) != ZFIN);
				ackbibi();
				if (cmdzack1flg & ZCACK1)
					exec2(secbuf);
				return ZCOMPL;
			}
			zshhdr(ZNAK, Txhdr); goto again;
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
rzfiles()
{
	register c;

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
rzfile()
{
	register c, n;
	long rxbytes;

	Eofseen=FALSE;
	if (procheader(secbuf) == ERROR) {
		zshhdr(ZSKIP, Txhdr);
		return ZSKIP;
	}

	n = 10; rxbytes = 0l;

	for (;;) {
		stohdr(rxbytes);
		zshhdr(ZRPOS, Txhdr);
nxthdr:
		switch (c = zgethdr(Rxhdr, 0)) {
		default:
			vfile("rzfile: zgethdr returned %d", c);
			return ERROR;
		case ZNAK:
		case TIMEOUT:
			if ( --n < 0) {
				vfile("rzfile: zgethdr returned %d", c);
				return ERROR;
			}
		case ZFILE:
			continue;
		case ZEOF:
			if (rclhdr(Rxhdr) != rxbytes) {
				continue;
			}
			if (closeit()) {
				Badclose = TRUE;
				vfile("rzfile: closeit returned <> 0");
				return ERROR;
			}
			vfile("rzfile: normal EOF");
			return c;
		case ERROR:	/* Too much garbage in header search error */
			if ( --n < 0) {
				vfile("rzfile: zgethdr returned %d", c);
				return ERROR;
			}
			zmputs(Attn);
			continue;
		case ZDATA:
			n = 10;
			if (rclhdr(Rxhdr) != rxbytes) {
				zmputs(Attn);
				continue;
			}
moredata:
			switch (c = zrbdata(secbuf, KSIZE)) {
			case ZCAN:
				vfile("rzfile: zgethdr returned %d", c);
				return ERROR;
			case ERROR:	/* CRC error */
				if ( --n < 0) {
					vfile("rzfile: zgethdr returned %d", c);
					return ERROR;
				}
				zmputs(Attn);
				continue;
			case TIMEOUT:
				if ( --n < 0) {
					vfile("rzfile: zgethdr returned %d", c);
					return ERROR;
				}
				continue;
			case GOTCRCW:
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				stohdr(rxbytes);
				zshhdr(ZACK, Txhdr);
				goto nxthdr;
			case GOTCRCQ:
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				stohdr(rxbytes);
				zshhdr(ZACK, Txhdr);
				goto moredata;
			case GOTCRCG:
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				goto moredata;
			case GOTCRCE:
				putsec(secbuf, Rxcount);
				rxbytes += Rxcount;
				goto nxthdr;
			}
		}
	}
}

/*
 * Send a string to the modem, processing for \336 (sleep 1 sec)
 *   and \335 (break signal)
 */
zmputs(s)
char *s;
{
	register c;

	while (*s) {
		switch (c = *s++) {
		case '\336':
			sleep(1); continue;
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
closeit()
{
	if (Topipe) {
		if (pclose(fout)) {
			return ERROR;
		}
		return OK;
	}
	if (fclose(fout)==ERROR) {
		fprintf(stderr, "file close ERROR\n");
		return ERROR;
	}
	if (Modtime) {
		timep[0] = time(NULL);
		timep[1] = Modtime;
		utime(Pathname, timep);
	}
	if (Filemode)
		chmod(Pathname, (07777 & Filemode));
	return OK;
}

/*
 * Ack a ZFIN packet, let byegones be byegones
 */
ackbibi()
{
	register n;

	vfile("ackbibi:");
	Readnum = 1;
	stohdr(0L);
	for (n=4; --n>=0; ) {
		zshhdr(ZFIN, Txhdr);
		for (;;) {
			switch (readline(100)) {
			case 'O':
				readline(1);	/* Discard 2nd 'O' */
				/* ***** FALL THRU TO ***** */
			case TIMEOUT:
				vfile("ackbibi complete");
				return;
			default:
				break;
			}
		}
	}
}

/*
 * Local console output simulation
 */
bttyout(c)
{
	if (Verbose || fromcu)
		putc(c, stderr);
}

/*
 * Strip leading ! if present, do shell escape. 
 */
sys2(s)
register char *s;
{
	if (*s == '!')
		++s;
	return system(s);
}
/*
 * Strip leading ! if present, do exec.
 */
exec2(s)
register char *s;
{
	if (*s == '!')
		++s;
	mode(0);
	execl("/bin/sh", "sh", "-c", s);
}
