/***************************** Module 6 ************************************/

#define C80
#include "hmodem80.h"

title()
{
   static char line1[] = "H M O D E M   II";
   static char line3[] = "For H/Z89 and H8/H19 Z80 Systems";
   static char line4[] = "Copyright (c) 1988 by Hal Maney";
   static char line5[] = "510 Barrack Hill Road";
   static char line6[] = "Ridgefield, CT 06877";
   static char line7[] = "Press any key";

   cls();
   locate(7,ctr(line1));
   printf(line1);
   locate(9,ctr(Version));
   printf(Version);
   locate(10,ctr(line3));
   printf(line3);
   locate(11,ctr(line4));
   printf(line4);
   locate(12,ctr(line5));
   printf(line5);
   locate(13,ctr(line6));
   printf(line6);
   putlabel(line7);
   hidecurs();
   flush();
   kbwait(10);
}

ctr(p)
char *p;
{
   return max((80 - strlen(p))/2,0);
}

getconfig()
{
   static int fd;
   static unsigned size, sectors;
   static char *p;

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
   Mdmport = Line.port;
   Mdmstat = Line.mstatus;
   Linestat = Line.lstatus;
}

mstrout(string,echo)
char *string;
int echo;             /* echo flag means send to screen also */
{
   static char c;

   while (c = *string++) {
      if ((c == RET) || (c == '\n')) {      /* RET is a ! */
         mcharout(CR);
         mcharout(LF);
         c = '\n';
      }
      else if (c == WAITASEC)
         wait(1);
      else
         mcharout(c);
      if (echo)
         putchar(c);
   }
   mswait(100);      /* wait 100 ms */
   purgeline();
}

capturetog(filename)
char *filename;
{
   if (!BFlag) {
      strcpy(Lastlog,filename);
      BFlag = TRUE;
      startcapture();
      if (allocerror(MainBuffer))
         BFlag = FALSE;
   }
   else {
      keep(Lastlog);
      BFlag = FALSE;
   }      
}

tlabel() /*print level 1 labels on the 25th line*/
{
   killlabel();
   printf(
   "%s1> \033pReceive%s  Log  %s Dir  %sPrScr%s Send%sHangup%s Level %s Help %s",
      Stline,Vl,Vl,Vl,Vl,Vl,Vl,Vl,Vl);
   printf(
   "Quit%s%02d%c%d%d%s%s%s%s%s%s\033q%s",Vl,Baudtable[Current.cbaudindex]/100,
    Current.cparity,Current.cdatabits,Current.cstopbits,Vl,BFlag?"LG":"--",Vl,
    PFlag?"PR":"--",Vl,FDx?"FDX":"HDX",Endline);
}

comlabel() /*print level 2 labels*/
{
   killlabel();
   printf(
   "%s2> \033p   Dial%sHost%sConfigure%sPrint%sDisk%sHangup%s Level %s",
    Stline,Vl,Vl,Vl,Vl,Vl,Vl,Vl);
   printf("Help%sQuit%s   %02d%c%d%d%s%s%s%s%s%s\033q%s",Vl,Vl,
    Baudtable[Current.cbaudindex]/100,Current.cparity,Current.cdatabits,
    Current.cstopbits,Vl,BFlag?"LG":"--",Vl,PFlag?"PR":"--",Vl,
    FDx?"FDX":"HDX",Endline);
}

scplabel()
{
   putlabel("READING THE SCREEN -> Please wait...");
}

slabel() /*print send mode labels on the 25th line*/
{
   putlabel("SEND FILE Mode:  Press ESC to Abort...");
}

rlabel() /*print receive mode labels on the 25th line*/
{
   putlabel("RECEIVE FILE Mode:  Press ESC to Abort...");
}

hlabel() /* host mode label */
{
   putlabel("HOST MODE:  Press ESC to Exit...");   
}

chlabel()  /* chat mode label */
{
   putlabel("CHAT MODE:  Press ESC to Return to Host");   
}

putlabel(string)
char string[];
{
   static char bar[] = 
"                                                                                ";

   killlabel();
   printf("%s\033p%s",Stline,bar);
   locate(24,ctr(string));
   printf("%s%s%s%s","\033p",string,"\033q",Endline);
}

killlabel() /*disable 25th line*/
{
   printf("\033y1");
}

getfunct(c)  /*see if function key was pressed*/
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

asciisend(file)  /* send ascii file with xon/xoff protocol */
char *file;
{
   char *grabmem();
   static int fd, status, bytes, c;
   static char *inbuf;
   static unsigned j, bufsize;

   status = NERROR;
   inbuf = grabmem(&bufsize);
   if (allocerror(inbuf))
      return NERROR;
   fd = fopen(file,"rb");
   if (openerror(fd,file)) {
      free(inbuf);
      return NERROR;
   }
#ifdef   DEBUG
   printf("\nbufsize = %d\n",bufsize);
#endif
   sprintf(Buf,"Sending %s, ASCII Transfer",file);
   putlabel(Buf);
   j = 0;
   QuitFlag = FALSE;
   while (bytes = read(fd,inbuf,bufsize)) {
      c = inbuf[0];
      for (j = 0; (j < bytes) && (c != CTRLZ); c = inbuf[++j]) {
         if (XonXoff && minprdy() && (mcharinp() == CTRLS)) {
            putlabel("\007XOFF Received, waiting for XON");
            while (readline(10) != CTRLQ) {
               if (QuitFlag == TRUE)
                  goto cleanup;
            }
            putlabel(Buf);
         }
         mcharout(c);
         putchar(c);
      }
   }
   status = OK;

cleanup:
   fclose(fd);
   free(inbuf);
   return status;
}

/***************** directory functions *************************************/

struct direntry {
   char userno;
   char flname[8];
   char ftype[3];
   char fextent;
   char reserved[2];
   char record;
   char map[16];
};

struct fcb {
   char drive;
   char filename[8];
   char filetype[3];
   char extent;
   int freserved;
   char recused;
   unsigned abused[8];
   char seqrec;
   unsigned ranrec;
   char ranreco;
} Thefcb;

struct dpb {
   unsigned spt;
   char bsh;
   char blm;
   char exm;
   unsigned dsm;
   unsigned drm;
   char al0;
   char al1;
   unsigned cks;
   unsigned off;
};

directory()
{
   static int j, factor, dircode;
   static unsigned i, dtotal, atotal, allen, remaining, bls;
   static char *alloca;
   static struct stat *statp;
   static struct dpb *thedpb;
   static struct direntry *dp;

   cls();
   statp = alloc(sizeof(struct stat));
   if (allocerror(statp))
      return;
   sprintf(Buf,"Directory for Drive %c:",Currdrive);
   putlabel(Buf);
   bdos(SETDMA,CPMBUF);         /* set dma address */
   dircode = getfirst("????????.???");
   for (j=1; dircode != 0xff; j++) {
      dp = CPMBUF + dircode*32;
      memcpy(statp->fname,dp->flname,8);
      memcpy(statp->fext,dp->ftype,3);
      statp->fname[8] = statp->fext[3] = '\0';
      strcpy(Pathname,statp->fname);
      strcat(Pathname,".");
      strcat(Pathname,statp->fext);
      fstat(Pathname,statp);
      sprintf(Buf,"%8s.%3s%3dk",statp->fname,statp->fext,
         roundup(statp->records,8));
      strcat(Buf,(j % COLUMNS) ? "  " : "\n");
      if (Inhost)
         mstrout(Buf,TRUE);
      else
         printf(Buf);
      dircode = getnext();
   }
   thedpb = nbdos(GETDPB,NULL);
   alloca = nbdos(GETALL,NULL);
   bls = 0x0001;
   bls <<= thedpb->bsh + 7;
   factor = bls/1024;
   dtotal = factor * (thedpb->dsm+1);
   allen = (thedpb->dsm/8) + 1;
   for (atotal=i=0; i<allen; i++)
      atotal += cntbits(*(alloca+i));
   atotal *= factor;
   remaining = dtotal - atotal;
   sprintf(Buf,"\n\nSpace remaining on %c:  %dk\n",Currdrive,remaining);
   if (Inhost)
      mstrout(Buf,TRUE);
   else
      printf(Buf);
   free(statp);
}

fstat(fname,status)
char *fname;
struct stat *status;
{
   unsigned filelength();

   makfcb(fname,&Thefcb);
   status->records = filelength(&Thefcb);
   getfirst(fname);
   makfcb("????????.???",&Thefcb);
}

unsigned
filelength(fcbp)
struct fcb *fcbp;
{
   bdos(0x23,fcbp);
   return fcbp->ranrec;
}

roundup(dividend,divisor)
int dividend, divisor;
{
   return (dividend/divisor + ((dividend%divisor) ? 1 : 0));
}

getfirst(aname)      /* ambiguous file name */
char *aname;
{
   makfcb(aname,&Thefcb);
   return bdos(SFF,&Thefcb) & 0xff;
}

getnext()
{
   return bdos(SFN,NULL) & 0xff;
}

nbdos()
{
#ifdef C80
#asm
   POP H
   POP D
   POP B
   PUSH B
   PUSH D
   PUSH H
   CALL 5
#endasm
#endif
}

memcpy(dest,source,count)
char *dest, *source;
int count;
{
   while (count--)
      *dest++ = *source++;
}

memset(dest,byte,count)
char *dest, byte;
int count;
{
   while (count--)
      *dest++ = byte;
}
   
/************************** END OF MODULE 6 *********************************/
