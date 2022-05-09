/**
 * @file http_task.h
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
 */

#ifndef _HTTP_TASK_H_
#define _HTTP_TASK_H_

// *****************************************************************************
// Includes

#include "driver/driver_common.h"
#include "mu_strbuf.h"
#include <stdbool.h>
#include <stddef.h>

// =============================================================================
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the http_task.
 *
 * If host_ipv4 is non-zero, it will be used as the host address.  Otherwise,
 * the http_task will perform a DNS lookup using host_name.  Therefore, one or
 * the other must be provided (but not both).
 *
 * @param winc_handle A handle on the WINC devise.
 * @param host_name The name of the host to connect to.  If host_ipv4 is zero,
 *        then host_name is used for a DNS lookup.  If host_ipv4 is non-zero,
 *        this host_name parameter is ignored and no DNS lookup is performed.
 * @param host_ipv4 An IP Version 4 numeric address that identifies the host.
          If is is NULL, then host_name must resolve to a valid host.
 * @param use_tls If true, use TLS to when connecting.
 * @param req_str The user-supplied buffer containing the entire HTTP request,
 *        including header and body.
 * @param req_length The length of the req_buf.
 * @param rsp_str The user supplied buffer for receiving the HTTP response.
 * @param rsp_length The length of the rsp_buf.  If the response exceeds this
 *        limit, it will be truncated.
 */
void http_task_init(DRV_HANDLE winc_handle,
                    const char *host_name,
                    const char *host_ipv4,
                    uint16_t host_port,
                    bool use_tls,
                    mu_strbuf_t *request_msg,
                    mu_strbuf_t *response_msg);

/**
 * @brief Advance the http_task state machine.
 */
void http_task_step(void);

bool http_task_succeeded(void);

bool http_task_failed(void);

/**
 * @brief Release any resources allocated by http_task.
 */
void http_task_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _HTTP_TASK_H_ */
