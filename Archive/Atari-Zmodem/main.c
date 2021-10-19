/*
 * 	Main Module
 *
 *	Jwahar Bammi
 * 	bang:   {any internet host}!dsrgsun.ces.CWRU.edu!bammi
 * 	domain: bammi@dsrgsun.ces.CWRU.edu
 *	GEnie:	J.Bammi
 */

#include "config.h"

#include "zmdm.h"
#include "common.h"

#ifndef Vsync 			/* Atari forgot these in osbind.h */
#define Vsync()	xbios(37)
#endif

#define	esc	27
#define cr	0x0d
#define mvto(r,c)	EscSeq('Y');Bconout(2,r+040);Bconout(2,c+040)

#ifdef PHONES	/* common to this module and phone.c */
char	*PhoneFile;
#endif
 	/* Globals belonging to this module only */

int	rs232 = 1,		/* Ports */
        console = 2;
int	speed,			  /* rs232 setup parameters */
  	flowctl = 0,
	ucr = -1,
	rsr = -1,
	tsr = -1,
	scr = -1;
static int duplex = 0;		/* full duplex_p */

#ifdef FLOW_CTRL
/*
 * setFlow() - set rs232 flow control
 */
#ifndef REMOTE
void setFlow ()
{
	long conin;
	int ch, i;
	char cc[4] = " 0=";

	Bconws("Flow control: ");
	EscSeq('p');
	for (i=0; i<4; i++) {
		Bconws(cc);
		Bconws(vflows[i].sflow);
		cc[1]++;
	}
	Bconws("\r\n");
	EscSeq('q');
	Bconout(2, '\t');
	EscSeq('p');
	Bconws("What type==>");
	EscSeq('q');
	
	conin = Bconin(console);	/* get flow type */
	if ((conin & 0x00FF0000L) == 0x00610000L)
	{
		his_screen();	/* hit UNDO */
 		ResetIoBuf();
		finish();
	}
	ch = (conin & 0x007f) - '0';
	Bconout(2, ' ');

	if ((ch >= 0) && (ch < i))
		flowctl = ch;

	Bconws(FLOW_STRING(flowctl));
	Bconws("\r\n");
	Rsconf(-1, flowctl, -1, -1, -1, -1);
}
#else
void setFlow ()
{
	long conin;
	int ch, i;
	char cc[4] = " 0=";

	Bauxws("Flow control: ");
	for (i=0; i<4; i++) {
		Bauxws(cc);
		Bauxws(vflows[i].sflow);
		cc[1]++;
	}
	Bauxws("\r\n");
	Bconout(1, '\t');
	Bauxws("What type==>");
	
	conin = Bconin(1);	/* get flow type */
	if (((int)(conin & 0x007f) & CTRL('U')) == CTRL('U'))
	{
		his_screen();	/* hit UNDO */
 		ResetIoBuf();
		finish();
	}
	ch = (conin & 0x007f) - '0';
	Bconout(1, ' ');

	if ((ch >= 0) && (ch < i))
		flowctl = ch;

	Bauxws(FLOW_STRING(flowctl));
	Bauxws("\r\n");
	Rsconf(-1, flowctl, -1, -1, -1, -1);
}
#endif /* REMOTE */
#endif /* FLOW_CTRL */
		
/*
 * setBaud() - set rs232 port speed
 */
#ifndef REMOTE
void setBaud ()
{
  	long conin;
	int ch, i;
	char cc[4] = " 0=";
	
	Bconws("Baud rate:");
	EscSeq('p');
	for (i=0; vbauds[i].sbaud != NULL; i++) {
		Bconws(cc);
		Bconws(vbauds[i].sbaud);
		cc[1]++;
	}
	Bconws("\r\n");
	EscSeq('q');
	Bconout(2, '\t');
	EscSeq('p');
	Bconws("What speed==>");
	EscSeq('q');
	
	conin = Bconin(console);	/* get speed */
	if ((conin & 0x00FF0000L) == 0x00610000L)
	{
		his_screen();
		ResetIoBuf();
		finish();
	}
	ch = (conin & 0x007f) - '0';
	Bconout(2, ' ');

	if ((ch >= 0) && (ch < i))
		speed = ch;
	else
 		speed = getbaud();
 
	Bconws(BAUD_STRING(speed));
	Baudrate = BAUD_RATE(speed);

 	Bconws(" Baud\r\n");
	
	/* Set new Baud rate */

/*	Txoff(); */
#ifndef	HIBAUD
	Rsconf(vbauds[speed].ibaud, flowctl, ucr, rsr, tsr, scr);
#else
	{
	register char *mfp_ptr = (char *) 0xfffffa00L;
	conin=Super(0L);
	mfp_ptr[0x29] &= 0x7f;	/* Clear divide by 16 mode */
	mfp_ptr[0x1d] &= 0xf0;	/* Stop TImer D */
	for (mfp_ptr[0x25] = cbauds[speed].tdd;	/* Load Timer D data */
		mfp_ptr[0x25] != cbauds[speed].tdd;
		mfp_ptr[0x25] = cbauds[speed].tdd);
	mfp_ptr[0x1d] |= cbauds[speed].tdc;	/* Set Timer D mode */
	Super(conin);
	}
#endif
	
	Vsync(); Vsync();
/*	Txon(); */	
}
#else
void setBaud ()
{
	char ch;
	long conin;
	
	Bauxws("Baud rate: ");
	Bauxws("0=19200 1=9600, 2=4800, 3=2400, 4=1200, 5=300\r\n");
	Bconout(1, '\t');
	Bauxws("What speed==>");
	
	conin = Bconin(1);	/* get speed */
	if (((int)(conin & 0x007f) & CTRL('U')) == CTRL('U'))
	{
		his_screen();
		ResetIoBuf();
		finish();
	}
	ch = (char) (conin & 0x007f);
	Bconout(1, ' ');
	switch (ch)
	{
	    case '0':		/* 19200 */
		speed = 0;
		Bauxws("19200");
		Baudrate = 19200;
		break;
		
	    case '1':
		speed = 1;	/* 9600 */
		Bauxws("9600");
		Baudrate = 9600;
		break;
		
	    case '2':
		speed = 2;	/* 4800 */
		Bauxws("4800");
		Baudrate = 4800;
		break;
		
	    case '3':
		speed = 4;	/* 2400 */
		Bauxws("2400");
		Baudrate = 2400;
		break;
		
	    case '4':
		speed = 7;	/* 1200 */
		Bauxws("1200");
		Baudrate = 1200;
		break;
		
	    case '5':
		speed = 9;	/* 300 */
		Bauxws("300");
		Baudrate = 300;
		break;

	    default:
		speed = getbaud();
		Bauxws(BAUD_STRING(speed));
		Baudrate = BAUD_RATE(speed);
	}
	Bauxws(" Baud\r\n");
	
	/* Set new Baud rate */

/*	Txoff(); */
	Rsconf(speed, flowctl, ucr, rsr, tsr, scr);
	Vsync(); Vsync();
/*	Txon(); */

	
}
#endif /* REMOTE */

/*
 * help() - display help info and menu
 */
#ifndef REMOTE
void help ()
{
	register long conin;
	register int x;
	extern char *r_filename();
	
	my_screen();		/* Switch to my screen memory */
	EscSeq('v');		/* wrap at end of line */
	EscSeq('E');		/* clear screen */
	
	mvto(2,11);
	EscSeq('p');
	Bconws("ZMDM Version: ");
	Bconws(ZMDMVERSION);
	EscSeq('q');
	EscSeq('p');
	x =(int) strlen(COMPILER);
	x = (80 - x)/2;
	mvto(3,x);
	Bconws(COMPILER);
	EscSeq('q');
	mvto(5,21);
	Bconws("bammi@cadence.com\r\n\n");

	/* Put up menu */
	Bconws("\r\n\t");
	EscSeq('p');		/* reverse video */
	Bconws("Undo");
	EscSeq('q');		/* quit reverse video */
	Bconws(" to exit.\r\n");
	
	Bconws("\t");
	EscSeq('p');		/* reverse video */
	Bconws("Help");
	EscSeq('q');		/* quit reverse video */
	Bconws(" for this message.\r\n");

	Bconws("\t");
	EscSeq('p');		/* reverse video */
	Bconws("Escape");
	EscSeq('q');		/* quit reverse video */
	Bconws(" to send a break.\r\n");
	
	Bconws("\t");
	EscSeq('p');		/* reverse video */
	Bconws("T or t");
	EscSeq('q');		/* quit reverse video */
	Bconws(" to do file transfers and local functions.\r\n");

#ifdef PHONES
	Bconws("\t");
	EscSeq('p');		/* reverse video */
	Bconws("P or p");
	EscSeq('q');		/* quit reverse video */
	Bconws(" for Phone services.\r\n");
#endif

	if(rez == 2)
	{
		Bconws("\t");
		EscSeq('p');		/* reverse video */
		Bconws("H or h");
		EscSeq('q');		/* quit reverse video */
		Bconws(" for Hi Rez Toggle (25/50 Lines).\r\n");
	}

	Bconws("\t");
	EscSeq('p');		/* reverse video */
	Bconws("I or i");
	EscSeq('q');		/* quit reverse video */
	Bconws(" to Invert screen colors.\r\n");

	Bconws("\t");
	EscSeq('p');		/* reverse video */
	Bconws("D or d");
	EscSeq('q');		/* quit reverse video */
	Bconws(" to toggle duplex.\r\n");
	
	Bconws("\t");
	EscSeq('p');		/* reverse video */
	Bconws("Return");
	EscSeq('q');		/* quit reverse video */
	Bconws(" to do nothing.\r\n");
	
#ifdef	FLOW_CTRL
	Bconws("\t");
	EscSeq('p');		/* reverse video */
	Bconws("F or f");
	EscSeq('q');		/* quit reverse video */
	Bconws(" to set flow control.  Currently is ");
	EscSeq('p');
	Bconws(FLOW_STRING(flowctl));
	Bconws(".\r\n");
	EscSeq('q');
#endif

	Bconws("\t");
	EscSeq('p');		/* reverse video */
	Bconws("B or b");
	EscSeq('q');		/* quit reverse video */
	Bconws(" to set baud rate.     Currently is ");
	EscSeq('p');
	Bconws(BAUD_STRING(speed));
	Bconws(" Baud.\r\n\r\n");
	EscSeq('q');

	/* get response */
	conin = Bconin(console);

	if ((conin & 0x00FF0000L) == 0x00610000L)
	{
		/* He hit <UNDO> */
		his_screen();
		ResetIoBuf();
		finish();
	}
	
	switch((int)(conin & 0x007f))
	{
	    case 'B':
	    case 'b':
		/* Set baud rate */
		setBaud();
		break;

#ifdef	FLOW_CTRL
	    case 'F':
	    case 'f':
		/* Set flow control */
		setFlow();
		break;
#endif
		
	    case 'T':
	    case 't':
	        EscSeq('E');		/* clear screen */

		    /* Set no flow Control */
#if 0
#ifdef FLOW_CTRL
		    Rsconf(-1,0,-1,-1,-1,-1);
		    Vsync(); Vsync();

#endif
#endif
		/* Go do transfers */
		transfer();
		    
#if 0
#ifdef FLOW_CTRL
		    /* Flow Control On */
/*		    Txoff(); */
		    Rsconf(-1,flowctl,-1,-1,-1,-1);
		    Vsync(); Vsync();
/*		    Txon(); */
#endif
#endif
		his_screen();
		return;
		
	    case '\033':
		/* Send a break */
		sendbrk();
		his_screen();	/* Don't wait for the key hit */

		return;

	      case 'd':
	      case 'D':
		duplex ^= 1;
		his_screen();
		return;
		
	    case 'i':
	    case 'I':
		/* Invert screen colors */
		his_screen();
		if(scolor == 0)
		{
			EscSeq('b');	/* Foreground color 0 */
			Bconout(2, 0);
			EscSeq('c');	/* Background color 1 */
			Bconout(2, 1);
			scolor = 1;
		}
		else
		{
			EscSeq('b');	/* Foreground color 1 */
			Bconout(2, 1);
			EscSeq('c');	/* Background color 0 */
			Bconout(2, 0);
			scolor = 0;
		}
		EscSeq('E');		/* Clear the screen */
		return;

#ifdef PHONES
	    case 'p':
	    case 'P':
		/* Phone Services */
		phone();
		return;
#endif
	    case 'h':
	    case 'H':
		/* Hi rez 25/50 toggle */
		if(rez == 2)
		{
			if(hlines < 50)
			{
				hlines = 50;
				hi50();
			}
			else
			{
				hlines = 25;
				hi25();
			}
			his_screen();
			EscSeq('E');		/* clear screen */
			return;
		}
		/* else fall Through */
		
	    default:
		Bconws("No Change\r\n");
	}

	/* Wait for a key hit */
	hit_key();
	/* back to terminal screen */
	his_screen();
}
#else
void help ()
{
	register long conin;
	extern char *r_filename();
	
	my_screen();		/* Switch to my screen memory */
	
	Bauxws("\r\n\n");
	Bauxws("          ZMDM Version: ");
	Bauxws(ZMDMVERSION);
	Bauxws("\r\n                         ");
	Bauxws(COMPILER);
	Bauxws("\r\n\n         bammi@cadence.com\r\n\n");

	/* Put up menu */
	Bauxws("\r\n\t");
	Bauxws("CTRL-U");
	Bauxws(" to exit.\r\n");
	
	Bauxws("\t");
	Bauxws("CTRL-Z");
	Bauxws(" for this message.\r\n");

	Bauxws("\t");
	Bauxws("Escape");
	Bauxws(" to send a break.\r\n");
	
	Bauxws("\t");
	Bauxws("T or t");
	Bauxws(" to do file transfers and local functions.\r\n");

	Bauxws("\t");
	Bauxws("Return");
	Bauxws(" to do nothing.\r\n");

#ifdef	FLOW_CTRL
	Bauxws("\t");
	Bauxws("F or f");
	Bauxws(" to set flow control.  Currently is ");
	Bauxws(FLOW_STRING(flowctl));
	Bauxws(".\r\n");
#endif /* FLOW_CTRL */
	
	Bauxws("\t");
	Bauxws("B or b");
	Bauxws(" to set baud rate.     Curently is ");
	Bauxws(BAUD_STRING(speed));
	Bauxws(" Baud.\r\n\r\n");

	/* get response */
	conin = Bconin(1);

	if (((int)(conin & 0x007f) & CTRL('U')) == CTRL('U'))
	{
		/* He hit <UNDO> */
		his_screen();
		ResetIoBuf();
		finish();
	}
	
	switch((int)(conin & 0x007f))
	{
	    case 'B':
	    case 'b':
		/* Set baud rate */
		setBaud();
		break;
		
	    case 'T':
	    case 't':
	        Bauxws("\r\n\n");		/* clear screen */

		    /* Set no flow Control */
#if 0
#ifdef FLOW_CTRL
		    Rsconf(-1,0,-1,-1,-1,-1);
		    Vsync(); Vsync();

#endif
#endif
		/* Go do transfers */
		transfer();
		    
#if 0
#ifdef FLOW_CTRL
		    /* Flow Control On */
/*		    Txoff(); */
		    Rsconf(-1,flowctl,-1,-1,-1,-1);
		    Vsync(); Vsync();
/*		    Txon(); */
#endif
#endif
		his_screen();
		return;
		
	    case '\033':
		/* Send a break */
		sendbrk();
		his_screen();	/* Don't wait for the key hit */

		return;

		/* else fall Through */
		
	    default:
		Bauxws("No Change\r\n");
	}

	/* back to terminal screen */
	his_screen();
}
#endif /* REMOTE */

main (argc, argv)
int argc;
char **argv;
{
	register int	c;		/* rs232 input */
	register int	i;
	register long	conin;
#ifndef REMOTE
	extern int *aaddress(); /* Routine that returns base address of
				 * line A variables
				 */
#ifdef OVERSCAN
	extern long *ms_log;
	long ms_off;
#endif
#else
	extern FILE *fopen();
#endif

#if (MWC || __GNUC__)
#ifdef MWC
	extern char *lmalloc();
#else	
#define lmalloc malloc
	extern void *lmalloc(unsigned long);
#endif
#endif

#ifdef PHONES
	if(argc > 1)
		PhoneFile = *++argv;
#endif
	/* Set up Dta */
	Fsetdta(&statbuf);

	/* Get screen rez */
	rez = Getrez();
	drv_map = Drvmap();

#ifndef	REMOTE
	aline_addr = aaddress();
#ifdef OVERSCAN
	ms_off=Logbase()-Physbase();
#endif
#endif

#if (MWC || MANX || __GNUC__)
#ifndef REMOTE
#ifndef OVERSCAN
#if (MWC || __GNUC__)
       if((m_screen = (long *)lmalloc(
	               (unsigned long)((8L*1024L+32L)*(long)sizeof(long))))
							== (long *)NULL)
#else
       if((m_screen = (long *)Malloc(
	               (unsigned long)((8L*1024L+32L)*(long)sizeof(long))))
							== (long *)NULL)
#endif
#else
       conin=(long)aline_addr[-1]*aline_addr[-2]+ms_off+512;
#if (MWC || __GNUC__)
       if((m_screen = (long *)lmalloc((unsigned long)conin))== (long *)NULL)
#else
       if((m_screen = (long *)Malloc((unsigned long)conin))== (long *)NULL)
#endif
#endif /* OVERSCAN */
	{
		Bconws("Sorry, could not allocate enough memory\r\n");
		Pterm(3);
	}
#ifdef OVERSCAN
	{
		register long j,k;
		k=0;
		if (rez == 2) {
			c=Setcolor(0,-1)&1;
			Setcolor(0,c);
			if (c)
				k=~k;
		};
		for (j=conin/4;j>0;j--)
			m_screen[j]=k;
	}
#endif /* OVERSCAN */
#endif /* REMOTE */
#ifndef DYNABUF
#ifdef MANX
	if((bufr = (unsigned char *)Malloc((unsigned long)BBUFSIZ))
					 == (unsigned char *)NULL)
#else
	if((bufr = (unsigned char *)lmalloc((unsigned long)BBUFSIZ))
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
#if (MWC || MANX || __GNUC__)
	ms_ptr = (long *) ((0xffffff00L & ((long)(m_screen))) + 0x00000100L);
#else
	ms_ptr = (long *) ((0xffffff00L & ((long)(&m_screen[0]))) + 0x00000100L);
#endif /* MWC */
#ifdef OVERSCAN
	ms_log=(long *)((long)ms_ptr+ms_off);
#endif

/*	EscSeq('e');		renamed to .ttp to  Turn on the cursor */
	EscSeq('v');		/* wrap at end of line */
	EscSeq('E');		/* clear screen */
#endif /* REMOTE */

	SetIoBuf();

#ifndef	HIBAUD
	speed = getbaud();
#else
	speed = 0;
#endif

/*	Txoff(); */
	Rsconf((int) speed, flowctl, ucr, rsr, tsr, scr);  /* init set */
	Vsync(); Vsync();
/*	Txon();	 */

	Baudrate = BAUD_RATE(speed);
	
#ifndef REMOTE
	if(rez == 2)
		ihlines = hlines = aline_addr[-21] + 1;	/* 42(lineAbase) */
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
	
#endif /* remote */

	help();

	i = 0;
	for (;;)	/* infinite loop */
	{
#ifndef REMOTE
		while (Bconstat(rs232) != 0)
		{
			/* Char at Modem */
			c = Bconin(rs232) & 0x007f;
			Bconout(console, c);

			/* Check the console once in a while */
			/* important at High speeds */
			if ((++i) & 32)
			{
			    if (Bconstat(console) != 0)
	      		    {
				    c = Bconin(console) & 0x007f;
				    Bconout(rs232, c);
				    if(duplex) Bconout(console, c);
			    }
			    i = 0;
			}
				
		}
		
		if (Bconstat(console) != 0)
		{
			/* Char at Console */
			conin = Bconin(console);
			if ((conin & 0x00FF0000L) == 0x00610000L)  /* Undo */
			{
				ResetIoBuf();
				finish();
			}
			
			if ((conin & 0x00FF0000L) == 0x00620000L)  /* Help */
			    help();
			else
			{
			    c = conin & 0x007f;
			    Bconout(rs232, c);
			    if(duplex) Bconout(console, c);
			}
		}
#else
		while (Bconstat(rs232) != 0)
		{
			/* Char at Modem */
			c = Bconin(rs232) & 0x007f;

			if ((c & CTRL('U')) == CTRL('U'))  /* Undo */
			{
				ResetIoBuf();
				finish();
			}
			
			if ((c & CTRL('Z')) == CTRL('Z'))  /* Help */
			    help();
			else
			    Bconout(rs232,c);
		}

#endif /* REMOTE */
	}
}

finish()
{
#ifdef PHONES
	/* Save phone directory if it changed */
	extern int dchanged;

	if(dchanged)
	{
		if(writedir() == 1)
		    hit_key();
	}
#endif
	/* restore hlines if changed */
	if((rez == 2) && (ihlines != hlines))
	{
	    if(hlines == 25)
		hi50();
	    else
		hi25();
	}
#if (MWC || MANX || __GNUC__)
#ifdef DYNABUF
	Mfree(bufr);
#else
	free(bufr);
#endif

#ifndef REMOTE
	free(m_screen);
#endif

#else

#ifdef DYNABUF
	Mfree(bufr);
#endif
#endif
	exit(0);
}

/* -eof- */
