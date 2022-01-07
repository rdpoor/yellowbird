/**
 * @file template.c
 */

// *****************************************************************************
// Includes

#include "app.h"

#include "configuration.h"
#include "spi_flash_map.h"
#include "system/debug/sys_debug.h"
#include "system/fs/sys_fs.h"
#include "user.h"
#include "wdrv_winc_client_api.h"
#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Private types and definitions

#define WINC_IMAGER_VERSION "0.0.1"

#define SDCARD_MOUNT_NAME "/mnt/mydrive"
#define SDCARD_DEV_NAME "/dev/mmcblka1"
#define FILE_IMAGE_NAME "winc.img"
#define MAX_MOUNT_RETRIES 255

#define SECTOR_TO_OFFSET(_sector) ((_sector)*FLASH_SECTOR_SZ)
#define WINC_SECTOR_COUNT 128 // TODO: look up dynamically
#define WINC_IMAGE_SIZE SECTOR_TO_OFFSET(WINC_SECTOR_COUNT)

#define DEFINE_STATES                                                          \
  DEFINE_STATE(STATE_INIT)                                                     \
  DEFINE_STATE(STATE_INITIALIZING_WINC)                                        \
  DEFINE_STATE(STATE_MOUNTING_FILESYSTEM)                                      \
  DEFINE_STATE(STATE_SET_CURRENT_DRIVE)                                        \
  DEFINE_STATE(STATE_CHECKING_FILE_INFO)                                       \
  DEFINE_STATE(STATE_OPENING_IMAGE_FILE)                                       \
  DEFINE_STATE(STATE_COMPARING_SECTORS)                                        \
  DEFINE_STATE(STATE_READING_FILE_SECTOR)                                      \
  DEFINE_STATE(STATE_READING_WINC_SECTOR)                                      \
  DEFINE_STATE(STATE_COMPARING_BUFFERS)                                        \
  DEFINE_STATE(STATE_REQUESTING_WINC_WRITE_PERMISSION)                         \
  DEFINE_STATE(STATE_ERASING_WINC_SECTOR)                                      \
  DEFINE_STATE(STATE_WRITING_WINC_SECTOR)                                      \
  DEFINE_STATE(STATE_ADVANCE_WRITE_SECTOR)                                     \
  DEFINE_STATE(STATE_REQUESTING_FILE_WRITE_PERMISSION)                         \
  DEFINE_STATE(STATE_OPENING_FILE_FOR_WRITE)                                   \
  DEFINE_STATE(STATE_READING_SECTORS)                                          \
  DEFINE_STATE(STATE_LOADING_WINC_SECTOR)                                      \
  DEFINE_STATE(STATE_WRITING_FILE_SECTOR)                                      \
  DEFINE_STATE(STATE_ADVANCE_READ_SECTOR)                                      \
  DEFINE_STATE(STATE_ERROR)                                                    \
  DEFINE_STATE(STATE_SUCCESS)                                                  \
  DEFINE_STATE(STATE_HALTED)

#undef DEFINE_STATE
#define DEFINE_STATE(_name) _name,
typedef enum { DEFINE_STATES } app_state_t;

typedef struct {
  app_state_t state;
  SYS_FS_HANDLE file_handle;
  DRV_HANDLE winc_handle;
  uint8_t sector;
  bool has_winc_write_permission;
  uint8_t mount_retries;
} app_ctx_t;

// *****************************************************************************
// Private (static, forward) declarations

static void set_state(app_state_t new_state);
static const char *state_name(app_state_t state);

// *****************************************************************************
// Private (static) storage

static app_ctx_t s_app_ctx;
static uint8_t CACHE_ALIGN s_file_buffer[FLASH_SECTOR_SZ];
static uint8_t CACHE_ALIGN s_winc_buffer[FLASH_SECTOR_SZ];

#undef DEFINE_STATE
#define DEFINE_STATE(_name) #_name,
static const char *s_state_names[] = {DEFINE_STATES};

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  /* Place the App state machine in its initial state. */
  s_app_ctx.state = STATE_INIT;
  SYS_DEBUG_ErrorLevelSet(SYS_ERROR_DEBUG);
}

void APP_Tasks(void) {
  switch (s_app_ctx.state) {

  case STATE_INIT: {
    // Arrive here on startup.  Print an introductory message...
    SYS_CONSOLE_PRINT(
        "\n========== WINC imaging tool, version %s =========="
        "\nRead and write raw WINC image from and to a file on the SD card."
        "\nRequires:"
        "\n  SAME54 Xplained Pro Development board"
        "\n  WINC1500 Xplained Pro Expansion board (in EXT-1)"
        "\n  IO/1 Xplained Pro Expansion board (in EXT-2)",
        WINC_IMAGER_VERSION);
    set_state(STATE_INITIALIZING_WINC);
  } break;

  case STATE_INITIALIZING_WINC: {
    // Open the WINC150 driver and store handle in s_app_ctx.winc_handle
    s_app_ctx.winc_handle = WDRV_WINC_Open(0, DRV_IO_INTENT_EXCLUSIVE);
    if (s_app_ctx.winc_handle != DRV_HANDLE_INVALID) {
      SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, "\nOpened WINC1500");
      s_app_ctx.mount_retries = MAX_MOUNT_RETRIES;
      set_state(STATE_MOUNTING_FILESYSTEM);
    } else {
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nUnable to open WINC1500");
      set_state(STATE_MOUNTING_FILESYSTEM);
    }
  } break;

  case STATE_MOUNTING_FILESYSTEM: {
    if (s_app_ctx.mount_retries == 0) {
      // failed to mount disk after MAX_MOUNT_RETRIES attempts
      SYS_CONSOLE_PRINT("\nfailed to mount SD card after %d attempts",
                        MAX_MOUNT_RETRIES);
      set_state(STATE_ERROR);
    } else if (SYS_FS_Mount(SDCARD_DEV_NAME, SDCARD_MOUNT_NAME, FAT, 0, NULL) != 0) {
      /* The disk was not mounted -- try again until success. */
      s_app_ctx.mount_retries -= 1;
      // Stay in this state...
      // set_state(STATE_MOUNTING_FILESYSTEM);
    } else {
      SYS_CONSOLE_PRINT("\nsetting current drive");
      set_state(STATE_SET_CURRENT_DRIVE);
    }
  } break;

  case STATE_SET_CURRENT_DRIVE: {
    // Set current drive so that we do not have to use absolute path.
    if (SYS_FS_CurrentDriveSet(SDCARD_MOUNT_NAME) == SYS_FS_RES_FAILURE) {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nUnable to select drive, error %d",
                      SYS_FS_Error());
      set_state(STATE_ERROR);
    } else {
      SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, "\nSelected mounted drive");
      set_state(STATE_CHECKING_FILE_INFO);
    }
  } break;

  case STATE_CHECKING_FILE_INFO: {
    // Checking to see winc.img file exists and is the correct size
    SYS_FS_FSTAT stat_buf;

    if (SYS_FS_FileStat(FILE_IMAGE_NAME, &stat_buf) == SYS_FS_RES_SUCCESS) {
      // fstat() completed normally.  Make sure image file is correct size.
      if (stat_buf.fsize == WINC_IMAGE_SIZE) {
        SYS_DEBUG_PRINT(
            SYS_ERROR_INFO, "\nFound valid %s file", FILE_IMAGE_NAME);
        set_state(STATE_OPENING_IMAGE_FILE);
      } else {
        SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                        "\nExpected %s to be %ld bytes, but found %ld",
                        WINC_IMAGE_SIZE,
                        stat_buf.fsize);
        set_state(STATE_ERROR);
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
        set_state(STATE_REQUESTING_FILE_WRITE_PERMISSION);
      } else {
        // some other error...
        SYS_DEBUG_PRINT(SYS_ERROR_ERROR, "\nError %d checking file", err);
        set_state(STATE_ERROR);
      }
    }
  } break;

  case STATE_OPENING_IMAGE_FILE: {
    s_app_ctx.file_handle =
        SYS_FS_FileOpen(FILE_IMAGE_NAME, SYS_FS_FILE_OPEN_READ);
    if (s_app_ctx.file_handle != SYS_FS_HANDLE_INVALID) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_INFO, "\nOpened image file %s", FILE_IMAGE_NAME);
      s_app_ctx.has_winc_write_permission = false;
      s_app_ctx.sector = 0;
      set_state(STATE_COMPARING_SECTORS);
    } else {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nUnable to open image file %s", FILE_IMAGE_NAME);
      set_state(STATE_ERROR);
    }
  } break;

  case STATE_COMPARING_SECTORS: {
    if (s_app_ctx.sector == WINC_SECTOR_COUNT) {
      SYS_DEBUG_PRINT(SYS_ERROR_INFO, "\nFinished comparison", FILE_IMAGE_NAME);
      set_state(STATE_SUCCESS);
    } else {
      set_state(STATE_READING_FILE_SECTOR);
    }
  } break;

  case STATE_READING_FILE_SECTOR: {
    size_t n_read =
        SYS_FS_FileRead(s_app_ctx.file_handle, s_file_buffer, FLASH_SECTOR_SZ);
    if (n_read == FLASH_SECTOR_SZ) {
      set_state(STATE_READING_WINC_SECTOR);
    } else if (n_read == -1) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nReading image file %s failed", FILE_IMAGE_NAME);
      set_state(STATE_ERROR);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nRead %ld bytes from %s, expected %ld",
                      n_read,
                      FILE_IMAGE_NAME,
                      FLASH_SECTOR_SZ);
      set_state(STATE_ERROR);
    }
  } break;

  case STATE_READING_WINC_SECTOR: {
    if (WDRV_WINC_NVMRead(s_app_ctx.winc_handle,
                          WDRV_WINC_NVM_REGION_RAW,
                          s_winc_buffer,
                          SECTOR_TO_OFFSET(s_app_ctx.sector),
                          FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      set_state(STATE_COMPARING_SECTORS);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nFailed to read %ld bytes from %s",
                      FLASH_SECTOR_SZ,
                      FILE_IMAGE_NAME);
    }
  } break;

  case STATE_COMPARING_BUFFERS: {
    bool buffers_differ = false;
    for (int i = 0; i < FLASH_SECTOR_SZ; i++) {
      if (s_winc_buffer[i] != s_file_buffer[i]) {
        uint32_t base = SECTOR_TO_OFFSET(s_app_ctx.sector);
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
      set_state(STATE_ADVANCE_WRITE_SECTOR);

    } else if (!s_app_ctx.has_winc_write_permission) {
      // buffers differ, but write permisson not yet granted.
      SYS_CONSOLE_PRINT(
          "\nWINC image and file image differ.  Update winc [y or Y]: ");
      s_app_ctx.state = STATE_REQUESTING_WINC_WRITE_PERMISSION;
    } else {
      // write permission already granted: go ahead and update...
      set_state(STATE_ERASING_WINC_SECTOR);
    }
  } break;

  case STATE_REQUESTING_WINC_WRITE_PERMISSION: {
    uint8_t buf[1];
    if (SYS_CONSOLE_ReadCountGet(SYS_CONSOLE_DEFAULT_INSTANCE) < sizeof(buf)) {
      // waiting for a response...
      // set_state(STATE_REQUESTING_WINC_UPDATE_PERMISSION);
    } else if (SYS_CONSOLE_Read(0, buf, sizeof(buf)) == sizeof(buf)) {
      if (buf[0] == 'y' || buf[0] == 'Y') {
        // grant write permission and update
        s_app_ctx.has_winc_write_permission = true;
        set_state(STATE_ERASING_WINC_SECTOR);
      } else {
        // stop now...
        set_state(STATE_SUCCESS);
      }
    } else {
      // Read failed for some reason...
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nFailed to read response");
    }
  } break;

  case STATE_ERASING_WINC_SECTOR: {
    // image file sector is already in s_sector_buf.  Write to WINC.
    if (WDRV_WINC_NVMEraseSector(s_app_ctx.winc_handle,
                                 WDRV_WINC_NVM_REGION_RAW,
                                 s_app_ctx.sector,
                                 1) == WDRV_WINC_STATUS_OK) {
      SYS_DEBUG_PRINT(SYS_ERROR_INFO, "\nErased sector %d", s_app_ctx.sector);
      set_state(STATE_WRITING_WINC_SECTOR);
    } else {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nErasing sector %s failed", s_app_ctx.sector);
      set_state(STATE_ERROR);
    }
  } break;

  case STATE_WRITING_WINC_SECTOR: {
    // image file sector is already in s_sector_buf.  Write to WINC.
    if (WDRV_WINC_NVMWrite(s_app_ctx.winc_handle,
                           WDRV_WINC_NVM_REGION_RAW,
                           s_file_buffer,
                           SECTOR_TO_OFFSET(s_app_ctx.sector),
                           FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      SYS_DEBUG_PRINT(SYS_ERROR_INFO, "\nWrote sector %d", s_app_ctx.sector);
      set_state(STATE_ADVANCE_WRITE_SECTOR);
    } else {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nWriting sector %s failed", s_app_ctx.sector);
      set_state(STATE_ERROR);
    }
  } break;

  case STATE_ADVANCE_WRITE_SECTOR: {
    s_app_ctx.sector += 1;
    set_state(STATE_COMPARING_SECTORS);
  } break;

  case STATE_REQUESTING_FILE_WRITE_PERMISSION: {
    uint8_t buf[1];
    if (SYS_CONSOLE_ReadCountGet(SYS_CONSOLE_DEFAULT_INSTANCE) < sizeof(buf)) {
      // waiting for a response...
      // set_state(STATE_REQUESTING_WINC_UPDATE_PERMISSION);
    } else if (SYS_CONSOLE_Read(0, buf, sizeof(buf)) == sizeof(buf)) {
      if (buf[0] == 'y' || buf[0] == 'Y') {
        set_state(STATE_OPENING_FILE_FOR_WRITE);
      } else {
        // stop now...
        set_state(STATE_SUCCESS);
      }
    } else {
      // Read failed for some reason...
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nFailed to read response");
      set_state(STATE_ERROR);
    }
  } break;

  case STATE_OPENING_FILE_FOR_WRITE: {
    s_app_ctx.file_handle =
        SYS_FS_FileOpen(FILE_IMAGE_NAME, SYS_FS_FILE_OPEN_WRITE);
    if (s_app_ctx.file_handle != SYS_FS_HANDLE_INVALID) {
      s_app_ctx.sector = 0;
      set_state(STATE_READING_SECTORS);
    } else {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nFailed to open %s for writing", FILE_IMAGE_NAME);
      set_state(STATE_ERROR);
    }
  } break;

  case STATE_READING_SECTORS: {
    if (s_app_ctx.sector == WINC_SECTOR_COUNT) {
      SYS_DEBUG_PRINT(SYS_ERROR_INFO,
                      "\nFinished reading WINC image to %s",
                      FILE_IMAGE_NAME);
      set_state(STATE_SUCCESS);
    } else {
      set_state(STATE_LOADING_WINC_SECTOR);
    }
  } break;

  case STATE_LOADING_WINC_SECTOR: {
    if (WDRV_WINC_NVMRead(s_app_ctx.winc_handle,
                          WDRV_WINC_NVM_REGION_RAW,
                          s_winc_buffer,
                          SECTOR_TO_OFFSET(s_app_ctx.sector),
                          FLASH_SECTOR_SZ) == WDRV_WINC_STATUS_OK) {
      set_state(STATE_WRITING_FILE_SECTOR);
    } else {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nFailed to read WINC sector %d", s_app_ctx.sector);
      set_state(STATE_ERROR);
    }
  } break;

  case STATE_WRITING_FILE_SECTOR: {
    size_t n_written =
        SYS_FS_FileWrite(s_app_ctx.file_handle, s_winc_buffer, FLASH_SECTOR_SZ);
    if (n_written == FLASH_SECTOR_SZ) {
      set_state(STATE_ADVANCE_READ_SECTOR);
    } else if (n_written == -1) {
      SYS_DEBUG_PRINT(
          SYS_ERROR_ERROR, "\nWriting image file %s failed", FILE_IMAGE_NAME);
      set_state(STATE_ERROR);
    } else {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nWrote %ld bytes from %s, expected",
                      n_written,
                      FILE_IMAGE_NAME,
                      FLASH_SECTOR_SZ);
      set_state(STATE_ERROR);
    }
  } break;

  case STATE_ADVANCE_READ_SECTOR: {
    s_app_ctx.sector += 1;
    set_state(STATE_READING_SECTORS);
  } break;

  case STATE_ERROR: {
    SYS_CONSOLE_PRINT("\nProgram ended with errors.");
    set_state(STATE_HALTED);
  } break;

  case STATE_SUCCESS: {
    SYS_CONSOLE_PRINT("\nProgram ended successfully.");
    set_state(STATE_HALTED);
  } break;

  default: {
    __asm("nop");
  } break;
  }
}

// *****************************************************************************
// Private (static) code

static void set_state(app_state_t new_state) {
  SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                  "\n%s => %s",
                  state_name(s_app_ctx.state),
                  state_name(new_state));
  s_app_ctx.state = new_state;
}

static const char *state_name(app_state_t state) {
  if (state < sizeof(s_state_names) / sizeof(s_state_names[0])) {
    return s_state_names[state];
  } else {
    return "unknown state";
  }
}
