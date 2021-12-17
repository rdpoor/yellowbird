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

#ifndef APP_H_
#define APP_H_

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Includes

#include <stdint.h>
#include <stdbool.h>

#include "osa.h"
#include "rtc.h"
#include "nw_winc.h"

// =============================================================================
// Public Types and Definitions

//messages  for app action thread 
typedef enum APP_MSG_TYPE
{
	APP_TIMER_EXPIRY_MSG,
	APP_RTCTIMER_EXPIRY_MSG,
}APP_MSG_TYPE;

/*  message format for app action thread */
typedef struct App_Msg_Req
{
    APP_MSG_TYPE  msg_type;
    unsigned int   msg_len;
    void* msg_buf;	
} App_MsgReq;

typedef struct APP
{
	osa_thread_handle h_app_thread;
	osa_msgq_type_t app_thread_msg_q;
	osa_timer_type_t h_timer;
    osa_mutex_type_t msg_q_mutex;
    RTC_T Rtc;
    NW_WINC Nw;
} APP;


// =============================================================================
// Public Declarations

void APP_Start(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _TEMPLATE_H_ */
