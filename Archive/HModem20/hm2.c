/***************************** Module 2 ************************************/

#define  C80
#include "hmodem80.h"

cntbits(byte)
char byte;
{
   static int i,count;

   for (count=i=0; i<8; i++) {
      count += (byte & 1);
      byte >>= 1;
   }
   return count;
}

reset(drive)
unsigned drive;
{
   drive = toupper(drive);
   if (isalpha(drive) && drive <= 'F') {
      Currdrive = drive;
      bdos(RESET,NULL);
      bdos(SELDSK,(Currdrive-'A')&0xff);
   }
}

addatadrive(filename)
char filename[];        /* must have room for at least 15 chars */
{
   addrive(filename,Datadrive);
}

addrive(filename,drive)
char *filename;
int drive;
{
   if (!isin(filename,":")) {
      strcpy(Buf,filename);
      filename[0] = (char)drive;
      sprintf(filename+1,":%s",Buf);
   }
}

sendout(prot)
int prot;
{
   static int kbdata, count, result;

   result = NERROR;
   if (!prot)
      kbdata = protocol(TRUE);
   else
      kbdata = prot;
   if (Inhost) {
      if (count=getpathname("(s) for Download"))
         mstrout("\nBegin your Download procedure...",TRUE);
   }
   else 
      count = getpathname("(s) for Transmit");
   if (count) {
      switch(kbdata) {
      case 'K':
         Blklen = KSIZE;
         Xmodem = TRUE;
         Zmodem = FALSE;
         result = wcsend(1,Pathlist);
         break;
      case 'X':
         Blklen = 128;
         Xmodem = TRUE;
         Zmodem = FALSE;
         result = wcsend(1,Pathlist);
         break;
      case 'Y':
         Zmodem = Xmodem = FALSE;
         result = wcsend(count,Pathlist);
         break;
      case 'Z':
         Zmodem = TRUE;
         Xmodem = FALSE;
         result = wcsend(count,Pathlist);
         break;
      case 'A':
         result = asciisend(Pathlist[0]);
         break;
      default:
         result = !OK;
         break;
      }
   }
   freepath(count);
   printf("\nTransfer %s\n",result==OK?"Successful":"Aborted");
   flush();
   return result;
}

bringin(prot)
int prot;
{
   static int kbdata, count, result;

   count = 0;
   result = NERROR;
   if (!prot)
      kbdata = protocol(FALSE);
   else
      kbdata = prot;
   switch(kbdata) {
   case 'X':
      if (Inhost) {
         if (count=getpathname(" to Upload"))
            mstrout("\nBegin your Upload procedure...",TRUE);
      }
      else 
         count = getpathname(" to Receive");
      if (!count)
         break;
      Zmodem = FALSE;
      Nozmodem = Xmodem = TRUE;
      result = wcreceive(Pathlist[0]);      /* just one file */
      break;
   case 'Y':
      Zmodem = Xmodem = FALSE;
      Nozmodem = TRUE;
      if (Inhost)
         mstrout("\nBegin your Upload procedure...",TRUE);
      result = wcreceive(NULL);
      break;
   case 'Z':
      Zmodem = TRUE;
      Nozmodem = Xmodem = FALSE;
      if (Inhost)
         mstrout("\nBegin your Upload procedure...",TRUE);
      result = wcreceive(NULL);
      break;
   default:
      break;
   }
   freepath(count);  /* should be just 1 */
   printf("\nTransfer %s\n",result==OK?"Successful":"Aborted");
   flush();
   return result;
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

/* command: expand wild cards in the command line.  (7/25/83)
 * usage: command(&argc, &argv) modifies argc and argv as necessary
 * uses sbrk to create the new arg list
 * NOTE: requires makfcb() and bdos() from file stdlib.c.  When used
 *	with a linker and stdlib.rel, remove the #include stdlib.c.
 *
 * Written by Dr. Jim Gillogly; Modified for CP/M by Walt Bilofsky.
 * Modified by HM to just get ambiguous fn for zmodem, ymodem.
 */

int COMnf,*COMfn,COMc,*COMv;
char *COMarg,*COMs;
static expand();

command(argcp,argvp)
int *argcp,*argvp;
{
	int f_alloc[MAXFILES];

	COMfn = f_alloc;
	COMc = *argcp;
	COMv = *argvp;
	COMnf = 0;
	for (COMarg = *COMv; COMc--; COMarg = *++COMv) {
#ifdef   DEBUG
   printf("\nDoing %s",COMarg);
#endif
   	for (COMs = COMarg; *COMs; COMs++)
			if (*COMs == '?' || *COMs == '*') {	
            expand();
				goto contn;  /* expand each name at most once */
			}
		COMfn[COMnf] = alloc(FNSIZE);
      strcpy(COMfn[COMnf++],COMarg);     /* no expansion */
      contn:;
	}
	*argcp = COMnf;
	COMfn[COMnf++] = -1;
	COMv = *argvp = alloc(2 * COMnf);
	while (COMnf--) 
      COMv[COMnf] = COMfn[COMnf];
}

static expand()
{
	char fcb[36];
	static char *p,*q;
	static int i,flg;

#ifdef   DEBUG
   printf("\nExpanding %s",COMarg);
#endif
	makfcb(COMarg,fcb);
	if (fcb[0] == -1) 
      fcb[0] = '?'; 	/* Check for all users */
	for (i = flg = 1; i <= 11; ++i) {	/* Expand *'s */
		if (i == 9) 
         flg = 1;
		if (fcb[i] == '*') 
         flg = 0;
		if (flg == 0) 
         fcb[i] = '?'; 
   }
	flg = 17;
	bdos(26,0x80);				/* Make sure DMA address OK */
	while ((i = bdos(flg,fcb)) != -1) {
		COMfn[COMnf++] = q = alloc(FNSIZE);
		if (COMnf >= MAXFILES-1) {
			for (p = "Too many file names.\n"; putchar(*p++); );
			exit(0); }
		p = 0x81 + i * 32;		/* Where to find dir. record */
		if (COMarg[1] == ':' && COMarg[0] != '?') {
			*q++ = COMarg[0]; 
         *q++ = ':';
      }
		for (i = 12; --i; ) {
			if (i == 3) 
            *q++ = '.';
			if ((*q = *p++ & 0177) != ' ') 
            ++q; 
      }
		*q = 0;
		flg = 18;
	}
}

/************************** END OF MODULE 2 *********************************/
