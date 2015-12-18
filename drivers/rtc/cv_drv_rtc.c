/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_drv_rtc.c
 @brief  : get date and time from gps,and sync system time
 @author : gexueyuan
 @history:
           2015-12-15    gexueyuan    Created file
           ...
******************************************************************************/
#include <time.h>
#include <sys/time.h>
#include "nmea.h"
#include "cv_osal.h"
#include <string.h>
#include <stdio.h>
#include <linux/rtc.h>

static const unsigned char rtc_days_in_month[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const unsigned short rtc_ydays[2][13] = {
	/* Normal years */
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	/* Leap years */
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

#define LEAPS_THRU_END_OF(y) ((y)/4 - (y)/100 + (y)/400)

static inline bool is_leap_year(unsigned int year)
{
	return (!(year % 4) && (year % 100)) || !(year % 400);
}


int rtc_month_days(unsigned int month, unsigned int year)
{
	return rtc_days_in_month[month] + (is_leap_year(year) && month == 1);
}
// Die if we can't open a file and return a fd.
int  xopen3(const char *pathname, int flags, int mode)
{
	int ret;

	ret = open(pathname, flags, mode);
	if (ret < 0) {
		osal_printf("can't open '%s'", pathname);
	}
	return ret;
}

// Die if we can't open a file and return a fd.
int  xopen(const char *pathname, int flags)
{
	return xopen3(pathname, flags, 0666);
}

int  usleep(unsigned usec)
{
	struct timespec ts;
	ts.tv_sec = usec / 1000000u;
	ts.tv_nsec = (usec % 1000000u) * 1000u;
	/*
	 * If a signal has non-default handler, nanosleep returns early.
	 * Our version of usleep doesn't return early
	 * if interrupted by such signals:
	 *
	 */
	while (nanosleep(&ts, &ts) != 0)
		continue;
	return 0;
}

/* rtc opens are exclusive.
 * Try to run two "hwclock -w" at the same time to see it.
 * Users wouldn't expect that to fail merely because /dev/rtc
 * was momentarily busy, let's try a bit harder on errno == EBUSY.
 */
static int open_loop_on_busy(const char *name, int flags)
{
	int rtc;
	/*
	 * Tested with two parallel "hwclock -w" loops.
	 * With try = 10, no failures with 2x1000000 loop iterations.
	 */
	int try = 1000 / 20;
 again:
	errno = 0;
	rtc = open(name, flags);
	if (errno == EBUSY) {
		usleep(20 * 1000);
		if (--try != 0)
			goto again;
		/* EBUSY. Last try, exit on error instead of returning -1 */
		return xopen(name, flags);
	}
	return rtc;
}


/* Never fails */
int  rtc_xopen(const char **default_rtc, int flags)
{
	int rtc;
	const char *name =
		"/dev/rtc""\0"
		"/dev/rtc0""\0"
		"/dev/misc/rtc""\0";

	if (!*default_rtc)
		goto try_name;
	name = ""; /*else: we have rtc name, don't try other names */

	for (;;) {
		rtc = open_loop_on_busy(*default_rtc, flags);
		if (rtc >= 0)
			return rtc;
		if (!name[0])
			return xopen(*default_rtc, flags);
 try_name:
		*default_rtc = name;
		name += strlen(name) + 1;
	}
}

static void gps_to_tm(t_time *gps_time,struct tm *tm_sys)
{

    time_t  tt;

    //struct tm tm_sys;

    //tm = & tm_sys;
    //memset(&tv,0,sizeof(struct timeval));
    //memset(tm,0,sizeof(struct tm));

    //UTC time->tm
    tm_sys->tm_year = gps_time->year - 1900;
    tm_sys->tm_mon = gps_time->mon - 1;
    tm_sys->tm_mday = gps_time->day;
    tm_sys->tm_hour = gps_time->hour;
    tm_sys->tm_min = gps_time->min;
    tm_sys->tm_sec = gps_time->sec;

    tt = mktime(tm_sys);

    tm_sys = gmtime(&tt);
   /**********test************ 
    char *string_date = asctime(tm_sys);
    printf("date is %s\n",string_date);
    ****************************/

}
/*
 * Does the rtc_time represent a valid date/time?
 */
static int gps_valid_tm(struct tm *tm)
{
	if (tm->tm_year < 70
		|| ((unsigned)tm->tm_mon) >= 12
		|| tm->tm_mday < 1
		|| tm->tm_mday > rtc_month_days(tm->tm_mon, tm->tm_year + 1900)
		|| ((unsigned)tm->tm_hour) >= 24
		|| ((unsigned)tm->tm_min) >= 60
		|| ((unsigned)tm->tm_sec) >= 60)
		return -1;

	return 0;
}

static int  gps_hctosys(t_time* gps_time)
{
	int err = -1;
	struct tm tm_sys;
	time_t tt;
	struct timezone tz;  
	struct timeval tv;
    
    gps_to_tm(gps_time,&tm_sys);
    
	err = gps_valid_tm(&tm_sys);
	if (err) {
		osal_printf("hctosys: invalid date/time\n");
		goto err_invalid;
	}
    
    tz.tz_minuteswest = timezone/60;
    tz.tz_dsttime = 0;
    
	tt = mktime(&tm_sys);
    tv.tv_sec = tt;
    tv.tv_usec = 0;
	if(settimeofday(&tv,NULL)){
        osal_printf("setting time error!error is %d\n",errno);
    }

	osal_printf("setting system clock to "
		"%d-%02d-%02d %02d:%02d:%02d UTC\n",
		tm_sys.tm_year + 1900, tm_sys.tm_mon + 1, tm_sys.tm_mday,
		tm_sys.tm_hour, tm_sys.tm_min, tm_sys.tm_sec);

err_invalid:

	return err;
}


static void from_sys_clock(const char **pp_rtcname, int utc)
{
	struct timeval tv;
	struct tm tm_time;
	int rtc;

	rtc = rtc_xopen(pp_rtcname, O_WRONLY);
	gettimeofday(&tv, NULL);
	/* Prepare tm_time */
	if (sizeof(time_t) == sizeof(tv.tv_sec)) {
		if (utc)
			gmtime_r((time_t*)&tv.tv_sec, &tm_time);
		else
			localtime_r((time_t*)&tv.tv_sec, &tm_time);
	} else {
		time_t t = tv.tv_sec;
		if (utc)
			gmtime_r(&t, &tm_time);
		else
			localtime_r(&t, &tm_time);
	}

	tm_time.tm_isdst = 0;
	ioctl(rtc, RTC_SET_TIME, &tm_time);

	close(rtc);
}


void gps_to_rtc(t_time* gps_time)
{
    int ret;
    
    ret = gps_hctosys(gps_time);
    if(-1 != ret ){

       // from_sys_clock("\0\0",0);
       system("hwclock -w");
       osal_printf("set rtc from system clock\n");
    }
}

