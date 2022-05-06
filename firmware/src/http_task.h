/**
 * @file http_task.h
 *
 * MIT License
 *
 * Copyright (c) 2022 R. Dunbar Poor
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

#ifndef _HTTP_TASK_H_
#define _HTTP_TASK_H_

// *****************************************************************************
// Includes

#include "driver/driver_common.h"
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
 * @param host_name The name of the host to connect to.  If host_ipv4 is zero,
 *        then host_name is used for a DNS lookup.  If host_ipv4 is non-zero,
 *        the host_name parameter is ignored and no DNS lookup is performed.
 * @param host_ipv4 An IP Version 4 numeric address that identifies the host.
 * @param use_tls If true, use TLS to when connecting.
 * @param req_str The user-supplied buffer containing the entire HTTP request,
 *        including header and body.
 * @param req_length The length of the req_buf.
 * @param rsp_str The user supplied buffer for receiving the HTTP response.
 * @param rsp_length The length of the rsp_buf.  If the response exceeds this
 *        limit, it will be truncated.
 * @param on_completion If non-null, specifies a callback to be triggered when
 *        the http_task completes, either successfully or on failure.
 * @param completion_arg A user-supplied argument to be passed to the callback.
 */
void http_task_init(const char *host_name,
                    uint32_t host_ipv4,
                    uint16_t host_port,
                    bool use_tls,
                    const char *req_str,
                    size_t req_length,
                    char *rsp_str,
                    size_t rsp_length,
                    void (*on_completion)(void *arg),
                    void *completion_arg);

/**
 * @brief Advance the http_task state machine.
 */
void http_task_step(DRV_HANDLE handle);

bool http_task_succeeded(void);

bool http_task_failed(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _HTTP_TASK_H_ */
