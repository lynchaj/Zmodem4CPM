/*
 * 	Time conversions Module
 *
 *	Jwahar Bammi
 * 	bang:   {any internet host}!dsrgsun.ces.CWRU.edu!bammi
 * 	domain: bammi@dsrgsun.ces.CWRU.edu
 *	GEnie:	J.Bammi
 */


#include "config.h"
#include <stdio.h>
#include "zmdm.h"

#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif

static int leap P_((int y));
static unsigned long ndays P_((unsigned int since, unsigned int year, unsigned int month, unsigned int day));

#undef P_

/*
 * days in a given year
 */
#define days_in_year(Y) (leap(Y) ? 366 : 365)

/* # of days / month in a normal year */
static unsigned int md[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int leap (y)
int y;
{
	y += 1900;
	if ((y % 400) == 0)
	    return (1);
	if ((y % 100) == 0)
	    return (0);
	return ((y % 4) == 0);
}

/* Return the number of days between Jan 1, Given Year and the given
 * broken-down time.
 */

static unsigned long ndays (since, year, month, day)
unsigned int since, year, month, day;
{
	register unsigned long n = day;
	register unsigned int m, y;
	
	for (y = since; y < year; y++)
	{
		n += 365;
		if (leap (y)) n++;
	}
	if(month > 0)
	    for (m = 0; m < (month-1); m++)
	        n += md[m] + ( ((m == 1) && leap(y))? 1 : 0);

	return (n);
}

/* Convert a broken-down time into seconds
 *
 */

unsigned long tm_to_time (base_year, year, month, day, hours, mins, secs)
unsigned int base_year, year, month, day, hours, mins, secs;
{
	register unsigned long t;
	extern unsigned long ndays();
	
	t = (ndays(base_year, year, month, day) - 1L) * (unsigned long)86400L
	    + hours * (unsigned long)3600L + mins * (unsigned long)60L + secs;

	return t;
}

/*
 * Convert ST time to Unix time
 *
 */
unsigned long st2unix(time, date)
unsigned int time, date;
{
	extern unsigned long tm_to_time();
	
	unsigned int yr = ((date >> 9) & 0x007f) + 80;  /* dissect the date */
	unsigned int mo = (date >> 5) & 0x000f;
	unsigned int dy = date & 0x1f;
	
	unsigned int hr = (time >> 11) & 0x001f;        /* dissect the time */
	unsigned int mm = (time >> 5)  & 0x003f;
	unsigned int ss = (time & 0x001f) * 2;

#ifdef SDEBUG
	printf("%d/%d/%d  %d:%d:%d\n", mo, dy, yr,hr, mm, ss);
#endif
	return (tm_to_time(70, yr, mo, dy, hr, mm, ss) -
		(unsigned long)GMTDIFF);
}

/*
 * Convert Unix Time to ST Time
 *
 */
void unix2st(Unix, time, date)
unsigned long Unix;
unsigned int *date, *time;
{
	long stbase;
	unsigned int hours, yrs, day, months, mins, seconds, t;
	long days, secs;
	extern unsigned long tm_to_time();

#ifdef DEBUG
printf("\n\nUnix Time %ld\n", Unix);
#endif

#if 0
	if((Unix - tm_to_time(70, 80, 0, 0, 0, 0, 0)) <= 0)  /* base 1980 */
#else
	if((Unix - 0x12dc5480) <= 0)  /* base 1980 */
#endif
	{
		/* thats before St's time */
		*time = 0;
		*date = (1 << 5) | 1;	/* Jan 1, 1980 00:00:00 GMT */
#ifdef DEBUG
printf("Before my time\n");
#endif
		return;
	}

	stbase = Unix;	/* do from base year 1970 */

	days = stbase / 86400L;	/* 3600*24 */
	secs = stbase % 86400L + GMTDIFF;
	if(secs < 0)	/* previous day here */
	{
		days -= 1;
		secs += 86400L;
	}

	/* extract hrs : mins : seconds */
	hours = secs / 3600;
	secs = secs - (hours * 3600);
	mins = secs / 60;
	seconds = secs - (mins * 60);
	seconds &= ~1L;			/* ST has 2 sec resolution */

	/* get the year and day of the year */
	for(t = 70; days >= days_in_year(t); t++)
		days -= days_in_year(t);
	yrs = t;
	day = days;

	/* get the month */
	if(days_in_year(yrs) == 366)
		md[1] = 29;

	for(t = 0; day >= md[t]; t++)
		day -= md[t];

	md[1] = 28;		
	day = day + 1;
	months = t + 1;

#ifdef DEBUG
printf("%d/%d/%d   %d:%d:%d\n", months, day, yrs, hours, mins, seconds);
#endif

	yrs -= 80;
	*date = (((yrs & 0x007f) << 9) | ((months & 0x000f) << 5)
	      | (day & 0x001f));
	
	*time = (((hours & 0x001f) << 11) | ((mins & 0x003f) << 5)
	      | (seconds & 0x001e));
}

#ifdef TEST
#include <stdio.h>
#include <osbind.h>

main()
{
	unsigned int time, date;
	unsigned long Unix;
	extern unsigned long st2unix();
	
	time = Tgettime();
	date = Tgetdate();
	Unix = st2unix(time, date);

	printd(time, date);
	printf("Unix Time %ld\n", Unix);

	unix2st(Unix, &time, &date);
	printd(time, date);
}

printd(time, date)
unsigned int time, date;
{
	
	unsigned int yr = ((date >> 9) & 0x007f) + 80;  /* dissect the date */
	unsigned int mo = (date >> 5) & 0x000f;
	unsigned int dy = date & 0x1f;
	
	unsigned int hr = (time >> 11) & 0x001f;        /* dissect the time */
	unsigned int mm = (time >> 5)  & 0x003f;
	unsigned int ss = (time & 0x001f) * 2;

	printf("%d/%d/%d\t%d:%d:%d\n", mo, dy, yr, hr, mm, ss);
}
#endif /* TEST */

/* -eof- */
