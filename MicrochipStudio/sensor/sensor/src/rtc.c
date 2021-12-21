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

#include "rtc.h"
#include "driver_init.h"
#include "utils.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "trace.h"

// =============================================================================
// Private types and definitions

// =============================================================================
// Private (forward) declarations
static void rtc_calender_set_alarm(unsigned int time_duration_sec);

static void rtc_alarm_callback_handler(struct calendar_descriptor *const descr);

// =============================================================================
// Private storage
RTC_T* pRtc;


// =============================================================================
// Public code

extern struct calendar_descriptor CALENDAR_0;
extern int32_t convert_timestamp_to_datetime(struct calendar_descriptor *const calendar, uint32_t ts,
struct calendar_date_time *dt);

void RTC_Init(RTC_T *pThis, RTC_mode mode)
{
    pRtc = pThis;

    memset ( pRtc ,0x00, sizeof(RTC));

    pRtc->mode = mode;

    switch( mode)
    {
        case RTC_MODE_32_COUNTER:
            break;

        case RTC_MODE_16_COUNTER:
            break;

        case RTC_CALENDER:
            /* calender is intialised as part of system_init() */
            calendar_enable(&CALENDAR_0);
            break;
    } 
}

void RTC_SetDateTime(unsigned char* datetime)
{
    struct calendar_time time; 
    struct calendar_date date;
    unsigned char* p = NULL;    
    unsigned char* q = NULL;
    unsigned char*  date_str = datetime;
    unsigned char*  time_str;

    /*expected time format  dd/mm/year hr/mm/ss */

    if( date_str)
    {
        p = date_str;
        q = (unsigned char*)strchr((char*)p,'/');
        *q =0x00;
        date.day = atoi((char*)p); 
        *q = '/';

        q++;
        p = q;
        q = (unsigned char*)strchr((char*)p,'/');
        *q =0x00;
        date.month = atoi((char*)p); 
        *q = '/';

        q++;
        p = q;
        q = (unsigned char*)strchr((char*)p,' ');
        *q =0x00;
        date.year = atoi((char*)p); 
    }
    q++;
    time_str = q;
    if( time_str)
    {
        /* Hour */
        p = time_str;
        q = (unsigned char*)strchr((char*)p,':');
        *q =0x00;
        time.hour = atoi((char*)p); 
        *q = ':';

        /* minute */
        q++;
        p = q;
        q = (unsigned char*)strchr((char*)p,':');
        *q =0x00;
        time.min = atoi((char*)p); 
        *q = ':';

        /* second */
        q++;
        p = q;
        time.sec = atoi((char*)p); 
    }
    if(pRtc->mode != RTC_CALENDER)
    {
        TRACE_WARN("RTC mode is not Calender \n");
        return;
    }
    calendar_set_date(&CALENDAR_0, &date);
    calendar_set_time(&CALENDAR_0, &time);
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
    current_ts = _calendar_get_counter(&CALENDAR_0.device);
	return current_ts;
}


// =============================================================================
// Private (static) code
static void rtc_calender_set_alarm(unsigned int time_duration_sec)
{
    uint32_t current_ts;

    //uint32_t value;

#if 0
    /* to set new date & time */
	struct calendar_date date;	
	struct calendar_time time;

	date.year  = 2000;
	date.month = 12;
	date.day   = 31;
	time.hour = 12;	
	time.min  = 59;
	time.sec  = 59;	
	calendar_set_date(&CALENDAR_0, &date);
	calendar_set_time(&CALENDAR_0, &time);
#endif

		
	TRACE_INFO("%s Entry \n", __FUNCTION__);

    current_ts = _calendar_get_counter(&CALENDAR_0.device);

	TRACE_DBG("Current Time Stamp = %lu \n",current_ts);
	
	convert_timestamp_to_datetime(&CALENDAR_0, current_ts, &(pRtc->alarm.cal_alarm.datetime));
	
	TRACE_DBG("The Current date/time  %d/%d/%d  %d:%d:%d\n",pRtc->alarm.cal_alarm.datetime.date.day, pRtc->alarm.cal_alarm.datetime.date.month,pRtc->alarm.cal_alarm.datetime.date.year, pRtc->alarm.cal_alarm.datetime.time.hour,pRtc->alarm.cal_alarm.datetime.time.min,pRtc->alarm.cal_alarm.datetime.time.sec);



    current_ts +=time_duration_sec;
    memset(&pRtc->alarm,0x00,sizeof(pRtc->alarm));
    convert_timestamp_to_datetime(&CALENDAR_0, current_ts, &pRtc->alarm.cal_alarm.datetime);
	TRACE_DBG("The Expected Alarm  date/time  %d/%d/%d  %d:%d:%d\n",pRtc->alarm.cal_alarm.datetime.date.day, pRtc->alarm.cal_alarm.datetime.date.month,pRtc->alarm.cal_alarm.datetime.date.year, pRtc->alarm.cal_alarm.datetime.time.hour,pRtc->alarm.cal_alarm.datetime.time.min,pRtc->alarm.cal_alarm.datetime.time.sec);
	
	
    pRtc->alarm.cal_alarm.option            = CALENDAR_ALARM_MATCH_YEAR ; //CALENDAR_ALARM_MATCH_YEAR; //CALENDAR_ALARM_MATCH_MONTH ; // CALENDAR_ALARM_MATCH_SEC; //CALENDAR_ALARM_MATCH_YEAR ; 
    pRtc->alarm.cal_alarm.mode              = ONESHOT ; //ONESHOT ;  //REPEAT; 


    calendar_set_alarm(&CALENDAR_0, &pRtc->alarm, rtc_alarm_callback_handler);

	TRACE_INFO("%s Exit \n", __FUNCTION__);
}

static void rtc_alarm_callback_handler(struct calendar_descriptor *const descr)
{ 
    /* alarm expired */
    TRACE_INFO("%s Entry \n", __FUNCTION__);
    if(pRtc->pFunction)
    {
        pRtc->pFunction(pRtc->pArg);
    }
}
