/**
 * @file console.h
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

// =============================================================================
// Includes

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Public types and definitions

// =============================================================================
// Public declarations

void console_init(void);
void console_puts(const char *s);
int console_printf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CONSOLE_H_ */
