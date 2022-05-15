/**
 * MIT License
 *
 * Copyright (c) 2021-2022 R. D. Poor <rdpoor@gmail.com>
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

/**
 * @file mu_cfg_parser.h
 *
 * @brief Parse "config.txt-like" formatted key-value pairs.  For example:
 *
 * key1=value1
 * key2 = value2
 *   key3 = value3
 * key4 = value4    # a hashtag introduces a comment to the end of line
 * # lines with no key-value pairs are allowed.
 *
 */

#ifndef _MU_CFG_PARSER_H_
#define _MU_CFG_PARSER_H_

// *****************************************************************************
// Includes

#include "mu_strbuf.h"
#include "mu_str.h"
#include <stdbool.h>

// *****************************************************************************
// C++ Compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

typedef enum {
    MU_CFG_PARSER_ERR_NONE,
    MU_CFG_PARSER_ERR_BAD_FMT,
} mu_cfg_parser_err_t;

typedef void (*mu_cfg_on_match_fn)(mu_str_t *key, mu_str_t *value);

typedef struct {
    mu_strbuf_t buf;
    mu_str_t key;
    mu_str_t value;
    mu_cfg_on_match_fn on_match;
} mu_cfg_parser_t;

// *****************************************************************************
// Public declarations

mu_cfg_parser_t *mu_cfg_parser_init(mu_cfg_parser_t *parser,
                                    mu_cfg_on_match_fn on_match);

mu_cfg_parser_err_t mu_cfg_parser_read_line(mu_cfg_parser_t *parser,
                                            const char *line);


// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MU_CFG_PARSER_H_ */
