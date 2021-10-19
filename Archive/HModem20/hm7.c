/************************** START OF MODULE 7 *******************************/

/* sz.c By Chuck Forsberg modified for cp/m by Hal Maney */

#define  C80

#include "hmodem80.h"
#include "zmodem.h"

/*
 * Attention string to be executed by receiver to interrupt streaming data
 *  when an error is detected.  A pause (0336) may be needed before the
 *  ^C (03) or after it.
 */

char *ltoa();
char Myattn[] = { 
	0 };

unsigned Txwindow = 0;	/* Control the size of the transmitted window */
unsigned Txwspac;	      /* Spacing between zcrcq requests */
unsigned Txwcnt;	      /* Counter used to space ack requests */
int Noeofseen;
int Totsecs;		      /* total number of sectors this file */
char *Txbuf;
int Filcnt; 		      /* count of number of files opened */
unsigned Rxbuflen = 16384;	/* Receiver's max buffer length */
int Rxflags = 0;
long Bytcnt;
int Wantfcs32 = TRUE;	/* want to send 32 bit FCS */
long Lastread;		      /* Beginning offset of last buffer read */
int Lastn;		         /* Count of last buffer read or -1 */
int Dontread;		      /* Don't read the buffer, it's still there */
long Lastsync;		      /* Last offset to which we got a ZRPOS */
int Beenhereb4;		   /* How many times we've been ZRPOS'd same place */
int Incnt;              /* count for chars not read from the Cpmbuf */

wcsend(argc, argp)
int argc;                     /* nr of files to send */
char *argp[];                 /* list of file names */
{
	static int n;

   slabel();
   QuitFlag = FALSE;
   Zctlesc = 0;
   Incnt = 0;
   Baudrate = Baudtable[Current.cbaudindex];
   Filcnt = Fd = Errors = 0;
   Txbuf = alloc(KSIZE);
   if (allocerror(Txbuf))
      return NERROR;      
   Cpmbuf = grabmem(&Cpbufsize);
   if (allocerror(Cpmbuf))
      return NERROR;
   Cpindex = 0;                        /* just in case */
	Crcflag  = FALSE;
	Firstsec = TRUE;
	Bytcnt = -1;
   Rxtimeout = 600;   
   savecurs();
   hidecurs();
   box();
   report(PROTOCOL,Xmodem?"XMODEM Send":Zmodem?"ZMODEM Send":"YMODEM Send");
	if (Zmodem) {
		stohdr(0L);
		zshhdr(ZRQINIT, Txhdr);
      if (getzrxinit()==NERROR)
         goto badreturn;
	}
	for (n=0; n<argc; ++n) {
      clrreports();
		Totsecs = 0;
		if (opabort() || wcs(argp[n])==NERROR)
         goto badreturn;
      tfclose();
	}
	Totsecs = 0;
	if (Filcnt==0) {	/* we couldn't open ANY files */
		canit();
      goto badreturn;
	}
   zperr("Complete",FALSE);
	if (Zmodem)
		saybibi();
	else if (!Xmodem)
		wctxpn("");
   free(Cpmbuf);
	free(Txbuf);
   showcurs();
	restcurs();
   return OK;

badreturn:
   free(Cpmbuf);
  	free(Txbuf);
   showcurs();
	restcurs();
   tfclose();
   return NERROR;
}

wcs(oname)
char *oname;
{
	static int c;
	static char *p;
   static unsigned length;
   static long flen;

	if ((Fd=fopen(oname,"rb"))==NULL) {
      zperr("Can't open file",TRUE);
      wait(2);
		return OK;	/* pass over it, there may be others */
	}
	++Noeofseen;  
	Lastread = 0L;  
	Lastn = -1; 
	Dontread = FALSE;
	++Filcnt;
   fstat(oname,&Fs);
	switch (wctxpn(oname)) {    /* transmit path name */
	case NERROR:
		return NERROR;
	case ZSKIP:
		return OK;
	}
   length = Fs.records;
   flen = (long)length * 128;
	if (!Zmodem && wctx(flen)==NERROR)
		return NERROR;
	return 0;
}

/*
 * generate and transmit pathname block consisting of
 *  pathname (null terminated),
 *  file length, mode time (null) and file mode (null)
 *  in octal.
 *  N.B.: modifies the passed name, may extend it!
 */
wctxpn(name)
char *name;
{
	static char *p, *q;
	char buf[20];
   static unsigned length;
   static long nrbytes;

   memset(Txbuf,'\0',KSIZE);
   length = Fs.records;
   nrbytes = (long)length * 128;
   report(PATHNAME,name);
   lreport(FILESIZE,nrbytes);
   dreport(FBLOCKS,length);
   report(SENDTIME,ttime(nrbytes));   
	if (Xmodem)                 /* xmodem, don't send path name */
		return OK;
	if (!Zmodem) {
      Blklen = KSIZE;
		if (getnak())
			return NERROR;
   }
   strcpy(Txbuf,name);
   p = Txbuf + strlen(Txbuf);
   ++p;
   strcpy(p,ltoa(nrbytes,buf));
	if (Zmodem)
		return zsendfile(Txbuf, 1+strlen(p)+(p-Txbuf));
	if (wcputsec(Txbuf, 0, SECSIZ)==NERROR)
		return NERROR;
	return OK;
}

/* ltoa - convert n to characters in s. */
char *ltoa(n, s)
char s[];
long n;
{
	static long c, k;
	static char *p, *q;

	if ((k = n) < 0)	/* record sign */
		n = -n; 	/* make n positive */
	q = p = s;
	do {		/* generate digits in reverse order */
		*p++ = n % 10 + '0';  /* get next digit */
	} while ((n /= 10) > 0);	/* delete it */
	if (k < 0) *p++ = '-';
	*p = 0;
/* reverse string in place */
	while (q < --p) {
		c = *q; *q++ = *p; *p = c; }
	return (s);
}

getnak()
{
	static int firstch;

	Lastrx = 0;
	for (;;) {
      if (opabort())
         return NERROR;
		switch (firstch = readock(800,1)) {
		case ZPAD:
			if (getzrxinit())
				return NERROR;
			return FALSE;
		case TIMEOUT:
			zperr("Timeout on PName",TRUE);
			return TRUE;
		case WANTCRC:
			Crcflag = TRUE;
		case NAK:
			return FALSE;
		case CAN:
			if ((firstch = readock(20,1)) == CAN && Lastrx == CAN)
				return TRUE;
		default:
			break;
		}
		Lastrx = firstch;
	}
   report(BLKCHECK,Crcflag?"CRC":"Checksum");
}

wctx(flen)
long flen;
{
	static int thisblklen, i;
	static unsigned sectnum, attempts, firstch;
	static long charssent;

	charssent = 0L;  
	Firstsec = TRUE;  
	thisblklen = Blklen;
   i = 0;

	while ((firstch=readock(1,2)) != NAK 
        && firstch != WANTCRC
        && firstch != CAN
        && !opabort()
        && ++i < Rxtimeout)
        ;
   if (QuitFlag)
      return NERROR;
	if (firstch==CAN) {
		zperr("Rcvr CANcelled",TRUE);
		return NERROR;
	}
	if (firstch==WANTCRC)
		Crcflag=TRUE;
   report(BLKCHECK,Crcflag?"CRC":"Checksum");
	sectnum=0;
	for (;;) {
      if (opabort())
         return NERROR;
		if (flen <= (charssent + 896L))
			Blklen = thisblklen = 128;
		if ( !filbuf(Txbuf, thisblklen))
			break;
		if (wcputsec(Txbuf, ++sectnum, thisblklen)==NERROR)
			return NERROR;
		charssent += thisblklen;
      sreport(sectnum);
	}
	attempts=0;
	do {
      dreport(ERRORS,attempts);
		purgeline();
		mcharout(EOT);
		++attempts;
	}
	while ((firstch=(readock(Rxtimeout, 1)) != ACK) 
        && attempts < RETRYMAX
        && !opabort() );
	if (attempts == RETRYMAX) {
		zperr("No ACK on EOT",TRUE);
		return NERROR;
	}
   else if (QuitFlag)  /* from opabort */
      return NERROR;
	else
		return OK;
}

wcputsec(buf, sectnum, cseclen)
char *buf;
int sectnum;
int cseclen;	/* data length of this sector to send */
{
	static unsigned checksum;
	static char *cp;
	static unsigned oldcrc;
   static int wcj;
	static int firstch;
	static int attempts;

	firstch=0;	/* part of logic to detect CAN CAN */

   dreport(ERRORS,0);
	for (attempts=0; attempts <= RETRYMAX; attempts++) {
      if (opabort())
         return NERROR;
      if (attempts)
         dreport(ERRORS,attempts);
		Lastrx= firstch;
		mcharout(cseclen==KSIZE?STX:SOH);
		mcharout(sectnum);
		mcharout(~sectnum);
		oldcrc=checksum=0;
		for (wcj=cseclen,cp=buf; --wcj>=0; ) {
			mcharout(*cp);
			oldcrc=updcrc((0377& *cp), oldcrc);
			checksum += *cp++;
		}
		if (Crcflag) {
			oldcrc=updcrc(0,updcrc(0,oldcrc));
			mcharout((int)oldcrc>>8);
			mcharout((int)oldcrc);
		}
		else
			mcharout(checksum);

		firstch = readock(Rxtimeout, (Noeofseen&&sectnum) ? 2:1);
gotnak:
		switch (firstch) {
		case CAN:
			if(Lastrx == CAN) {
cancan:
				zperr("Rcvr CANcelled",TRUE);  
				return NERROR;
			}
			break;
		case TIMEOUT:
			zperr("Timeout on ACK",TRUE); 
			continue;
		case WANTCRC:
			if (Firstsec)
				Crcflag = TRUE;
         report(BLKCHECK,Crcflag?"CRC":"Checksum");
		case NAK:
			zperr("NAK on sector",TRUE); 
			continue;
		case ACK: 
			Firstsec=FALSE;
			Totsecs += (cseclen>>7);
			return OK;
		case NERROR:
			zperr("Got burst",TRUE); 
			break;
		default:
			zperr("Bad sector ACK",TRUE);
			break;
		}
		for (;;) {
         if (opabort())
            return NERROR;
			Lastrx = firstch;
			if ((firstch = readock(Rxtimeout, 2)) == TIMEOUT)
				break;
			if (firstch == NAK || firstch == WANTCRC)
				goto gotnak;
			if (firstch == CAN && Lastrx == CAN)
				goto cancan;
		}
	}
	zperr("Retry Exceeded",TRUE);
	return NERROR;
}

/* fill buf with count chars padding with ^Z for CPM */

filbuf(buf, count)
char *buf;
int count;
{
	static int c, m;

	c = m = newload(buf, count);
	if (m <= 0)
		return 0;
	while (m < count)
		buf[m++] = CTRLZ;
	return c;
}

newload(buf, count)
int count;
char *buf;
{
   static int j;

   j = 0;
   while (count--) {
      if (Incnt <= 0) {
         Incnt = read( Fd, Cpmbuf, Cpbufsize);
         Cpindex = 0;      
         if (Incnt <= 0)
            break;
      }
      buf[j++] = Cpmbuf[Cpindex++];
      --Incnt;
   }
   return (j ? j : -1);
}

/*
 * readock(timeout, count) reads character(s) from modem
 *  (1 <= count <= 3)
 * it attempts to read count characters. If it gets more than one,
 * it is an error unless all are CAN
 * (otherwise, only normal response is ACK, CAN, or C)
 *
 * timeout is in tenths of seconds
 */

readock(timeout, count)
int timeout, count;
{
	static int c;
	static char byt[5];

	c = mread(byt,count,timeout);
	if (c < 1)
		return TIMEOUT;
	if (c == 1)
		return (byt[0] & 0xff);
	else
		while (c)
   		if (byt[--c] != CAN)
	   		return NERROR;
	return CAN;
}

readline(n)
int n;
{
	return (readock(n,1));
}

/*
 * Get the receiver's init parameters
 */

getzrxinit()
{
	static int n;
	static struct stat f;

	for (n=10; --n>=0; ) {
      if (opabort())
         return NERROR;
		switch (zgethdr(Rxhdr, 1)) {
		case ZCHALLENGE:	/* Echo receiver's challenge numbr */
			stohdr(Rxpos);
			zshhdr(ZACK, Txhdr);
			continue;
		case ZCOMMAND:		/* They didn't see out ZRQINIT */
			stohdr(0L);
			zshhdr(ZRQINIT, Txhdr);
			continue;
		case ZRINIT:
			Rxflags = 0377 & Rxhdr[ZF0];
			Txfcs32 = (Wantfcs32 && (Rxflags & CANFC32));
			Zctlesc |= Rxflags & TESCCTL;
			Rxbuflen = (0377 & Rxhdr[ZP0])+((0377 & Rxhdr[ZP1])<<8);
			return (sendzsinit());
		case ZCAN:
		case TIMEOUT:
			return NERROR;
		case ZRQINIT:
			if (Rxhdr[ZF0] == ZCOMMAND)
				continue;
		default:
			zshhdr(ZNAK, Txhdr);
			continue;
		}
	}
	return NERROR;
}

/* Send send-init information */

sendzsinit()
{
	return OK;
}

/* Send file name and related info */

zsendfile(buf, blen)
char *buf;
int blen;
{
	static int c;

	for (;;) {
      if (opabort())
         return NERROR;
		Txhdr[ZF0] = 0;	/* file conversion request */
		Txhdr[ZF1] = 0;	/* file management request */
		Txhdr[ZF2] = 0;	/* file transport request */
		Txhdr[ZF3] = 0;
		zsbhdr(ZFILE, Txhdr);
		zsdata(buf, blen, ZCRCW);
again:
		c = zgethdr(Rxhdr, 1);
		switch (c) {
		case ZRINIT:
			while ((c = readline(INTRATIME)) > 0)
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
			return NERROR;
		case ZSKIP:
			return c;
		case ZRPOS:
			/*
			 * Suppress zcrcw request otherwise triggered by
			 * lastyunc==Bytcnt
			 */
			Lastsync = (Bytcnt = Txpos = Rxpos) -1L;
			seek(Fd, (int)(Rxpos/256), 3);            /* offset records */
         seek(Fd, (int)(Rxpos%256), 1);            /* plus bytes */
         clrline(KBYTES);
		   Incnt = 0;
         Dontread = FALSE;
			return zsndfdata();
		}
	}
}

/* Send the data in the file */

zsndfdata()
{
	static int c, e, n;
	static int newcnt;
	static long tcount;
	static int junkcount;      /* Counts garbage chars received by TX */

   tcount = 0L;
   Blklen = 128;
   if (Baudrate > 300)
		Blklen = 256;
	if (Baudrate > 1200)
		Blklen = 512;
	if (Baudrate > 2400)
		Blklen = KSIZE;
	if (Rxbuflen && Blklen>Rxbuflen)
		Blklen = Rxbuflen;
	Lrxpos = 0L;
	junkcount = 0;
	Beenhereb4 = FALSE;
somemore:
	if (NULL) {
waitack:
		junkcount = 0;
		c = getinsync(0);
      if (QuitFlag)
         return NERROR;
gotack:
		switch (c) {
		default:
		case ZCAN:
			return NERROR;
		case ZSKIP:
			return c;
		case ZACK:
		case ZRPOS:
			break;
		case ZRINIT:
			return OK;
		}
		/*
		 * If the reverse channel can be tested for data,
		 *  this logic may be used to detect error packets
		 *  sent by the receiver, in place of setjmp/longjmp
		 *  minprdy() returns non 0 if a character is available
		 */
		while (minprdy()) {
         if (QuitFlag)
            return NERROR;
			switch (readline(1)) {
			case CAN:
			case ZPAD:
				c = getinsync(1);
				goto gotack;
			case XOFF:		/* Wait a while for an XON */
			case XOFF|0200:
				readline(100);
			}
		}
	}

	newcnt = Rxbuflen;
	Txwcnt = 0;
	stohdr(Txpos);
	zsbhdr(ZDATA, Txhdr);
	do {
      if (QuitFlag)
         return NERROR;
		if (Dontread) {
			n = Lastn;
		} 
		else {
			n = filbuf(Txbuf, Blklen);
			Lastread = Txpos;  
			Lastn = n;
		}
		Dontread = FALSE;
		if (n < Blklen)
			e = ZCRCE;
		else if (junkcount > 3)
			e = ZCRCW;
		else if (Bytcnt == Lastsync)
			e = ZCRCW;
		else if (Rxbuflen && (newcnt -= n) <= 0)
			e = ZCRCW;
		else if (Txwindow && (Txwcnt += n) >= Txwspac) {
			Txwcnt = 0;  
			e = ZCRCQ;
		}
		else
		   e = ZCRCG;
		zsdata(Txbuf, n, e);
      Txpos += (long)n;
		Bytcnt = Txpos;
      report(BLKCHECK,Crc32t?"CRC-32":"CRC-16");
      lreport(KBYTES,Bytcnt);
		if (e == ZCRCW)
			goto waitack;

       /*
		 * If the reverse channel can be tested for data,
		 *  this logic may be used to detect error packets
		 *  sent by the receiver, in place of setjmp/longjmp
		 *  minprdy() returns non 0 if a character is available
		 */

		while (minprdy()) {
         if (QuitFlag)
            return NERROR;
			switch (readline(1)) {
   		case CAN:
			case ZPAD:
				c = getinsync(1);
				if (c == ZACK)
					break;
            purgeline();
		/* zcrce - dinna wanna starta ping-pong game */
				zsdata(Txbuf, 0, ZCRCE);
				goto gotack;
			case XOFF:		/* Wait a while for an XON */
			case XOFF|0200:
				readline(100);
			default:
				++junkcount;
			}
		}
		if (Txwindow) {
			while ((tcount = Txpos - Lrxpos) >= Txwindow) {
            if (QuitFlag)
               return NERROR;
				if (e != ZCRCQ)
					zsdata(Txbuf, 0, e = ZCRCQ);
				c = getinsync(1);
				if (c != ZACK) {
               purgeline();
					zsdata(Txbuf, 0, ZCRCE);
					goto gotack;
				}
			}
		}
	} 
	while (n == Blklen);

	for (;;) {
      if (QuitFlag)
         return NERROR;
		stohdr(Txpos);
		zsbhdr(ZEOF, Txhdr);
		switch (getinsync(0)) {
		case ZACK:
			continue;
		case ZRPOS:
			goto somemore;
		case ZRINIT:
			return OK;
		case ZSKIP:
			return c;
		default:
			return NERROR;
		}
	}
}

/*
 * Respond to receiver's complaint, get back in sync with receiver
 */

getinsync(flag)      /* flag means that there was an error */
int flag;
{
	static int c;

	for (;;) {
      if (opabort())
         return NERROR;
		c = zgethdr(Rxhdr, 0);
      c = c < FRTYPES ? c : FRTYPES-1;
      sprintf(Buf,"Got %s", frametypes[c+FTOFFSET]);
		zperr(Buf,flag);
		switch (c) {
		case ZCAN:
		case ZABORT:
		case ZFIN:
		case TIMEOUT:
			return NERROR;
		case ZRPOS:
			/* ************************************* */
			/*  If sending to a modem beuufer, you   */
			/*   might send a break at this point to */
			/*   dump the modem's buffer.		 */

			if (Lastn >= 0 && Lastread == Rxpos) {
				Dontread = TRUE;
			} 
			else {
   			seek(Fd, (int)(Rxpos/256), 3);            /* offset records */
            seek(Fd, (int)(Rxpos%256), 1);            /* plus bytes */
            clrline(KBYTES);
            Incnt = 0;
			}
			Bytcnt = Lrxpos = Txpos = Rxpos;
			if (Lastsync == Rxpos) {
				if (++Beenhereb4 > 4)
					if (Blklen > 256)
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
			return c;
		case NERROR:
		default:
			zsbhdr(ZNAK, Txhdr);
			continue;
		}
	}
}

/* Say "bibi" to the receiver, try to do it cleanly */

saybibi()
{
	for (;;) {
		stohdr(0L);		/* CAF Was zsbhdr - minor change */
		zshhdr(ZFIN, Txhdr);	/*  to make debugging easier */
		switch (zgethdr(Rxhdr, 0)) {
		case ZFIN:
			mcharout('O'); 
			mcharout('O'); 
		case ZCAN:
		case TIMEOUT:
			return;
		}
	}
}

char *
ttime(fsize)
long fsize;
{
   static int efficiency, cps, seconds;
   static char buffer[10];

   efficiency = Zmodem ? 9 : 8;
   cps = (Baudrate/100) * efficiency;   
   seconds = (int)(fsize/cps);     
   sprintf(buffer,"%d:%02d",seconds/60,seconds%60);
   return buffer;
}

tfclose()          /* close file if still open */
{
   if (Fd)
      fclose(Fd);
   Fd = 0;
}

/************************** END OF MODULE 7 *********************************/

