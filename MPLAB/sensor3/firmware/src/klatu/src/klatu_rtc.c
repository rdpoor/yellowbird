/**
 * MIT License
 *
 * Copyright (c) 2021 Klatu Networks
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// =============================================================================
// Includes

#include "klatu_rtc.h"
//#include "driver_init.h"
//#include "utils.h"
#include "definitions.h"  
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "trace.h"

// =============================================================================
// Private types and definitions

// =============================================================================


#define SECS_IN_LEAP_YEAR 31622400
#define SECS_IN_NON_LEAP_YEAR 31536000
#define SECS_IN_31DAYS 2678400
#define SECS_IN_30DAYS 2592000
#define SECS_IN_29DAYS 2505600
#define SECS_IN_28DAYS 2419200
#define SECS_IN_DAY 86400
#define SECS_IN_HOUR 3600
#define SECS_IN_MINUTE 60
#define DEFAULT_BASE_YEAR 1900


// Private (forward) declarations
static void rtc_calender_set_alarm(unsigned int time_duration_sec);

static void rtc_alarm_callback_handler();

static bool rtc_leap_year(uint16_t year);

static uint32_t rtc_get_secs_in_month(uint32_t year, uint8_t month);

static int32_t rtc_convert_timestamp_to_datetime(unsigned int base_year, uint32_t ts,
                                             struct tm* dt);
static uint32_t rtc_convert_datetime_to_timestamp(int base_year, struct tm* dt);
// =============================================================================
// Private storage
RTC_T* pRtc;


// =============================================================================
// Public code

void RTC_Init(RTC_T *pThis, RTC_mode mode)
{
    pRtc = pThis;

    memset ( pRtc ,0x00, sizeof(RTC_T));

    pRtc->mode = mode;

    switch( mode)
    {
        case RTC_MODE_32_COUNTER:
            break;

        case RTC_MODE_16_COUNTER:
            break;

        case RTC_CALENDER:
            /* calender is intialised as part of system_init() */
         //   calendar_enable(&CALENDAR_0);
            break;
    } 
}

void RTC_SetDateTime(unsigned char* datetime)
{
    struct tm  cur_time;
    unsigned char* p = NULL;    
    unsigned char* q = NULL;
    unsigned char*  date_str = datetime;
    unsigned char*  time_str;
    uint32_t current_ts;
    /*expected time format  dd/mm/year hr/mm/ss */

    if( date_str)
    {
        p = date_str;
        q = (unsigned char*)strchr((char*)p,'/');
        *q =0x00;
        cur_time.tm_mday = atoi((char*)p); 
        *q = '/';

        q++;
        p = q;
        q = (unsigned char*)strchr((char*)p,'/');
        *q =0x00;
        cur_time.tm_mon = atoi((char*)p); 
        *q = '/';

        q++;
        p = q;
        q = (unsigned char*)strchr((char*)p,' ');
        *q =0x00;
        cur_time.tm_year = atoi((char*)p); 

//cur_time.tm_year -=2016;
    }
    q++;
    time_str = q;
    if( time_str)
    {
        /* Hour */
        p = time_str;
        q = (unsigned char*)strchr((char*)p,':');
        *q =0x00;
        cur_time.tm_hour = atoi((char*)p); 
        *q = ':';

        /* minute */
        q++;
        p = q;
        q = (unsigned char*)strchr((char*)p,':');
        *q =0x00;
        cur_time.tm_min = atoi((char*)p); 
        *q = ':';

        /* second */
        q++;
        p = q;
        cur_time.tm_sec = atoi((char*)p); 
    }
    if(pRtc->mode != RTC_CALENDER)
    {
        TRACE_WARN("RTC mode is not Calender \n");
        return;
    }

//	TRACE_DBG("YEAR = %d \n",cur_time.tm_year);
#if 0
    RTC_RTCCTimeSet(&cur_time);
#else 
   current_ts = rtc_convert_datetime_to_timestamp(2016, &cur_time);
   RTC_REGS->MODE2.RTC_CLOCK  = current_ts ;
	while((RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk) == RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk)
    {
        /* Synchronization before reading value from CLOCK Register */
    }
#endif
}


void  RTC_SetAlaram(unsigned int time_duration_sec,Callback_1_Arg function, void *arg)
{    
    pRtc->pFunction = function;
    pRtc->pArg = arg;

    switch( pRtc->mode)
    {
        case RTC_MODE_32_COUNTER:
            break;

        case RTC_MODE_16_COUNTER:
            break;

        case RTC_CALENDER:
			rtc_calender_set_alarm(time_duration_sec);
            break;
    } 
}
unsigned int RTC_GetTimeStamp()
{
    uint32_t current_ts;
    struct tm  cur_time;
	while((RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk) == RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk)
    {
        /* Synchronization before reading value from CLOCK Register */
    }
    current_ts = RTC_REGS->MODE2.RTC_CLOCK ;

    rtc_convert_timestamp_to_datetime(2016, current_ts,&cur_time);
  // 	TRACE_DBG("Current Time Stamp = %lu \n",current_ts); 
  //  TRACE_DBG("The Current date/time  %d/%d/%d  %d:%d:%d\n",cur_time.tm_mday, cur_time.tm_mon,cur_time.tm_year, cur_time.tm_hour,cur_time.tm_min,cur_time.tm_sec);
//RTC_RTCCTimeGet ( &cur_time);
//TRACE_DBG("The Current date/time NEW   %d/%d/%d  %d:%d:%d\n",cur_time.tm_mday, cur_time.tm_mon,cur_time.tm_year, cur_time.tm_hour,cur_time.tm_min,cur_time.tm_sec);

	return current_ts;
}


// =============================================================================
// Private (static) code

static bool rtc_leap_year(uint16_t year)
{
    if (year & 3) {
        return false;
    } else {
        return true;
    }
}

static uint32_t rtc_get_secs_in_month(uint32_t year, uint8_t month)
{
    uint32_t sec_in_month = 0;

    if (rtc_leap_year(year)) {
        switch (month) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            sec_in_month = SECS_IN_31DAYS;
            break;
        case 2:
            sec_in_month = SECS_IN_29DAYS;
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            sec_in_month = SECS_IN_30DAYS;
            break;
        default:
            break;
        }
    } else {
        switch (month) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            sec_in_month = SECS_IN_31DAYS;
            break;
        case 2:
            sec_in_month = SECS_IN_28DAYS;
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            sec_in_month = SECS_IN_30DAYS;
            break;
        default:
            break;
        }
    }

    return sec_in_month;
}
static int32_t rtc_convert_timestamp_to_datetime(unsigned int base_year, uint32_t ts,
                                             struct tm* dt)
{
    uint32_t tmp, sec_in_year, sec_in_month;
    uint32_t tmp_year    = base_year;
    uint8_t  tmp_month   = 1;
    uint8_t  tmp_day     = 1;
    uint8_t  tmp_hour    = 0;
    uint8_t  tmp_minutes = 0;

    tmp = ts;

    /* Find year */
    while (true) {
        sec_in_year = rtc_leap_year(tmp_year) ? SECS_IN_LEAP_YEAR : SECS_IN_NON_LEAP_YEAR;

        if (tmp >= sec_in_year) {
            tmp -= sec_in_year;
            tmp_year++;
        } else {
            break;
        }
    }
    /* Find month of year */
    while (true) {
        sec_in_month = rtc_get_secs_in_month(tmp_year, tmp_month);

        if (tmp >= sec_in_month) {
            tmp -= sec_in_month;
            tmp_month++;
        } else {
            break;
        }
    }
    /* Find day of month */
    while (true) {
        if (tmp >= SECS_IN_DAY) {
            tmp -= SECS_IN_DAY;
            tmp_day++;
        } else {
            break;
        }
    }
    /* Find hour of day */
    while (true) {
        if (tmp >= SECS_IN_HOUR) {
            tmp -= SECS_IN_HOUR;
            tmp_hour++;
        } else {
            break;
        }
    }
    /* Find minute in hour */
    while (true) {
        if (tmp >= SECS_IN_MINUTE) {
            tmp -= SECS_IN_MINUTE;
            tmp_minutes++;
        } else {
            break;
        }
    }

    dt->tm_year = tmp_year;
    dt->tm_mon = tmp_month;
    dt->tm_mday   = tmp_day;
    dt->tm_hour = tmp_hour;
    dt->tm_min   = tmp_minutes;
    dt->tm_sec   = tmp;

    return -1;
}

static uint32_t rtc_convert_datetime_to_timestamp(int base_year, struct tm* dt)
{
	uint32_t tmp = 0;
	uint32_t i   = 0;
	uint8_t  year, month, day, hour, minutes, seconds;

	year    = dt->tm_year - base_year;
	month   = dt->tm_mon;
	day     = dt->tm_mday;
	hour    = dt->tm_hour;
	minutes = dt->tm_min;
	seconds = dt->tm_sec;

	/* tot up year field */
	for (i = 0; i < year; ++i) {
		if (rtc_leap_year(base_year + i)) {
			tmp += SECS_IN_LEAP_YEAR;
		} else {
			tmp += SECS_IN_NON_LEAP_YEAR;
		}
	}

	/* tot up month field */
	for (i = 1; i < month; ++i) {
		tmp += rtc_get_secs_in_month(dt->tm_year , i);
	}

	/* tot up day/hour/minute/second fields */
	tmp += (day - 1) * SECS_IN_DAY;
	tmp += hour * SECS_IN_HOUR;
	tmp += minutes * SECS_IN_MINUTE;
	tmp += seconds;

	return tmp;
}
static void rtc_calender_set_alarm(unsigned int time_duration_sec)
{
    uint32_t current_ts;
    uint16_t inter_status;
	struct tm  cur_time;
	TRACE_INFO("%s Entry \n", __FUNCTION__);


	while((RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk) == RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk)
    {
        /* Synchronization before reading value from CLOCK Register */
    }
    current_ts = RTC_REGS->MODE2.RTC_CLOCK;
 	rtc_convert_timestamp_to_datetime(2016, current_ts,&cur_time);
   //	TRACE_DBG("Current Time Stamp = %lu \n",current_ts); 
    TRACE_DBG("Present  date/time  %d/%d/%d  %d:%d:%d\n",cur_time.tm_mday, cur_time.tm_mon,cur_time.tm_year, cur_time.tm_hour,cur_time.tm_min,cur_time.tm_sec);

    current_ts +=time_duration_sec;
  	RTC_REGS->MODE2.RTC_ALARM0 = current_ts;
 	while((RTC_REGS->MODE2.RTC_SYNCBUSY & RTC_MODE2_SYNCBUSY_ALARM0_Msk) == RTC_MODE2_SYNCBUSY_ALARM0_Msk)
    {
        /* Synchronization after writing value to CLOCK Register */
    }
 	rtc_convert_timestamp_to_datetime(2016, current_ts,&cur_time);
   //	TRACE_DBG("Alarm Time Stamp = %lu \n",current_ts); 
    TRACE_DBG("Alarm Expected  date/time  %d/%d/%d  %d:%d:%d\n",cur_time.tm_mday, cur_time.tm_mon,cur_time.tm_year, cur_time.tm_hour,cur_time.tm_min,cur_time.tm_sec);

	RTC_REGS->MODE2.RTC_MASK0 = 6; //matching year 

    inter_status = RTC_REGS->MODE2.RTC_INTENSET;
//	TRACE_INFO("%s Alaram Interrupt Status  = %x \n", __FUNCTION__,inter_status);
	RTC_REGS->MODE2.RTC_INTENSET |=0x100;

	inter_status = RTC_REGS->MODE2.RTC_INTENSET;
     
	TRACE_INFO("%s Exit = %x \n", __FUNCTION__,inter_status);
}

static void rtc_alarm_callback_handler()
{ 
    /* alarm expired */
    TRACE_INFO("%s Entry \n", __FUNCTION__);
    if(pRtc->pFunction)
    {
        pRtc->pFunction(pRtc->pArg);
    }
}
