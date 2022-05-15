/**
 * @file winc_imager.c
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

#include "winc_imager.h"

#include "definitions.h"
#include "spi_flash_map.h"
#include "wdrv_winc_client_api.h"
#include "yb_log.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// *****************************************************************************
// Local (private) types and definitions

#define SECTOR_TO_OFFSET(_sector) ((_sector)*FLASH_SECTOR_SZ)
// #define WINC_SECTOR_COUNT 128 // TODO: look up dynamically
#define WINC_SECTOR_COUNT 256 // TODO: look up dynamically
#define WINC_IMAGE_SIZE SECTOR_TO_OFFSET(WINC_SECTOR_COUNT)

#define EXPAND_WINC_IMAGER_STATES                                              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_INIT)                             \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_OPENING_WINC)                     \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_VALIDATING_IMAGE_FILE)            \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_OPENING_IMAGE_FILE)               \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_COMPARING_SECTORS)                \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_READING_FILE_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_READING_WINC_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_COMPARING_BUFFERS)                \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_ERASING_WINC_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_WRITING_WINC_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_INCREMENT_WRITE_SECTOR)           \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_CLOSING_RESOURCES)                \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_SUCCESS)                          \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_ERROR)

#undef DEFINE_WINC_IMAGER_STATE
#define DEFINE_WINC_IMAGER_STATE(_name) _name,
typedef enum { EXPAND_WINC_IMAGER_STATES } winc_imager_state_t;

typedef struct {
  winc_imager_state_t state;
  const char *filename;
  void (*on_completion)(void *arg);
  void *completion_arg;
  SYS_FS_HANDLE file_handle;
  DRV_HANDLE winc_handle;
  uint16_t sector;
} winc_imager_ctx_t;

// *****************************************************************************
// Local (private, static) forward declarations

static void winc_imager_set_state(winc_imager_state_t new_state);
static const char *winc_imager_state_name(winc_imager_state_t state);

/**
 * @brief Set the state to final_state and invoke on_completion.
 */
static void endgame(winc_imager_state_t final_state);

// *****************************************************************************
// Local (private, static) storage

static winc_imager_ctx_t s_winc_imager_ctx;

static uint8_t CACHE_ALIGN s_file_buffer[FLASH_SECTOR_SZ];
static uint8_t CACHE_ALIGN s_winc_buffer[FLASH_SECTOR_SZ];

#undef DEFINE_WINC_IMAGER_STATE
#define DEFINE_WINC_IMAGER_STATE(_name) #_name,
static const char *s_winc_imager_state_names[] = {EXPAND_WINC_IMAGER_STATES};

// *****************************************************************************
// Public code

void winc_imager_init(const char *filename,
                      void (*on_completion)(void *arg),
                      void *completion_arg) {
  s_winc_imager_ctx.state = WINC_IMAGER_STATE_INIT;
  s_winc_imager_ctx.filename = filename;
  s_winc_imager_ctx.on_completion = on_completion;
  s_winc_imager_ctx.completion_arg = completion_arg;
}

void winc_imager_step(void) {
  switch (s_winc_imager_ctx.state) {

  case WINC_IMAGER_STATE_INIT: {
    winc_imager_set_state(WINC_IMAGER_STATE_OPENING_IMAGE_FILE);
  } break;

  case WINC_IMAGER_STATE_OPENING_WINC: {
    s_winc_imager_ctx.winc_handle = WDRV_WINC_Open(0, DRV_IO_INTENT_EXCLUSIVE);
    if (s_winc_imager_ctx.winc_handle != DRV_HANDLE_INVALID) {
      printf("\nOpened WINC1500");
      winc_imager_set_state(WINC_IMAGER_STATE_VALIDATING_IMAGE_FILE);
    } else {
      printf("\nUnable to open WINC1500");
      endgame(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_VALIDATING_IMAGE_FILE: {
    // Checking to see winc.img file exists and is the correct size.
    // Note: we deckare stat_buf as static since it is large and we don't want
    // to overflow the stack.
    static SYS_FS_FSTAT stat_buf;

    if (SYS_FS_FileStat(s_winc_imager_ctx.filename, &stat_buf) !=
        SYS_FS_RES_SUCCESS) {
      // fstat failed.
      printf("\nUnable to determine size of %s", s_winc_imager_ctx.filename);
      endgame(WINC_IMAGER_STATE_ERROR);
    } else if (stat_buf.fsize != WINC_IMAGE_SIZE) {
      // File size doesn't match WINC image size
      printf("\nExpected %s to be %ld bytes, but found %ld",
             s_winc_imager_ctx.filename,
             WINC_IMAGE_SIZE,
             stat_buf.fsize);
      endgame(WINC_IMAGER_STATE_ERROR);
    } else {
      // File looks like a WINC image.
      printf("\nFound valid %s file", s_winc_imager_ctx.filename);
      winc_imager_set_state(WINC_IMAGER_STATE_OPENING_IMAGE_FILE);
    }
  } break;

  case WINC_IMAGER_STATE_OPENING_IMAGE_FILE: {
    s_winc_imager_ctx.file_handle =
        SYS_FS_FileOpen(s_winc_imager_ctx.filename, SYS_FS_FILE_OPEN_READ);
    if (s_winc_imager_ctx.file_handle != SYS_FS_HANDLE_INVALID) {
      printf("\nComparing image file %s against WINC contents ",
             s_winc_imager_ctx.filename);
      s_winc_imager_ctx.sector = 0;
      winc_imager_set_state(WINC_IMAGER_STATE_COMPARING_SECTORS);
    } else {
      printf("\nUnable to open image file %s", s_winc_imager_ctx.filename);
      endgame(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_COMPARING_SECTORS: {
    if (s_winc_imager_ctx.sector == WINC_SECTOR_COUNT) {
      printf("\nWINC firmware matches %s", s_winc_imager_ctx.filename);
      winc_imager_set_state(WINC_IMAGER_STATE_CLOSING_RESOURCES);
    } else {
      printf("\nComparing sector %d out of %d",
             s_winc_imager_ctx.sector,
             WINC_SECTOR_COUNT);
      winc_imager_set_state(WINC_IMAGER_STATE_READING_FILE_SECTOR);
    }
  } break;

  case WINC_IMAGER_STATE_READING_FILE_SECTOR: {
    size_t n_read = SYS_FS_FileRead(
        s_winc_imager_ctx.file_handle, s_file_buffer, FLASH_SECTOR_SZ);
    if (n_read == FLASH_SECTOR_SZ) {
      winc_imager_set_state(WINC_IMAGER_STATE_READING_WINC_SECTOR);
    } else if (n_read == -1) {
      printf("\nReading image file %s failed", s_winc_imager_ctx.filename);
      endgame(WINC_IMAGER_STATE_ERROR);
    } else {
      printf("\nRead %u bytes from %s, expected %ld",
             n_read,
             s_winc_imager_ctx.filename,
             FLASH_SECTOR_SZ);
      endgame(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_READING_WINC_SECTOR: {
    if (WDRV_WINC_NVMRead(s_winc_imager_ctx.winc_handle,
                          WDRV_WINC_NVM_REGION_RAW,
                          s_winc_buffer,
                          SECTOR_TO_OFFSET(s_winc_imager_ctx.sector),
                          FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      winc_imager_set_state(WINC_IMAGER_STATE_COMPARING_BUFFERS);
    } else {
      printf("\nFailed to read %ld bytes from %s",
             FLASH_SECTOR_SZ,
             s_winc_imager_ctx.filename);
    }
  } break;

  case WINC_IMAGER_STATE_COMPARING_BUFFERS: {
    bool buffers_differ = false;
    for (int i = 0; i < FLASH_SECTOR_SZ; i++) {
      if (s_winc_buffer[i] != s_file_buffer[i]) {
        buffers_differ = true;
        break;
      }
    }
    if (!buffers_differ) {
      // buffers match - advance to next sector
      printf(".");
      winc_imager_set_state(WINC_IMAGER_STATE_INCREMENT_WRITE_SECTOR);
    } else {
      // buffers differ - erase and overwrite this sector
      winc_imager_set_state(WINC_IMAGER_STATE_ERASING_WINC_SECTOR);
    }
  } break;

  case WINC_IMAGER_STATE_ERASING_WINC_SECTOR: {
    // Erase sector in preparation for overwriting
    printf("!");
    if (WDRV_WINC_NVMEraseSector(s_winc_imager_ctx.winc_handle,
                                 WDRV_WINC_NVM_REGION_RAW,
                                 s_winc_imager_ctx.sector,
                                 1) == WDRV_WINC_STATUS_OK) {
      printf("\nErased sector %d", s_winc_imager_ctx.sector);
      winc_imager_set_state(WINC_IMAGER_STATE_WRITING_WINC_SECTOR);
    } else {
      printf("\nErasing sector %d failed", s_winc_imager_ctx.sector);
      endgame(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_WRITING_WINC_SECTOR: {
    // image file sector is already in s_sector_buf.  Write to WINC.
    if (WDRV_WINC_NVMWrite(s_winc_imager_ctx.winc_handle,
                           WDRV_WINC_NVM_REGION_RAW,
                           s_file_buffer,
                           SECTOR_TO_OFFSET(s_winc_imager_ctx.sector),
                           FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      printf("\nWrote sector %d", s_winc_imager_ctx.sector);
      winc_imager_set_state(WINC_IMAGER_STATE_INCREMENT_WRITE_SECTOR);
    } else {
      printf("\nWriting sector %d failed", s_winc_imager_ctx.sector);
      endgame(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_INCREMENT_WRITE_SECTOR: {
    s_winc_imager_ctx.sector += 1;
    winc_imager_set_state(WINC_IMAGER_STATE_COMPARING_SECTORS);
  } break;

  case WINC_IMAGER_STATE_CLOSING_RESOURCES: {
    // Close file and WINC before declaring success
    WDRV_WINC_Close(s_winc_imager_ctx.winc_handle);
    SYS_FS_FileClose(s_winc_imager_ctx.file_handle);
    endgame(WINC_IMAGER_STATE_SUCCESS);
  } break;

  case WINC_IMAGER_STATE_SUCCESS: {
    // stay in this state
  } break;

  case WINC_IMAGER_STATE_ERROR: {
    // stay in this state
  } break;

  } // switch
}

bool winc_imager_succeeded(void) {
  return s_winc_imager_ctx.state == WINC_IMAGER_STATE_SUCCESS;
}

bool winc_imager_failed(void) {
  return s_winc_imager_ctx.state == WINC_IMAGER_STATE_ERROR;
}

// *****************************************************************************
// Local (private, static) code

static void winc_imager_set_state(winc_imager_state_t new_state) {
  if (new_state != s_winc_imager_ctx.state) {
    YB_LOG_STATE_CHANGE(winc_imager_state_name(s_winc_imager_ctx.state),
                        winc_imager_state_name(new_state));
    s_winc_imager_ctx.state = new_state;
  }
}

static const char *winc_imager_state_name(winc_imager_state_t state) {
  return s_winc_imager_state_names[state];
}

static void endgame(winc_imager_state_t final_state) {
  winc_imager_set_state(final_state);
  if (s_winc_imager_ctx.on_completion != NULL) {
    s_winc_imager_ctx.on_completion(s_winc_imager_ctx.completion_arg);
  }
}
