/*
 * Phone dialing Module (from XMDM)
 *
 *
 *	Jwahar Bammi
 * 	bang:   {any internet host}!dsrgsun.ces.CWRU.edu!bammi
 * 	domain: bammi@dsrgsun.ces.CWRU.edu
 *	GEnie:	J.Bammi
 */

#include "config.h"

#ifndef STANDALONE
#ifdef PHONES

#include "zmdm.h"

typedef int WORD;
typedef long LONG;


#define sendchar(c)	Bconout(1, c);
#define clear_screen()  Bconws("\033H\033J")
#define inv()		EscSeq('p')
#define normal()	EscSeq('q')
#define mvto(r,c)	EscSeq('Y');Bconout(2, r+040);Bconout(2, c+040)
#define ceol()		EscSeq('K')
#define show_cursor()	EscSeq('e')
#define hide_cursor()	EscSeq('f')

typedef struct _dir {		/* The Directory type */
	char	*name;		/* Name (11 chars max) */
	char 	*number;	/* Phone # (24 chars max) */
	WORD	baud;		/* baud Rate		 */
	struct _dir *next;	/* Ptr to next entry */
} *DIR;


 	/* External Variables */
extern WORD speed;		/* Current Baud Rate */
extern WORD Baudrate;
extern BAUDS vbauds[];
WORD dchanged = 0;			/* Has the directory been updated */
extern char *PhoneFile;

 	/* Globals */
static WORD ndir = 0;			/* # of entries in phone directory   */
static DIR directory = (DIR)NULL;	/* The phone directory  	     */
static DIR lastdir   = (DIR)NULL; 	/* Pointer to last entry	     */
static char *dirfile = (char *)NULL;	/* Name of file conatining directory */
static WORD rflag = 0;			/* Read directory as yet?? 	     */
static WORD firstTime = 1;		/* first time around 		     */

#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif

static WORD readdir P_((void));
static void freedir P_((DIR dir));
static WORD showdir P_((void));
static void putdir P_((WORD first, WORD last));
static void putstr P_((char *s, WORD l));
static DIR nth P_((WORD n));
static void addir P_((void));
static WORD tobaud P_((char *s));
static void opendir P_((void));
static void delentry P_((void));
static WORD jbaud P_((WORD bd));
static char *preadl P_((void));
void dial P_((void));
void redial P_((void));

#undef P_

/*
 * Read the Telephone directory
 *  returns -2 on read error
 *	    -1 if cancelled
 *	    -3 if file not found
 *	    -4 if env var PHONE not avail or phone file given as arg not accs.
 *	     n # of entries  otherwise
 */
static WORD readdir()
{
	register char *filename;
	register WORD handle;
	register WORD nentries;
	register DIR last;
	register DIR present;
	extern char *preadl();
#ifdef __GNUC__
	extern void *malloc(size_t);
#else
	extern char *malloc();
#endif
	extern char *getenv();
	
	if(firstTime == 1)
	{
		firstTime = 0;
		filename = (char *)NULL;
		if(PhoneFile != (char *)NULL)
		{
			filename = PhoneFile;
			if((handle = Fopen(filename, 0)) < 0)
				filename = (char *)NULL;
			else
				Fclose(handle);
		}
		if(filename == (char *)NULL)	
			if((filename = getenv("PHONE")) == (char *)NULL)
				return -4;
	}
	else
	{
	  Bconws("Enter Filename of Phone Directory or <CR> to Cancel: ");

	  if((filename = preadl()) == (char *)NULL)
	    /* Cancelled */
	    return -1;
	}
	if((dirfile = malloc(strlen(filename)+1)) == (char *)NULL)
	{
		/* Out of memory */
		Bconws("Out of Memory\r\n");
		return 0;
	}
	strcpy(dirfile,filename);

	if((handle = Fopen(filename,0)) < 0)
	    /* File does not exist */
	    return -3;
	
	/* Read in the file */
	if(Fread(handle, 2L, &ndir) != 2L)
	{
		Bconws("Error Reading ");
		Bconws(filename);
		Bconws("\r\n");
		Fclose(handle);
		return -2;
	}

	/* Read in the directory */
	last = (DIR)NULL;
	directory = (DIR)NULL;
	lastdir = (DIR)NULL;
	rflag = 1;
	
	for(nentries = 0; nentries < ndir; nentries++)
	{
		/* Allocate an entry */
		if((present = (DIR)malloc(sizeof(struct _dir))) == (DIR)NULL)
		{
			/* Out of memory */
			Bconws("Out of Memory\r\n");
			Fclose(handle);
			return nentries;
		}
		
#ifdef __GNUC__
		if((present->name = malloc((size_t)12)) == (char *)NULL)
#else
		if((present->name = malloc(12)) == (char *)NULL)
#endif
		{
			/* Out of memory */
			Bconws("Out of Memory\r\n");
			Fclose(handle);
			return nentries;
		}

#ifdef __GNUC__
		if((present->number = malloc((size_t)25)) == (char *)NULL)
#else
		if((present->number = malloc(25)) == (char *)NULL)
#endif
		{
			/* Out of memory */
			Bconws("Out of Memory\r\n");
			Fclose(handle);
			return nentries;
		}

		present->next = (DIR)NULL;
		
		/* Read in the entry */
		if(Fread(handle,11L,present->name) != 11L)
		{
			Bconws("Error Reading ");
			Bconws(filename);
			Bconws("\r\n");
			Fclose(handle);
			rflag = 0;
			freedir(directory);
			return -2;

		}

		if(Fread(handle,24L,present->number) != 24L)
		{
			Bconws("Error Reading ");
			Bconws(filename);
			Bconws("\r\n");
			Fclose(handle);
			rflag = 0;
			freedir(directory);
			return -2;
		}

		if(Fread(handle,2L,&(present->baud)) != 2L)
		{
			Bconws("Error Reading ");
			Bconws(filename);
			Bconws("\r\n");
			Fclose(handle);
			rflag = 0;
			freedir(directory);
			return -2;
		}
		

		present->name[11] = '\0';
		present->number[24] = '\0';

		/* Link it on with the directory */
		if(last == (DIR)NULL)
			/* first entry */
			directory = present;
		else
			last->next = present;
		
		last = present;
	}
	lastdir = last;
	
	return nentries;	
}

/*
 * Free space allocated to a phone directory
 *
 */
static void freedir(dir)
register DIR dir;
{
	register DIR next;
	register DIR present;

	for(present = dir; present != (DIR)NULL; present = next)
	{
		next = present->next;
		free(present->name);
		free(present->number);
		free(present);
	}
	
	if(dirfile != (char *)NULL)
	{
		free(dirfile);
		dirfile   = (char *)NULL;
	}
	
	directory = (DIR)NULL;
	lastdir	  = (DIR)NULL;
	ndir	  = 0;
	rflag	  = 0;
}

/*
 * Write out the phone directory
 *  -returns 0 on success 1 otherwise
 *
 */
int writedir()
{
	register DIR dir;
	register WORD fd;
	
	if((rflag == 0) || (dirfile == (char *)NULL) || (dchanged == 0))
	    /* Nothing to Save */
	    return 0;

	/* Create/Open file for write - overwrite if it exists */
	if ((fd = Fcreate(dirfile,0)) < 0) /* Will fail if file is present */
	{
		/* Overwrite existing file */
		if((fd = Fopen(dirfile,1)) < 0)
		{
			Bconws("Cannot Open ");
			Bconws(dirfile);
			Bconws("\r\n");
			return 1;
		}
	}

	if(Fwrite(fd,2L,&ndir) != 2L)
	{
		Bconws("Error Writing ");
		Bconws(dirfile);
		Bconws("\r\n");
		Fclose(fd);
		return 1;
	}
	
	for(dir = directory; dir != (DIR)NULL; dir = dir->next)
	{
		if(Fwrite(fd,11L,dir->name) != 11L)
		{
			Bconws("Error Writing ");
			Bconws(dirfile);
			Bconws("\r\n");
			Fclose(fd);
			return 1;
		}

		if(Fwrite(fd,24L,dir->number) != 24L)
		{
			Bconws("Error Writing ");
			Bconws(dirfile);
			Bconws("\r\n");
			Fclose(fd);
			return 1;
		}

		if(Fwrite(fd,2L,&(dir->baud)) != 2L)
		{
			Bconws("Error Writing ");
			Bconws(dirfile);
			Bconws("\r\n");
			Fclose(fd);
			return 1;
		}
	}
	
	Fclose(fd);
	return 0;
}

/*
 * Show the phone directory
 * return the entry # or -1 if cancelled
 *
 */
static WORD showdir()
{
	register WORD first, last;
	register WORD n;
	register char *line;
	extern WORD atoi();
	extern char *preadl();
	
	first = 0;

	while(1)
	{
	    again:
		clear_screen();

		mvto(0,19);
		Bconws("Phone Directory: ");
		Bconws(dirfile);
		Bconws("  ");
		printf("%d",ndir); fflush(stdout);
		Bconws(" Entry(s)");

		last = (ndir < (first + 44)) ? ndir : first + 44;
		putdir(first,last);
		
		/* mvto(25,0); */
		Bconws("\r\n");
		inv();
		Bconws("Enter a # or <SPACE><RETURN> for Next Page or <RETURN> to Cancel:");
		normal();
		Bconout(2, ' ');
		if((line = preadl()) == (char *)NULL)
		{
			return -1;
		}
		
		if(isdigit(*line))
		{
			if((n = atoi(line)) >= ndir)
			{
				Bconws("Invalid Number ");
				hit_key();
				goto again;
			}
		    
			return n;
		}

		if(last == ndir)
		    first = 0;
		else
		    first += 44;
	}
}

/*
 * Put up directory entries on the screen
 *
 */
static void putdir(first,last)
register WORD first;
register WORD last;
{
	register DIR dir;
	register WORD row;
	extern DIR nth();
	
	/* Find the first entry */
	dir = nth(first);
	row = (first % 44) + 1;

	hide_cursor();
	inv();
	for(; first < last; first++)
	{
		mvto(row,((first & 1)?41:0));
		if(first < 10)
			Bconout(2, ' ');
		printf("%d",first); fflush(stdout);
		Bconout(2, '|');
		putstr(dir->name,11);
		Bconout(2, '|');
		putstr(dir->number,24);
		dir = dir->next;
		if(first & 1)
			row++;
	}

	if(first & 1)
	{
		mvto(row,41);
		Bconws("  |           |                        ");
		row++;
	}

	for(; row < 23; row++)
	{
		Bconws("  |           |                        ");
		mvto(row,41);
		Bconws("  |           |                        ");
	}

	normal();
	show_cursor();
}

/*
 * Put a string padding to len on screen
 */
static void putstr(s,l)
register char *s;
register WORD l;
{
	register WORD pad;

	Bconws(s);
	if((pad = l - (int)strlen(s)) <= 0)
		return;
	for(; pad > 0; pad--)
		Bconout(2, ' ');
}


/*
 * Return the nth entry in the phone directory
 *
 */
static DIR nth(n)
register WORD n;
{
	register WORD i;
	register DIR dir;
	
	for(i = 0, dir = directory; (dir != (DIR)NULL) & (i < n);
	    i++, dir = dir->next)
	    /* Skip */;
	return(dir);
}

/*
 * Add a entry in the phonebook
 */
static void addir()
{
	register DIR dir;
	register char *s;
#ifdef __GNUC__
	extern void *malloc(size_t);
#else
	extern char *malloc();
#endif
	extern char *preadl();
	
	/* If a directory file already exists add, else read or create */
	if(rflag == 0)
	{
		switch(readdir())
		{
		    case -3:
			/* Doesnt exist, but we will create it when we
			   write out the directory */
			rflag = 1;
			break;
			
		    case 0:
		    case -2:
		    case -1:
			hit_key();
			return;
			
		    default:
			rflag = 1;
			break;
		}
	}
	
	/* Allocate space for the new entry */
	if((dir = (DIR)malloc(sizeof(struct _dir))) == (DIR)NULL)
	{
		Bconws("Out of Memory\r\n");
		hit_key();
		return;
	}
	
	if((dir->name = malloc(12)) == (char *)NULL)
	{
		/* Out of memory */
		Bconws("Out of Memory\r\n");
		hit_key();
		return;
	}

	if((dir->number = malloc(25)) == (char *)NULL)
	{
		/* Out of memory */
		Bconws("Out of Memory\r\n");
		hit_key();
		return;
	}

		
	/* Get the entry */
	
	do {
		Bconws("Name: ");
		s = preadl();
	} while(s == (char *)NULL);
	
	strncpy(dir->name,s,11);
	
	do {
		Bconws("Number: ");
		s = preadl();
	} while(s == (char *)NULL);
	
	strncpy(dir->number,s,24);
	
	do {
		Bconws("Baud Rate: ");
		s = preadl();
	} while((s == (char *)NULL) || ((dir->baud = tobaud(s)) == -1));
	
	dir->next = (DIR)NULL;
	
	if(directory == (DIR)NULL)
	    directory = dir;
	else
	    lastdir->next = dir;

	lastdir = dir;
	dchanged = 1;
	ndir++;
}



/*
 * Convert a string to a baud rate
 * return int or -1 if invalid
 */
static WORD tobaud(s)
register char *s;
{
	register WORD i;
	
	for(i = 0; vbauds[i].sbaud != (char *)NULL; i++)
	{
		if(strcmp(vbauds[i].sbaud,s) == 0)
		    return vbauds[i].ibaud;
	}

	Bconws(s);
	Bconws(": Invalid Baud Rate\r\nValid Baud Rates are:\r\n");
	for(i = 0; vbauds[i].sbaud != (char *)NULL; i++)
	{
		Bconout(2, '\t');
		if((i != 0) && (vbauds[i].ibaud != vbauds[i-1].ibaud))
		{
			Bconws(vbauds[i].sbaud);
			Bconws("\r\n");
		}
	}
		
	return -1;
}

/*
 * Dial a number 
 */
static void dial()
{
	register WORD n;
	register DIR dir;
	extern DIR nth();
	
	/* Has the directory been read so far */
	if(rflag == 0)
	{
		/* Go read it */
		switch(readdir())
		{
		    case -1:
		    case -2:
		    case -3:
		    case -4:
		    case 0:
			rflag = 0;
			hit_key();
			his_screen();
			return;
		    default:
			break;
		}
	}
	
	if((n = showdir()) == -1)
	{
		/* Cancelled */
		hit_key();
		his_screen();
		return;
	}
	
	his_screen();
	dir = nth(n);
	if(dir->baud != speed)
	{
		speed = dir->baud;
		Baudrate = jbaud(speed);
		Rsconf(speed, -1, -1, -1, -1, -1);
		sendchar('\r');
		flushinput();
	}
	write_modem(PREDIAL, (int)strlen(PREDIAL));
	write_modem(dir->number,(int)strlen(dir->number));
	sendchar('\r');
}


/*
 * Re-dial the previous number
 *
 *	Does it cheaply, by sending the modem its Re-dial
 *	sequence, instead of remembering the last number dialed etc.
 *
 */
static void redial()
{
	his_screen();
	write_modem(REDIAL ,(int)strlen(REDIAL));
}


/*
 * Open a phone directory 
 *
 */
static void opendir()
{
	/* if one is open, save it if changed, then deallocate memory
 	 * and then open a new directory
	 */
	register int i;

	if(rflag)
	{
		if(dchanged)
		{
			if(writedir() == 1)
			{
				hit_key();
				return;
			}
			dchanged = 0;
		}
		freedir(directory);
	}
	
	if((i = readdir()) <= 0)
	{
		if(i == (-4))
			return;
		hit_key();
		return;
	}
}

/*
 * Delete an entry.
 */
static void delentry()
{
	register DIR dir, del;
	register WORD n;
	extern DIR nth();
	
	if(rflag == 0)
	{
		Bconws("Nothing to delete\r\n");
		hit_key();
		return;
	}
	
	if((n = showdir()) == -1)
	{
		/* Cancelled */
		hit_key();
		return;
	}
	
	if(ndir == 1)
	{
		del = directory;
		directory = (DIR)NULL;
		lastdir = (DIR)NULL;
	}
	else
	{
		if(n == 0)
		{
			del = directory;
			dir = directory->next;
			directory = dir;
		}
		else
		{
			dir = nth(n-1);
			del = dir->next;
			dir->next = del->next;
			if(lastdir == del)
			    lastdir = dir;
		}
	}
	
	free(del->name);
	free(del->number);
	free(del);
	ndir--;
	dchanged = 1;
	
}

	
/*
 * Phone services - top level
 *
 */
void phone()
{
	register LONG conin;
	
	if(firstTime == 1)
		opendir();

	while(1)
	{
		clear_screen();
		mvto(2,32);
		inv();
		Bconws("Phone  Services");
		normal();

		mvto(5,0);
		
		if(rflag)
		{
			Bconws("\tThe Phone Directory \"");
			Bconws(dirfile);
			Bconws("\" containing ");
			printf("%d", ndir); fflush(stdout);
			Bconws(" Entry(s) is Currently Open.\r\n\n");
		}
		else
		    Bconws("\tNo Phone Directory is Currently Open.\r\n\n");
		
		
		/* Put up menu */
		Bconws("\r\n\t");
		EscSeq('p');		/* reverse video */
		Bconws("Undo");
		EscSeq('q');		/* quit reverse video */
		Bconws(" to exit the emulator.\r\n");
		
		Bconws("\t");
		EscSeq('p');		/* reverse video */
		Bconws("d");
		EscSeq('q');		/* quit reverse video */
		Bconws(" to dial a number.\r\n");
		
		Bconws("\t");
		EscSeq('p');		/* reverse video */
		Bconws("a");
		EscSeq('q');		/* quit reverse video */
		Bconws(" to add an entry to the phone directory.\r\n");
		
		Bconws("\t");
		EscSeq('p');		/* reverse video */
		Bconws("D");
		EscSeq('q');		/* quit reverse video */
		Bconws(" to delete an entry from the phone directory.\r\n");
		
		Bconws("\t");
		EscSeq('p');		/* reverse video */
		Bconws("o");
		EscSeq('q');		/* quit reverse video */
		Bconws(" to open another phone directory.\r\n");

		Bconws("\t");
		EscSeq('p');		/* reverse video */
		Bconws("r");
		EscSeq('q');		/* quit reverse video */
		Bconws(" to re-dial last number.\r\n");
		
		Bconws("\t");
		EscSeq('p');		/* reverse video */
		Bconws("Return");
		EscSeq('q');		/* quit reverse video */
		Bconws(" to return to the emulator.\r\n\n\n\n");
		
		/* get response */
		conin = Bconin(2);
		
		if ((conin & 0x00FF0000L) == 0x610000L)
		{
			/* He hit <UNDO> */
			his_screen();
			ResetIoBuf();
			finish();
		}
		
		switch((WORD)(conin & 0x7f))
		{
		    case 'd':
			/* Dial a number */
			dial();
			return;
			
		    case 'D':
			/* Delete an entry */
			delentry();
			break;
			
		    case 'a':
			/* Add an entry */
			addir();
			break;
			
		    case 'o':
			/* Open another phone directory */
			opendir();
			break;
			

		    case 'r':
			/* Re-Dial # */
			redial();
			return;
			
		    case '\r':
			his_screen();
			return;
			
		    default:
			break;
		}
		
	}
	
}

/*
 * Convert a baud rate to its int
 * return int or -1 if invalid
 */
static WORD jbaud(bd)
register WORD bd;
{
	register WORD i;
	
	for(i = 0; vbauds[i].sbaud != (char *)NULL; i++)
	{
		if(vbauds[i].ibaud == bd)
		    return vbauds[i].jbaud;
	}
	return -1;
}

static char scrth[80];
/*
 * Read a line from Standard Input and return a 
 * NULL terminated pointer to it
 */
static char *preadl()
{
	/* Use the scrth bufr for storage */
	scrth[0] = 80;
	Cconrs(scrth);
	Bconws("\r\n");
	if(scrth[1] == 0)
	{
		/* User Cancelled */
		Bconws("Cancelled\r\n");
		return((char *)NULL);
	}
	/* Terminate string that starts at scrth[2] */
	scrth[scrth[1]+2] = '\0';
	return(&scrth[2]);
}	

#endif /* PHONES */
#endif /* STANDALONE */

/* -eof - */
