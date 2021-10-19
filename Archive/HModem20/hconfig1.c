/************************* START OF HCONFIG1 ********************************/

#define  C80

#include "hmodem80.h"
#include "hconfig.h"

sethost()
{
   int c;

start:
   cls();
   printf("\tA - Welcome string.....%s\n",Host.welcome);
   printf("\tB - Autoanswer string..%s\n",Host.autoanswer);
   printf("\tC - Password...........%s\n",Host.password);
   printf("\tD - Connection type....%s\n",Host.modemconnection ? 
      "MODEM" : "DIRECT");
   printf("\tZ - Exit\n\n");
   printf("\tPlease enter your selection:  ");
   c = toupper(getchar());
   cls();
   switch (c) {
   case 'A':
      gnewstr("welcome string",Host.welcome);
      break;      

   case 'B':
      gnewstr("autoanswer string",Host.autoanswer);
      break;      

   case 'C':
      gnewstr("password",Host.password);
      break;      

   case 'D':
      printf("(M)odem or (D)irect connection? <M> :  ");
      if (toupper(getchar()) == 'D')
         Host.modemconnection = FALSE;
      else
         Host.modemconnection = TRUE;
      break;      

   case ESC:
   case 'Z':
      return;
      break;

   default:
      printf("\007\nInvalid Entry\n");
      wait(1);
      break;
   }
   goto start;
}

phonedit()
{
   int i, c, change;
   char *answer;

   loadnos();
   answer = Pathname;
   while(TRUE) {
      flush();
      shownos();
      printf("\nPlease enter letter of number to change/enter,\n");
      printf("or anything else to EXIT:  ");
      c = toupper(getchar()) - 'A';
      if (c < 0 || c > 20)
         break;
      change = TRUE;
      flush();
      printf("\n          Name:  %s\nEnter new name:  ",PBook[c].name);
      if (getline(answer,18))
         strcpy(PBook[c].name,answer);
      printf("\n          Number:  %s\nEnter new number:  ",PBook[c].number);
      if (getline(answer,18))
         strcpy(PBook[c].number,answer);
      printf("\n          Bit rate:  %d\nEnter new bit rate:  ",
         Baudtable[PBook[c].pbaudindex]);
      if (getline(answer,18)) {
         for (i=0; i<11; i++) {
            if (atoi(answer) == Baudtable[i]) {
               PBook[c].pbaudindex = i;
               break;
            }
         }
      }
      printf("\n          Parity:  %c\nEnter new parity:  ",PBook[c].pparity);
      if (getline(answer,18))
         PBook[c].pparity = toupper(answer[0]);
      printf("\n    Nr data bits:  %d\nEnter new number:  ",PBook[c].pdatabits);
      if (getline(answer,18))
         PBook[c].pdatabits = atoi(answer);
      printf("\n    Nr stop bits:  %d\nEnter new number:  ",PBook[c].pstopbits);
      if (getline(answer,18))
         PBook[c].pstopbits = atoi(answer);
      printf("\n                Duplex:  %s\nEnter (H)alf or (F)ull:  ",
         PBook[c].echo?"Half":"Full");
      if (getline(answer,18))
         PBook[c].echo = (toupper(answer[0]) == 'H');
   }
   flush();
   cls();
}

ldedit()
{
   char *p, *answer;
   int c;

   answer = Pathname;
   printf("\n\nEdit which long distance access code? (+ or -) <+>  ");
   if ((c=getchar()) == '-')
      p = Mci;
   else {
      c = '+';
      p = Sprint;
   }
   printf("\n\nCurrent code for %c:  %s\n\n",c,p);
   printf("Enter new code:  ");
   if (getline(answer,20))
      strcpy(p,answer);
}

edit()
{
   static int i;
   static char *buffer;
   static char keypad[] = "pqrstuvwxynM";
   static char keybuf[2];

   buffer = Pathname;
   while (TRUE) {
      cls();
      flush();
      printf("       KEYPAD MACRO LIST\n\n");
      for (i=0; i<10; i++)
         printf("%d - %s\n",i,KbMacro[i]);
      printf(". (PERIOD) - %s\n",KbMacro[10]);
      printf("ENTER - %s\n",KbMacro[11]);
      printf("\nPress key of macro to edit, or anything else to EXIT:  ");
      if (!(keybuf[0]=getfunction(getchar())))
         break;
      keybuf[1] = '\0';
      i = index(keypad,keybuf);
      if (i<0 || i>11)
         break;
      flush();
      printf("\nIf you want the macro to end with a RETURN,\n");
      printf("add a '!' to the end of your entry (20 characters max).");
      printf("\n\nOld Macro:  %s",KbMacro[i]);
      printf("\n\nNew Macro:  ");
      if (getline(buffer,21))
         strcpy(KbMacro[i],buffer);
   }
   flush();
   cls();
}

getfunction(c)  /*see if function key was pressed*/
unsigned c;
{
   static unsigned m;

   if (c == ESC) {
      mswait(50);                    /* wait 50 ms */
      if ((m=getch()) == '?') {
         mswait(50);
         m = getch();
      }
      return m;
   }
   else
      return 0;
}

getch()
{
   return bdos(DIRCTIO,INPUT);
}

getconfig()
{
   int fd, size, sectors;
   char *p;

   fd = fopen(Cfgfile,"rb");
   if (fd) {
      p = Waste;
      size = (unsigned)p - (unsigned)&BFlag;
      sectors = roundup(size,128);
#ifdef   DEBUG
      printf("\nsize=%d\nsectors=%d",size,sectors);
      printf("\nwaste=%x\n&bflag=%x\n",Waste,&BFlag);
#endif
      read(fd,&BFlag,sectors*128);
      fclose(fd);
   }
}

savephone()
{
   int fd, sectors, size;

   size = 20 * sizeof(struct phonebook);
   sectors = roundup(size,128);
#ifdef   DEBUG
   printf("\nsize=%d\nsectors=%d",size,sectors);
#endif
   fd = fopen(Phonefile,"wb");
   if (!openerror(fd,Phonefile)) {
      printf("\nSaving Phone numbers...");
      if (!write(fd,PBook,sectors*128))
         wrerror(Phonefile);
      else
         printf("Successful.\n");
      fclose(fd);
   }
}

saveconfig()
{
   int fd, sectors, size;
   char *p;

   p = Waste;
   size = (unsigned)p - (unsigned)&BFlag;
   sectors = roundup(size,128);
#ifdef   DEBUG
   printf("\nsize=%d\nsectors=%d",size,sectors);
   printf("\nwaste=%x\n&bflag=%x\n",Waste,&BFlag);
#endif
   fd = fopen(Cfgfile,"wb");
   if (!openerror(fd,Cfgfile)) {
      printf("\n\nSaving Configuration...");
      if (!write(fd,&BFlag,sectors*128))
         wrerror(Cfgfile);
      else
         printf("Successful.\n");
      fclose(fd);
   }
}

setbaud()
{
   int baud;
   char *buffer;

   buffer = Pathname;
   do {
      printf("\nPlease enter modem bit rate:  ");
      if (!(getline(buffer,6)))
         break;
      baud = atoi(buffer);
      printf("\n");
   }
   while (!goodbaud(baud));
}

goodbaud(value)
int value;
{
   int i;
   
   for (i=0; i<11; i++) {
      if (value == Baudtable[i]) {
         Line.baudindex = i;
         return TRUE;
      }
   }
   printf("\nInvalid entry\n");
   wait(1);
   return FALSE;
}

cls() /*clear the screen*/
{
   printf("\n\033E\b\b\b\b\b");
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

wait(seconds)
unsigned seconds;
{
   mswait(seconds*1000);
}

mswait(milliseconds)
unsigned milliseconds;
{
        static unsigned *p = TICCNT;

        milliseconds /= 2;         /* 2-ms ticks */
        milliseconds += *p;
        while (milliseconds != *p);  /*wait until ready*/
}

flush()
{
   while(bdos(GCS,NULL))          /*clear type-ahead buffer*/
      bdos(CONIN,NULL);
   getch();                       /*and anything else*/
}

shownos()
{
   int i, j;

   cls();
   printf("\033p         NAME                NUMBER          B   P D S E\033q");
   for (i=0,j=1; i<20; i++,j++) {
      locate(i+1,0);
      printf("%c - %s",i+'A',PBook[i].name);
      locate(i+1,41-strlen(PBook[i].number));
      printf(PBook[i].number);
      locate(i+1,44);
      printf("%4d %c %d %d %c\n",Baudtable[PBook[i].pbaudindex],PBook[i].pparity,
         PBook[i].pdatabits,PBook[i].pstopbits,PBook[i].echo?'H':'F');
   }   
}

loadnos()
{
   static unsigned amount;
   static int fd, result;
   static int sectors;

   result = NERROR;
   amount = 20 * sizeof(struct phonebook);
   sectors = roundup(amount,128);
   if (fd = fopen(Phonefile,"rb")) {
      read(fd,PBook,sectors*128);
      result = OK;
      fclose(fd);
   }
   return result;
}      

roundup(dividend,divisor)
int dividend, divisor;
{
   return (dividend/divisor + ((dividend%divisor) ? 1 : 0));
}

locate(r,c)
int r, c;
{
   printf("\033Y%c%c",r+32,c+32);
}

/*
/* exec: function to chain to another C-generated com file, with
/*	 text argument passing.
/* Calling sequence:
/*	 exec(prog, args);
/*	 char *prog, *args;
/* where
/*	 prog is the name of the program being executed next
/*	 args is a pointer to a string of arguments separated by
/*	   blanks or tabs.  Embedded blanks within the arguments are
/*	   not allowed, unless the called program does not use the
/*	   default FCB parameters (and most don't) and can parse the
/*	   command line parameter list itself (like C80 programs can).
*/
exec() {
#ifdef   C80
#asm
	JMP @exec
;
;	CP/M memory pointers
;
@BASE	   EQU 0000H	;either 0 or 4200h for CP/M systems
@FCB	   EQU @BASE+5CH	;default file control block
@TBUFF	EQU @BASE+80H	;sector buffer
@BDOS	   EQU @BASE+5	;bdos entry point
@TPA	   EQU @BASE+100H	;transient program area
@ERRV	   EQU 255 	;error value returned by bdos calls
;
;	CP/M BDOS CALL MNEMONICS
;
@OPENC	EQU 15		;open a file
@READS	EQU 20		;read a sector (sequential)
@SDMA	   EQU 26		;set dma
;
;	Argument pointers
;
@ARGS:	DS 0
@ARG1:	DS 2
@ARG2:	DS 2
@ARG3:	DS 2
@ARG4:	DS 2
@ARG5:	DS 2
@ARG6:	DS 2
;
@exec:
	LXI   H,4
	DAD   SP
	MOV   E,M
	INX   H
	MOV   D,M 	   ;DE points to program name now
	LXI   H,-60
	DAD   SP		   ; compute &newfcb for use here
	PUSH  H		   ; save for much later (will pop into bc)
	PUSH  H		   ;make a few copies for local use below
	PUSH  H
	CALL  x?fcb##	;set up com file for exec-ing
	POP   H		   ;get new fcb addr
	LXI   B,9 	   ;set extension to com
	DAD   B
	MVI   M,'C'
	INX   H
	MVI   M,'O'
	INX   H
	MVI   M,'M'
	POP   D		      ;get new fcb addr again
	MVI   C,@OPENC	   ;open the file for reading
	CALL  @BDOS
	CPI   @ERRV
	JNZ   @NOERR
	POP   H		      ;if can't (like it doesn't exist), return -1
	LXI   H,-1
	DB    0C9H        ; return instruction

@NOERR:  
   LXI   H,4 	      ;get args pointer
	DAD   SP
	CALL  h@##	      ;HL = *HL
	CALL  @SPARG	   ;separate them into individual strings
	LHLD  @ARG1
	MOV   A,H
	ORA   L
	JNZ   @EXCL0
	LXI   D,@ARG1	   ;no arguments -- create a blank FCB
	PUSH  D		      ;call x?fcb with null string
	LXI   H,@FCB
	CALL  x?fcb
	POP   H
	JMP   @EXCL6

@EXCL0: 
   XCHG
	LXI   H,@FCB
	CALL  x?fcb	      ;stick first param into default FCB slot
	LHLD  @ARG2	      ;and stick second param string
	MOV   A,H
	ORA   L
	JNZ   @EXCL6
	LXI   H,@ARG2

@EXCL6: 
   XCHG		         ;into second default fcb slot
	LXI   H,@FCB+16
	CALL  x?fcb
	LXI   D,@TBUFF+1	 ;now construct command line:
	LXI   H,4
	DAD   SP		   ;HL points to arg string pointer
	CALL  h@ 	   ;HL points to arg string
	MVI   B,0 	   ;char count for com. line buf.
	MOV   A,H 	   ;are there any arguments?
	ORA   L
	JZ    @EXCL9
	ORA   M		   ; (Bug fix 7/83 WB)
	JNZ   @EXCL5
@EXCL9: 
   STAX  D		;no--zero TBUFF and TBUFF+1
	JMP   @EXCL2
@EXCL5:  
   MVI   A,' '	;yes--start buffer off with a ' '
	STAX  D
	INX   D
	INR   B
@EXCL1: 
   MOV   A,M 	      ;now copy argument string to command line
	CALL  NAM@U##	   ;make sure they're upper case
	STAX  D
	INX   D
	INX   H
	INR   B
	ORA   A
	JNZ   @EXCL1
	DCR   B

@EXCL2: 
   LXI   H,@TBUFF	   ;set length of command line
	MOV   M,B 	      ;at location tbuff

	LXI   D,@CODE0	   ;copy loader down to end of tbuff
	LXI   H,@TPA-42
	MVI   B,42	      ;length of loader
@EXCL4:  LDAX D
	MOV   M,A
	INX   D
	INX   H
	DCR   B
	JNZ   @EXCL4

	POP   B			   ;get back working fcb pointer
	LHLD  @BASE+6
	SPHL
	LXI   H,@BASE
	PUSH  H			   ;set base of ram as return addr
	JMP   @TPA-42		;(go to `CODE0:')
;
; THIS LOADER CODE IS NOW: 42 BYTES LONG.
;
@CODE0:
   LXI   D,@TPA	;destination address of new program
@CODE1: 
   PUSH  D			;push dma addr
	PUSH  B			;push fcb pointer
	MVI   C,@SDMA		;set dma address for new sector
	CALL  @BDOS
	POP   D			;get pointer to working fcb in de
	PUSH  D			;and re-push it
	MVI   C,@READS		;read a sector
	CALL  @BDOS
	POP   B			;restore fcb pointer into bc
	POP   D			;and dma address into de
	ORA   A			;end of file?
	JZ    @TPA-8		;if not, get next sector (goto `CODE2:')
	MVI   C,@SDMA		;reset dma pointer
	LXI   D,@TBUFF
	CALL  @BDOS
	JMP   @TPA		;and go invoke the program

@CODE2: 
   LXI   H,80H		; bump dma address
	DAD   D
	XCHG
	JMP   @TPA-39		;and go loop (at CODE1)
;
; this routine takes the string pointed to by HL,
; seperates it into non-white strings,
; and places them contiguously in array ARGST.
; also places pointers to these individual strings in ARGS
;
@SPARG: 
   XCHG		      ;DE = original string
	LXI   B,@ARGST	;BC = new string (w/ each substr 0-terminated)
	LXI   H,@ARGS	;HL = pointer to ARGS space
@SEP0:
	DCX   D
@SEP1:
	INX   D		;scan over white space
	LDAX  D
	CPI   ' '
	JZ    @SEP1
	CPI   9
	JZ    @SEP1
	CPI   0		; char = 0?
	JZ    @SPRET	; yes -- return
	MOV   M,C 	; no -- store local pointer at proper args
	INX   H
	MOV   M,B 	;argsn = BC
	INX   H
@TOWSP:
   STAX  B		;store non-white
	INX   B
	INX   D		;now scan to next white space
	LDAX  D
	CPI   0
	JZ    @SEP2
	CPI   ' '
	JZ    @SEP2
	CPI   9
	JNZ   @TOWSP
@SEP2:
	XRA   A
	STAX  B		;store 0 to terminate this string
	INX   B
	JMP   @SEP0	; and loop
@SPRET:
   MOV   M,A 	;set last argn to 0 and return
	INX   H
	MOV   M,A
	DB    0C9H  ; return instruction
@ARGST: DS 100
#endasm
#endif
}

/************************* END OF HCONFIG1 **********************************/

