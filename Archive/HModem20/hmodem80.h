/*************************** hmodem80.h *************************************/

#ifdef   C80
#include "e:printf.h"
#endif

#ifndef        MAIN
#define        EXTERN   extern
#else
#define        EXTERN
#endif
                                    
#define        PBUFSIZ         2048    /*2k printer buffer*/
#define        DFLTPORT        0330    /*default modem port*/
#define        DFLTINDEX       7       /* pointer to default baud */
#define        BUFSTART        16384 /*16k text buffer starting size*/
#define        DIRCTIO         6       /*cpm bdos direct-console io command*/
#define        INPUT           0xff    /*direct-console io input*/
#define        FALSE           0
#define        TRUE            1
#define        OK              0
#define        NERROR         (-1)
#define        NULL            0
#define        ERROR           0
#define        GETCUR          25      /* bdos get current disk command */
#define        KEYHIT          0
#define        MAXFILES        255	/* max number of expanded files */
#define        FNSIZE          15   /* filename: 2(A:)+8+1+3+null */

/**************************************************************************/
/*                                                                        */
/*      Miscellaneous ASCII characters.                                   */
/*                                                                        */
/**************************************************************************/

#define        LF              10
#define        CR              13
#define        CTRLZ           26      /*end of text-file character*/

/*************************************************************************/
/*                                                                       */
/*      These #defines determine which keys will be interpreted as       */
/*  command characters.                                                  */
/*                                                                       */
/*************************************************************************/

#define        VT52            '/'     /*code to indicate VT52 seq*/
#define        F1              'S'
#define        F2              'T'
#define        F3              'U'
#define        F4              'V'
#define        F5              'W'
#define        ERASE           'J'
#define        BLUE            'P'
#define        RED             'Q'
#define        WHITE           'R'
#define        LEVELFLAG       0xff00
#define        RECEIVE         F1      /*receive file*/
#define        CAPTURE         F2      /*toggle capture mode*/
#define        DIR             F3      /*get disk directory*/
#define        PRTSCRN         F4      /*print screen*/
#define        SEND            F5      /*send file*/
#define        HANGUP          ERASE   /*erase key to hangup*/
#define        COMMAND         BLUE    /*change command mode*/
#define        HELP            RED     /*get instructions*/
#define        QUIT            WHITE   /*quit*/
#define        DIAL            (F1 | LEVELFLAG) /*make phone call*/
#define        HOST            (F2 | LEVELFLAG) /* host mode */
#define        CONFIG          (F3 | LEVELFLAG) /*configure system*/
#define        TOGPRT          (F4 | LEVELFLAG) /*toggle printer*/
#define        DISK            (F5 | LEVELFLAG)  /*reset disk, erase file*/
#define        CHELP           (RED | LEVELFLAG)
#define        CCOMMAND        (BLUE | LEVELFLAG)
#define        CHANGUP         (ERASE | LEVELFLAG)
#define        CQUIT           (WHITE | LEVELFLAG)

/******************* constants used in file transfer **********************/

/* Ward Christensen / CP/M parameters - Don't change these! */

#define        SOH            1
#define        ENQ            5
#define        CAN            ('X'&037)
#define        XOFF           ('s'&037)
#define        XON            ('q'&037)
#define        STX            2
#define        CPMEOF         26
#define        EOF            (-1)
#define        TIMEOUT        (-2)
#define        RCDO           (-3)
#define        ERRORMAX       5
#define        RETRYMAX       10
#define        WCEOT          (-10)
#define        SECSIZ         128	/* cp/m's Magic Number record size */
#define        PATHLEN        257	/* ready for 4.2 bsd ? */
#define        KSIZE          1024	/* record size with k option */
#define        EOT             4
#define        ACK             6
#define        NAK             21
#define        WANTCRC         'C'

#define        TICCNT          0x0b    /*address of H8/H89 timer*/
#define        ORIGIN          0x0100  /*starting point for cp/m programs*/
#define        COMPSIZ         35      /*cpm bdos compute file size command*/
#define        CTRLX           24
#define        ESC             27
#define        COLUMNS         4       /*number of dir columns displayed*/
#define        CPMBUF          0x80    /*address of cpm file buffer*/
#define        SELDSK          14      /*bdos select disk command*/
#define        SFF             17      /*bdos search for first command*/
#define        SFN             18      /*bdos search for next command*/
#define        SETDMA          26      /*bdos set dma address */
#define        RESET           13      /*bdos reset disk command*/
#define        GETDPB          31      /*get disk paramenter block addr*/
#define        GETALL          27      /*get allocation vector*/
#define        GCS             11      /*get console status*/
#define        DONE            0xff    /*flag for no more entries*/
#define        BS              8
#define        DEL             127
#define        CTRLE           5
#define        RET             '!'     /*symbol for CR string terminator */
#define        WAITASEC        '~'     /* one second delay */
#define        BIOSADDR        1       /*ptr to base of bios + 3*/
#define        LISTST          42      /*index to list status routine*/
#define        MBUFSIZ         1024    /*size of modem input buffer*/
#define        ENABLE          0xfb    /*code for EI*/
#define        RTN             0xc9    /*code for RET*/
#define        JUMP            0xc3    /*code for JMP*/
#define        CONIN           1       /*bdos call for console input*/
#define        OUTSTAT         040         /*ready-for-output status*/
#define        INSTAT          001         /*ready-for-input status*/
#define        CONNECTSTAT     0x80        /* carrier detect */
#define        NOPARITY        0x00        /* line settings */
#define        EVENPARITY      0x18
#define        ODDPARITY       0x08
#define        SEVENBITS       0x02
#define        EIGHTBITS       0x03
#define        ONEBIT          0x00
#define        TWOBITS         0x40
#define        CTRLS           19          /* xoff */
#define        CTRLQ           17          /* xon */

/************************ configuration variables **********************************/

EXTERN int BFlag
#ifdef   MAIN
= FALSE
#endif
;
EXTERN int PFlag
#ifdef   MAIN
= FALSE
#endif
;
EXTERN int FDx
#ifdef   MAIN
= TRUE
#endif
;
EXTERN int Crcflag
#ifdef   MAIN
= TRUE
#endif
;
EXTERN int XonXoff
#ifdef   MAIN
= FALSE
#endif
;
EXTERN char Msgfile[20]
#ifdef   MAIN
= "HMODEM.MSG"
#endif
;
EXTERN char Phonefile[20]
#ifdef   MAIN
= "HMODEM.FON"
#endif
;
EXTERN char Logfile[20]
#ifdef   MAIN
= "HMODEM.LOG"
#endif
;
EXTERN char Cfgfile[20]
#ifdef   MAIN
= "HMODEM.CFG"
#endif
;
EXTERN char KbMacro[12][22]
#ifdef   MAIN
 = { "Macro Key 0!",
     "Macro Key 1!",
     "Macro Key 2!",
     "Macro Key 3!",
     "Macro Key 4!",
     "Macro Key 5!",
     "Macro Key 6!",
     "Macro Key 7!",
     "Macro Key 8!",
     "Macro Key 9!",
     "Macro Key .!",
     "Macro Key ENTER!" }
#endif
;

EXTERN struct modemparms {
   char init[20];
   char dialcmd[8];
   char dialsuffix[8];
   char connect[20];
   char busy1[20];
   char busy2[20];
   char busy3[20];
   char busy4[20];
   char hangup[20];
   int timeout;
   int pause;
} Modem
#ifdef   MAIN
   = {
   "ATE0V1X4S0=0!",
   "ATDT",
   "!",
   "CONNECT",
   "BUSY",
   "NO CARRIER",
   "NO RESPONSE TO DIAL",
   "ERROR",
   "~+++~ATH0!",
   40,8
   }
#endif
;

EXTERN struct hostparms {
   char welcome[40];
   char autoanswer[20];
   char password[20];
   char user[20];
   int modemconnection;
} Host
#ifdef MAIN
= {
   "Welcome to HModem Host!",
   "ATS0=2!",
   "PASSWORD",
   "null",
   TRUE }
#endif
;

EXTERN struct settings {
   int cbaudindex;
   char cparity;
   int cdatabits;
   int cstopbits;
} Current;
         
EXTERN struct lineparms {
   int baudindex;
   char parity;
   int databits;
   int stopbits;
   int port;
   int lstatus;
   int mstatus;
} Line
#ifdef MAIN
= { DFLTINDEX,'N', 8, 1, DFLTPORT, DFLTPORT+5, DFLTPORT+6 }
#endif
;
EXTERN char Mci[20];
EXTERN char Sprint[20];
EXTERN int Zmodem
#ifdef MAIN
 = FALSE
#endif
;
EXTERN int Nozmodem
#ifdef MAIN
 = FALSE
#endif
;
EXTERN int Blklen
#ifdef MAIN
 = SECSIZ
#endif
;
EXTERN int Xmodem
#ifdef MAIN
 = FALSE
#endif
;
EXTERN int Zrwindow	      /* RX window size (controls garbage count) */
#ifdef MAIN
 = 1400
#endif
;
EXTERN unsigned Bufsize
#ifdef   MAIN
= BUFSTART
#endif
;
EXTERN unsigned Pbufsiz
#ifdef   MAIN
= PBUFSIZ
#endif
;
EXTERN unsigned *Ticptr
#ifdef   MAIN
= TICCNT
#endif
;
EXTERN int Intlevel
#ifdef   MAIN
= 5
#endif
;
EXTERN int Datadrive
#ifdef   MAIN
= 'A'
#endif
;
EXTERN char Waste[128];     /* pad to allow config to be multiple of 128 */

/************************ global variables **********************************/

EXTERN struct phonebook {
   char name[18];
   char number[18];
   int pbaudindex;        
   char pparity;
   int pdatabits;
   int pstopbits;
   int echo;
} *Book;

EXTERN unsigned Baudtable[11]
#ifdef MAIN
 = { 50,75,110,300,450,600,1200,2400,4800,9600,19200 }
#endif
;
EXTERN unsigned Divtable[11]
#ifdef MAIN
 = { 2304,1536,1047,384,256,192,96,48,24,12,6 }
#endif
;
EXTERN int QuitFlag
#ifdef   MAIN
= FALSE
#endif
;
EXTERN char Pathname[128];
EXTERN char Buf[128];                /* general purpose buffer */
EXTERN char *MainBuffer;
EXTERN char *Prtbuf;
EXTERN unsigned Buftop;
EXTERN unsigned TxtPtr
#ifdef   MAIN
= 0
#endif
;
EXTERN char Stline[]
#ifdef   MAIN
= "\033x5\033j\033x1\033Y8 "
#endif
;
EXTERN char Endline[]
#ifdef   MAIN
= "\033k\033y5"
#endif
;
EXTERN char Vl[]          /* graphics vertical line */
#ifdef   MAIN
 = "\033F`\033G"
#endif
;
EXTERN char *Mdmhead;
EXTERN char *Mdmtail;
EXTERN char *Mdmtop;
EXTERN char *Mdmbottom;
EXTERN char *Prthead;
EXTERN char *Prttail;
EXTERN char *Prttop;
EXTERN char *Prtbottom;
EXTERN int Stopped
#ifdef MAIN
 = FALSE
#endif
;
EXTERN int Inhost
#ifdef MAIN
 = FALSE
#endif
;
EXTERN char *MdmInBuffer;
EXTERN char Version[]
#ifdef MAIN
=  "Version 2.06.00 (CP/M 2.2) 12 May 1988"
#endif
;
EXTERN char Lastlog[20];
extern char Cmode;
struct stat {
   char fname[9];
   char fext[4];
   int dcode;
   int records;
};
EXTERN char **Pathlist;
EXTERN int Hlap
#ifdef MAIN
 = 0
#endif
;
EXTERN int Tlap
#ifdef MAIN
 = 0
#endif
;
EXTERN int Mdmport;
EXTERN int Mdmstat;
EXTERN int Linestat;
EXTERN int Dialing;
EXTERN int Lastkey;
EXTERN int Invokdrive, Currdrive;

/**************************** end of hmodem80.h ****************************/
