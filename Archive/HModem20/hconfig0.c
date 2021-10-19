/*************************************************************************/
/*                                                                       */
/*                      H M O D E M   II                                 */  
/*                For H/Z89 and H8/H19 Z80 Systems                       */
/*                  Developed by Harold D. Maney                         */
/*                    510 Barrack Hill Road                              */
/*                    Ridgefield, CT 06877                               */
/*                                                                       */
/*                    GEnie address HMANEY                               */
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  3 December 1984     Written by Harold D. Maney                       */
/*  1 November 1987     Improved for Linda's 2400 bps modem              */
/*  20 November 1987    Greatly improved for Linda                       */
/*  29 November 1987    Added YMODEM, ZMODEM support                     */
/*  12 May 1988         Released HModem II Ver 2.05.05 as shareware      */
/*  15 May 1988         Released source code for HModem II               */
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  This source code may be distributed freely without restriction       */
/*  for the express purpose of promoting the use of the ZMODEM           */
/*  file transfer protocol.  Programmers are requested to include        */
/*  credit (in the source code) to the developers for any code           */
/*  incorporated from this program.                                      */
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  This program was inspired by lmodem.c written by David D. Clark      */
/*  in the November 83 issue of Byte.  Although it bears no resemblance  */
/*  to David's program, I can credit him with sparking my interest in    */
/*  modem programs written in C.                                         */
/*                                                                       */
/*  Hmodem is somewhat machine specific.  It has been set up to run on   */
/*  H89 computers with the 8250 ACE.  It is tailored to use the H19/89   */
/*  function keys for commands.  The Heath system tick counter is used   */
/*  for timing.                                                          */
/*                                                                       */
/*  Compile and assemble with Software Toolworks C/80 ver 3.0 or later.  */
/*  The optional Mathpak is required to support long integers.           */
/*                                                                       */
/*  The following files comprise HModem II:                              */
/*                                                                       */
/*    hmodem80.h        the header file                                  */
/*    hm0.c             the main program                                 */
/*    hm1.c-hm6.c       everything but x/y/zmodem                        */
/*    hm7.c             Chuck Forsberg's sz.c modified for cp/m          */
/*    hm8.c             Chuck Forsberg's rz.c modified for cp/m          */
/*    hzm.c             Chuck Forsberg's zm.c modified for cp/m          */
/*    zmodem.h          zmodem header file                               */
/*    hconfig0.c        configuration overlay                            */
/*    hconfig1.c        configuration overlay                            */
/*    hconfig.h         configuration overlay header                     */
/*    hm.sub            sample submit file for compiling and linking     */
/*                                                                       */
/*    Hints for using C80:                                               */
/*                                                                       */
/*      1.  You must use C80 with M80/L80.                               */
/*      2.  If you need to speed the program up or make it smaller,      */
/*          convert all structures to simple variables.                  */
/*      3.  Always use static storage rather than automatic.             */
/*                                                                       */
/*************************************************************************/

/*********************** START OF HCONFIG0 *******************************/

#define  MAIN
#define  C80

#include "hmodem80.h"
#include "hconfig.h"

main(argc,argv)
int argc;
char *argv[];
{
   int c, d;
   int cfgchanged, phonechanged;

   printf("\033x7");  /* alternate keypad */
   Cmode = 0;
   cfgchanged = phonechanged = FALSE;
   getconfig();
start:
   cls();
   printf("\t%s\n\n",Version);
   printf("\t\t\033pCONFIGURATION MENU\033q\n\n");
   printf("\tA - Edit phone number list\n");
   printf("\tB - Edit long distance access number\n");
   printf("\tC - Edit keyboard macros\n");
   printf("\tD - Set line parameters\n");
   printf("\tE - Set modem parameters\n");
   printf("\tF - Set host mode parameters\n");
   printf("\tG - Set file transfer parameters\n");
   printf("\tH - Set system parameters\n");
   printf("\tZ - Exit\n");
   printf("\nPlease enter your choice:  ");
   flush();
   c = toupper(getchar());
   switch (c) {

      case 'A':
         phonedit();
         phonechanged = TRUE;
         break;

      case 'B':
         ldedit();
         cfgchanged = TRUE;
         break;

      case 'C':
         edit();
         cfgchanged = TRUE;
         break;

      case 'D':
         setline();
         cfgchanged = TRUE;
         break;

      case 'E':
         setmodem();
         cfgchanged = TRUE;
         break;

      case 'F':
         sethost();
         cfgchanged = TRUE;
         break;

      case 'G':
         settransfer();
         cfgchanged = TRUE;
         break;

      case 'H':
         setsys();
         cfgchanged = TRUE;
         break;

      case ESC:
      case 'Z':
         if (cfgchanged)
            saveconfig();
         if (phonechanged)
            savephone();
         if (argc > 1) {
            printf("\nConfiguration complete, returning to HModem...");
            exec(argv[1],"hconfig");
         }
         printf("\033y7");  /* normal keypad */
         exit(0);
         break;

      default:
         printf("\007\nInvalid Entry\n");
         wait(1);
         break;
   }
   goto start;
}

settransfer()
{
   int c;

start:
   cls();
   printf("\tA - Set Checksum/CRC default - %s\n",Crcflag?"CRC":"Checksum");
   printf("\tB - Toggle X-on/X-off protocol - %s\n",XonXoff?"Enabled":"Disabled");
   printf("\tC - Change Zmodem receive window size - %d\n",Zrwindow);
   printf("\tD - Change default drive for downloads - %c:\n",Datadrive);
   printf("\tZ - Exit\n\n");
   printf("\tPlease enter your selection:  ");
   c = toupper(getchar());
   putchar('\n');
   switch (c) {
   case 'A':
      Crcflag = !Crcflag;
      break;      

   case 'B':
      XonXoff = !XonXoff;
      break;

   case 'C':
      gnewint("window size",&Zrwindow);
      break;

   case 'D':
      printf("\n\nEnter new default download drive:  ");
      if (isalpha(c=getchar()))
         Datadrive = toupper(c);
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

setsys()
{
   int c;

start:
   cls();
   printf("\tA - Set print buffer size - %d bytes\n",Pbufsiz);
   printf("\tB - Set tick counter address - %d decimal\n",Ticptr);
   printf("\tC - Set serial interrupt number - %d\n",Intlevel);
   printf("\tZ - Exit\n\n");
   printf("\tPlease enter your selection:  ");
   c = toupper(getchar());
   putchar('\n');
   switch (c) {
   case 'A':
      gnewint("print buffer size",&Pbufsiz);
      Pbufsiz = Pbufsiz < 1 ? 512 : Pbufsiz;
      break;      

   case 'B':
      gnewint("tick counter address (decimal)",&Ticptr);
      break;

   case 'C':
      gnewint("interrupt number",&Intlevel);
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

char *Mdmstrings[] = {
   "Modem init string.....",
   "Dialing command.......",
   "Connect string........",
   "No Connect string 1...",
   "No Connect string 2...",
   "No Connect string 3...",
   "No Connect string 4...",
   "Hangup string.........",
   "Redial timeout delay..",
   "Redial pause delay...."
};

setmodem()
{
   int c;

start:
   cls();
   printf("\t\tA - %s%s\n",Mdmstring[0],Modem.init);
   printf("\t\tB - %s%s\n",Mdmstring[1],Modem.dialcmd);
   printf("\t\tC - %s%s\n",Mdmstring[2],Modem.connect);
   printf("\t\tD - %s%s\n",Mdmstring[3],Modem.busy1);
   printf("\t\tE - %s%s\n",Mdmstring[4],Modem.busy2);
   printf("\t\tF - %s%s\n",Mdmstring[5],Modem.busy3);
   printf("\t\tG - %s%s\n",Mdmstring[6],Modem.busy4);
   printf("\t\tH - %s%s\n",Mdmstring[7],Modem.hangup);
   printf("\t\tJ - %s%d\n",Mdmstring[8],Modem.timeout);
   printf("\t\tK - %s%d\n",Mdmstring[9],Modem.pause);
   printf("\t\tZ - Exit\n\n");
   printf("\tPlease enter your selection:  ");
   c = toupper(getchar());
   putchar('\n');
   switch (c) {
   case 'A':
      gnewstr(Mdmstring[0],Modem.init);
      break;      

   case 'B':
      gnewstr(Mdmstring[1],Modem.dialcmd);
      break;      

   case 'C':
      gnewstr(Mdmstring[2],Modem.connect);
      break;      

   case 'D':
      gnewstr(Mdmstring[3],Modem.busy1);
      break;      

   case 'E':
      gnewstr(Mdmstring[4],Modem.busy2);
      break;      

   case 'F':
      gnewstr(Mdmstring[5],Modem.busy3);
      break;      

   case 'G':
      gnewstr(Mdmstring[6],Modem.busy4);
      break;      

   case 'H':
      gnewstr(Mdmstring[7],Modem.hangup);
      break;      

   case 'J':
      gnewint(Mdmstring[8],&Modem.timeout);
      break;      

   case 'K':
      gnewint(Mdmstring[9],&Modem.pause);
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

gnewint(prompt,intp)
char *prompt;
int *intp;
{
   static char *temp;

   temp = Pathname;
   printf("\n\nEnter new %s:  ",prompt);
      if (getline(temp,20))
         *intp = atoi(temp);
}

gnewstr(prompt,mstring)
char *prompt, *mstring;
{
   char *temp;

   temp = Pathname;
   printf("\n\nEnter new %s:  ",prompt);
      if (getline(temp,20))
         strcpy(mstring,temp);
}

setline()
{
   int c;

start:
   cls();
   printf("\t\tA - Bits per second.......%d\n",Baudtable[Line.baudindex]);
   printf("\t\tB - Parity................%c\n",Line.parity);
   printf("\t\tC - Number data bits......%d\n",Line.databits);
   printf("\t\tD - Number stop bits......%d\n",Line.stopbits);
   printf("\t\tE - Serial port address...%oQ\n",Line.port);
   printf("\t\tF - Duplex mode...........%s DUPLEX\n",FDx?"FULL":"HALF");
   printf("\t\tZ - Exit\n\n");
   printf("\tPlease enter your selection:  ");
   c = toupper(getchar());
   cls();
   switch (c) {
   case 'A':
      setbaud();
      break;      

   case 'B':
      setparity();
      break;

   case 'C':
      setdatabits();
      break;      

   case 'D':
      setstopbits();
      break;      

   case 'E':
      setaddress();
      break;

   case 'F':
      FDx = !FDx;
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

setparity()
{
   int c;

   cls();
   do {
      printf("(N)o parity, (O)dd parity, or (E)ven parity?  ");
   } while ((c=toupper(getchar())) != 'N' && c != 'O' && c != 'E' && c != 0);
   if (c)
      Line.parity = c;
}

setdatabits()
{
   int c;

   cls();
   do {
      printf("(7) data bits or (8) data bits?  ");
   } while ((c=(getchar()-'0')) != 7 && c != 8 && c != 0);
   if (c)
      Line.databits = c;
}
   
setstopbits()
{
   int c;

   cls();
   do {
      printf("(1) stop bit or (2) stop bits?  ");
   } while ((c = (getchar()-'0')) != 1 && c != 2 && c != 0);
   if (c);
      Line.stopbits = c;
}

setaddress()
{
   int c, i, length;
   char *buffer;

   buffer = Pathname;
start:
   cls();
   printf("Enter serial port address in octal:  ");
   if (!getline(buffer,4))
      return;
   length = i = strlen(buffer) - 1;
   while (i >= 0) {
      c |= (buffer[i]-'0') << (3*(length-i));
      i--;
   }
   if (c) {
      Line.port = c;
      Line.lstatus = c + 5;
      Line.mstatus = c + 6;
      return;
   }
   printf("\nInvalid Entry\n");
   wait(1);
   goto start;
}

/************************* END OF HCONFIG0 **********************************/

