/*
 *   Z M . C
 *    ZMODEM protocol primitives
 *    07-28-87  Chuck Forsberg Omen Technology Inc
 *
 * Entry point Functions:
 *	zsbhdr(type, hdr) send binary header
 *	zshhdr(type, hdr) send hex header
 *	zgethdr(hdr, eflag) receive header - binary or hex
 *	zsdata(buf, len, frameend) send data
 *	zrdata(buf, len) receive data
 *	stohdr(pos) store position data in Txhdr
 *	long rclhdr(hdr) recover position offset from header
 */

#define  ZM
#define  C80

#include "hmodem80.h"
#include "zmodem.h"

long updc32();


/* Send ZMODEM binary header hdr of type type */
zsbhdr(type, hdr)
char *hdr;
{
	static int n;
	static unsigned crc;

	if (type == ZDATA)
		for (n = Znulls; --n >=0; )
			zsendline(0);

	mcharout(ZPAD); 
	mcharout(ZDLE);

	if (Crc32t=Txfcs32)
		zsbh32(hdr, type);
	else {
		mcharout(ZBIN); 
		zsendline(type); 
		crc = updcrc(type, 0);

		for (n=4; --n >= 0; ++hdr) {
			zsendline(*hdr);
			crc = updcrc((0377& *hdr), crc);
		}
		crc = updcrc(0,updcrc(0,crc));
		zsendline(crc>>8);
		zsendline(crc);
	}
	if (type != ZDATA)
		purgeline();
}


/* Send ZMODEM binary header hdr of type type */

zsbh32(hdr, type)
char *hdr;
{
	static int n;
	static long crc;

	mcharout(ZBIN32);  
	zsendline(type);
	crc = 0xFFFFFFFFL; 
	crc = updc32(type, crc);

	for (n=4; --n >= 0; ++hdr) {
		crc = updc32((0377 & *hdr), crc);
		zsendline(*hdr);
	}
	crc = ~crc;
	for (n=4; --n >= 0;) {
		zsendline((int)crc);
		crc >>= 8;
	}
}

/* Send ZMODEM HEX header hdr of type type */

zshhdr(type, hdr)
char *hdr;
{
	static int n;
	static unsigned crc;

   mcharout(ZPAD); 
   mcharout(ZPAD); 
   mcharout(ZDLE); 
   mcharout(ZHEX);
	zputhex(type);
	Crc32t = 0;

	crc = updcrc(type, 0);
	for (n=4; --n >= 0; ++hdr) {
		zputhex(*hdr); 
		crc = updcrc((0377 & *hdr), crc);
	}
	crc = updcrc(0,updcrc(0,crc));
	zputhex(crc>>8); 
	zputhex(crc);

	/* Make it printable on remote machine */
   mcharout(CR); 
   mcharout(LF);
	/*
	 * Uncork the remote in case a fake XOFF has stopped data flow
	 */
	if (type != ZFIN && type != ZACK)
	   mcharout(CTRLQ);
	purgeline();
}

/*
 * Send binary array buf of length length, with ending ZDLE sequence frameend
 */
zsdata(buf, length, frameend)
int length, frameend;
char *buf;
{
	static unsigned crc;

	if (Crc32t)
		zsda32(buf, length, frameend);
	else {
		crc = 0;
		for (;--length >= 0; ++buf) {
			zsendline(*buf); 
			crc = updcrc((0377 & *buf), crc);
		}
		mcharout(ZDLE); 
		mcharout(frameend);
		crc = updcrc(frameend, crc);

		crc = updcrc(0,updcrc(0,crc));
		zsendline(crc>>8); 
		zsendline(crc);
	}
	if (frameend == ZCRCW) {
		mcharout(XON);  
		purgeline();
	}
}

zsda32(buf, length, frameend)
char *buf;
int length, frameend;
{
	static long crc;

	crc = 0xFFFFFFFFL;
	for (;--length >= 0;++buf) {
		crc = updc32((0377 & *buf), crc);
		zsendline(*buf);
	}
	mcharout(ZDLE); 
	mcharout(frameend);
	crc = updc32(frameend, crc);

	crc = ~crc;
	for (length=4; --length >= 0;) {
		zsendline((int)crc);  
		crc >>= 8;
	}
}

/*
 * Receive array buf of max length with ending ZDLE sequence
 *  and CRC.  Returns the ending character or error code.
 *  NB: On errors may store length+1 bytes!
 */
zrdata(buf, length)
char *buf;
{
	static int c;
	static unsigned crc;
	static char *end;
	static int d;

	if (Rxframeind == ZBIN32)
		return zrdat32(buf, length);

	crc = Rxcount = 0;  
	end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				crc = updcrc((d=c)&0377, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updcrc(c, crc);
				if (crc & 0xFFFF) {
					zperr("Bad data CRC",TRUE);
					return NERROR;
				}
				Rxcount = length - (end - buf);
				return d;
			case GOTCAN:
				zperr("Sender CANceled",TRUE);
				return ZCAN;
			case TIMEOUT:
				zperr("TIMEOUT",TRUE);
				return c;
			default:
				zperr("Bad data subpkt",TRUE);
				return c;
			}
		}
		*buf++ = c;
		crc = updcrc(c, crc);
	}
	zperr("Subpkt too long",TRUE);
	return NERROR;
}

zrdat32(buf, length)
char *buf;
{
	static int c, d;
	static long crc;
	static char *end;

	crc = 0xFFFFFFFFL;  
	Rxcount = 0;  
	end = buf + length;
	while (buf <= end) {
		if ((c = zdlread()) & ~0377) {
crcfoo:
			switch (c) {
			case GOTCRCE:
			case GOTCRCG:
			case GOTCRCQ:
			case GOTCRCW:
				d = c;  
				c &= 0377;
				crc = updc32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updc32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updc32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updc32(c, crc);
				if ((c = zdlread()) & ~0377)
					goto crcfoo;
				crc = updc32(c, crc);
				if (crc != 0xDEBB20E3) {
					zperr("Bad data CRC",TRUE);
					return NERROR;
				}
				Rxcount = length - (end - buf);
				return d;
			case GOTCAN:
				zperr("Sender CANceled",TRUE);
				return ZCAN;
			case TIMEOUT:
				zperr("TIMEOUT",TRUE);
				return c;
			default:
				zperr("Bad data subpkt",TRUE);
				return c;
			}
		}
		*buf++ = c;
		crc = updc32(c, crc);
	}
	zperr("Subpkt too long",TRUE);
	return NERROR;
}


/*
 * Read a ZMODEM header to hdr, either binary or hex.
 *  eflag controls local display of non zmodem characters:
 *	0:  no display
 *	1:  display printing characters only
 *	2:  display all non ZMODEM characters
 *  On success, set Zmodem to 1, set Rxpos and return type of header.
 *   Otherwise return negative on error.
 *   Return NERROR instantly if ZCRCW sequence, for fast error recovery.
 */
zgethdr(hdr, eflag)
char *hdr;
int eflag;
{
	static int c, n, cancount;

	n = Zrwindow + Baudrate;	   /* Max bytes before start of frame */
	Rxframeind = Rxtype = 0;

startover:
	cancount = 5;
again:
	/* Return immediate NERROR if ZCRCW sequence seen */
	switch (c = readline(Rxtimeout)) {
	case RCDO:
	case TIMEOUT:
		goto fifi;
	case CAN:
gotcan:
		if (--cancount <= 0) {
			c = ZCAN; 
			goto fifi;
		}
		switch (c = readline(INTRATIME)) {
		case TIMEOUT:
			goto again;
		case ZCRCW:
			c = NERROR;
			/* **** FALL THRU TO **** */
		case RCDO:
			goto fifi;
		default:
			break;
		case CAN:
			if (--cancount <= 0) {
				c = ZCAN; 
				goto fifi;
			}
			goto again;
		}
		/* **** FALL THRU TO **** */
	default:
agn2:
		if ( --n == 0) {
			zperr("Grbg ct exceeded",TRUE);
			return(NERROR);
		}
		goto startover;
	case ZPAD|0200:		/* This is what we want. */
	case ZPAD:		/* This is what we want. */
		break;
	}
	cancount = 5;
splat:
	switch (c = noxrd7()) {
	case ZPAD:
		goto splat;
	case RCDO:
	case TIMEOUT:
		goto fifi;
	default:
		goto agn2;
	case ZDLE:		/* This is what we want. */
		break;
	}

	switch (c = noxrd7()) {
	case RCDO:
	case TIMEOUT:
		goto fifi;
	case ZBIN:
		Rxframeind = ZBIN;  
		Crc32 = FALSE;
		c =  zrbhdr(hdr);
		break;
	case ZBIN32:
		Crc32 = Rxframeind = ZBIN32;
		c =  zrb32hdr(hdr);
		break;
	case ZHEX:
		Rxframeind = ZHEX;  
		Crc32 = FALSE;
		c =  zrhhdr(hdr);
		break;
	case CAN:
		goto gotcan;
	default:
		goto agn2;
	}
	Rxpos = (unsigned)(hdr[ZP3] & 0377);
	Rxpos = (Rxpos<<8) + (unsigned)(hdr[ZP2] & 0377);
	Rxpos = (Rxpos<<8) + (unsigned)(hdr[ZP1] & 0377);
	Rxpos = (Rxpos<<8) + (unsigned)(hdr[ZP0] & 0377);
fifi:
	switch (c) {
	case GOTCAN:
		c = ZCAN;
		/* **** FALL THRU TO **** */
	case ZNAK:
	case ZCAN:
	case NERROR:
	case TIMEOUT:
	case RCDO:
      sprintf(Buf,"Got %s", frametypes[c+FTOFFSET]);
		zperr(Buf,TRUE);
		/* **** FALL THRU TO **** */
	default:
      break;
	}
	return c;
}

/* Receive a binary style header (type and position) */
zrbhdr(hdr)
char *hdr;
{
	static int c, n;
	static unsigned crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = updcrc(c, 0);

	for (n=4; --n >= 0; ++hdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = updcrc(c, crc);
		*hdr = c;
	}
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc(c, crc);
	if ((c = zdlread()) & ~0377)
		return c;
	crc = updcrc(c, crc);
	if (crc & 0xFFFF) {
		zperr("Bad Header CRC",TRUE); 
		return NERROR;
	}
	Zmodem = 1;
	return Rxtype;
}

/* Receive a binary style header (type and position) with 32 bit FCS */
zrb32hdr(hdr)
char *hdr;
{
	static int c, n;
	static long crc;

	if ((c = zdlread()) & ~0377)
		return c;
	Rxtype = c;
	crc = 0xFFFFFFFFL; 
	crc = updc32(c, crc);
#ifdef DEBUGZ
#endif

	for (n=4; --n >= 0; ++hdr) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = updc32(c, crc);
		*hdr = c;
#ifdef DEBUGZ
#endif
	}
	for (n=4; --n >= 0;) {
		if ((c = zdlread()) & ~0377)
			return c;
		crc = updc32(c, crc);
#ifdef DEBUGZ
#endif
	}
	if (crc != 0xDEBB20E3) {
		zperr("Bad Header CRC",TRUE); 
		return NERROR;
	}
	Zmodem = 1;
	return Rxtype;
}


/* Receive a hex style header (type and position) */
zrhhdr(hdr)
char *hdr;
{
	static int c;
	static unsigned crc;
	static int n;

	if ((c = zgethex()) < 0)
		return c;
	Rxtype = c;
	crc = updcrc(c, 0);

	for (n=4; --n >= 0; ++hdr) {
		if ((c = zgethex()) < 0)
			return c;
		crc = updcrc(c, crc);
		*hdr = c;
	}
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc(c, crc);
	if ((c = zgethex()) < 0)
		return c;
	crc = updcrc(c, crc);
	if (crc & 0xFFFF) {
		zperr("Bad Header CRC",TRUE); 
		return NERROR;
	}
	if (readline(INTRATIME) == '\r')	/* Throw away possible cr/lf */
		readline(INTRATIME);
	Zmodem = 1; 
	return Rxtype;
}

/* Send a byte as two hex digits */
zputhex(c)
int c;
{
	static char	digits[]	= "0123456789abcdef";

   mcharout(digits[(c&0xF0)>>4]);
   mcharout(digits[(c)&0xF]);
}

/*
 * Send character c with ZMODEM escape sequence encoding.
 *  Escape XON, XOFF. Escape CR following @ (Telenet net escape)
 */
zsendline(c)
int c;
{
	static lastsent;

	switch (c &= 0377) {
	case ZDLE:
		mcharout(ZDLE);
		mcharout (lastsent = (c ^= 0100));
		break;
	case 015:
	case 0215:
		if (!Zctlesc && (lastsent & 0177) != '@')
			goto sendit;
		/* **** FALL THRU TO **** */
	case 020:
	case 021:
	case 023:
	case 0220:
	case 0221:
	case 0223:
		mcharout(ZDLE);
		c ^= 0100;
sendit:
		mcharout(lastsent = c);
		break;
	default:
		if (Zctlesc && ! (c & 0140)) {
			mcharout(ZDLE);
			c ^= 0100;
		}
		mcharout(lastsent = c);
	}
}

/* Decode two lower case hex digits into an 8 bit byte value */
zgethex()
{
	int c;

	c = zgeth1();
	return c;
}
zgeth1()
{
	static int c, n;

	if ((c = noxrd7()) < 0)
		return c;
	n = c - '0';
	if (n > 9)
		n -= ('a' - ':');
	if (n & ~0xF)
		return NERROR;
	if ((c = noxrd7()) < 0)
		return c;
	c -= '0';
	if (c > 9)
		c -= ('a' - ':');
	if (c & ~0xF)
		return NERROR;
	c += (n<<4);
	return c;
}

/*
 * Read a byte, checking for ZMODEM escape encoding
 *  including CAN*5 which represents a quick abort
 */

zdlread()
{
	static int c;

again:
	switch (c = readline(Rxtimeout)) {
	case ZDLE:
		break;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again;
	default:
		if (Zctlesc && !(c & 0140)) {
			goto again;
		}
		return c;
	}
again2:
	if ((c = readline(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = readline(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = readline(Rxtimeout)) < 0)
		return c;
	if (c == CAN && (c = readline(Rxtimeout)) < 0)
		return c;
	switch (c) {
	case CAN:
		return GOTCAN;
	case ZCRCE:
	case ZCRCG:
	case ZCRCQ:
	case ZCRCW:
		return (c | GOTOR);
	case ZRUB0:
		return 0177;
	case ZRUB1:
		return 0377;
	case 023:
	case 0223:
	case 021:
	case 0221:
		goto again2;
	default:
		if (Zctlesc && ! (c & 0140)) {
			goto again2;
		}
		if ((c & 0140) ==  0100)
			return (c ^ 0100);
		break;
	}
   sprintf(Buf,"Bad escape %x", c);
	zperr(Buf,TRUE);
	return NERROR;
}

/*
 * Read a character from the modem line with timeout.
 *  Eat parity, XON and XOFF characters.
 */
noxrd7()
{
	static int c;

	for (;;) {
		if ((c = readline(Rxtimeout)) < 0)
			return c;
		switch (c &= 0177) {
		case XON:
		case XOFF:
			continue;
		default:
			if (Zctlesc && !(c & 0140))
				continue;
		case '\r':
		case '\n':
		case ZDLE:
			return c;
		}
	}
}

/* Store long integer pos in Txhdr */
stohdr(pos)
long pos;
{
	Txhdr[ZP0] = pos;
	Txhdr[ZP1] = (pos>>8);
	Txhdr[ZP2] = (pos>>16);
	Txhdr[ZP3] = (pos>>24);
}

/* Recover a long integer from a header */
long
rclhdr(hdr)
char *hdr;
{
	static long l;

	l = (unsigned)(hdr[ZP3] & 0377);
	l = (l << 8) | (unsigned)(hdr[ZP2] & 0377);
	l = (l << 8) | (unsigned)(hdr[ZP1] & 0377);
	l = (l << 8) | (unsigned)(hdr[ZP0] & 0377);
#ifdef DEBUG
   lreport(FBLOCKS,l);
#endif
	return l;
}

/***************************** End of zm.c *********************************/
