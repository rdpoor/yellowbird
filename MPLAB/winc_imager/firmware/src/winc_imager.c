/**
 * @file template.c
 */

// *****************************************************************************
// Includes

#include "winc_imager.h"

#include "spi_flash_map.h"
#include "wdrv_winc_client_api.h"
#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Private types and definitions

#define SD_DEVICE_NAME "/dev/mmcblka1"
#define SD_MOUNT_NAME "/mnt/mydrive"
#define FILE_IMAGE_NAME "winc.img"

#define SECTOR_TO_OFFSET(_sector) ((_sector)*FLASH_SECTOR_SZ)
#define WINC_SECTOR_COUNT 128 // TODO: look up dynamically
#define WINC_IMAGE_SIZE SECTOR_TO_OFFSET(WINC_SECTOR_COUNT)

typedef struct {
  winc_imager_state_t state;
  SYS_FS_HANDLE file_handle;
  DRV_HANDLE winc_handle;
  uint8_t sector;
  bool has_winc_write_permission;
  uint32_t mount_retries;
} winc_imager_ctx_t;

// *****************************************************************************
// Private (static, forward) declarations

static void set_state(winc_imager_state_t new_state);
static const char *state_name(winc_imager_state_t state);
static void flush_console_input(void);

/**
 * @brief Set the state depending on what the user types.
 *
 * If the user types 'y' or 'Y', set the state to y_state.
 * If the user types anything else, set the state to n_state.
 * If the user types nothing, leave the state unchanged.
 * If there is an error during read, set the state to WINC_IMAGER_STATE_ERROR
 */
static void get_y(winc_imager_state_t y_state, winc_imager_state_t n_state);

// *****************************************************************************
// Private (static) storage

winc_imager_ctx_t s_winc_imager_ctx;

static uint8_t CACHE_ALIGN s_file_buffer[FLASH_SECTOR_SZ];
static uint8_t CACHE_ALIGN s_winc_buffer[FLASH_SECTOR_SZ];

#undef DEFINE_WINC_IMAGER_STATE
#define DEFINE_WINC_IMAGER_STATE(_name) #_name,
static const char *s_state_names[] = {DEFINE_WINC_IMAGER_STATES};

// *****************************************************************************
// Public code

void winc_imager_init() { s_winc_imager_ctx.state = WINC_IMAGER_STATE_INIT; }

winc_imager_state_t winc_imager_step(void) {
  switch (s_winc_imager_ctx.state) {

  case WINC_IMAGER_STATE_INIT: {
    s_winc_imager_ctx.mount_retries = 0;
    set_state(WINC_IMAGER_STATE_OPENING_WINC);
  } break;

  case WINC_IMAGER_STATE_OPENING_WINC: {
    s_winc_imager_ctx.winc_handle = WDRV_WINC_Open(0, DRV_IO_INTENT_EXCLUSIVE);
    if (s_winc_imager_ctx.winc_handle != DRV_HANDLE_INVALID) {
      SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, "\nOpened WINC1500");
      set_state(WINC_IMAGER_STATE_MOUNTING_DRIVE);
    } else {
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nUnable to open WINC1500");
      set_state(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_MOUNTING_DRIVE: {
    s_winc_imager_ctx.mount_retries += 1;
    if (SYS_FS_Mount(SD_DEVICE_NAME, SD_MOUNT_NAME, FAT, 0, NULL) ==
        SYS_FS_RES_SUCCESS) {
      SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                      "\nSD card mounted after %ld attempts",
                      s_winc_imager_ctx.mount_retries);
      set_state(WINC_IMAGER_STATE_SETTING_DRIVE);
    } else if (s_winc_imager_ctx.mount_retries % 100000 == 0) {
      SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                      "\nSD card not ready after %ld attempts",
                      s_winc_imager_ctx.mount_retries);
    }
  } break;

  case WINC_IMAGER_STATE_SETTING_DRIVE: {
    // Set current drive so that we do not have to use absolute path.
    if (SYS_FS_CurrentDriveSet(SD_MOUNT_NAME) == SYS_FS_RES_FAILURE) {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nUnable to select drive, error %d",
                      SYS_FS_Error());
      set_state(WINC_IMAGER_STATE_ERROR);
    } else {
      SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, "\nSelected mounted drive");
      set_state(WINC_IMAGER_STATE_CHECKING_FILE_INFO);
    }
  } break;

  case WINC_IMAGER_STATE_CHECKING_FILE_INFO: {
    // Checking to see winc.img file exists and is the correct size
    // TEST: allocate as static since SYS_FS_FSTAT is large and may exceed
    // stack.
    static SYS_FS_FSTAT stat_buf;

    if (SYS_FS_FileStat(FILE_IMAGE_NAME, &stat_buf) == SYS_FS_RES_SUCCESS) {
      // fstat() completed normally.  Make sure image file is correct size.
      if (stat_buf.fsize == WINC_IMAGE_SIZE) {
        SYS_DEBUG_PRINT(
            SYS_ERROR_INFO, "\nFound valid %s file", FILE_IMAGE_NAME);
        set_state(WINC_IMAGER_STATE_OPENING_IMAGE_FILE);
      } else {
        SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                        "\nExpected %s to be %ld bytes, but found %ld",
                        FILE_IMAGE_NAME,
                        WINC_IMAGE_SIZE,
                        stat_buf.fsize);
        set_state(WINC_IMAGER_STATE_ERROR);
      }
    } else {
      // fstat() failed.  If image file doesn't exist, offer to create one.
      SYS_FS_ERROR err = SYS_FS_Error();
      if (err == SYS_FS_ERROR_NO_FILE) {
        // file doesn't exist -- plan b...
        SYS_CONSOLE_PRINT(
            "\nFile image %s not found,"
            "\nWould you like to copy WINC image to %s [y or Y]? ",
            FILE_IMAGE_NAME,
            FILE_IMAGE_NAME);
        flush_console_input();
        set_state(WINC_IMAGER_STATE_REQUESTING_FILE_WRITE_PERMISSION);
      } else {
        // some other error...
        SYS_DEBUG_PRINT(SYS_ERROR_ERROR, "\nError %d checking file", err);
        set_state(WINC_IMAGER_STATE_ERROR);
      }
    }
  } break;

  case WINC_IMAGER_STATE_OPENING_IMAGE_FILE: {
    s_winc_imager_ctx.file_handle =
        SYS_FS_FileOpen(FILE_IMAGE_NAME, SYS_FS_FILE_OPEN_READ);
    if (s_winc_imager_ctx.file_handle != SYS_FS_HANDLE_INVALID) {
      SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                      "\nComparing image file %s against WINC contents...",
                      FILE_IMAGE_NAME);
      s_winc_imager_ctx.has_winc_write_permission = false;
      s_winc_imager_ctx.sector = 0;
      set_state(WINC_IMAGER_STATE_COMPARING_SECTORS);
    } else {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nUnable to open image file %s", FILE_IMAGE_NAME);
      set_state(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_COMPARING_SECTORS: {
    if (s_winc_imager_ctx.sector == WINC_SECTOR_COUNT) {
      SYS_DEBUG_PRINT(SYS_ERROR_INFO, "\nFinished comparison", FILE_IMAGE_NAME);
      set_state(WINC_IMAGER_STATE_SUCCESS);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                      "\nComparing sector %d out of %d",
                      s_winc_imager_ctx.sector,
                      WINC_SECTOR_COUNT);
      set_state(WINC_IMAGER_STATE_READING_FILE_SECTOR);
    }
  } break;

  case WINC_IMAGER_STATE_READING_FILE_SECTOR: {
    size_t n_read = SYS_FS_FileRead(
        s_winc_imager_ctx.file_handle, s_file_buffer, FLASH_SECTOR_SZ);
    if (n_read == FLASH_SECTOR_SZ) {
      set_state(WINC_IMAGER_STATE_READING_WINC_SECTOR);
    } else if (n_read == -1) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nReading image file %s failed", FILE_IMAGE_NAME);
      set_state(WINC_IMAGER_STATE_ERROR);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nRead %ld bytes from %s, expected %ld",
                      n_read,
                      FILE_IMAGE_NAME,
                      FLASH_SECTOR_SZ);
      set_state(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_READING_WINC_SECTOR: {
    if (WDRV_WINC_NVMRead(s_winc_imager_ctx.winc_handle,
                          WDRV_WINC_NVM_REGION_RAW,
                          s_winc_buffer,
                          SECTOR_TO_OFFSET(s_winc_imager_ctx.sector),
                          FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      set_state(WINC_IMAGER_STATE_COMPARING_BUFFERS);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nFailed to read %ld bytes from %s",
                      FLASH_SECTOR_SZ,
                      FILE_IMAGE_NAME);
    }
  } break;

  case WINC_IMAGER_STATE_COMPARING_BUFFERS: {
    bool buffers_differ = false;
    for (int i = 0; i < FLASH_SECTOR_SZ; i++) {
      if (s_winc_buffer[i] != s_file_buffer[i]) {
        uint32_t base = SECTOR_TO_OFFSET(s_winc_imager_ctx.sector);
        SYS_CONSOLE_PRINT("\nat winc[0x%lx] = 0x%02x, file[0x%lx] = 0x%02",
                          base + i,
                          s_winc_buffer[i],
                          base + i,
                          s_file_buffer[i]);
        buffers_differ = true;
        break;
      }
    }
    if (!buffers_differ) {
      // buffers match - advance to next sector
      set_state(WINC_IMAGER_STATE_INCREMENT_WRITE_SECTOR);

    } else if (!s_winc_imager_ctx.has_winc_write_permission) {
      // buffers differ, but write permisson not yet granted.
      SYS_CONSOLE_PRINT(
          "\nWINC image and file image differ.  Update winc [y or Y]: ");
      flush_console_input();
      set_state(WINC_IMAGER_STATE_REQUESTING_WINC_WRITE_PERMISSION);
    } else {
      // write permission already granted: go ahead and update...
      set_state(WINC_IMAGER_STATE_ERASING_WINC_SECTOR);
    }
  } break;

  case WINC_IMAGER_STATE_REQUESTING_WINC_WRITE_PERMISSION: {
    get_y(WINC_IMAGER_STATE_ERASING_WINC_SECTOR, WINC_IMAGER_STATE_SUCCESS);
  } break;

  case WINC_IMAGER_STATE_ERASING_WINC_SECTOR: {
    // image file sector is already in s_sector_buf.  Write to WINC.
    if (WDRV_WINC_NVMEraseSector(s_winc_imager_ctx.winc_handle,
                                 WDRV_WINC_NVM_REGION_RAW,
                                 s_winc_imager_ctx.sector,
                                 1) == WDRV_WINC_STATUS_OK) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_INFO, "\nErased sector %d", s_winc_imager_ctx.sector);
      set_state(WINC_IMAGER_STATE_WRITING_WINC_SECTOR);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nErasing sector %s failed",
                      s_winc_imager_ctx.sector);
      set_state(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_WRITING_WINC_SECTOR: {
    // image file sector is already in s_sector_buf.  Write to WINC.
    if (WDRV_WINC_NVMWrite(s_winc_imager_ctx.winc_handle,
                           WDRV_WINC_NVM_REGION_RAW,
                           s_file_buffer,
                           SECTOR_TO_OFFSET(s_winc_imager_ctx.sector),
                           FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_INFO, "\nWrote sector %d", s_winc_imager_ctx.sector);
      set_state(WINC_IMAGER_STATE_INCREMENT_WRITE_SECTOR);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nWriting sector %s failed",
                      s_winc_imager_ctx.sector);
      set_state(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_INCREMENT_WRITE_SECTOR: {
    s_winc_imager_ctx.sector += 1;
    set_state(WINC_IMAGER_STATE_COMPARING_SECTORS);
  } break;

  case WINC_IMAGER_STATE_REQUESTING_FILE_WRITE_PERMISSION: {
    get_y(WINC_IMAGER_STATE_OPENING_FILE_FOR_WRITE, WINC_IMAGER_STATE_SUCCESS);
  } break;

  case WINC_IMAGER_STATE_OPENING_FILE_FOR_WRITE: {
    s_winc_imager_ctx.file_handle =
        SYS_FS_FileOpen(FILE_IMAGE_NAME, SYS_FS_FILE_OPEN_WRITE);
    if (s_winc_imager_ctx.file_handle != SYS_FS_HANDLE_INVALID) {
      s_winc_imager_ctx.sector = 0;
      set_state(WINC_IMAGER_STATE_READING_SECTORS);
    } else {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nFailed to open %s for writing", FILE_IMAGE_NAME);
      set_state(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_READING_SECTORS: {
    if (s_winc_imager_ctx.sector == WINC_SECTOR_COUNT) {
      SYS_FS_FileClose(s_winc_imager_ctx.file_handle);
      SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                      "\nFinished writing WINC image to %s",
                      FILE_IMAGE_NAME);
      set_state(WINC_IMAGER_STATE_SUCCESS);
    } else {
      set_state(WINC_IMAGER_STATE_LOADING_WINC_SECTOR);
    }
  } break;

  case WINC_IMAGER_STATE_LOADING_WINC_SECTOR: {
    uint32_t offset = SECTOR_TO_OFFSET(s_winc_imager_ctx.sector);
    if (WDRV_WINC_NVMRead(s_winc_imager_ctx.winc_handle,
                          WDRV_WINC_NVM_REGION_RAW,
                          s_winc_buffer,
                          offset,
                          FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      set_state(WINC_IMAGER_STATE_WRITING_FILE_SECTOR);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nFailed to read WINC sector %d",
                      s_winc_imager_ctx.sector);
      set_state(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_WRITING_FILE_SECTOR: {
    size_t n_written = SYS_FS_FileWrite(
        s_winc_imager_ctx.file_handle, s_winc_buffer, FLASH_SECTOR_SZ);
    if (n_written == FLASH_SECTOR_SZ) {
      set_state(WINC_IMAGER_STATE_INCREMENT_READ_SECTOR);
    } else if (n_written == -1) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nWriting image file %s failed", FILE_IMAGE_NAME);
      set_state(WINC_IMAGER_STATE_ERROR);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nWrote %ld bytes from %s, expected",
                      n_written,
                      FILE_IMAGE_NAME,
                      FLASH_SECTOR_SZ);
      set_state(WINC_IMAGER_STATE_ERROR);
    }
  } break;

  case WINC_IMAGER_STATE_INCREMENT_READ_SECTOR: {
    s_winc_imager_ctx.sector += 1;
    set_state(WINC_IMAGER_STATE_READING_SECTORS);
  } break;

  case WINC_IMAGER_STATE_SUCCESS: {
  } break;

  case WINC_IMAGER_STATE_ERROR: {
  } break;

  } // switch
  return winc_imager_state();
}

winc_imager_state_t winc_imager_state(void) { return s_winc_imager_ctx.state; }

// *****************************************************************************
// Private (static) code

static void set_state(winc_imager_state_t new_state) {
  if (new_state != s_winc_imager_ctx.state) {
    SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                    "\n%s => %s",
                    state_name(s_winc_imager_ctx.state),
                    state_name(new_state));
    s_winc_imager_ctx.state = new_state;
  }
}

static const char *state_name(winc_imager_state_t state) {
  if (state < sizeof(s_state_names) / sizeof(s_state_names[0])) {
    return s_state_names[state];
  } else {
    return "unknown state";
  }
}

static void flush_console_input(void) {
  uint8_t ch;
  while (SYS_CONSOLE_Read(SYS_CONSOLE_DEFAULT_INSTANCE, &ch, 1) > 0) {
    asm("nop");
  }
}

static void get_y(winc_imager_state_t y_state, winc_imager_state_t n_state) {
  uint8_t ch;
  ssize_t n_read = SYS_CONSOLE_Read(SYS_CONSOLE_DEFAULT_INSTANCE, &ch, 1);
  if (n_read == 0) {
    // waiting for a response...
  } else if (n_read == -1) {
    // some form of error
    SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nError while reading response");
    set_state(WINC_IMAGER_STATE_ERROR);
  } else if (ch == 'y' || ch == 'Y') {
    set_state(y_state);
  } else {
    set_state(n_state);
  }
}
