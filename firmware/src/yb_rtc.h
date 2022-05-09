/**
 * @file yb_rtc.h
 *
 * MIT License
 *
 * Copyright (c) 2022 Klatu Networks, Inc.
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

#ifndef _YB_RTC_H_
#define _YB_RTC_H_

// *****************************************************************************
// Includes

#include <stdbool.h>
#include <stdint.h>

// =============================================================================
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

/**
 * @brief Representation of time is an unsigned 32 bit value.
 */
typedef uint32_t yb_rtc_tics_t;

/**
 * @brief Durations are expressed as milliseconds.
 */
typedef float yb_rtc_ms_t;

#define YB_RTC_MINIMUM_HIBERNATE_MS ((yb_rtc_ms_t)10.0)

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the rtc (note: resets RTC to 0)
 */
void yb_rtc_init(void);

/**
 * @brief Return the current RTC count.
 */
yb_rtc_tics_t yb_rtc_now(void);

/**
 * @brief Return elapsed milliseconds since the given time.
 */
yb_rtc_ms_t yb_rtc_elapsed_ms(yb_rtc_tics_t since);

/**
 * @brief Return the difference (in milliseconds) between two times: t1-t2
 */
yb_rtc_ms_t yb_rtc_difference_ms(yb_rtc_tics_t t1, yb_rtc_tics_t t2);

/**
 * @brief Add an offset to a time.
 */
yb_rtc_tics_t yb_rtc_offset(yb_rtc_tics_t t, yb_rtc_ms_t offset_ms);

/**
 * @brief Hibernate until the specified time arrives.
 *
 * NOTE: The alarm will be set for t or now + MINIMUM_HIBERNATE_MS, whichever
 * arrives later.
 */
void yb_rtc_hibernate_until(yb_rtc_tics_t t);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _YB_RTC_H_ */
