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

#ifndef RTC_H_
#define RTC_H_

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Includes

//#include "hpl_calendar.h"
//#include "hal_calendar.h"

#include <stdint.h>
#include <stdbool.h>
#include "osa.h"

// =============================================================================
// Public Types and Definitions

typedef enum 
{
    RTC_MODE_32_COUNTER,
    RTC_MODE_16_COUNTER,
    RTC_CALENDER
} RTC_mode;


typedef struct RTC_T
{
    RTC_mode  mode;
   // struct calendar_alarm alarm;
    Callback_1_Arg  pFunction;
    void*          pArg;

} RTC_T;

// =============================================================================
// Public Declarations

void RTC_Init(RTC_T *pThis, RTC_mode mode);

void RTC_SetDateTime(unsigned char* time);

void RTC_SetAlaram(unsigned int time_duration_sec,Callback_1_Arg function, void *arg);

unsigned int RTC_GetTimeStamp();

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _TEMPLATE_H_ */
