/**
 * MIT License
 *
 * Copyright (c) 2020 R. Dunbar Poor <rdpoor@gmail.com>
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

#ifndef _MU_STR_H_
#define _MU_STR_H_

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Includes

#include "mu_strbuf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// =============================================================================
// Types and definitions

#define MU_STR_NOT_FOUND SIZE_MAX

typedef struct {
  const mu_strbuf_t *buf; // reference to underlying buffer
  size_t s;               // index of next byte to be read, or start of string
  size_t e;               // index of next byte to be written, or end of string
} mu_str_t;

// =============================================================================
// Declarations

/**
 * @brief Initialize a mu_str
 *
 * @param str The mu_str to be initialized
 * @param buf The underlying mu_str_buf
 * @return str
 */
mu_str_t *mu_str_init_rd(mu_str_t *str, const mu_strbuf_t *buf);

mu_str_t *mu_str_init_wr(mu_str_t *str, const mu_strbuf_t *buf);

/**
 * @brief Reset start and end pointers for a read buffer
 *
 * Note: s is set to 0, e is set to buf->capacity.
 *
 * @param str A mu_str
 * @return str
 */
mu_str_t *mu_str_reset_rd(mu_str_t *str);

/**
* @brief Reset start and end pointers for a write buffer
*
* Note: s and e are set to 0.
*
* @param str A mu_str
* @return str
 */
mu_str_t *mu_str_reset_wr(mu_str_t *str);

/**
 * @brief Make a shallow copy of a mu_str
 *
 * After calling this, dst and src will refer to the same underlying string.
 *
 * @param dst The mu_str to be copied into.
 * @param src The mu_str to copy from
 * @return dst
 */
mu_str_t *mu_str_copy(mu_str_t *dst, const mu_str_t *src);

/**
 * @brief Search for a byte in a string.
 *
 * @param str the mu_str
 * @param byte the byte to search for.
 * @return the index of the byte, relative to the str start index, or
 *         MU_STR_NOT_FOUND if if the byte was not found between the
 *         start and end of str.
 */
size_t mu_str_index(mu_str_t *str, uint8_t byte);

/**
 * @brief Make a slice of a mu_str
 *
 * mu_str_slice takes a substring of a mu_str.  A negative index mean "from
 * the end of src".  Assume src refers to the string "abcdefg":
 *
 *   start end  result     comment
 *       0   1  "a"
 *       0   7  "abcdefg"
 *       0   8  "abcdefg"  Out-of-range indeces are clamped
 *       0  -1  "abcdef"   Negative index counts from end
 *      -2  -1  "f"        Negative indexing works for start as well
 *       0   0  ""         Emptry string
 *       1   0  ""         Clamping is in effect here as well.
 *
 * Notes:
 * * The resulting dst is always equal to or shorter than the src
 * * mu_str_slice manipulates indeces; it does not move any data bytes.
 * * dst and src may point to the same object.
 *
 * @param dst The ref to be modified
 * @param src The ref from which to take a slice
 * @param start The starting index, relative to src start
 * @param end The ending index, relative to src start
 * @return dst
 */
mu_str_t *mu_str_slice(mu_str_t *dst, const mu_str_t *src, int start, int end);

/**
 * @brief Return the number of bytes available for reading.
 *
 * @param str A mu_str
 * @return The number of bytes available for reading, i.e. e-s
 */
size_t mu_str_available_rd(const mu_str_t *str);

/**
 * @brief Return the number of bytes available for writing.
 *
 * @param str A mu_str
 * @return The number of bytes available for writing, i.e. capacity - e
 */
size_t mu_str_available_wr(const mu_str_t *str);

/**
 * @brief Advance the start index of a mu_str
 *
 * This adds n_bytes to the start index of a mu_str.  It does not move any
 * data.
 *
 * @param str The mu_str to modify
 * @param n_bytes The amount by which to increment the start index. The start
 *        index is never moved beyond the end index.
 * @return The number of bytes by which the start index was incremented.
 */
size_t mu_str_increment_start(mu_str_t *str, size_t n_bytes);

/**
 * @brief Advance the end index of a mu_str
 *
 * This adds n_bytes to the end index of a mu_str.  It does not move any
 * data.
 *
 * @param str The mu_str to modify
 * @param n_bytes The amount by which to increment the end index. The end index
 *        is never moved beyond the capacity of the underlying string.
 * @return The number of bytes by which the end index was incremented.
 */
size_t mu_str_increment_end(mu_str_t *str, size_t n_bytes);

/**
 * @brief Return a pointer to the start of the underlyng string.
 *
 * @param str The mu_str
 * @return A pointer to the start of the underlying string
 */
const uint8_t *mu_str_ref_rd(const mu_str_t *str);

/**
 * @brief Return a pointer to the end of the underlyng string.
 *
 * @param str The mu_str
 * @return A pointer to the end of the underlying string
 */
uint8_t *mu_str_ref_wr(const mu_str_t *str);

/**
 * @brief Read one byte from the underlying string.  Return it by reference.
 *
 * @param str The mu_str
 * @param byte Pointer to the byte the receive the read byte.
 * @return True if the byte was read, false if there were no bytes available for
 *         reading.
 */
bool mu_str_read_byte(mu_str_t *str, uint8_t *byte);

/**
 * @brief Write one byte to the underlying string.
 *
 * @param str The mu_str
 * @param byte The byte to write.
 * @return True if the byte was written, false if the underlying string was
 *         already full.
 */
bool mu_str_write_byte(mu_str_t *str, uint8_t byte);

/**
 * @brief Append the contents of one mu_str to another.
 *
 * @param dst The mu_str to receive the bytes.
 * @param src The mu_str providing the bytes.
 * @return The number of bytes copied.
 */
size_t mu_str_append(mu_str_t *dst, const mu_str_t *src);

/**
 * @brief Append a null-terminated C-style string to a mu_str
 *
 * Note: the null byte itself is not written.
 *
 * @param dst The mu_str to receive the bytes.
 * @param cstr The null-terminated C-style string to provide the bytes.
 * @return The number of bytes copied.
 */
size_t mu_str_append_cstr(mu_str_t *dst, const char *cstr);

/**
 * @brief Printf to a mu_str.
 *
 * @param dst The mu_str to receive the bytes.
 * @param fmt The printf-style format string
 * @param ... Arguments to the printf
 * @return The number of bytes copied.
 */
size_t mu_str_printf(mu_str_t *dst, const char *fmt, ...);

/**
 * @brief Copy the contents of a mu_str to a C-style string
 *
 * Note: the resulting string is never longer than len, and is always null
 * terminated.
 *
 * @param src The mu_str from which to copy
 * @param cstr A pointer to the C-style string to receive the bytes
 * @param len The cacpacity of the C-string
 * @return The number of bytes copied, not including the null byte.
 */
size_t mu_str_to_cstr(const mu_str_t *src, char *cstr, size_t len);

/**
 * @brief Compare str1 and str2, using the c lib function strncmp()
 * *
 *
 * Note: If either string is shorter than n, the comparison will cease there
 *
 * @param str1 The first mu_str to compare
 * @param str2 The second mu_str to compare
 * @param n The maximum number of characters to compare
 * @return returning less than, equal to or greater than zero if str1 is
 * lexicographically less than, equal to or greater than str2.
 */
int mu_str_strncmp(mu_str_t *str1, mu_str_t *str2, size_t len);

/**
 * @brief Compare str1 and str2, using the c lib function strncmp(), passing in
 * the length of the shorter of the two strings
 * *
 * @param str1 The first mu_str to compare
 * @param str2 The second mu_str to compare
 * @return returning less than, equal to or greater than zero if str1 is
 * lexicographically less than, equal to or greater than str2.
 */
int mu_str_strcmp(mu_str_t *str1, mu_str_t *str2);

/**
 * @brief Search for C-style string in a mu_str.
 *
 * Note: The search begins at the current start index of the mu_str and leaves
 * it unmolested.
 *
 * @param src The mu_str to search
 * @param cstr A pointer to the C-style string to look for
 * @return The offset index from str->s at which the cstr begins, -1 if not
 * found, 0 if cstr is empty
 */
int mu_str_find(mu_str_t *str, char *substring);

mu_str_t *mu_str_trim_left(mu_str_t *str, bool (*predicate)(char ch));
mu_str_t *mu_str_trim_right(mu_str_t *str, bool (*predicate)(char ch));
mu_str_t *mu_str_trim(mu_str_t *str, bool (*predicate)(char ch));

// TBD:
// mu_str_equals -- mu_str_cmp() == 0
// mu_str_eq ? -- do two mu_str object point to the same bytes?

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MU_STR_H_ */
