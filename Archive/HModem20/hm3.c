/************************ Module 3 ***************************************/
/*                                                                       */
/*      The following functions are hardware dependent.  These are       */
/*  set up to use a Hayes modem running through the H8-4 board           */
/*  on the HEATH H8, or the serial board on the H89.  The modem port     */
/*  is driven at interrupt level 5 and must be configured for interrupt  */
/*  5 on the serial board.  This can be changed to another level if      */
/*  needed.                                                              */
/*                                                                       */
/*************************************************************************/

#define  C80

/* box characters for H-19 */

#define     LC       38
#define     TR       2
#define     HT       12
#define     WD       41
#define     HORIZ    'a'
#define     VERT     '`'
#define     UL       'f'
#define     UR       'c'
#define     LL       'e'
#define     LR       'd'     
#define     RC       LC+WD-1
#define     BR       TR+HT-1

/***************************/

#include "hmodem80.h"

prtservice()    /*printer service routine*/
{
   if (pready()) {
      if (Prthead != Prttail) {
         bdos(5,*Prttail++);        /* write list byte */
         if (Prttail > Prttop)
            Prttail = Prtbottom;
      }
   }
}

pready()   /*get printer status using bios call*/
{
#ifdef   C80
#asm
         LXI     H,prdy1
         PUSH    H               ;SAVE RETURN ADDRESS
         LHLD    BIOSADDR        ;start of bios
         LXI     D,LISTST        ;offset of list status call
         DAD     D
         PCHL                    ;BIOS CALL TO LISTST
prdy1:   MOV     L,A             ;RESULT IN A
         MVI     H,0
#endasm
#endif
}

screenprint()
{  
   static int c, pos;

   scplabel();
   flush();
   if (!PFlag)
      if (getprtbuf() != OK)
         return;
   pos = 0;
   outp(0xe9,0);             /* disable console interrupt */
   outp(0xe8,ESC);           /* send ESC # to transmit screen */
   mswait(6);
   outp(0xe8,'#');
   while ((c=dio()) != CR) {
      if (c == ESC)
         dio();
      else {
         *Prthead++ = c;
         adjustprthead();
         pos++;
         if (pos == 80) {
            *Prthead++ = CR;
            adjustprthead();
            *Prthead++ = LF;
            adjustprthead();
            pos = 0;
         }
      }
   }
   outp(0xe9,1);             /* enable console interrupt */
   flush();
   if (!PFlag)
      free(Prtbuf);
}

adjustprthead()
{
   if (Prthead > Prttop)
      Prthead = Prtbottom;
}

dio()          /* direct console port inp when bdos is too slow */
{
#asm
INLP:	   IN	   0EDH     ;check port status
	      ANI	1		   ;char ready?
	      JZ	   INLP		;wait for char
   	   IN	   0E8H
	      ANI	177Q		;strip parity
	      MOV	L,A
	      MVI   H,0
	      RET
#endasm
}

initializemodem()
{
   resetace();
   mstrout("\n\n",FALSE);
   mstrout(Modem.init,FALSE);
   while (readline(10) != TIMEOUT);   /*gobble echoed characters*/
}

getch()
{
   return bdos(DIRCTIO,INPUT);
}

flush()
{
   while(bdos(GCS,NULL))          /*clear type-ahead buffer*/
      bdos(CONIN,NULL);
   getch();           /*and anything else*/
}

purgeline()
{
   while (minprdy())              /*while there are characters...*/
     mcharinp();             /*gobble them*/
}

mread(buffer,count,timeout)      /* time in tenths of seconds */
char *buffer;
int count, timeout;
{
   static int i, t;

   timeout *= 50;   /* 2 ms ticks */

   i = 0;
   t = timeout + *Ticptr;
   while (!minprdy() && (t != *Ticptr) && !opabort());  /*wait until ready*/
   while (count--) {
      if (minprdy())
         buffer[i++] = mcharinp();           /*get it*/
      else
         break;
   }
   return i;
}

mcharinp()
{
   static unsigned c;

   c = *Mdmtail++;
   if (Mdmtail > Mdmtop) {
      Tlap++;
      Mdmtail = Mdmbottom;
   }
   if (Stopped) {
      if ((Hlap == Tlap) || ((Mdmtail-Mdmhead) > 256)) {
         outp(Mdmport,CTRLQ);
         Stopped = FALSE;
      }
   }
   return c;
}

mcharout(c)
char c;
{
   while (!moutrdy())
      opabort();         /* do nothing until ready except check for abort */
   outp(Mdmport,c);
}

moutrdy()
{
   return inp(Linestat) & OUTSTAT;
}

minprdy()
{
   return (Mdmtail != Mdmhead) || Stopped;
}

cls() /*clear the screen*/
{
   printf("\n\033E\b\b\b\b\b");
}

locate(r,c)
int r, c;
{
   printf("\033Y%c%c",r+32,c+32);
}

box()          /* put box on screen for file transfer */
{
   register int i;
   static char *headings[] = { "","PROTOCOL:","FILE NAME:","FILE SIZE:",
                               "BLOCK CHECK:","TRANSFER TIME:",
                               "BYTES TRANSFERRED:","BLOCKS TRANSFERRED:",
                               "SECTORS IN FILE:","ERROR COUNT:",
                               "LAST MESSAGE:  NONE" };
   static int start[] = { 0,13+LC,12+LC,12+LC,10+LC,8+LC,4+LC,3+LC,6+LC,
                          10+LC,9+LC };
      
   printf("\033F");
   locate(TR,LC);
   putchar(UL);
   for (i=1; i<WD-1; i++)
      putchar(HORIZ);
   putchar(UR);
   locate(BR,LC);
   putchar(LL);
   for (i=1; i<WD-1; i++)
      putchar(HORIZ);
   putchar(LR);
   for (i=1; i<HT-1; i++) {
      locate(TR+i,LC);
      putchar(VERT);
      locate(TR+i,RC);
      putchar(VERT);
   }
   printf("\033G");
   clrbox();
   for (i=1; i<11; i++) {
      locate(TR+i,start[i]);
      printf(headings[i]);
   }   
}

clrbox()
{
   register int i;

   for (i=TR+1; i < BR; i++) {
      locate(i,LC+1);
      printf("                                       ");
   }
}

hidecurs()
{
   printf("\033x5");
}

showcurs()
{
   printf("\033y5");
}

savecurs()
{
   printf("\033j");    /* save cursor position */
}

restcurs()
{
   printf("\033k");    /* return cursor to previously saved position */
}

minterrupt()    /*modem input interrupt service routine*/
{
#ifdef   C80
#asm
   PUSH    PSW
   PUSH    B
   PUSH    D
   PUSH    H
#endasm
   if (inp(Linestat) & INSTAT) {
      *Mdmhead++ = inp(Mdmport);
      if (Mdmhead > Mdmtop) {
         Hlap++;
         Mdmhead = Mdmbottom;
      }
      if (  !Stopped 
         && XonXoff
         && (Hlap != Tlap) 
         && ((Mdmtail-Mdmhead) < 128)) {
            outp(Mdmport,CTRLS);
            Stopped = TRUE;
      }
   }
#asm
   POP     H
   POP     D
   POP     B
   POP     PSW
   EI
#endasm
#endif
}

initvector()
{
   static char *p;
   static unsigned *q;

   p = Intlevel * 8;
   q = p + 1;
   *p = JUMP;
   *q = minterrupt;
   Mdmhead = Mdmtail = Mdmbottom = MdmInBuffer = alloc(MBUFSIZ);
   Mdmtop = MdmInBuffer + MBUFSIZ - 1;
   if (allocerror(MdmInBuffer))
      deinitvector();
}

deinitvector()
{
   static char *p;

   outp(Mdmport+1,0);   /*disable modem interrupts*/
   p = Intlevel * 8;
   *p = ENABLE;
   p++;
   *p = RTN;
}

openerror(chan,fname)
int chan;
char *fname;
{
   if (!chan) {
      printf("\n\nERROR - Cannot open %s\n\n",fname);
      wait(3);
   }
   return !chan;
}

wrerror(fname)
char *fname;
{
   printf("\n\nERROR - Cannot write to %s\n\n",fname);
   wait(3);
}

allocerror(p)
char *p;
{
   static int status;

   if (status=(p == -1))
      perror("Memory allocation failure");
   return status;   
}

perror(string)
char *string;
{
   printf("\007\nERROR - %s\n\n",string);
   wait(3);
}

mdmerror()
{  
   return inp(Linestat) & 0x0e;
}

hangup()
{
   printf("\nHanging up...");
   outp(Mdmport+4,02);             /*kill DTR momentarily*/
   mswait(200);
   outp(Mdmport+4,03);
   mstrout(Modem.hangup,FALSE);
   printf("\033p Line Disconnected \033q\n");
   resetace();
}

kbwait(seconds)
unsigned seconds;
{
   static unsigned t;
   static int c;

   t = seconds * 500;
   t += *Ticptr;
   while(!(c=getch()) && (t != *Ticptr));
   return ((c & 0xff) == ESC);
}

wait(seconds)
unsigned seconds;
{
   mswait(seconds*1000);
}

mswait(milliseconds)
unsigned milliseconds;
{
   static unsigned t;

   t = milliseconds >> 1;         /* 2-ms ticks */
   t += *Ticptr;
   while (t != *Ticptr);  /*wait until ready*/
}

setace(n)   /* for a particular phone call */
int n;
{
   Current.cbaudindex = Book[n].pbaudindex;
   Current.cparity = Book[n].pparity;
   Current.cdatabits = Book[n].pdatabits;
   Current.cstopbits = Book[n].pstopbits;
   updateace();
}

resetace()  /* to default values */
{
   Current.cbaudindex = Line.baudindex;
   Current.cparity = Line.parity;
   Current.cdatabits = Line.databits;
   Current.cstopbits = Line.stopbits;
   updateace();
}

updateace()
{
   initace(Mdmport,Current.cbaudindex,Current.cparity,
      Current.cdatabits,Current.cstopbits);
}


initace(port,b,p,d,s)                               /*for 8250 ace*/
int port,b,p,d,s;
{
   static int parity, databits, stopbits;

   switch (p) {
   case 'E':
      parity = EVENPARITY;
      break;
   case 'O':
      parity = ODDPARITY;
      break;
/* case 'N': */
   default:
      parity = NOPARITY;
      break;
   }
   switch (d) {
   case 7:
      databits = SEVENBITS;
      break;
/* case 8:  */
   default:
      databits = EIGHTBITS;
      break;
   }
   switch (s) {
   case 1:
      stopbits = ONEBIT;
      break;
/* case 2:   */
   default:
      stopbits = TWOBITS;
      break;
   }
   outp(port+3,0x80);           /*set baud rate access bit*/
   outp(port,Divtable[b] & 0xff);   /*set baud rate lsb*/
   outp(port+1,Divtable[b]>>8);    /*then set msb*/
   outp(port+3,parity | databits | stopbits);
   outp(port+4,03);             /*set DTR and RTS high*/
   outp(port+1,01);             /*enable received data interrupt*/
}

outp(port,c)
{
#ifdef   C80
#asm
            POP     H               ;RETURN ADDRESS
            POP     D               ;CHARACTER TO OUTPUT
            POP     B               ;PORT TO OUTPUT
            PUSH    B
            PUSH    D
            PUSH    H
.Z80  
            OUT    (C),E            ;NEED Z80 TO DO THIS
.8080
#endasm
#endif
}

inp(port)
{
#ifdef   C80
#asm
            POP     H               ;RETURN ADDRESS
            POP     B               ;PORT TO INPUT
            PUSH    B
            PUSH    H
.Z80
            IN      L,(C)           ;Z80 ONLY
.8080
            MVI     H,0
#endasm
#endif
}

/************************ end of module 3 **********************************/
