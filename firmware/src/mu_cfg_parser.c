/**
 * @file mu_cfg_parser.c
 *
 * MIT License
 *
 * Copyright (c) 2020 R. D. Poor <rdpoor@gmail.com>
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

#include "mu_cfg_parser.h"

#include "mu_str.h"
#include "mu_strbuf.h"
#include <limits.h>
#include <stdbool.h>
#include <string.h>

// *****************************************************************************
// Local (private) types and definitions

// *****************************************************************************
// Local (private, static) forward declarations

static bool is_whitespace(char ch);

// static char *print_mu_str(mu_str_t *str); // debugging

// *****************************************************************************
// Local (private, static) storage

// *****************************************************************************
// Public code

mu_cfg_parser_t *mu_cfg_parser_init(mu_cfg_parser_t *parser,
                                    mu_cfg_on_match_fn on_match) {
  parser->on_match = on_match;
  return parser;
}

mu_cfg_parser_err_t mu_cfg_parser_read_line(mu_cfg_parser_t *parser,
                                            const char *line) {
  mu_str_t trimmed;
  size_t idx;
  size_t key_len;

  // initialize a strbuf and a str from the input line for no-copy manipulation
  mu_strbuf_init_from_cstr(&parser->buf, line);
  mu_str_init_rd(&trimmed, &parser->buf);

  // remove '#' comment char and any chars that follow it
  idx = mu_str_index(&trimmed, '#');
  if (idx != MU_STR_NOT_FOUND) {
    mu_str_slice(&trimmed, &trimmed, 0, idx);
  }

  // strip leading and trailing whitespace
  mu_str_trim(&trimmed, is_whitespace);

  if (mu_str_available_rd(&trimmed) == 0) {
    // Line is empty after removing comment and whitespace.. Return without err.
    return MU_CFG_PARSER_ERR_NONE;
  }

  // find '=' that separates key and value
  idx = mu_str_index(&trimmed, '=');
  if (idx == MU_STR_NOT_FOUND) {
    // No '=' was found.  Can't process...
    return MU_CFG_PARSER_ERR_BAD_FMT;
  }

  // split the line on either side of the = sign
  mu_str_slice(&parser->key, &trimmed, 0, idx);
  mu_str_slice(&parser->value, &trimmed, idx + 1, INT_MAX);

  // strip leading and trailing whitespace around the key and value
  mu_str_trim(&parser->key, is_whitespace);
  mu_str_trim(&parser->value, is_whitespace);

  // key cannot be empty (but value may be).
  key_len = mu_str_available_rd(&parser->key);
  if (key_len == 0) {
    return MU_CFG_PARSER_ERR_BAD_FMT;
  }

  parser->on_match(&parser->key, &parser->value);
  return MU_CFG_PARSER_ERR_NONE;
}

// *****************************************************************************
// Local (private, static) code

static bool is_whitespace(char ch) {
  return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r');
}

// debugging -- NOTE: returned value gets overwritten at next call
// static char *print_mu_str(mu_str_t *str) {
//   static char buf[50];
//   memset(buf, '\0', sizeof(buf));
//   memcpy(buf, mu_str_ref_rd(str), mu_str_available_rd(str));
//   return buf;
// }

// *****************************************************************************
// standalone test

/*
(gcc -DMU_CFG_PARSER_STANDALONE_TEST -Wall -g -I../core -o mu_cfg_parser \
   mu_cfg_parser.c ../core/mu_str.c ../core/mu_strbuf.c \
   && ./mu_cfg_parser \
   && rm ./mu_cfg_parser)
*/

#ifdef MU_CFG_PARSER_STANDALONE_TEST

#include <assert.h>
#include <stdio.h>
#define ASSERT assert

static mu_str_t *s_key;
static mu_str_t *s_val;

static void on_match(mu_str_t *key, mu_str_t *value) {
  s_key = key;
  s_val = value;
}

static bool matched_key_equals(const char *str) {
  size_t len = mu_str_available_rd(s_key);
  if (len != strlen(str)) {
    return false;
  } else {
    return strncmp(str, (const char *)mu_str_ref_rd(s_key), len) == 0;
  }
}

static bool matched_value_equals(const char *str) {
  size_t len = mu_str_available_rd(s_val);
  if (len != strlen(str)) {
    return false;
  } else {
    return strncmp(str, (const char *)mu_str_ref_rd(s_val), len) == 0;
  }
}

int main(void) {
  mu_cfg_parser_t parser;
  printf("Beginning standalone tests...\n");

  // Case sensitive parser.
  ASSERT(mu_cfg_parser_init(&parser, on_match) == &parser);

  // a blank line is legal
  ASSERT(mu_cfg_parser_read_line(&parser, "") == MU_CFG_PARSER_ERR_NONE);

  // a line with only a comment is legal
  ASSERT(
      mu_cfg_parser_read_line(&parser, "# a line containing only a comment") ==
      MU_CFG_PARSER_ERR_NONE);

  // legitimate key/value
  ASSERT(mu_cfg_parser_read_line(&parser, "fruit=apple") ==
         MU_CFG_PARSER_ERR_NONE);
  ASSERT(matched_key_equals("fruit"));
  ASSERT(matched_value_equals("apple"));

  // legitimate key/value with whitespace
  ASSERT(mu_cfg_parser_read_line(&parser, " weight = 23 ") ==
         MU_CFG_PARSER_ERR_NONE);
  ASSERT(matched_key_equals("weight"));
  ASSERT(matched_value_equals("23"));

  // legitimate key/value with trailing comment
  ASSERT(mu_cfg_parser_read_line(&parser, " color = red # trailing comment") ==
         MU_CFG_PARSER_ERR_NONE);
  ASSERT(matched_key_equals("color"));
  ASSERT(matched_value_equals("red"));

  // blank value is accepted
  ASSERT(mu_cfg_parser_read_line(&parser, " color = # trailing comment") ==
         MU_CFG_PARSER_ERR_NONE);
  ASSERT(matched_key_equals("color"));
  ASSERT(matched_value_equals(""));

  // missing = delimeter
  ASSERT(mu_cfg_parser_read_line(&parser, " color") ==
         MU_CFG_PARSER_ERR_BAD_FMT);

  // not fooled by trailing comment
  ASSERT(mu_cfg_parser_read_line(&parser, " color # = red") ==
         MU_CFG_PARSER_ERR_BAD_FMT);

  // missing key
  ASSERT(mu_cfg_parser_read_line(&parser, " = 15") ==
         MU_CFG_PARSER_ERR_BAD_FMT);

  printf("...done\n");
}

#endif
