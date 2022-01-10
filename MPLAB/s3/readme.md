# WINC Image Manager

This application an load or store the contents of a WINC1500 WiFi controller
processor.

For hardware, it requires:
* [SAME54 Xplained Pro](https://ww1.microchip.com/downloads/en/DeviceDoc/70005321A.pdf) evaluation kit ("SAME54 XPRO"), which is designed using the
[ATSAME54P20A](https://ww1.microchip.com/downloads/aemDocuments/documents/MCU32/ProductDocuments/DataSheets/SAM_D5x_E5x_Family_Data_Sheet_DS60001507G.pdf)
* [WINC1500 Xplained Pro](https://ww1.microchip.com/downloads/en/DeviceDoc/50002616A.pdf) extension kit, connected to EXT1 on the SAME54 XPRO
* [IO1 Xplained Pro](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42078-IO1-Xplained-Pro_User-Guide.pdf) extension kit, connected to EXT2 on the SAME54 XPRO

I/O assignments are as follows:

## WINC1500 XPRO <=> SAME54 XPRO EXT1

| EXT1 Pin | WINC1500 Name | SAME54 Name | Comments |
| --- | --- | --- | --- |
| EXT1.05 | RESET_N | PA06 |  |
| EXT1.09 | IRQ_N | PB07 | EXT IRQ7 |
| EXT1.15 | SPI_SSN | PB28 | SERCOM4[2] |
| EXT1.16 | SPI_MOSI | PB27 | SERCOM4[0] |
| EXT1.17 | SPI_MISO | PB29 | SERCOM4[3] |
| EXT1.18 | SPI_SCK | PB26 | SERCOM4[1] |

Notes:
* SPI driver is Asynchronous
* Use DMA: TX channel 1, RX channel 0
* Interrupt source is EIC7
* WINC1500 Driver Mode is Socket Mode

## IO1 XPRO <=> SAME54 XPRO EXT2

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
