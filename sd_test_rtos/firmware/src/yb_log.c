/**
 * @file yb_log.c
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

#include "yb_log.h"

#include "app.h"
#include <stdarg.h>
#include <stdio.h>

// *****************************************************************************
// Local (private) types and definitions

// *****************************************************************************
// Local (private, static) storage

#define EXPAND_LEVEL_NAMES(_enum_id, _name) _name,
static const char *s_level_names[] = {YB_LOG_LEVELS(EXPAND_LEVEL_NAMES)};

static yb_log_level_t s_reporting_level;

// *****************************************************************************
// Local (private, static) forward declarations

static const char *get_level_name(yb_log_level_t level);

// *****************************************************************************
// Public code

void yb_log_init(void) { yb_log_set_reporting_level(YB_LOG_LEVEL_INFO); }

void yb_log_set_reporting_level(yb_log_level_t reporting_level) {
  s_reporting_level = reporting_level;
}

void yb_log(yb_log_level_t level, const char *fmt, ...) {
  if (level >= s_reporting_level) {
    va_list ap;
    const char *s = get_level_name(level);
    // printf("%f", ...) hard faults?!?
    // yb_rtc_ms_t t = app_uptime_ms();
    // printf("\n%f [%s] ", t, s);
    int ms = app_uptime_ms();
    printf("\n%08d [%s] ", ms, s);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }
}

// *****************************************************************************
// Local (private, static) code

static const char *get_level_name(yb_log_level_t level) {
  const char *s = s_level_names[level];
  return s;
}
