 	/***********************************************\
 	*						*
 	*  Decl.h - extern declarations for C libarary  *
 	*      must come after <stdio.h> is included    *
	*						*
	*    Preprocessor symbol `MWC' should be        *
	*    defined when using Mark Williams C		*
	*						*
	*    Preprocessor symbol `MANX' should be       *
	*    defined when using Manx Aztec C		*
	*						*
	*    Preprocessor symbol `__GNUC__' should be   *
	*    defined when using GNU C			*
 	*						*
	\***********************************************/

#ifndef DECL_H

#ifdef __GNUC__
#include <types.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#else /* __GNUC__ */

extern	FILE *fopen(), *freopen(), *fdopen();
#if (!(MANX || MWC))
#ifndef DLIBS
extern	FILE *fopena(), *freopa();
extern	FILE *fopenb(), *freopb();
#endif /* DLIBS */
#endif /* MANX || MWC */

#if MANX
extern FILE *tmpfile();
#endif

extern	char	*etoa();
extern	char	*ftoa();
#if (!(MWC || MANX))
#ifndef DLIBS
extern	char	*getpass();
#endif
#endif

#ifndef MANX
extern	char	*index();
extern	char	*rindex();
#endif

extern	char	*mktemp();
extern	char	*strcat();
extern	char	*strcpy();
extern	char	*strncat();
extern	char	*strncpy();
extern	char	*calloc(), *malloc(), *realloc();
#if (MWC || MANX || DLIBS)
extern char *getenv();
#endif

#if (!(MWC || MANX))
extern	char	*sbrk();
#endif
extern	char 	*gets(), *fgets();
#if (!(MWC || MANX))
#ifndef DLIBS
extern	char 	*ttyname();
#endif
#endif

#if (MWC || MANX)
extern  char    *lmalloc(), *lcalloc(), *lrealloc();
#endif

extern	double	atan();
extern	double	atof();
extern	double	ceil();
extern	double	cos();
extern	double	exp();
extern	double	fabs();
extern	double	floor();
extern	double	fmod();
extern	double	log();
extern	double	pow();
extern	double	sin();
extern	double	sinh();
extern	double	sqrt();
extern	double	tan();
extern	double	tanh();

extern	int	strlen();
extern	int	(*signal())();

extern	long 	atol();
extern	long 	ftell();
extern	long	getl();

#ifndef DLIBS
extern	long 	lseek(), tell();
#endif

#if (MWC || MANX)
extern char	*memchr(), *memcpy(), *memset();
extern char	*strchr(), *strerror(), *strpbrk(), *strrchr();
extern char	*strstr(), *strtok();
extern double	log10();
extern double	frexp();
extern double	ldexp();
extern double	modf();
extern double	asin(), acos(), atan2(), cosh();
#endif /* MWC || MANX */

#ifdef MANX
extern char	*memmove(), *memccpy(), *lmemccpy(), *lmemcpy();
extern char	*lmemmove(), *lmemchr(), *lmemset(), *strdup();
extern char	*scdir(), *tmpnam(), *bsearch();
extern long	labs();
extern double	cotan();

/* BSD compitibility */
#define index	strchr
#define rindex	strrchr

#endif /* MANX */

#endif /* __GNUC__ */

#define DECL_H
#endif

/* -eof- */
