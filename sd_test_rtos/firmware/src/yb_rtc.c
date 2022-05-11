/**
 * @file yb_rtc.c
 *
 * MIT License
 *
 * Copyright (c) 2022 Klatu Networks
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
 *
 */

// *****************************************************************************
// Includes

#include "yb_rtc.h"

#include "definitions.h"
#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Local (private) types and definitions

// *****************************************************************************
// Local (private, static) storage

static yb_rtc_tics_t to_tics(yb_rtc_ms_t offset);

static yb_rtc_ms_t to_ms(yb_rtc_tics_t tics);

// *****************************************************************************
// Local (private, static) forward declarations

// *****************************************************************************
// Public code


void yb_rtc_init(void) {
  RTC_Initialize();   // Reset only on cold boot
  RTC_Timer32Start(); // start RTC
}

yb_rtc_tics_t yb_rtc_now(void) {
  return RTC_Timer32CounterGet();
}

yb_rtc_ms_t yb_rtc_elapsed_ms(yb_rtc_tics_t since) {
  return yb_rtc_difference_ms(RTC_Timer32CounterGet(), since);
}

yb_rtc_ms_t yb_rtc_difference_ms(yb_rtc_tics_t t1, yb_rtc_tics_t t2) {
  return to_ms(t1 - t2);
}

yb_rtc_tics_t yb_rtc_offset(yb_rtc_tics_t t, yb_rtc_ms_t offset_ms) {
  return t + to_tics(offset_ms);
}

void yb_rtc_hibernate_until(yb_rtc_tics_t t) {
  yb_rtc_tics_t now = RTC_Timer32CounterGet();

  yb_rtc_ms_t duration_ms = to_ms(t - now);
  if (duration_ms < YB_RTC_MINIMUM_HIBERNATE_MS) {
    t = now + to_tics(YB_RTC_MINIMUM_HIBERNATE_MS);
  }
  RTC_Timer32Compare0Set(t);
  RTC_Timer32InterruptEnable(RTC_TIMER32_INT_MASK_CMP0);
  PM_HibernateModeEnter();
}

// *****************************************************************************
// Local (private, static) code

static yb_rtc_tics_t to_tics(yb_rtc_ms_t offset) {
  yb_rtc_tics_t t = (offset * RTC_Timer32FrequencyGet()) / 1000;
  return t;
}

static yb_rtc_ms_t to_ms(yb_rtc_tics_t tics) {
  int32_t dt = (int32_t)tics;  // convert to signed: 0xffffffff => -1, etc...
  yb_rtc_ms_t ms = dt * 1000.0 / RTC_Timer32FrequencyGet();
  return ms;
}
