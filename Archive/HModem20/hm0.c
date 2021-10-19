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

#define  MAIN
#define  C80

#include "hmodem80.h"

main(argc,argv)
int argc;
char *argv[];
{
   static unsigned nextfunction, addr;
   static int i, count;
   static int kbdata, mdmdata;
   static char keypad[] = "pqrstuvwxynM";
   static char keybuf[2];
   static unsigned commandflag = 0;
   static int dolabel, result;

   Cmode = 0;
   if (!parmatch(argc,argv,"HCONFIG")) 
      title();
   printf("\033G\033x7"); /* no graphics mode and select alternate keypad */
   getconfig();
   initvector();                 /* set up interrupt vector */
   initializemodem();
   cls();
   tlabel();
   Invokdrive = bdos(GETCUR,NULL) + 'A';
   Currdrive = Datadrive;
   reset(Currdrive);
   printf("READY\n");
   showcurs();

   /************** the main loop ************************/

   while (TRUE) {
      if (kbdata = getch())  {         /* get any char at kbd */
         if (nextfunction = getfunct(kbdata)) {
            dolabel = TRUE;
            flush();
            nextfunction |= commandflag;
            switch (nextfunction) {

            case RECEIVE:
               keep(Lastlog);
               bringin(NULL);
               startcapture();
               break;

            case CAPTURE:
               capturetog(Logfile);
               break;

            case DIR:
               killlabel();
               keep(Lastlog);
               directory();
               startcapture();
               break;

            case PRTSCRN:
               screenprint();
               break;

            case SEND:
               keep(Lastlog);
               sendout(NULL);
               startcapture();
               break;

            case HANGUP:
            case CHANGUP:
               hangup();
               dolabel = FALSE;
               break;

            case COMMAND:
            case CCOMMAND:
               commandflag ^= LEVELFLAG;
               break;

            case DIAL:
               keep(Lastlog);
               dial();
               purgeline();
               startcapture();
               break;

            case HOST:
               keep(Lastlog);
               QuitFlag = FALSE;
               Inhost = TRUE;
               while (!QuitFlag)
                  dohost();
               Inhost = FALSE;
               flush();
               cls();
               startcapture();
               break;

            case CONFIG:
               killlabel();
               cls();
               configure();
               perror("HCONFIG.COM not found");
               break;

            case TOGPRT:
               toggleprt();
               break;

            case DISK:
               diskstuff();
               break;

            case HELP:
            case CHELP:
               help();               
               break;

            case QUIT:
            case CQUIT:
               doexit(QUIT);
               break;

            default:
               dolabel = FALSE;
               keybuf[0] = nextfunction;
               keybuf[1] = '\0';
               if ((i=index(keypad,keybuf)) != -1) {
                  mstrout(KbMacro[i],TRUE);
               }
               break;
            }                            /* end of switch*/
            if (dolabel)
               commandflag ? comlabel() : tlabel();
         }                             /*end of if nextfunction*/
         else {
            mcharout(kbdata);
lfloop:
            tobuffer(kbdata);
            if (!FDx) {
               putchar(kbdata);
               toprinter(kbdata);
               if (kbdata == CR) {
                  kbdata = LF;
                  goto lfloop;
               }
            }
         }
      }                    /*  end of if char at kbd  */

      if (minprdy()) {
         mdmdata = mcharinp();
         putchar(mdmdata);
         tobuffer(mdmdata);
         toprinter(mdmdata);
      }
      prtservice();     /* service printer at the end of each loop */
   }    /* end of while */
}  /* end of main */

tobuffer(c)
int c;
{
   if (BFlag) {
      MainBuffer[TxtPtr++] = (char)c;
      if (TxtPtr > Buftop) {
         keep(Lastlog);
         startcapture();
      }
   }
}

toprinter(c)
int c;
{
   if (PFlag) {
      *Prthead++ = (char)c;
      adjustprthead();
   }
}

keep(filename)
char *filename;
{
   static int fl;

   if (!BFlag)
      return;
   if (!TxtPtr)
      goto cleanup;
   mcharout(CTRLS);
   while (TxtPtr % 128)
      MainBuffer[TxtPtr++] = 0;
   addatadrive(filename);
   if (!(fl = fopen(filename,"ub")) && !(fl = fopen(filename,"wb")))
      openerror(fl,filename);
   else {
      seek(fl,0,2);       
      write(fl,MainBuffer,TxtPtr);
      fclose(fl);
   }
   mcharout(CTRLQ);
   TxtPtr = 0;
cleanup:
   free(MainBuffer);
}

toggleprt()
{
   PFlag = !PFlag;
   if (PFlag) {
      if (getprtbuf() != OK)
         PFlag = FALSE;
   }
   else {
      free(Prtbuf);
   }
}

getprtbuf()
{
   keep(Lastlog);                    /* need to steal some of the buffer */
   Prtbuf = alloc(Pbufsiz);
   if (allocerror(Prtbuf))
      return NERROR;
   Prthead = Prttail = Prtbottom = Prtbuf;
   Prttop = Prtbuf + Pbufsiz - 1;
   startcapture();
#ifdef DEBUG
   printf("\nPrtbuf = %x\n",Prtbuf);
#endif
   return OK;
}

startcapture()     /* allocate capture buffer */
{
   char *grabmem();

   if (!BFlag)
      return;
   MainBuffer = grabmem(&Bufsize);
   Buftop = Bufsize - 1;
   TxtPtr = 0;
#ifdef DEBUG
   printf("\ncapture Bufsize = %u\n",Bufsize);
#endif
}

char *
grabmem(sizep)         /* grab all available memory */
unsigned *sizep;       /* place to store how much we got */
{
   static char *p;
   static unsigned size;

   size = BUFSTART;
   while ((p=alloc(size)) == -1) {
      size -= 1024;
      if (size < 2048) {
         size = 0;
         break;
      }
   }
#ifdef DEBUG
   printf("\ngrabmem = %x %d\n",p,size);
#endif
   *sizep = size;
   return p;
}

protocol(for_send)
int for_send;        /* select block size in transmit only */
{
   static int c;
   static char *buffer;
   
   buffer = Pathname;
   sprintf(buffer,"\n%sXmodem, %sYmodem, or Zmodem? (%sX,%sY,Z) <X>  ",
      for_send ? "ASCII, " : "",
      for_send ? "Xmodem-1k, " : "",
      for_send ? "A," : "",
      for_send ? "K," : "" );
   if (Inhost) {
      mstrout(buffer,TRUE);
      purgeline();
      c = toupper(mgetchar(20));
      if (c == CR)
         c = 'X';
      mcharout(c);
      return c;
   }
   else {
      printf(buffer);
      flush();
      c = toupper(getchar());
      if (c == '\n')
         c = 'X';
   }
   return c;
}

getpathname(string)
char *string;
{
   static int c;
   static char *buffer;

   buffer = Pathname;
   sprintf(buffer,"\nPlease enter file name%s:  ",string);
   if (Inhost) {
      mstrout(buffer,TRUE);
      if (!(c=mgetline(Pathname,PATHLEN,TRUE)))
         return 0;
   }
   else {
      printf(buffer);
      if (!(c=getline(Pathname,PATHLEN)))
         return 0;
   }
   return linetolist();
}

linetolist()   /* expand and put Pathnames in Pathlist, return count */
{
   static char *p;
   static int count, i;
   static char **tempalloc;

   tempalloc = Pathlist = alloc(510);
   if (allocerror(tempalloc))
      return 0;
#ifdef   DEBUG
      printf("Pathlist = %x\n",Pathlist);
#endif
   count = 0;
   Pathlist[count++] = Pathname;
   for (p=Pathname; *p; p++) {         /* break up into substrings */
      if (*p == ' ') {
         *p = '\0';
         while (*++p == ' ');         /* dump extra spaces */
         Pathlist[count++] = p;
      }
   }
#ifdef   DEBUG
   printf("\nbefore command\n");
   for (i=0; i < count; i++)
      printf("%d %s\n",i,Pathlist[i]);
#endif
   command(&count,&Pathlist);
#ifdef   DEBUG
   printf("\nafter command\n");
   for (i=0; i < count; i++)
      printf("%d %s\n",i,Pathlist[i]);
#endif
   free(tempalloc);
   return count;   
}

freepath(n)
int n;
{
   if (n) {
      while (n)
         free(Pathlist[--n]);
      free(Pathlist);
   }
}

configure()
{
   doexit(CONFIG);
}

parmatch(argc,argv,string)
int argc;
char *argv[], *string;
{
   if (argc < 1)
      return FALSE;
   return (!strcmp(string,argv[1]));
}

/***************************END OF MODULE 0*********************************/
