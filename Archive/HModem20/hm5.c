/***************************** Module 5 ************************************/

#define  C80
#include "hmodem80.h"

dohost()
{
   static int i, c, d, valid, n;
   static int result;

   flush();
   mstrout(Host.autoanswer,FALSE);
   cls();
   hlabel();
   printf("Waiting for call...");
   while (!connect()) {                 /* wait for connect */
      if (QuitFlag)
         goto cleanup;
   }
   wait(3);
   autobaud();
   if (QuitFlag)
      goto cleanup;
   mstrout("\n\nHModem ",TRUE);
   mstrout(Version,TRUE);
   mstrout("\n\n",TRUE);
   mstrout(Host.welcome,TRUE);
   purgeline();
   do {
      mstrout("\nUser ID:  ",TRUE);
      purgeline();
   } while (!(i=mgetline(Host.user,20,TRUE)));
   if (i == TIMEOUT)
      goto cleanup;
   for (i=0; i<3; i++) {
      valid = getpassword();
      if (valid)
         break;
   }
   if (!valid || (valid == TIMEOUT))
      goto cleanup;
   c = 0;
   while ((c != 'G') && connect()) {
      hostmenu();
      hlabel();
      for (i=0; (i < 60) && connect(); i++) {
         c = mgetchar(1);
         if (c != TIMEOUT)
            break;
      }
      if (c != TIMEOUT) {
         c = toupper(c);
         mcharout(c);
         mstrout("\n\n",FALSE);
      }
      switch (c) {
      
      case TIMEOUT:
         goto cleanup;
         break;

      case 'C':
         chat();
         break;

      case 'G':
         mstrout("\nThank you for calling...\n",TRUE);
         break;

      case 'D':
         result = dotransfer(SEND,NULL);
         if (result == TIMEOUT)
            goto cleanup;
         QuitFlag = FALSE;
         break;

      case 'F':
         directory();
         break;

      case 'U':
         result = dotransfer(RECEIVE,NULL);
         if (result == TIMEOUT)
            goto cleanup;
         QuitFlag = FALSE;
         break;

      case 'M':
         if (getmessage() == TIMEOUT)
            goto cleanup;
         break;

      case 'R':
         if ((mgetchar(1) == 'z') && ((mgetchar(1)&0xff) == CR)) {
            result = dotransfer(RECEIVE,'Z');
            if (result == TIMEOUT)
               goto cleanup;
            QuitFlag = FALSE;
            break;
         }

      case 'S':      /* backdoor to set default drive */
         if (mgetchar(1) == 'i')
            reset(mgetchar(1));
         break;

      default:
         break;
      }
   }
cleanup:
   printf("\n\nExiting Host Mode...");
   if (connect())
      hangup();
   initializemodem();
   wait(3);
}

dotransfer(which,prot)
int which, prot;
{
   static int result;

   if (which == RECEIVE)
      result = bringin(prot);
   else if (which == SEND)
      result = sendout(prot);
   switch (result) {
      case NERROR:
         mstrout("\007\nTRANSFER ABORTED\n",FALSE);
         break;
      case OK:
         mstrout("\007\nTransfer Successful\n",FALSE);
         break;
      default:
         break;
   }
   return result;
}

getpassword()
{
   static char *password;

   password = Pathname;
   mstrout("\nPassword:  ",TRUE);
   if (mgetline(password,20,FALSE) == TIMEOUT)
      return TIMEOUT;
   if (strcmp(password,Host.password))
      return FALSE;
   else
      return TRUE;
}

getmessage()
{
   static char *buffer;
   static int fm, i;
   static char *p;

   buffer = Pathname;
   mstrout("\nPlease enter date and time:  ",TRUE);
   if ((i=mgetline(buffer,40,TRUE)) == TIMEOUT)
      return TIMEOUT;
   if (!i)
      return 0;
   addatadrive(Msgfile);
   if (!(fm = fopen(Msgfile,"u")) && !(fm = fopen(Msgfile,"w"))) {
      openerror(fm,Msgfile);
      return TIMEOUT;
   }
   seek(fm,0,2);
   fprintf(fm,"From:  %s\n",Host.user);
   fprintf(fm,"Date:  %s\n",buffer);
   mstrout(">",TRUE);
   p = buffer;
   while (((i = mgetline(buffer,80,TRUE)) != TIMEOUT) && i) {
      while (*p)
         putc(*p++,fm);
      putc('\n',fm);
      mstrout(">",TRUE);
      p = buffer;
   }
   putc('\n',fm);
   putc('\n',fm);
   fclose(fm);
   return 0;
}      

/* mgetline - get a string from the modem.  Returns length of the string,
	0 terminated, without the newline at the end. */

mgetline(s,lim,echo)
char *s;
int lim, echo;
{	
   static char *t;
   static int i;

	for (t=s,*t='\0' ; --lim > 0 ; ++t) {
      for (i=0; i<60; i++) {
         if (!connect())
            return TIMEOUT;
         if ((*t = mgetchar(1)) != TIMEOUT)
            break;
      }
      if (!connect())
         return TIMEOUT;
      if (*t == '\r')
         break;
      if (*t == TIMEOUT)
         return TIMEOUT;
      if ((*t == BS) && (t == s)) {
         --t;
         ++lim;
         continue;
      }
      else {
         if (*t == BS) {
            mstrout("\b \b",TRUE);
            t -= 2;
            lim += 2;
         }
         else {
            putchar(*t);
            if (echo)
               mcharout(*t);
            else
               mcharout('*');
         }
      }
   }
	*t = '\0';
   mstrout("\n",TRUE);
	return (t - s);
}

hostmenu()
{
   purgeline();
   flush();
   mstrout("\n\n(C)hat, (D)ownload, (F)iles, (U)pload, (M)essage, or (G)oodbye?  ",TRUE);
}

connect()
{
   opabort();                 /* sets QuitFlag if esc hit */
   if (Host.modemconnection)
      return (inp(Mdmstat) & CONNECTSTAT) && !QuitFlag;
   else
      return !QuitFlag;
}

autobaud() 
{
   static int baudindex[] = { 3, 6, 7 };
   static int c, i;

   printf("\nAutomatic baud rate adjust...");
   purgeline();
   Current.cparity = 'N';
   Current.cdatabits = 8;
   Current.cstopbits = 1;
   for (i=0; mdmerror() || ((c=(mgetchar(1) & 0xff)) != CR); i %= 3) {
#ifdef   DEBUG
      printf("\nc = %x",c);
#endif
      if (!connect())
         break;
      Current.cbaudindex = baudindex[i++];
      updateace();
      purgeline();
   }
   purgeline();
}

chat()
{
   static int c;

   cls();
   c = 0;
   chlabel();
   mstrout("\nSignaling sysop...",TRUE);
   printf("\007 \007 \007");
   while (c != ESC) {
      if (c=getch()) {
         putchar(c);
         mcharout(c);
         if (c == CR) {
            putchar(LF);
            mcharout(LF);            
         }
         else if (c == BS)
            mstrout(" \b",TRUE);
      }
      if (minprdy()) {
         putchar(c=mcharinp());
         mcharout(c);
         if (c == CR) {
            putchar(LF);
            mcharout(LF);
         }
         else if (c == BS)
            mstrout(" \b",TRUE);
      }
   }
   mstrout("\007\n\nReturning to Host mode...\n\n",TRUE);
   hlabel();
}

mgetchar(seconds)         /* allows input from modem or operator */
int seconds;
{
   static int c, tenths;

   Lastkey = 0;
   tenths = seconds * 10;
   if ((c=readline(tenths)) != TIMEOUT)
      return (c & 0xff);
   else if (Lastkey)
      return Lastkey;
   return TIMEOUT;
}

/********************** end of module 5 ***********************************/
