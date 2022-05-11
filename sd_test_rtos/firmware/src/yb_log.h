/**
 * @file yb_log.h
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

#ifndef _YB_LOG_H_
#define _YB_LOG_H_

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

#define YB_LOG_LEVELS(M)                                                       \
  M(YB_LOG_LEVEL_TRACE, "TRACE")                                               \
  M(YB_LOG_LEVEL_DEBUG, "DEBUG")                                               \
  M(YB_LOG_LEVEL_INFO, "INFO")                                                 \
  M(YB_LOG_LEVEL_WARN, "WARN")                                                 \
  M(YB_LOG_LEVEL_ERROR, "ERROR")                                               \
  M(YB_LOG_LEVEL_FATAL, "FATAL")

#define EXPAND_LOG_LEVEL_ENUM(_enum_id, _name) _enum_id,
typedef enum { YB_LOG_LEVELS(EXPAND_LOG_LEVEL_ENUM) } yb_log_level_t;

#define YB_LOG_TRACE(...) yb_log(YB_LOG_LEVEL_TRACE, __VA_ARGS__)
#define YB_LOG_DEBUG(...) yb_log(YB_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define YB_LOG_INFO(...) yb_log(YB_LOG_LEVEL_INFO, __VA_ARGS__)
#define YB_LOG_WARN(...) yb_log(YB_LOG_LEVEL_WARN, __VA_ARGS__)
#define YB_LOG_ERROR(...) yb_log(YB_LOG_LEVEL_ERROR, __VA_ARGS__)
#define YB_LOG_FATAL(...) yb_log(YB_LOG_LEVEL_FATAL, __VA_ARGS__)

// *****************************************************************************
// Public declarations

void yb_log_init(void);

void yb_log_set_reporting_level(yb_log_level_t reporting_level);

void yb_log(yb_log_level_t level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _YB_LOG_H_ */
