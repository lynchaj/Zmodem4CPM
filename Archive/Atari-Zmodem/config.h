/*
 * 	Common Configuration file
 *
 *	Jwahar Bammi
 * 	bang:   {any internet host}!dsrgsun.ces.CWRU.edu!bammi
 * 	domain: bammi@dsrgsun.ces.CWRU.edu
 */


/*
 *	Compiler Type
 *
 *		Gnu C compiler
 *			V 1.35 or higher REQUIRED
 *
 *		Mark Williams C
 *			V2.0 or higher REQUIRED
 *			V3 tested. (V 3.0.6 is what i got)
 *
 *		Alcyon C
 *			V 4.14 only tested
 *			DLIBS compatible
 *			ALN OK
 *
 *		MegaMax C/Laser C (or whatever they call it)
 *			I refuse to use braindamaged tools !!
 *
 *		Manx Aztec C
 *			Version 3.6a tested. Produces great code!
 *
 *		Lattice C
 *			Don't have a copy
 *
 *	Comment out one of the #if 0 lines below corresponding to your
 *	compiler.
 */

	/************** if using Gnu C **************/

/* #if 0 */	/* Comment Out this line if using Gnu C */

#define COMPILER	"Compiled with Gnu C V1.39"
#ifdef __GNUC__
#undef __GNUC__
#endif
#define __GNUC__ 1

#ifdef MWC
#undef MWC 
#endif /* MWC */
#ifdef DLIBS
#undef DLIBS		/* only Alcyon version of DLIBS tested */
#endif /* DLIBS */
#ifdef ALCYON
#undef ALCYON
#endif /* ALCYON */
#ifdef MANX
#undef MANX
#endif /* MANX */

/* #endif */	 /* Comment Out this line if using GNU C */

	/************** if using Mark Williams C **************/

#if 0 	/* Comment Out this line if using Mark Williams C */

#define COMPILER	"Compiled with Mark Williams C V3.0.6"
#ifndef MWC
#define MWC 1
#endif /* MWC */
#ifdef DLIBS
#undef DLIBS		/* only Alcyon version of DLIBS tested */
#endif /* DLIBS */
#ifdef ALCYON
#undef ALCYON
#endif /* ALCYON */
#ifdef MANX
#undef MANX
#endif /* MANX */
#ifdef __GNUC__
#undef __GNUC__
#endif /* __GNUC__ */

#endif 	 /* Comment Out this line if using Mark Williams C */

	/************** if using Alcyon C **************/

#if 0	/* comment out this line if using Alcyon C */

#define COMPILER	"Compiled with Alcyon C V4.14"
#ifndef ALCYON
#define ALCYON 1
#endif /* ALCYON */
#ifdef MWC
#undef MWC
#endif /* MWC */
#ifdef MANX
#undef MANX
#endif /* MANX */
#ifdef __GNUC__
#undef __GNUC__
#endif /* __GNUC__ */

#endif	/* Comment Out this line if using Alcyon C */

	/************** if using MANX C **************/

#if 0	/* comment out this line if using MANX C */

#define COMPILER	"Compiled with Manx Aztec C V3.6a"
#ifndef MANX
#define MANX 1
#endif /* MANX */
#ifdef MWC
#undef MWC
#endif /* MWC */
#ifdef ALCYON
#undef ALCYON
#endif /* ALCYON */
#ifdef __GNUC__
#undef __GNUC__
#endif /* __GNUC__ */

#endif 	/* Comment Out this line if using MANX C */

/* ------------------------------------------------------------------------ */

/*
 * Compile Stand Alone  versions of RZ and SZ
 *
 *	STANDALONE is #define'd, to compile `stand alone' alone versions
 *	of RZ and SZ instead of the integrated ZMDM. A lot of folks
 *	requested this feature.
 */

#ifndef STANDALONE
/* #define STANDALONE 1 */	/* define for standalone compilation */
#endif /* STANDALONE */

/* ------------------------------------------------------------------------ */

/*
 * Compile a REMOTE version of ZMDM
 *	REMOTE is #define'd to compile a `remote' version of ZMDM. A remote
 *	versions talks through the serial port, instead of the keyboard/screen
 *	Useful for dialing up your ST to download/upload files. One of the
 *	local BBS's has this version available through a door to enable
 *	people to Up/Down load batch using Zmodem protocol.
 *
 */
#ifndef REMOTE
/* #define REMOTE 1 */		/* define for remote version */
#endif /* REMOTE */
/* ------------------------------------------------------------------------ */

/*
 * Phone Services
 *	PHONES is #define'd to compile in the Phone Services module.
 *	if PHONES is #define'd you must also define PREDIAL and
 *	REDIAL, your modems dial and re-dial commands respectively.
 *	NOTE: The REMOTE version of ZMDM does not support the
 *	phones module.
 */

#ifndef REMOTE
/* #if 0 */ 		/* comment this line if you want the PHONES module */

#define PHONES 1
	/*********** CAUTION: modem specific ***********/
#define PREDIAL		"ATDT"	/* Modems dial command */
#define REDIAL		"A/"	/* Modems Re-dial command */

/* #endif */		/* comment this line if you want the PHONES module */
#endif /* REMOTE */

/* ------------------------------------------------------------------------ */

/*
 * Do flow control while doing terminal emulation -
 * this only needs to be defined if you are
 * going to be running Zmdm at 19200 Baud AND
 * you are going to be sending it (from the host)
 * greater that 16k characters in one blast, without
 * any pause whatsoever. For 99.95 % of us mortals
 * this will never be required. Even if it is defined
 * its no big deal, as flow control will not happen
 * unless the condition described above exists.
 * NOTE: flow control is turned off during file transfers.
 */
#define FLOW_CTRL	1 	/* do flow control */

/* ------------------------------------------------------------------------ */

/*
 * Use high-speed baud-rate code - tends to have very high
 * error rates...
 *	CAUTION: do not define this
 */
#if 0
/* #define	HIBAUD		1  */
#endif

/* ------------------------------------------------------------------------*/

/*
 * OVERSCAN : define if you want Overscan mod support
 */
#define OVERSCAN 1

/* ------------------------------------------------------------------------*/

/*
 *  DYNABUF
 *	If #define'd use up rest up memory less LEAVEALONE bytes for
 *	file buffers. The minimum acceptable size is MINACC
 */
/* #if 0 */	/* Comment this line to use DYNABUF */

#ifndef DYNABUF
#define DYNABUF 1
#define LEAVEALONE 16384L
#define MINACC 	   8096L
#endif

/* #endif */	/* Comment this line to use DYNABUF */

/* ------------------------------------------------------------------------*/

/*
 * Size of file buffer
 *
 *	Must be defined. Must be a long. (only define when
 *	DYNABUF is NOT define'd).
 *
 */
#ifndef DYNABUF
#define BBUFSIZ		32768L		/* Size of file/capture buffer */
#endif /* DYNABUF */

/* ------------------------------------------------------------------------*/

/*
 * Size of Rs232 buffer
 *
 *	Must be defined. Must be int (16 bits).
 */
#define IBUFSIZ		16*1024	/* Size of my Rs232 receive buffer */

/* ------------------------------------------------------------------------*/

/*
 * GMTDIFF
 *	Number of seconds from GMT (signed).
 *	Must be defined.
 *	Must be long.
 */
#define GMTDIFF	(-5L*3600L)	/* EST (usa) -ve means behind GMT */

/* ------------------------------------------------------------------------*/

/*
 * Define RECURSE only if you want the expand a directory
 * to all its children feature for `sz', when you specify
 * a directory name as an arg to `sz'.
 *
 */
#define RECURSE 1

/* ------------------------------------------------------------------------*/

/* 
 * define BIGSTACK if you have very deep directory hierarchies
 *
 */
#ifdef RECURSE
/* #define BIGSTACK 1 */
#endif /* RECURSE */

/* ------------------------------------------------------------------------*/

#include "proto.h"
/* -eof- */
