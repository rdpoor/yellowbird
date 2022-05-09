/**
 * @file app.h
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

#ifndef _APP_H_
#define _APP_H_

// *****************************************************************************
// Includes

#include "mu_strbuf.h"
#include "yb_rtc.h"
#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

#define APP_HOST_NAME "example.com"
#define APP_HOST_IP_ADDR NULL        // use APP_HOST_NAME
#define APP_HOST_PORT 443            // https
#define APP_HOST_USE_TLS true

/**
 * @brief Data that is preserved across reboots.
 */
typedef struct {
  uint32_t reboot_count;   // # of times system rebooted
  uint32_t success_count;  // # of times app completed HTTP exchange
  yb_rtc_tics_t wake_at;   // RTC time at which app woke up
} app_nv_data_t;

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the app.
 */
void APP_Initialize(void);

/**
 * @brief Advance the app state machine
 */
void APP_Tasks(void);

/**
 * @brief Return true if the most recent wake was NOT an RTC wake from sleep.
 */
bool app_is_cold_boot(void);

/**
 * @brief Return the number of milliseconds since the last reboot
 */
yb_rtc_ms_t app_uptime_ms(void);

/**
 * @brief Return a reference to a buffer holding the HTTP request (header and
 * body)
 */
mu_strbuf_t *app_request_msg();

/**
 * @brief Return a reference to a buffer to receive the HTTP response.
 */
mu_strbuf_t *app_response_msg();

// *****************************************************************************
// EOF

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _APP_H_ */
