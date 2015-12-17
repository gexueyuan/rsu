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
#if 1
#include <time.h>
#include <sys/time.h>
//#include "nmea.h"
#include "cv_osal.h"
#include <string.h>
#include <stdio.h>
static void get_gps_time(time_t *gps_time)
{

    struct timeval tv;

    struct tm tm_sys;

    memset(&tv,0,sizeof(struct timeval));
    memset(&tm_sys,0,sizeof(struct tm));

    tm_sys.tm_year = gps_time-> 




}
#endif
#if 0
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
		return -EINVAL;

	return 0;
}

static int  gps_hctosys(t_time* gps_time)
{
	int err = -1;
	struct tm tm_sys;
	struct timeval tv;

    tm_sys. = gps_time
	err = rtc_valid_tm(&tm);
	if (err) {
		dev_err(rtc->dev.parent,
			"hctosys: invalid date/time\n");
		goto err_invalid;
	}

	rtc_tm_to_time(&tm, &tv.tv_sec);

	do_settimeofday(&tv);

	dev_info(rtc->dev.parent,
		"setting system clock to "
		"%d-%02d-%02d %02d:%02d:%02d UTC (%u)\n",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		(unsigned int) tv.tv_sec);

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
	xioctl(rtc, RTC_SET_TIME, &tm_time);

	if (ENABLE_FEATURE_CLEAN_UP)
		close(rtc);
}

#endif
/*****************************************************************************
 * implementation of functions                                               *
*****************************************************************************/

