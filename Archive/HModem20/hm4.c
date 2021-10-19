/************************** START OF MODULE 4 *******************************/

#define  C80
#include "hmodem80.h"

dial()
{
   static char *number;      /* buffer for number to be sent to modem */
   static char *result;      /* buffer for responses from modem */
   static char *instr;       /* buffer for numbers entered at keyboard */
   static int connect;
   static int status, i, j, k, n, nocnt, action, c;
   static char *p;

   if (allocerror(number = alloc(128)))
      return;
   if (allocerror(result = alloc(128)))
      return;
   if (allocerror(instr = alloc(128)))
      return;
   status = shownos();
   putlabel("Dialing Menu:  Enter letters and/or numbers, separated by commas");
   QuitFlag = connect = FALSE;
   Dialing = TRUE;
   printf("\nPlease enter number(s) to dial:  ");
   if (j=getline(instr,80)) {
      putlabel("Automatic Redial:  Press ESC to stop");
      for (i=0,nocnt=1; instr[i]; i++)
         if  (instr[i] == ',') {
            instr[i] = 0;
            nocnt++;
         }
      i = nocnt;
      while (TRUE) {
         p = instr;
         nocnt = i;
         while (nocnt--) {
            n = -1;
            strcpy(number,Modem.dialcmd);
            if (*p == '+') {
               strcat(number,Sprint);
               p++;
            }
            else if (*p == '-') {
               strcat(number,Mci);
               p++;
            }
            if ((status == OK) && (j=strlen(p))==1) {
               if (isalpha(n = *p)) {
                  n = toupper(n) - 'A';
                  setace(n);
                  strcat(number,Book[n].number);    
                  strcat(number,Modem.dialsuffix);
                  mstrout(number,FALSE);
                  printf("\nDialing %s...",Book[n].name);
               }
               else {
                  printf("\nInvalid Number\n");
                  goto abort;
               }
            }     
            else {
               strcat(number,p);
               strcat(number,Modem.dialsuffix);
               mstrout(number,FALSE);
               printf("\nDialing %s...",p);
            }
            while (readline(10) != TIMEOUT);    /*flush modem input*/
            do {
               action = readstr(result,Modem.timeout);
               if (action == TIMEOUT)
                  goto abort;
               printf("%s\n",result);
            } while (!(c=isin(result,Modem.connect))
                  && !isin(result,Modem.busy1)
                  && !isin(result,Modem.busy2)
                  && !isin(result,Modem.busy3)
                  && !isin(result,Modem.busy4));

            if (c) {                   /* got connect string */
               printf("\007\nOn Line to %s\n",n >= 0 ? Book[n].name : p);
               if (n >= 0)
                  FDx = !Book[n].echo;
               connect = TRUE;
               goto done;
            }
            mcharout(CR);
            while (readline(10) != TIMEOUT);    /* wait for modem */
            p += j+1;
         }
         if (kbwait(Modem.pause))
            goto abort;
      }
   }

abort:
   printf("Call Aborted.\n");
   mcharout(CR);
   readstr(result,1);      /*gobble last result*/
   resetace();

done:
   flush();
   if (Book != -1)
      free(Book);
   free(instr);
   free(result);
   free(number);
   Dialing = FALSE;
   return connect;
}

shownos()
{
   static int i, j, status;

   cls();
   if ((status=loadnos()) == OK) {
      printf("\033p         NAME                NUMBER          B   P D S E\033q");
      for (i=0,j=1; i<20; i++,j++) {
         locate(i+1,0);
         printf("%c - %s",i+'A',Book[i].name);
         locate(i+1,41-strlen(Book[i].number));
         printf(Book[i].number);
         locate(i+1,44);
         printf("%4d %c %d %d %c\n",Baudtable[Book[i].pbaudindex],Book[i].pparity,
            Book[i].pdatabits,Book[i].pstopbits,Book[i].echo?'H':'F');
      }
   }
   return status;
}

loadnos()
{
   static unsigned amount, loc;
   static int fd, i, result;

   fd = 0;
   result = NERROR;
   amount = 128 * roundup(20 * sizeof(struct phonebook));
   Book = alloc(amount);
   if (!allocerror(Book)) {
      strcpy(Pathname,Phonefile);
      addrive(Pathname,Invokdrive);
      if (!openerror((fd = fopen(Pathname,"rb")))) {
         read(fd, Book, amount);
         result = OK;
         fclose(fd);
      }
   }
   return result;
}      

readstr(p,t)
char *p;
int t;
{
   static int c;

   t *= 10;                /* convert to tenths */
   flush();
   while (((c=readline(t)) != CR) && (c != TIMEOUT)) {
      if (c != LF)
         *p++ = c;
   }
   *p = 0;
   return c;
}

isin(received,expected)
char *received, *expected;
{  
   return (index(received,expected) != -1);
}

diskstuff()
{
   static int c;

   for (;;) {
      cls();
      printf("\t\tA - Change disk in default drive\n");
      printf("\t\tB - Change default drive (currently %c:)\n",Currdrive);
      printf("\t\tC - Delete file on default drive\n");
      printf("\t\tZ - Exit\n");
      printf("\n\tPlease enter choice:  ");
      putlabel("Disk Operations Menu");
      flush();
      c = toupper(getchar());
      switch (c) {

         case 'A':
            printf("\nChange disk in %c: then press any key...",Currdrive);
            flush();
            getchar();
            reset(Currdrive);
            break;

         case 'B':
            printf("\nPlease enter the new default drive:  ");
            flush();
            reset(getchar());
            break;

         case 'C':
            directory();
            printf("\nDelete what file?  ");
            if (!getline(Pathname,16))
               break;
            printf("\nAre you sure? (Y/N) <N>  ");
            flush();
            c = toupper(getchar());
            if (c == 'Y')
               unlink(Pathname);
            break;
      
          default:
            cls();
            return;
            break;
      }
   }
}

#define     UPARROW     'x'
#define     DNARROW     'r'

help()
{
   static int fd, function, c, i;
   static int pagend[10], k, kbdata;

   strcpy(Pathname,"HMODEM.HLP");
   addrive(Pathname,Invokdrive);
   fd = fopen(Pathname,"rb");
   if (openerror(fd,Pathname))
      return;
   cls();
   putlabel("Help Screens:  Use Up-Arrow, Down-Arrow, ESC to exit");
   c = k = pagend[0] = 0;

newpage:
   for (i=0; i<22 && (c=getc(fd)) != CTRLZ; ) {
      putchar(c);
      if (c == CR)
         ++i;
   }
again:
   kbdata = getchar();
   if ((function = getfunction(kbdata)) == UPARROW) {
      if (k > 0)
         k--;
      seek(fd,pagend[k],0);
      cls();
      goto newpage;
   }
   else if (function == DNARROW) {
      if (k < 3) {
         pagend[++k] = ftell(fd);
         goto newpage;
      }
      else
         goto again;
   }
   else if (kbdata == ESC) {
      fclose(fd);
      cls();
      return;
   }
   goto again;
}

/************************* end of module 4 **********************************/
