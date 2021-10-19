/************************* START OF MODULE 8 ********************************/

/* rz.c By Chuck Forsberg modified for cp/m by Hal Maney */

#define  C80

#include "hmodem80.h"
#include "zmodem.h"

int Tryzhdrtype;	   /* Header type to send corresponding to Last rx close */

wcreceive(filename)
char *filename;
{
   char *grabmem();
	static int c;

   rlabel();
   QuitFlag = FALSE;
   Zctlesc = 0;
   Baudrate = Baudtable[Current.cbaudindex];
   Tryzhdrtype = ZRINIT;
   Secbuf = alloc(KSIZE + 1);
   if (allocerror(Secbuf))
      return NERROR;
   Cpmbuf = grabmem(&Cpbufsize);
   if (allocerror(Cpmbuf))
      return NERROR;
   Cpindex = 0;                        /* just in case */
	Rxtimeout = 100;           /* 10 seconds */
   Errors = 0;
#ifdef   DEBUG
   printf("\nbuffer size = %u\n",Cpbufsize);
#endif
   savecurs();
   hidecurs();
   box();
	if (filename==0) {                  /* batch transfer */
		Crcflag=(Wcsmask==0377);
		if (c=tryz()) {                  /* zmodem transfer */
         report(PROTOCOL,"ZMODEM Receive");
			if (c == ZCOMPL)
				goto good;
			if (c == NERROR)
				goto fubar;
			c = rzmfile();
			if (c)
				goto fubar;
		} 
		else {                            /* ymodem batch transfer */
         report(PROTOCOL,"YMODEM Receive");
         report(BLKCHECK,Crcflag?"CRC":"Checksum");
			for (;;) {
            if (opabort())
               goto fubar;
				if (wcrxpn(Secbuf)== NERROR)
					goto fubar;
				if (Secbuf[0]==0)
					goto good;
				if (procheader(Secbuf) == NERROR)
					goto fubar;
				if (wcrx()==NERROR)
					goto fubar;
			}
		}
	} 
	else {
      report(PROTOCOL,"XMODEM Receive");
      strcpy(Pathname,filename);
		checkpath(Pathname);
		Fd=fopen(Pathname, "wb");
      if (openerror(Fd,Pathname))
         goto fubar1;
		if (wcrx()==NERROR)                   /* xmodem */
			goto fubar;
	}
good:
   free(Cpmbuf);
   free(Secbuf);
   showcurs();
   restcurs();
   return OK;

fubar:
	canit();
	if (Fd)
		fclose(Fd);
fubar1:
   free(Cpmbuf);
   free(Secbuf);
   showcurs();
   restcurs();
	return NERROR;
}

/*
 * Fetch a pathname from the other end as a C ctyle ASCIZ string.
 * Length is indeterminate as long as less than Blklen
 * A null string represents no more files (YMODEM)
 */

wcrxpn(rpn)
char *rpn;	/* receive a pathname */
{
	static int c;

	purgeline();

et_tu:
	Firstsec=TRUE;  
	Eofseen=FALSE;
	mcharout(Crcflag?WANTCRC:NAK);
	while ((c = wcgetsec(rpn, 100)) != 0) {
      if (QuitFlag)
         return NERROR;
		if (c == WCEOT) {
			mcharout(ACK);
			readline(INTRATIME);
			goto et_tu;
		}
		return NERROR;
	}
	mcharout(ACK);
	return OK;
}

/*
 * Adapted from CMODEM13.C, written by
 * Jack M. Wierda and Roderick W. Hart
 */

wcrx()
{
	static int sectnum, sectcurr;
	static char sendchar;
	static char *p;
	static int cblklen;			/* bytes to dump this block */

	Firstsec=TRUE;
	sectnum=0; 
	Eofseen=FALSE;
	sendchar=Crcflag?WANTCRC:NAK;
   report(BLKCHECK,Crcflag?"CRC":"Checksum");

	for (;;) {
      if (opabort())
         return NERROR;
		mcharout(sendchar);	            /* send it now, we're ready! */
		sectcurr = wcgetsec(Secbuf,Firstsec||(sectnum&0177)?50:130);
		if (sectcurr==(sectnum+1 &Wcsmask)) {
   		sreport(++sectnum);
			cblklen = Blklen;
			if (putsec(Secbuf, cblklen)==NERROR)
				return NERROR;
			sendchar=ACK;
		}
		else if (sectcurr==(sectnum&Wcsmask)) {
			zperr("Duplicate Sector",TRUE);
			sendchar=ACK;
		}
		else if (sectcurr==WCEOT) {
         if (closeit())
				return NERROR;
			mcharout(ACK);
			return OK;
		}
		else if (sectcurr==NERROR)
			return NERROR;
		else {
			zperr( "Sync Error",TRUE);
			return NERROR;
		}
	}
}

/*
 * Wcgetsec fetches a Ward Christensen type sector.
 * Returns sector number encountered or NERROR if valid sector not received,
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
	static int checksum, wcj, firstch;
	static unsigned oldcrc;
	static char *p;
	static int sectcurr;
   static int tries;

	for (Lastrx=Errors=0; Errors < RETRYMAX; ) {  /* errors incr by zperr */
      if (opabort())
         return NERROR;
		if ((firstch=readline(maxtime))==STX) {
			Blklen=KSIZE; 
			goto get2;
		}
		if (firstch==SOH) {
			Blklen=SECSIZ;
get2:
			sectcurr=readline(INTRATIME);
			if ((sectcurr+(oldcrc=readline(INTRATIME)))==Wcsmask) {
				oldcrc=checksum=0;
				for (p=rxbuf,wcj=Blklen; --wcj>=0; ) {
					if ((firstch=readline(INTRATIME)) < 0)
						goto bilge;
					oldcrc=updcrc(firstch, oldcrc);
					checksum += (*p++ = firstch);
				}
				if ((firstch=readline(INTRATIME)) < 0)
					goto bilge;
				if (Crcflag) {
					oldcrc=updcrc(firstch, oldcrc);
					if ((firstch=readline(INTRATIME)) < 0)
						goto bilge;
					oldcrc=updcrc(firstch, oldcrc);
					if (oldcrc & 0xFFFF)
						zperr( "CRC Error",TRUE);
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
					zperr("Checksum error",TRUE);
			}
			else
				zperr("Block nr garbled",TRUE);
		}
		/* make sure eot really is eot and not just mixmash */
		else if (firstch==EOT && readline(10)==TIMEOUT)
			return WCEOT;
		else if (firstch==CAN) {
			if (Lastrx==CAN) {
				zperr( "Sender CANcelled",TRUE);
				return NERROR;
			} 
			else {
				Lastrx=CAN;
				continue;
			}
		}
		else if (firstch==TIMEOUT) {
			if (Firstsec) {
   			zperr( "TIMEOUT",TRUE);
				goto humbug;
         }
bilge:
			zperr( "TIMEOUT",TRUE);
		}
		else
			zperr( "Bad header",TRUE);

humbug:
		Lastrx=0;
		while(readline(50) != TIMEOUT)
         if (QuitFlag)
            return NERROR;
		if (Firstsec) {
         if (Xmodem && (Errors == RETRYMAX/2))
            Crcflag = !Crcflag;           
         report(BLKCHECK,Crcflag?"CRC":"Checksum");
			mcharout(Crcflag?WANTCRC:NAK);
		} 
		else {
			maxtime=40; 
			mcharout(NAK);
		}
	}
	/* try to stop the bubble machine. */
	canit();
	return NERROR;
}

/*
 * Process incoming file information header
 */
procheader(name)
char *name;
{
   long atol();
	static char *p, *ap;
	
	/*
	 *  Process YMODEM,ZMODEM remote file management requests
	 */

   clrreports();
	p = name + 1 + strlen(name);
	if (*p) {	/* file coming from Unix or DOS system */
      ap = p;
      while (*p != ' ')     /* find first space */
         ++p;
      *p = '\0';            /* ap now points to a long integer in ascii */
      report(FILESIZE,ap);
      report(SENDTIME,ttime(atol(ap)));
   }
	strcpy(Pathname, name);
	checkpath(Pathname);
   Fd=fopen(Pathname, "wb");
   if (openerror(Fd,Pathname))
      return NERROR;
	return OK;
}

/*
 * Putsec writes the n characters of buf to receive file Fd.
 */
putsec(buf, n)
char *buf;
int n;
{
   static int status;

   status = 0xff;
   while (n--) {
      Cpmbuf[Cpindex++] = *buf++;
      if (Cpindex >= Cpbufsize) {
         status = write(Fd,Cpmbuf,Cpbufsize);
         if (!status)
            zperr("Disk write error",TRUE);
         Cpindex = 0;
      }
   }
   return status ? 0 : NERROR;
}

/*
 * substr(string, token) searches for token in string s
 * returns pointer to token within string if found, NULL otherwise
 */
char *
substr(s, t)
char *s,*t;
{
   static int i;

   if ((i=index(s,t)) != -1)
      return s+i;
   else
      return NULL;
}

/* send cancel string to get the other end to shut up */
canit()
{
	static char canistr[] = {
		24,24,24,24,24,24,24,24,24,24,8,8,8,8,8,8,8,8,8,8,0
	};

	mstrout(canistr,FALSE);
   purgeline();
}

clrreports()
{
   static int i;

   for (i=4; i<13; i++)
      clrline(i);
}


zperr(string,incrflag)
char *string;
int incrflag;
{
   clrline(MESSAGE);
   report(MESSAGE,string);
   if (incrflag)
      dreport(ERRORS,++Errors);
}

report(row,string)
int row;
char *string;
{
   locate(row,61);
   printf(string);
}

dreport(row,value)
int row, value;
{
   static char buf[7];
   
   report(row,itoa(value,buf));
}

lreport(row,value)
int row;
long value;
{
   static char buf[20];

   report(row,ltoa(value,buf));
}

sreport(sct)
int sct;
{  
   static long bytes;

   bytes = (long)Blklen;
   bytes *= sct;
   dreport(BLOCKS,sct);
   lreport(KBYTES,bytes);
}

clrline(line)
int line;
{
   report(line,"                ");
}

checkpath(name)  /* eliminate bad paths */
char *name;
{
   static char *p;
   static int i;

   for (p=name; *p; p++) {             /* dump strange characters */
      if (!isalpha(*p) && !isdigit(*p) && (*p != '.')) {
         *p = '\0';
         strcat(name,p+1);
      }
   }
   if ((i=index(name,".")) > 8) {
      p = name + i;
      name[8] = '.';
      name[9] = '\0';
      p[3] = '\0';
      strcat(name,p);
   }
   name[12] = '\0';
   addatadrive(name);
   report(PATHNAME,name);
}

/*
 * Initialize for Zmodem receive attempt, try to activate Zmodem sender
 *  Handles ZSINIT frame
 *  Return ZFILE if Zmodem filename received, -1 on error,
 *   ZCOMPL if transaction finished,  else 0
 */
tryz()
{
	static int c, n, *ip;
	static int cmdzack1flg;

	if (Nozmodem)		/* ymodem has been forced */
		return 0;

	for (n=Zmodem?15:5; --n>=0; ) {
      if (opabort())
         return NERROR;
		/* Set buffer length (0) and capability flags */
		stohdr(0L);
		Txhdr[ZF0] = CANFC32 | CANFDX;
		if (Zctlesc)
			Txhdr[ZF0] |= TESCCTL;
      ip = (int *)&Txhdr[ZP0];
      *ip = Cpbufsize;
		zshhdr(Tryzhdrtype, Txhdr);
		if (Tryzhdrtype == ZSKIP)	/* Don't skip too far */
			Tryzhdrtype = ZRINIT;	/* CAF 8-21-87 */
again:
		switch (zgethdr(Rxhdr, 0)) {
		case ZRQINIT:
			continue;
		case ZEOF:
			continue;
		case TIMEOUT:
			continue;
		case ZFILE:
			Zconv = Rxhdr[ZF0];
			Zmanag = Rxhdr[ZF1];
			Ztrans = Rxhdr[ZF2];
			Tryzhdrtype = ZRINIT;
			c = zrdata(Secbuf, KSIZE);
			if (c == GOTCRCW)
				return ZFILE;
			zshhdr(ZNAK, Txhdr);
			goto again;
		case ZSINIT:
			Zctlesc = TESCCTL & Rxhdr[ZF0];
			if (zrdata(Attn, ZATTNLEN) == GOTCRCW) {
				zshhdr(ZACK, Txhdr);
				goto again;
			}
			zshhdr(ZNAK, Txhdr);
			goto again;
		case ZFREECNT:
			stohdr(0L);
			zshhdr(ZACK, Txhdr);
			goto again;
		case ZCOMMAND:
			cmdzack1flg = Rxhdr[ZF0];
			if (zrdata(Secbuf, KSIZE) == GOTCRCW) {
   			stohdr(0L);
				purgeline();	/* dump impatient questions */
				do {
					zshhdr(ZCOMPL, Txhdr);
               zperr("Waiting for ZFIN",FALSE);
               if (opabort())
                  return NERROR;
				}
				while (++Errors<20 && zgethdr(Rxhdr,1) != ZFIN);
				ackbibi();
				return ZCOMPL;
			}
			zshhdr(ZNAK, Txhdr); 
			goto again;
		case ZCOMPL:
			goto again;
		default:
			continue;
		case ZFIN:
			ackbibi(); 
			return ZCOMPL;
		case ZCAN:
			return NERROR;
		}
	}
	return 0;
}

/*
 * Receive 1 or more files with ZMODEM protocol
 */

rzmfile()
{
	static int c;

	for (;;) {
      if (opabort())
         return NERROR;
		switch (c = rzfile()) {
		case ZEOF:
		case ZSKIP:
			switch (tryz()) {
			case ZCOMPL:
				return OK;
			default:
				return NERROR;
			case ZFILE:
				break;
			}
			continue;
		default:
			return c;
		case NERROR:
			return NERROR;
		}
	}
}

/*
 * Receive a file with ZMODEM protocol
 *  Assumes file name frame is in Secbuf
 */
rzfile()
{
	static int c, n;
	static long rxbytes;

	Eofseen=FALSE;
	if (procheader(Secbuf) == NERROR) {
		return (Tryzhdrtype = ZSKIP);
	}

	n = 20; 
	rxbytes = 0L;

	for (;;) {
      if (opabort())
         return NERROR;
		stohdr(rxbytes);
		zshhdr(ZRPOS, Txhdr);
nxthdr:
		switch (c = zgethdr(Rxhdr, 0)) {
		default:
			return NERROR;
		case ZNAK:
		case TIMEOUT:
			if ( --n < 0) {
				return NERROR;
			}
		case ZFILE:
			zrdata(Secbuf, KSIZE);
			continue;
		case ZEOF:
			if (rclhdr(Rxhdr) != rxbytes) {
				/*
				 * Ignore eof if it's at wrong place - force
				 *  a timeout because the eof might have gone
				 *  out before we sent our zrpos.
				 */
				Errors = 0;  
				goto nxthdr;
			}
			if (closeit()) {
				Tryzhdrtype = ZFERR;
				return NERROR;
			}
         lreport(KBYTES,rxbytes);
         report(BLKCHECK,Crc32?"CRC-32":"CRC-16");
			return c;
		case NERROR:	/* Too much garbage in header search error */
			if ( --n < 0) {
				return NERROR;
			}
			zmputs(Attn);
			continue;
		case ZDATA:
			if (rclhdr(Rxhdr) != rxbytes) {
				if ( --n < 0) {
					return NERROR;
				}
				zmputs(Attn);  
				continue;
			}
moredata:
         lreport(KBYTES,rxbytes);
         report(BLKCHECK,Crc32?"CRC-32":"CRC-16");
			switch (c = zrdata(Secbuf, KSIZE)) {
			case ZCAN:
				return NERROR;
			case NERROR:	/* CRC error */
				if ( --n < 0) {
					return NERROR;
				}
				zmputs(Attn);
				continue;
			case TIMEOUT:
				if ( --n < 0) {
					return NERROR;
				}
				continue;
			case GOTCRCW:
				n = 20;
				putsec(Secbuf, Rxcount);
				rxbytes += Rxcount;
				stohdr(rxbytes);
#ifdef   DEBUG
            lreport(BLOCKS,rxbytes);     /* put it any old place */
#endif;
				zshhdr(ZACK, Txhdr);
				mcharout(XON);
				goto nxthdr;
			case GOTCRCQ:
				n = 20;
				putsec(Secbuf, Rxcount);
				rxbytes += Rxcount;
				stohdr(rxbytes);
				zshhdr(ZACK, Txhdr);
				goto moredata;
			case GOTCRCG:
				n = 20;
				putsec(Secbuf, Rxcount);
				rxbytes += Rxcount;
				goto moredata;
			case GOTCRCE:
				n = 20;
				putsec(Secbuf, Rxcount);
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
	static int c;

	while (*s) {
      if (opabort())
         return NERROR;
		switch (c = *s++) {
		case '\336':
			wait(1); 
			continue;
		case '\335':
			sendbrk(); 
			continue;
		default:
			mcharout(c);
		}
	}
}

sendbrk(){}  /* do later */

/*
 * Close the receive dataset, return OK or NERROR
 */
closeit()
{  
   static int status;

   status = OK;
   if (Cpindex) {
      status = write(Fd,Cpmbuf,128*roundup(Cpindex,128)) ? OK : NERROR;
      Cpindex = 0;
   }
   if (status == NERROR)
      zperr("Disk write error",TRUE);
	if (fclose(Fd)==NERROR) {
      Fd = 0;
		zperr("File close error",TRUE);
		return NERROR;
	}
	return status;
}

/*
 * Ack a ZFIN packet, let byegones be byegones
 */

ackbibi()
{
	static int n;

	stohdr(0L);
	for (n=3; --n>=0; ) {
		purgeline();
		zshhdr(ZFIN, Txhdr);
		switch (readline(100)) {
		case 'O':
			readline(INTRATIME);	/* Discard 2nd 'O' */
			return;
		case RCDO:
			return;
		case TIMEOUT:
		default:
			break;
		}
	}
}

opabort()
{
   Lastkey = getch() & 0xff;
   if (Lastkey == ESC) {
      flush();
      if (!Inhost && !Dialing)
         report(MESSAGE,"Operator abort");
      QuitFlag = TRUE;
   }
   return QuitFlag;
}
   
long
atol(string)
char *string;
{
   static long value, lv;
   static char *p;
   
   value = 0L;
   p = string + strlen(string);     /* end of string */
   while (!isdigit(*p))
      p--;
   for (lv = 1L; isdigit(*p) && p >= string; lv *= 10)
      value += ((*p--) - '0') * lv;
   return value;
}

/************************** END OF MODULE 8 *********************************/
