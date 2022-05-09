/**
 * @file imager_task.c
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

#include "imager_task.h"

#include "definitions.h"
#include "spi_flash_map.h"
#include "wdrv_winc_client_api.h"
#include "yb_log.h"
#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Local (private) types and definitions

#define SECTOR_TO_OFFSET(_sector) ((_sector)*FLASH_SECTOR_SZ)
// #define WINC_SECTOR_COUNT 128 // TODO: look up dynamically
#define WINC_SECTOR_COUNT 256 // TODO: look up dynamically
#define WINC_IMAGE_SIZE SECTOR_TO_OFFSET(WINC_SECTOR_COUNT)

#define STATES(M)                                                              \
  M(IMAGER_TASK_STATE_INIT)                                                    \
  M(IMAGER_TASK_STATE_OPENING_WINC)                                            \
  M(IMAGER_TASK_STATE_VALIDATING_IMAGE_FILE)                                   \
  M(IMAGER_TASK_STATE_OPENING_IMAGE_FILE)                                      \
  M(IMAGER_TASK_STATE_COMPARING_SECTORS)                                       \
  M(IMAGER_TASK_STATE_READING_FILE_SECTOR)                                     \
  M(IMAGER_TASK_STATE_READING_WINC_SECTOR)                                     \
  M(IMAGER_TASK_STATE_COMPARING_BUFFERS)                                       \
  M(IMAGER_TASK_STATE_ERASING_WINC_SECTOR)                                     \
  M(IMAGER_TASK_STATE_WRITING_WINC_SECTOR)                                     \
  M(IMAGER_TASK_STATE_INCREMENT_WRITE_SECTOR)                                  \
  M(IMAGER_TASK_STATE_CLOSING_RESOURCES)                                       \
  M(IMAGER_TASK_STATE_SUCCESS)                                                 \
  M(IMAGER_TASK_STATE_ERROR)

#define EXPAND_ENUM_ID(_name) _name,
typedef enum { STATES(EXPAND_ENUM_ID) } imager_task_state_t;

typedef struct {
  imager_task_state_t state;
  const char *filename;
  SYS_FS_HANDLE file_handle;
  DRV_HANDLE winc_handle;
  uint16_t sector;
} imager_task_ctx_t;

// *****************************************************************************
// Local (private, static) forward declarations

static void imager_task_set_state(imager_task_state_t new_state);

static const char *imager_task_state_name(imager_task_state_t state);

static void cleanup(void);

// *****************************************************************************
// Local (private, static) storage

static imager_task_ctx_t s_imager_task_ctx;

static uint8_t CACHE_ALIGN s_file_buffer[FLASH_SECTOR_SZ];
static uint8_t CACHE_ALIGN s_winc_buffer[FLASH_SECTOR_SZ];

#define EXPAND_NAME(_name) #_name,
static const char *s_imager_task_state_names[] = {STATES(EXPAND_NAME)};

// *****************************************************************************
// Public code

void imager_task_init(const char *filename) {
  s_imager_task_ctx.state = IMAGER_TASK_STATE_INIT;
  s_imager_task_ctx.filename = filename;
}

void imager_task_step(void) {
  switch (s_imager_task_ctx.state) {

  case IMAGER_TASK_STATE_INIT: {
    imager_task_set_state(IMAGER_TASK_STATE_OPENING_IMAGE_FILE);
  } break;

  case IMAGER_TASK_STATE_OPENING_WINC: {
    s_imager_task_ctx.winc_handle = WDRV_WINC_Open(0, DRV_IO_INTENT_EXCLUSIVE);
    if (s_imager_task_ctx.winc_handle != DRV_HANDLE_INVALID) {
      YB_LOG_DEBUG("Opened WINC1500");
      imager_task_set_state(IMAGER_TASK_STATE_VALIDATING_IMAGE_FILE);
    } else {
      YB_LOG_ERROR("Unable to open WINC1500");
      imager_task_set_state(IMAGER_TASK_STATE_ERROR);
    }
  } break;

  case IMAGER_TASK_STATE_VALIDATING_IMAGE_FILE: {
    // Checking to see winc.img file exists and is the correct size.
    // Note: we deckare stat_buf as static since it is large and we don't want
    // to overflow the stack.
    static SYS_FS_FSTAT stat_buf;

    if (SYS_FS_FileStat(s_imager_task_ctx.filename, &stat_buf) !=
        SYS_FS_RES_SUCCESS) {
      // fstat failed.
      YB_LOG_ERROR("Unable to determine size of %s",
                   s_imager_task_ctx.filename);
      imager_task_set_state(IMAGER_TASK_STATE_ERROR);
    } else if (stat_buf.fsize != WINC_IMAGE_SIZE) {
      // File size doesn't match WINC image size
      YB_LOG_ERROR("Expected %s to be %ld bytes, but found %ld",
                   s_imager_task_ctx.filename,
                   WINC_IMAGE_SIZE,
                   stat_buf.fsize);
      imager_task_set_state(IMAGER_TASK_STATE_ERROR);
    } else {
      // File looks like a WINC image.
      YB_LOG_DEBUG("Found valid %s file", s_imager_task_ctx.filename);
      imager_task_set_state(IMAGER_TASK_STATE_OPENING_IMAGE_FILE);
    }
  } break;

  case IMAGER_TASK_STATE_OPENING_IMAGE_FILE: {
    s_imager_task_ctx.file_handle =
        SYS_FS_FileOpen(s_imager_task_ctx.filename, SYS_FS_FILE_OPEN_READ);
    if (s_imager_task_ctx.file_handle != SYS_FS_HANDLE_INVALID) {
      YB_LOG_INFO("Comparing image file %s against WINC contents ",
                  s_imager_task_ctx.filename);
      s_imager_task_ctx.sector = 0;
      imager_task_set_state(IMAGER_TASK_STATE_COMPARING_SECTORS);
    } else {
      YB_LOG_ERROR("Unable to open image file %s", s_imager_task_ctx.filename);
      imager_task_set_state(IMAGER_TASK_STATE_ERROR);
    }
  } break;

  case IMAGER_TASK_STATE_COMPARING_SECTORS: {
    if (s_imager_task_ctx.sector == WINC_SECTOR_COUNT) {
      YB_LOG_INFO("WINC firmware matches %s", s_imager_task_ctx.filename);
      imager_task_set_state(IMAGER_TASK_STATE_CLOSING_RESOURCES);
    } else {
      YB_LOG_DEBUG("Comparing sector %d out of %d",
                   s_imager_task_ctx.sector,
                   WINC_SECTOR_COUNT);
      imager_task_set_state(IMAGER_TASK_STATE_READING_FILE_SECTOR);
    }
  } break;

  case IMAGER_TASK_STATE_READING_FILE_SECTOR: {
    size_t n_read = SYS_FS_FileRead(
        s_imager_task_ctx.file_handle, s_file_buffer, FLASH_SECTOR_SZ);
    if (n_read == FLASH_SECTOR_SZ) {
      imager_task_set_state(IMAGER_TASK_STATE_READING_WINC_SECTOR);
    } else if (n_read == -1) {
      YB_LOG_ERROR("Reading image file %s failed", s_imager_task_ctx.filename);
      imager_task_set_state(IMAGER_TASK_STATE_ERROR);
    } else {
      YB_LOG_ERROR("Read %ld bytes from %s, expected %ld",
                   n_read,
                   s_imager_task_ctx.filename,
                   FLASH_SECTOR_SZ);
      imager_task_set_state(IMAGER_TASK_STATE_ERROR);
    }
  } break;

  case IMAGER_TASK_STATE_READING_WINC_SECTOR: {
    if (WDRV_WINC_NVMRead(s_imager_task_ctx.winc_handle,
                          WDRV_WINC_NVM_REGION_RAW,
                          s_winc_buffer,
                          SECTOR_TO_OFFSET(s_imager_task_ctx.sector),
                          FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      imager_task_set_state(IMAGER_TASK_STATE_COMPARING_BUFFERS);
    } else {
      YB_LOG_ERROR("Failed to read %ld bytes from %s",
                   FLASH_SECTOR_SZ,
                   s_imager_task_ctx.filename);
    }
  } break;

  case IMAGER_TASK_STATE_COMPARING_BUFFERS: {
    bool buffers_differ = false;
    for (int i = 0; i < FLASH_SECTOR_SZ; i++) {
      if (s_winc_buffer[i] != s_file_buffer[i]) {
        buffers_differ = true;
        break;
      }
    }
    if (!buffers_differ) {
      // buffers match - advance to next sector
      YB_LOG_INFO(".");
      imager_task_set_state(IMAGER_TASK_STATE_INCREMENT_WRITE_SECTOR);
    } else {
      // buffers differ - erase and overwrite this sector
      imager_task_set_state(IMAGER_TASK_STATE_ERASING_WINC_SECTOR);
    }
  } break;

  case IMAGER_TASK_STATE_ERASING_WINC_SECTOR: {
    // Erase sector in preparation for overwriting
    YB_LOG_INFO("!");
    if (WDRV_WINC_NVMEraseSector(s_imager_task_ctx.winc_handle,
                                 WDRV_WINC_NVM_REGION_RAW,
                                 s_imager_task_ctx.sector,
                                 1) == WDRV_WINC_STATUS_OK) {
      YB_LOG_DEBUG("Erased sector %d", s_imager_task_ctx.sector);
      imager_task_set_state(IMAGER_TASK_STATE_WRITING_WINC_SECTOR);
    } else {
      YB_LOG_ERROR("Erasing sector %s failed", s_imager_task_ctx.sector);
      imager_task_set_state(IMAGER_TASK_STATE_ERROR);
    }
  } break;

  case IMAGER_TASK_STATE_WRITING_WINC_SECTOR: {
    // image file sector is already in s_sector_buf.  Write to WINC.
    if (WDRV_WINC_NVMWrite(s_imager_task_ctx.winc_handle,
                           WDRV_WINC_NVM_REGION_RAW,
                           s_file_buffer,
                           SECTOR_TO_OFFSET(s_imager_task_ctx.sector),
                           FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      YB_LOG_DEBUG("Wrote sector %d", s_imager_task_ctx.sector);
      imager_task_set_state(IMAGER_TASK_STATE_INCREMENT_WRITE_SECTOR);
    } else {
      YB_LOG_ERROR("Writing sector %s failed", s_imager_task_ctx.sector);
      imager_task_set_state(IMAGER_TASK_STATE_ERROR);
    }
  } break;

  case IMAGER_TASK_STATE_INCREMENT_WRITE_SECTOR: {
    s_imager_task_ctx.sector += 1;
    imager_task_set_state(IMAGER_TASK_STATE_COMPARING_SECTORS);
  } break;

  case IMAGER_TASK_STATE_CLOSING_RESOURCES: {
    // Close file and WINC before declaring success
    cleanup();
    imager_task_set_state(IMAGER_TASK_STATE_SUCCESS);
  } break;

  case IMAGER_TASK_STATE_SUCCESS: {
    // stay in this state
  } break;

  case IMAGER_TASK_STATE_ERROR: {
    // stay in this state
  } break;

  } // switch
}

bool imager_task_succeeded(void) {
  return s_imager_task_ctx.state == IMAGER_TASK_STATE_SUCCESS;
}

bool imager_task_failed(void) {
  return s_imager_task_ctx.state == IMAGER_TASK_STATE_ERROR;
}

void imager_task_shutdown(void) { cleanup(); }

// *****************************************************************************
// Local (private, static) code

static void imager_task_set_state(imager_task_state_t new_state) {
  if (new_state != s_imager_task_ctx.state) {
    YB_LOG_INFO("%s => %s",
                imager_task_state_name(s_imager_task_ctx.state),
                imager_task_state_name(new_state));
    s_imager_task_ctx.state = new_state;
  }
}

static const char *imager_task_state_name(imager_task_state_t state) {
  return s_imager_task_state_names[state];
}

static void cleanup(void) {
  if (s_imager_task_ctx.winc_handle != 0) {
    // TODO: is 0 a valid file handle?  If so, allocate a flag to know if we
    // need to call Close
    WDRV_WINC_Close(s_imager_task_ctx.winc_handle);
  }
  if (s_imager_task_ctx.file_handle != 0) {
    // TODO: is 0 a valid file handle?  If so, allocate a flag to know if we
    // need to call Close
    SYS_FS_FileClose(s_imager_task_ctx.file_handle);
  }
}
