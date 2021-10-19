
	/* 
	 * 	Examine each argument given to expandargs()
	 * 	If it is a directory, then expand it
	 *	to all its component files, recursively
	 *	till you bottom out.
	 *	If it is not a directory, then just pass it on.
	 *
	 *	Inputs: routine, argc, argv
	 *	Outputs: nargc, nargv (calls routine(nargc, nargv))
	 *	To test: compile with -DTEST
	 *		 define MWC if using Mark Williams C
	 *		 run with a directory as an arg
	 *	Author: JRB	bammi@mandrill.ces.CWRU.edu
	 *	Requirements: Mark Williams C or Alcyon C
	 *		Wants lots of Dynamic memory. It
	 *		all depends upon how many files you
	 *		have. Use the -P option to prune
	 *		out subdirectories and do things
	 *		one at a time, if you keep running
	 *		out of memory.
	 *		With Mark Williams i use _stksize = 128K
	 *		With Alcyon i use memory model 2 (half of
	 *		of avail memory) in GEMSTART.S
	 *
         *      WARNINGS: Be CAREFUL about the 40 folder bug. Use
	 *	          GEMBOOT or FOLDRXXX when dealing with
	 *		  a deeply nested file structure.
	 *
	 *	Added -P name prune option at the suggestion of dietz@zhmti
	 *	Multiple -P's may be given on the command line.
	 *	-P name may be given anywhere on the command line. -P name
	 *	will prune the subdirectories named as arguement to -P.
	 *	ie: when the program is decending the file hierarchy it
	 *	will not visit the pruned branches.
	 * 	NOTE that the option is -P and not -p
	 *	      (-p is a valid sz option).
	 *	NOTE that the arguement given to -P can be the name
	 * 	of a file or a directory. In case it is a file,
	 *	that file is skipped.
	 *
	 */

/*
 * Expand argc, so that the called routine(nargc,nargv) receives only
 * filenames, in nargv[][]
 *
 ************************************************************************
 *									*
 *      WARNING:  Be CAREFUL about the 40 folder bug. Use		*
 *	          GEMBOOT or FOLDRXXX when dealing with			*
 *		  a deeply nested file structure.			*
 *									*
 ************************************************************************
 *
 *	Jwahar Bammi
 * 	bang:   {any internet host}!dsrgsun.ces.CWRU.edu!bammi
 * 	domain: bammi@dsrgsun.ces.CWRU.edu
 *	GEnie:	J.Bammi
 */

#ifdef TEST
#include <stdio.h>
#include <osbind.h>
#include <ctype.h>
#endif

#ifdef TRUE
#undef TRUE
#endif
#ifdef OK
#undef OK
#endif
#ifdef FALSE
#undef FALSE
#endif

#define TRUE	 1
#define OK	 0
#define FALSE	 0
#define Realloc	 realloc

#ifdef TEST
struct	stat
{
	char	st_sp1[21];    /* Junk 	   */
	char	st_mode;       /* File attributes */
	int	st_time;       /* Mod Time	  */
	int     st_date;       /* Mod date	  */
	long	st_size;       /* File size	   */
	char	st_name[14];   /* File name	   */
};
#endif

typedef struct _prunelist {
	char *name;			/* name of subdirectory to prune */
	struct _prunelist *next;	/* ptr to next */
} PRUNELIST;

static char *ProgName;
static PRUNELIST *PruneList = (PRUNELIST *)NULL; /* Head of PruneList */

#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif

static char **CopyToNargv P_((char *string, int nargc, char **nargv));
static char **ExpandStack P_((char **Stack));
static void FreeStack P_((void));
static void FreeNargv P_((int nargc, char *nargv[]));
static void FreePrune P_((void));
static void FreeUp P_((int nargc, char **nargv));
static int PushDir P_((char *name));
static char *PopDir P_((void));
static int ProcessDirs P_((int *nargc, char ***nargv));
static PRUNELIST *AddPrune P_((PRUNELIST *list, char *name));
static int OnPruneList P_((PRUNELIST *list, char *name));
static int isdir P_((char *name, int attr));
char *alltolower P_((char *s));

#undef P_

extern int existd();

expandargs(routine, argc, argv)
int (*routine)();
int argc;
char **argv;
{
	register int status;
	int nargc;
	char **nargv;
	extern char **CopyToNargv();
	extern PRUNELIST *AddPrune();
	extern int existd();

	nargc = 0;
	nargv = (char **)NULL;
	ProgName = *argv;
	
	/* copy argv[0] blindly */
	if((nargv = CopyToNargv(*argv, nargc, nargv)) == (char **)NULL)
	{
	    FreeUp(nargc, nargv);
	    return(~OK);
	}

	nargc++;
	
	while((--argc) > 0)
	{
		argv++;
		if(**argv == '-')
		{
			/* copy any options except -P */
#ifdef TEST
			/* some shell pass -P as -p */
			if( ((*argv)[1] == 'P') || ((*argv)[1] == 'p'))
#else
			if( (*argv)[1] == 'P')
#endif
			{
				if((--argc) <= 0)
				{
					fprintf(STDERR,"no argument given to -P\n");
					FreeUp(nargc, nargv);
					return(~OK);
				}
				if((PruneList = AddPrune(PruneList,*++argv))
				   == (PRUNELIST *)NULL)
				{
				    FreeUp(nargc, nargv);
				    return(~OK);
				}
			}
			else
			{
				if((nargv = CopyToNargv(*argv, nargc, nargv))
				   == (char **)NULL)
				{
				    FreeUp(nargc, nargv);
				    return(~OK);
				}
				else
				    nargc++;
			}
			
		}
		else
		{
			/* If its not on the PruneList then */
			if(!OnPruneList(PruneList, *argv))
			{
				/* if it is a directory, push it */
				if(existd(*argv))
				{
					if((status = PushDir(*argv)) != OK)
					{
						FreeUp(nargc, nargv);
						return(status);
					}
				}
				else
				{
				    /* it is NOT a directory, copy to nargv */
					if((nargv = CopyToNargv(*argv, nargc, nargv))
					   == (char **)NULL)
					{
					    FreeUp(nargc, nargv);
					    return(~OK);
					}
					else
					    nargc++;
				}
			}
		}
	} /* while */

	/* process pushed directories if any */
	if((status = ProcessDirs(&nargc, &nargv)) != OK)
	{
	    FreeUp(nargc, nargv);
	    return(status);
	}
	/* else Free the Stack and Prune List, call *routine */
	FreeStack();
	FreePrune();
	status =  (*routine)(nargc, nargv);
	FreeNargv(nargc, nargv);

	return status;

}

/*
 * Expand nargv by an element and copy a String into the new element
 *
 */
static char **CopyToNargv(string, nargc, nargv)
char *string;
int nargc;
char **nargv;
{
#ifdef __GNUC__
	extern void *malloc(size_t), *Realloc(void *, size_t);
#else
	extern char *malloc(), *Realloc();
#endif
	extern char  *strcpy();
#ifdef __GNUC__
	extern size_t strlen();
#else
	extern int strlen();
#endif
	
	/* expand nargv by 1 element */
	if(nargv == (char **)NULL)
	    /* do it with malloc for the first one */
	    nargv = (char **)malloc(sizeof(char **));
	else
	    /* do it with Realloc for others */
	    nargv = (char **)Realloc(nargv, (nargc+1)*sizeof(char **));
	
	if(nargv == (char **)NULL)
	{
		/* failed to get memory */
		fprintf(STDERR,"%s(CopyToNargv()): Out of Memory\n", ProgName);
		return ((char **)NULL);
	}

	/* Get mem for string */
	if(( nargv[nargc] = malloc(strlen(string)+1)) == (char *)NULL)
	{
		/* failed to get memory */
		fprintf(STDERR,"%s(CopyToNargv()): Out of Memory\n", ProgName);
		return ((char **)NULL);
	}

	/* copy string into nargv[nargc] */
	(void)strcpy( nargv[nargc], string);
	return(nargv);
}

static char **Stack = (char **)NULL;	/* directory stack */
static char StackSize = 0;		/* Size of current Stack */
static int    Top   = -1;
#define STACK_EMPTY	(Top < 0)
#define CHUNKSIZE	16

/*
 * Grow the Stack by one chunk of CHUNKSIZE elements
 *
 */
static char **ExpandStack(Stack)
char **Stack;
{
#ifdef __GNUC__
	extern void *malloc(size_t), *Realloc(void *, size_t);
#else
	extern char *malloc(), *Realloc();
#endif
	
	/* Grow Stack */
	if(Stack == (char **)NULL)
	    /* with malloc */
	    Stack = (char **)malloc(CHUNKSIZE * sizeof(char **));
	else
	    /* with Realloc */
	    Stack = (char **)Realloc(Stack, (StackSize+CHUNKSIZE) *
				             sizeof(char **));

	if(Stack == (char **)NULL)
	{
		/* outa mem */
		fprintf(STDERR,"%s(ExpandStack()): Out of Memory\n", ProgName);
		return((char **)NULL);
	}
	StackSize += CHUNKSIZE;
	return(Stack);
}

/*
 * Free the Stack
 *
 */
static void FreeStack()
{
	if(StackSize > 0)
		(void)free(Stack);
	Stack = (char **)NULL;
	StackSize = 0;
	Top   = -1;
}

/*
 * Free Nargv
 *
 */
static void FreeNargv(nargc, nargv)
int nargc;
char *nargv[];
{
	register int i;

	for(i = 0; i < nargc; i++)
		(void)free(nargv[i]);
	if(nargc > 0)
		(void)free(nargv);
}

/*
 * Free the PruneList
 *
 */
static void FreePrune()
{
	register PRUNELIST *p, *next;

	for(p = PruneList; p != (PRUNELIST *)NULL; p = next)
	{
		next = p->next;
		(void)free(p);
	}
	PruneList = (PRUNELIST *)NULL;
}

/*
 * FreeUp before bad exit
 *
 */
static void FreeUp(nargc, nargv)
int nargc;
char **nargv;
{
	FreeStack();
	FreePrune();
	FreeNargv(nargc, nargv);
}

/*
 * Push a directory name on Stack
 *
 */
static int PushDir(name)
char *name;
{
#ifdef __GNUC__
	extern void *malloc(size_t);
#else
	extern char *malloc();
#endif
	extern char *strcpy();
#ifdef __GNUC__
	extern size_t strlen();
#else	
	extern int strlen();
#endif
	extern char **ExpandStack();

#ifdef DDEBUG
printf("PushDir: %s\n", name);
#endif

	++Top;
	if(Top >= StackSize)
	{
		if((Stack = ExpandStack(Stack)) == (char **)NULL)
		    return(~OK);
	}
	
	if((Stack[Top] = malloc(strlen(name)+1)) == (char *)NULL)
	{
		/* outa mem */
		fprintf(STDERR,"%s(PushDir()): Out of Memory\n", ProgName);
		return(~OK);
	}
	(void)strcpy(Stack[Top], name);
	return(OK);
}

/*
 * Pop a directory name from the stack
 *
 */
static char *PopDir()
{
	register char *r;
	extern char **ShrinkStack();
	
	if(STACK_EMPTY)
	    return ((char *)NULL);
	
	r = Stack[Top];
	Top--;
	return(r);
}

static int BadStatus = FALSE;
#define BADSTATUS (BadStatus != FALSE)
#define MAXNAMLEN 128

/*
 * Process directories on the Stack, by adding all the
 * files in a directory to nargv.
 */
static int ProcessDirs(nargc, nargv)
int *nargc;
char ***nargv;
{
	register char *name;
	register struct stat *dp;
	register int status, slashp;
	char path[MAXNAMLEN+1];
	extern char **CopyToNargv();
	extern char *PopDir();
	extern char *alltolower();
	
	if(BADSTATUS)
	    return(~OK);

	if(STACK_EMPTY)
	    /* Nothing more to do */
	    return(OK);

	/* Pop a directory from Stack and process */
	if((name = PopDir()) == (char *)NULL)
	{
		/* Oh Oh */
		fprintf(STDERR,"Internal Error (BUG), PopDir returns NULL\n");
		BadStatus = (~FALSE);
		return(~OK);
	}
	
	strcpy(path, name);
	if(path[((int)strlen(path)-1)] == '\\')
	{
		strcat(path,"*.*");
		slashp = TRUE;
	}
	else
	{
		strcat(path,"\\*.*");
		slashp = FALSE;
	}
		
	/* Open the directory */
	if(Fsfirst(path, 0x0020| 0x0010 | 0x0001) != 0)
	{
		/* trouble opening directory */
		fprintf(STDERR,"Trouble opening %s\n",path);
		/* set BADSTATUS and return */
		BadStatus = (~FALSE);
		return(~OK);
	}

	/* get the DTA */
	dp = (struct stat *)Fgetdta();
	
	/* for each entry in the directory, if it is a file
	   add to nargv. If it is a directory, Push it onto
	   the directory stack.
	 */
	do
	{
		if(! ((strcmp(dp->st_name,".") == 0) ||
		      (strcmp(dp->st_name,"..") == 0)) )
		{
			strcpy(path, name);
			if(!slashp)
				strcat(path,"\\");
			strcat(path,dp->st_name);

			/* If this path is on the PruneList skip */
			if(OnPruneList(PruneList, alltolower(path)))
			    continue;

			if(!isdir(path, dp->st_mode))
			{
				/* not a dir -- add this to nargv */
				if((*nargv = CopyToNargv(path, *nargc, *nargv))
				    == (char **)NULL)
				{
					BadStatus = (~FALSE);
					return(~OK);
				}
				else
				{
					*nargc += 1;
				}
				
			}
			else
			{

				/* Push This directory */
				if((status = PushDir(path)) != OK)
				{
					BadStatus = (~FALSE);
					return(status);
				}
			}
		}
	} while(Fsnext() == 0);

	free(name);	/* done with this directory */
	
	/* go do the rest */
	return (ProcessDirs(nargc, nargv));
}

/*
 * Add a name to PruneList
 *
 */
static PRUNELIST *AddPrune(list, name)
PRUNELIST *list;
char *name;
{
#ifdef __GNUC__
	extern void *malloc(size_t);
#else
	extern char *malloc();
#endif
	register PRUNELIST *new;
	
	if((new = (PRUNELIST *)malloc(sizeof(PRUNELIST))) == (PRUNELIST *)NULL)
	{
		/* outa mem */
		fprintf(STDERR,"%s(AddPrune()): Out of Memory\n", ProgName);
		return(new);
	}

	new->name = name;
	new->next = list;
	
	return(new);
}

/*
 * Search for name on PruneList
 *
 */
static int OnPruneList(list, name)
register PRUNELIST *list;
register char *name;		
{
	for(; list != (PRUNELIST *)NULL; list = list->next)
	{
		if(strcmp(list->name, name) == 0)
		    return(~FALSE);
	}
	
	return(FALSE);
}

/*
 * test if a subdirectory exists, without touching the DTA
 * include special case of 'D:\' that Fsfirst does'nt handle correctly
 */
static int isdir(name, attr)
register char *name;
register int attr;
{
	/* assuming the DTA buffer is already set up */
	extern long drv_map;
	register int drive;
	
	if(attr & 0x0010)
		return TRUE;

	/* Gemdos doesn't like d:\ style dirs */
	if((name[3] == '\0') && (name[2] == '\\') && (name[1] == ':'))
	{
		drive = name[0];
		if(isupper(drive))
			drive = tolower(drive);

		drive = drive - 'a';
		if((drv_map & (1L << drive)) == 0)
			return FALSE;
		else
			return TRUE;
	}
	/* Nor does Gemdos understand '.' or '..' */
	/* Hey Atari, don't you guys ever test anything */
	if((strcmp(name,".") == 0) || (strcmp(name,"..") == 0) ||
	   (strcmp(name,".\\") == 0) || (strcmp(name,"..\\") == 0))
		return TRUE;

	return FALSE;
}


#ifdef TEST

/*
 * convert string to all lower case
 */
char *alltolower(s)
char *s;
{
	register char *p;

	for(p = s; *p != '\0'; p++)
		if(isupper(*p))
			*p = tolower(*p);

	return s;
}
	
/*
 * test if a subdirectory exists
 * include special case of 'D:\' that Fsfirst does'nt handle correctly
 * (this routine not needed for zmdm, as it is defined in common.c)
 */
int existd(name)
register char *name;
{
	/* assuming the DTA buffer is already set up */
	/* assumes drv_map has been read in drv_map externally */
	extern long drv_map;
	register int drive;
	extern struct stat statbuf;
	
	if (Fsfirst(name , 0x0021|0x0010) == 0)
	{
		if((statbuf.st_mode & 0x0010) == 0x0010)
			return TRUE;
	}

	/* Gemdos doesn't like d:\ style dirs */
	if((name[3] == '\0') && (name[2] == '\\') && (name[1] == ':'))
	{
		drive = name[0];
		if(isupper(drive))
			drive = tolower(drive);

		drive = drive - 'a';
		if((drv_map & (1L << drive)) == 0)
			return FALSE;
		else
			return TRUE;
	}
	/* Nor does Gemdos understand '.' or '..' */
	/* Hey Atari, don't you guys ever test anything */
	if((strcmp(name,".") == 0) || (strcmp(name,"..") == 0) ||
	   (strcmp(name,".\\") == 0) || (strcmp(name,"..\\") == 0))
		return TRUE;

	return FALSE;
}

int MAIN(argc, argv)
int argc;
char **argv;
{
	register int i;
	
	for(i = 0; i < argc; i++)
	    printf("%d:\t%s\n", i, argv[i]);


	return(0);
}


struct stat statbuf;	  /* Disk Transfer address for Find first etc */
long drv_map;

#if (MWC || __GNUC__)
long _stksize = 128L * 1024L;
#endif

#ifdef MANX
long _STKSIZ = 128L * 1024L;
#endif

main(argc, argv)
int argc;
char **argv;
{
	/* Set up Dta */
	Fsetdta(&statbuf);
	drv_map = Drvmap();

	exit(expandargs(MAIN, argc, argv));
}

#endif /* TEST */

/* -eof- */
