# WINC Imager

The WINC Imager application is designed to extract, compare and update the contents of the flash memory of a Microchip WINC1500 WiFi module.

## Requirements

### Hardware

* [SAME54 Xplained Pro](https://ww1.microchip.com/downloads/en/DeviceDoc/70005321A.pdf) evaluation kit ("SAME54 XPRO"), which is designed using the
[ATSAME54P20A](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/DataSheets/SAM_D5x_E5x_Family_Data_Sheet_DS60001507G.pdf)
* [WINC1500 Xplained Pro](https://ww1.microchip.com/downloads/en/DeviceDoc/50002616A.pdf) extension kit, connected to EXT1 on the SAME54 XPRO
* [IO1 Xplained Pro](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42078-IO1-Xplained-Pro_User-Guide.pdf) extension kit, connected to EXT2 on the SAME54 XPRO
* A micro-SD card

### Software

* MPLAB.X v 5.50 or later
* The WINC Imager project files

## Setup and Operation

1. To the SAME54 XPRO evaluation board, connect the WINC1500 XPRO expansion
board to the EXT1 connector and the IO1 XPRO expansion board to the EXT2
connector.
2. Insert a Micro-SD card into the SD card slot of the IO1 XPRO expansion board.
3. Connect your computer's USB port to the EDBG port on the SAME54 XPRO board.
4. Open a terminal emulator such as PuTTY, Terraterm or CoolTerm.  Select the port for the SAME54 XPRO and set the baud rate to 115200.
5. Launch MPLAB.X and open the WINC Imager project
6. In MPLAB.X, select `Production => Make and Program Device Main Project`
7. In the terminal emulator, you should see a startup message like this:

```
======
WINC Imager v 1.0.0
  Connect WINC1500 XPRO to SAME54 XPRO EXT1
  Connect IO1 XPRO to SAME54 XPRO EXT2
======
```

From here, there are three possibilities:

### If there is no `winc.img` file on the SD card:

If there is no file named `winc.img` on the SD card, WINC Imager will offer to create `winc.img` and copy the contents of the WINC flash into it.  You will see a message like this:
```
Allocating system resources...
Opened WINC1500
SD card mounted after 64055 attempts
Selected mounted drive
File image winc.img not found,
Would you like to copy WINC image to winc.img [y or Y]?
```

If you type 'y', it will read the contents of the WINC flash memory and
create `winc.img` on the SD card:

```
Would you like to copy WINC image to winc.img [y or Y]?
Finished writing WINC image to winc.img
winc_imager completed successfully.
```

### If `winc.img` matches the WINC1500 flash memory:

If `winc.img` exists on the SD card, WINC Imager will compare its contents against the WINC1500 flash memory.  If they are identical,
you will see a message like this:

```
Found valid winc.img file
Comparing image file winc.img against WINC contents...
Finished comparison
winc_imager completed successfully.
```

### If `winc.img` differs from the WINC1500 flash memory:

If `winc.img` exists on the SD card, WINC Imager will compare its contents against the WINC1500 flash memory.  If they differ,
WINC Imager gives you the option of updating the WINC flash:

```
Found valid winc.img file
Comparing image file winc.img against WINC contents...
at winc[0x7097] = 0x49, file[0x7097] = 0x00
WINC image and file image differ.  Update winc [y or Y]:
```

If you type 'y', the app will update the WINC1500 firmware to match the
contents of `winc.img`, ending up like this:

```
WINC image and file image differ.  Update winc [y or Y]:
Erased sector 7
Wrote sector 7
Finished comparison
winc_imager completed successfully.
```

## Developer Notes

I/O assignments are as follows:

### WINC1500 XPRO <=> SAME54 XPRO EXT1

| EXT1 Pin | WINC1500 Name | SAME54 Name | Comments |
| --- | --- | --- | --- |
| EXT1.05 | RESET_N | PA06 | WDRV_WINC_RESETN |
| EXT1.09 | IRQ_N | PB07 | EXT IRQ7 |
| EXT1.10 | CHIP_EN | PA27 | WDV_WINC_CHIP_EN |
| EXT1.15 | SPI_SSN | PB28 | SERCOM4[2] |
| EXT1.16 | SPI_MOSI | PB27 | SERCOM4[0] |
| EXT1.17 | SPI_MISO | PB29 | SERCOM4[3] |
| EXT1.18 | SPI_SCK | PB26 | SERCOM4[1] |

Notes:
* SPI driver is Asynchronous
* Use DMA: TX channel 1, RX channel 0
* Interrupt source is EIC7
* WINC1500 Driver Mode is Socket Mode

### IO1 XPRO <=> SAME54 XPRO EXT2

| EXT2 Pin | IO1 Name | SAME54 Name | Comments |
| --- | --- | --- | --- |
| EXT2.07 | IO1_LED | PB14 | TCC4/WO[4] |
| EXT2.10 | SD_DETECT | PB02 | Might not use... |
| EXT2.15 | SPI_SSN | PC06 | SERCOM6[2] |
| EXT2.16 | SPI_MOSI | PC04 | SERCOM6[0] |
| EXT2.17 | SPI_MISO | PC07 | SERCOM6[3] |
| EXT2.18 | SPI_SCK | PC05 | SERCOM6[1] |

Notes:
* SPI Interrupts enabled
* SPI Asynchronous Mode
* SD Card: client: 1, Speed: 5MHz, xfer queue: 4, CD: polling, polling 1000 ms
* SD Card: Use DMA for RX
* Chip select: PC06

### Other SAME54-XPRO Board assignments

 | Name | SAME54 Name | Comments |
 | --- | --- | --- |
 | SW0 | PB31 | User button (low true, needs pullup) |
 | LED0 | PC18 | User LED (low true) |
 | RXD | PB24 | SERCOM2[1] |
 | TXD | PB25 | SERCOM2[0] |

 Notes:
 * TC3 16 bit mode, GCLK_TC
 * Timer Period Unit: millisecond
 * Enable Timer Compare Interrupt
 * Core: App Files, System Interrupt, System Ports, System DMA, OSAL
 * SERCOM2: TX Ring: 2048, RX Ring: 128, SYS Console ERROR_INFO, Debug yes
