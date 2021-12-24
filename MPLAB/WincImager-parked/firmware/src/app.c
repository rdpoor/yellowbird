/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

/*******************************************************************************
 * Copyright (C) 2020-21 Microchip Technology Inc. and its subsidiaries.
 *
 * Subject to your compliance with these terms, you may use Microchip software
 * and any derivatives exclusively with Microchip products. It is your
 * responsibility to comply with third party license terms applicable to your
 * use of third party software (including open source software) that may
 * accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
 * ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

// =============================================================================
// Includes

// NOTE: See firmware\src\config\same54_xpri\driver\winc\include\wdrv_winc_nvm.h
// for the API to read and write the WINC flash memory.
// See also
// https://github.com/Microchip-MPLAB-Harmony/wireless_wifi/blob/master/driver/winc/docs/winc_driver.md

// TODO: print info about the WINC chip - version number, MAC, etc...
// See firmware/src/config/same54_xpri/driver/winc/include/wdrv_winc.h

#include "app.h"
#include "wdrv_winc_client_api.h"

#include "spi_flash_map.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// Private types and definitions

#define WINC_IMAGER_VERSION "1.0.0"
#define IMAGE_NAME "0:winc.img"

#define IMAGE_SECTOR_COUNT 128 // TODO: look up dynamically
#define IMAGE_SIZE ((uint32_t)(IMAGE_SECTOR_COUNT * FLASH_SECTOR_SZ))

typedef enum {
  STATE_INIT,
  STATE_GET_CONSOLE_HANDLE,
  STATE_ANNOUNCE,
  STATE_INITIALIZING_FILESYSTEM,
  STATE_INITIALIZING_WINC,
  STATE_SEEKING_FILE_IMAGE,
  STATE_COMPARING_SECTORS,
  STATE_REQUESTING_WINC_UPDATE,
  STATE_WRITING_WINC_SECTOR,
  STATE_REQUESTING_FILE_CREATION,
  STATE_WRITING_FILE_SECTOR,
  STATE_ERROR,
  STATE_SUCCESS,
} app_state_t;

typedef struct {
  app_state_t state;
  SYS_CONSOLE_HANDLE consoleHandle;
  DRV_HANDLE winc_handle;
} app_ctx_t;

// =============================================================================
// Private (static) storage

// static FATFS s_filesys;
static app_ctx_t s_app_ctx;

// =============================================================================
// Private (forward) declarations

#if 0
static bool prompt(const char *fmt, ...);

static bool init_filesystem(void);
static bool file_exists_for_read(const char *filename);
static FIL *open_file_for_read(const char *filename, FIL *file);
static FIL *open_file_for_write(const char *filename, FIL *file);
static FIL *open_aux(const char *filename, FIL *file, BYTE mode);

static bool init_winc(void);
static void extract_winc_contents(const char *image_name);
static void compare_and_update(const char *image_name);
#endif

// =============================================================================
// Public code

void APP_Initialize(void) { s_app_ctx.state = STATE_INIT; }

void APP_Tasks(void) {
  switch (s_app_ctx.state) {

  case STATE_INIT: {
    if (SYS_CONSOLE_Status(SYS_CONSOLE_INDEX_0) == SYS_STATUS_READY) {
      s_app_ctx.state = STATE_GET_CONSOLE_HANDLE;
    }
  } break;

  case STATE_GET_CONSOLE_HANDLE: {
    s_app_ctx.consoleHandle = SYS_CONSOLE_HandleGet(SYS_CONSOLE_INDEX_0);
    if (s_app_ctx.consoleHandle != SYS_CONSOLE_HANDLE_INVALID) {
      s_app_ctx.state = STATE_ANNOUNCE;
    } else {
      s_app_ctx.state = STATE_ERROR;
    }
  } break;

  case STATE_ANNOUNCE: {
     SYS_CONSOLE_Print(s_app_ctx.consoleHandle,
        "\nWINC imaging tool, v %s"
        "\nRead and write raw WINC image from and to a file on the SD card."
        "\nRequires:"
        "\n  SAME54 Xplained Pro Development board"
        "\n  WINC1500 Xplained Pro Expansion board (in EXT-1)"
        "\n  IO/1 Xplained Pro Expansion board (in EXT-2)"
        "\n",
        WINC_IMAGER_VERSION);
    s_app_ctx.state = STATE_SUCCESS;
  } break;
  default: {
    LED_Toggle();
  } break;
  }
}

// =============================================================================
// Private (static) code

#if 0

static bool prompt(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vSYS_CONSOLE_PRINT(fmt, ap);
  va_end(ap);

  char resp = getchar();
  putchar('\n');
  return (resp == 'y' || resp == 'Y');
}

// ==================

static bool init_filesystem(void) {
  FRESULT file_status = FR_OK;
  memset(&s_filesys, 0, sizeof(FATFS));

  if ((file_status = f_mount(&s_filesys, "", 1)) != FR_OK) {
    SYS_CONSOLE_PRINT("\nFailed to mount filesystem");
    return false;
  }
  return true;
}

static bool file_exists_for_read(const char *filename) {
  FIL file;
  if (open_aux(filename, &file, FA_READ) == NULL) {
    return false;
  } else {
    f_close(&file);
    return true;
  }
}

static FIL *open_file_for_read(const char *filename, FIL *file) {
  return open_aux(filename, file, FA_READ);
}

static FIL *open_file_for_write(const char *filename, FIL *file) {
  return open_aux(filename, file, FA_CREATE_ALWAYS | FA_WRITE);
}

static FIL *open_aux(const char *filename, FIL *file, BYTE mode) {
  if (f_open(file, filename, mode) != FR_OK) {
    return NULL;
  }
  return file;
}

// ==================

static bool init_winc(void) {
  bool success = false;

  s_app_ctx.winc_handle = WDRV_WINC_Open(0, DRV_IO_INTENT_EXCLUSIVE);
  if (s_app_ctx.winc_handle == DRV_HANDLE_INVALID) {
    // open failed
    success = false;
  } else {
    // open succeeded - winc_handle is valid.
    success = true;
  }
  return success;
}

static void extract_winc_contents(const char *image_name) {
  FRESULT file_status;
  FIL file;
  UINT bw;
  WDRV_WINC_STATUS winc_status;
  uint8_t buf[FLASH_SECTOR_SZ];

  if (open_file_for_write(image_name, &file) == NULL) {
    SYS_CONSOLE_PRINT("\ncould not open %s for writing", image_name);
    return;
  }

  uint32_t addr = 0;

  while (addr < IMAGE_SIZE) {
    // Sector at a time: read WINC flash into buf, write buf to disk.
    uint32_t bytes_to_read = IMAGE_SIZE - addr;
    if (bytes_to_read > FLASH_SECTOR_SZ) {
      bytes_to_read = FLASH_SECTOR_SZ;
    }

    // read from WINC flash into buf
    winc_status = WDRV_WINC_NVMRead(s_app_ctx.winc_handle,
                                    WDRV_WINC_NVM_REGION_RAW,
                                    buf,
                                    addr,
                                    bytes_to_read);
    if (winc_status != WDRV_WINC_STATUS_OK) {
      SYS_CONSOLE_PRINT("\nUnable to read sector at %ld (0x%lx)", addr, addr);
      break;
    }

    // write to SD card FATFS
    file_status = f_write(&file, buf, bytes_to_read, &bw);
    if (file_status != FR_OK) {
      SYS_CONSOLE_PRINT("\nwrite error %d", file_status);
    }

    if (bw != bytes_to_read) {
      break;
    }
    putchar('.');
    addr += bytes_to_read;
  }
  if (addr == IMAGE_SIZE) {
    SYS_CONSOLE_PRINT("\n\nsuccessfully transferred %ld bytes from WINC to disk", addr);
  } else {
    SYS_CONSOLE_PRINT("\nwanted to transfer %ld bytes from WINC, but only transferred %ld",
           IMAGE_SIZE,
           addr);
  }
  f_close(&file);
}

static uint32_t sector_to_addr(uint8_t sector) {
  return sector * FLASH_SECTOR_SZ;
}

static void compare_and_update(const char *image_name) {
  FRESULT file_status;
  FIL file;
  UINT bw;
  WDRV_WINC_STATUS winc_status;
  uint8_t sector_number = 0;
  uint8_t winc_buf[FLASH_SECTOR_SZ];
  uint8_t file_buf[FLASH_SECTOR_SZ];

  bool overwrite_enabled = false;

  if (open_file_for_read(image_name, &file) == NULL) {
    SYS_CONSOLE_PRINT("\ncould not open %s for reading", image_name);
    return;
  }

  while (sector_number < IMAGE_SECTOR_COUNT) {
    uint32_t bytes_to_read = (IMAGE_SECTOR_COUNT - sector_number) * FLASH_SECTOR_SZ;

    // bytes_to_read = MIN(bytes_remaining, FLASH_SECTOR_SIZE)
    if (bytes_to_read > FLASH_SECTOR_SZ) {
      bytes_to_read = FLASH_SECTOR_SZ;
    }

    // read from WINC flash into winc_buf
    LED_On();
    winc_status = WDRV_WINC_NVMRead(s_app_ctx.winc_handle,
                                    WDRV_WINC_NVM_REGION_RAW,
                                    winc_buf,
                                    sector_to_addr(sector_number),
                                    bytes_to_read);
    LED_Off();
    if (winc_status != WDRV_WINC_STATUS_OK) {
      SYS_CONSOLE_PRINT("\nUnable to read WINC sector at %d (addr 0x%lx)", sector_number, sector_to_addr(sector_number));
      break;
    }

    // read corresponding data into file_buf
    LED_On();
    file_status = f_read(&file, file_buf, bytes_to_read, &bw);
    LED_Off();
    if ((file_status != FR_OK) || bw != bytes_to_read) {
      SYS_CONSOLE_PRINT("\nUnable to read file sector at %d (addr 0x%lx)", sector_number, sector_to_addr(sector_number));
      break;
    }

    // commpare winc_buf against file_buf
    bool buffers_differ = false;
    for (int i = 0; i < FLASH_SECTOR_SZ; i++) {
      if (winc_buf[i] != file_buf[i]) {
        uint32_t addr = sector_to_addr(sector_number);
        SYS_CONSOLE_PRINT("\nat address 0x%lx, flash=%02x, file=%02x",
               addr + i,
               winc_buf[addr + i],
               file_buf[addr + i]);
        buffers_differ = true;
        break;
      }
    }

    // buffers differ.  has user given permission to update WINC?
    if (buffers_differ && !overwrite_enabled) {
      if (prompt("update WINC from disk image? (y or Y to update): ")) {
        overwrite_enabled = true;
      } else {
        // don't update WINC
        break;
      }
    }

    if (buffers_differ) {
      // Update sector.
      winc_status = WDRV_WINC_NVMEraseSector(s_app_ctx.winc_handle,
                                             WDRV_WINC_NVM_REGION_RAW,
                                             sector_number,
                                             1);
      if (winc_status != WDRV_WINC_STATUS_OK) {
        SYS_CONSOLE_PRINT("\nUnable to erase sector #%d", sector_number);
        break;
      }

      // write file buf to WINC flash
      winc_status = WDRV_WINC_NVMWrite(s_app_ctx.winc_handle,
                                       WDRV_WINC_NVM_REGION_RAW,
                                       (void *)file_buf,
                                       sector_to_addr(sector_number),
                                       FLASH_SECTOR_SZ);
      if (winc_status != WDRV_WINC_STATUS_OK) {
        SYS_CONSOLE_PRINT("\nUnable to write WINC sector %d (addr 0x%lx)", sector_number, sector_to_addr(sector_number));
        break;
      }
    }
    putchar('.');
    sector_number += 1;
  }

  // endgame
  if (sector_number == IMAGE_SECTOR_COUNT) {
    SYS_CONSOLE_PRINT("\n\nsuccessfully compared %d sectors in WINC and disk", sector_number);
  } else {
    SYS_CONSOLE_PRINT("\nwanted to compare %d sectors from WINC, but only compared %d",
           IMAGE_SECTOR_COUNT,
           sector_number);
  }
  f_close(&file);
}
#endif
