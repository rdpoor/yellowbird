# Klatu Networks Yellowbird v0.1.0
_rdpoor@gmail.com, Feb 2022_

## Overview

## Context
Klatu develops and sells remote monitoring systems, with a primary focus on
performance and predictive monitoring for ultra-low refrigeration equipment.

An key offering in the Klatu product line is "Blackbird", built around the
Microchip SAME54 microcontroller and the WINC1500 network processor.

At some customer sites, the Blackbird has trouble communicating: it goes offline
sporadically, and sometimes fails to reconnect altogether.

The key goal of this Yellowbird project is to develop a simple application, also
based on the SAME54 and WINC1500, that communicates periodically with remoted
servers in order to identify the root cause behind the communication failures:

* Is there a problem with the firmware in the Blackbird?
* Is there a problem with version v 19.5.4 WINC1500 firmware used by the
Blackbird?  (Note that the newest version is v 19.7.7)
* Is there a problem with the configuration of the customers' WIFI networks?

This document describes the setup and operation of Yellowbird 0.1.0.

## Prerequisites

Yellowbird 0.1.0 requires the following hardware and software components:

### Hardware

* [SAM E54 Xplained Pro evaluation kit](https://ww1.microchip.com/downloads/en/DeviceDoc/70005321A.pdf)
* [WINC1500 Xplained Pro extension board](https://ww1.microchip.com/downloads/en/DeviceDoc/50002616A.pdf)
with 3-pin header soldered to the "DEBUG UART" (J103) footprint.
* [IO1 Xplained Pro extension board](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42078-IO1-Xplained-Pro_User-Guide.pdf)
* Laptop or PC (Windows, macOS or Linux) with at least two available USB ports.
* microSD card
* USB A to USB micro cable
* [FTDI serial to USB cable](https://www.digikey.com/en/products/detail/ftdi-future-technology-devices-international-ltd/TTL-232R-RPI/4382044)

### Software

* MPLAB.X v5.0 or later
* A terminal emulator for the PC (TeraTerm or puTTY for Windows, Terminal for macOS, etc)
* WINC1500 v 19.7.6 or later loaded into the WINC1500
* [Yellowbird v0.1.0 firmware](https://github.com/klatunetworks/yellowbird/blob/yellowbird_v0_1_0/release/yb.X.production.hex)

## Setup

### Create the configuration file

1. Insert the microSD card into your PC
2. If needed, format initialize with FATFS filesystem
3. Create "config.txt" containing the following:

```
wifi_ssid = <SSID for the local Access Point>
wifi_pass = <Password for the local Access Point>
wake_interval_ms = 60000.0  # Repeat wake sequence every 60 seconds
timeout_ms = 20000.0        # Set software watchdog to 20 seconds
```

4. Safely eject the microSD card from the PC.
5. Insert the microSD card into the IO1 Xplained Pro

### Connect boards and cables

Connect boards and cables as pictured below:

![Yellowbird Harness](/docs/YellowbirdHarness.png)

1. Plug the WINC1500 XPRO into the EXT1 connector of the SAME54 XPRO
2. Plug the IO1 XPRO into the EXT2 connector of the SAME54 XPRO
3. Connect the three FTDI sockets (black, yellow, orange) into J103 on the WINC1500 XPRO.  Observe the proper order for the three wires.
4. Connect the other end of the FTDI cable to a USB port on the PC.
5. Connect the USB micro connector to the EDBG port of the WINC1500 XPRO.
6. Connect the other end of the USB connector to a USB port on the PC.

### Load and configure software

1. Launch the terminal emulator of your choice.  Attach it to the the serial
port associated with the EDBG port and configure it for 115200 baud, 8n1.
2. Launch a second instance of the terminal emulator.  Attach it to the serial
port associated with the FTDI cable and configure it for 460800 baud, 8n1.
3. Launch MPLAB.X
4. Open the yellowbird project
5. Build and run the yellowbird project.  It will load the firmware into the
SAME54 XPRO.

## Running Yellowbird 0.1.0

When you first launch the Yellowbird 0.1.0 firmware, you should see something
along these lines on the serial console.  Several things to note:
* On the first boot-up ("cold boot"), the system will consult the microSD card
and load configuration parameters from there.  It skips that step in subsequent
boot-ups ("warm boot").  So if you need to change the parameters, update the
microSD card, re-insert it, and press the `Reset` button on the SAME54 XPRO.
* Pay attention to the WINC firmware version and make sure it matches the
desired version.  Update it as required.
* The numbers in the left hand column refer to the number of milliseconds since
system boot-up.
* The penultimate line before the system hibernates shows the number of times
that the system has re-started (i.e. once every minute) and the number of times
it successfully communicated with the remote server.


```
##############################
# Klatu Networks Yellowbird, v 0.1.0 (cold boot) #1
##############################
00000232 [INFO] APP_STATE_INIT => APP_STATE_AWAIT_FILESYS
00001025 [DEBUG] Mounted FS after 98764 calls to SYS_FS_Mount()
00001030 [INFO] APP_STATE_AWAIT_FILESYS => APP_STATE_START_CONFIG_TASK
00001037 [INFO] Reading configuration file from config.txt
00001042 [INFO] APP_STATE_START_CONFIG_TASK => APP_STATE_AWAIT_CONFIG_TASK
00001048 [INFO] CONFIG_TASK_STATE_INIT => CONFIG_TASK_SETTING_DRIVE
00001054 [INFO] CONFIG_TASK_SETTING_DRIVE => CONFIG_TASK_STATE_OPENING_FILE
00001065 [INFO] FileStat of config.txt: ret=0, siz=152, fattrib=0x20, name=config.txt
00001073 [INFO] CONFIG_TASK_STATE_OPENING_FILE => CONFIG_TASK_STATE_READING_FILE
00001084 [INFO] cfg parse: key='wifi_ssid = xxxx', value='xxxx'
00001091 [INFO] cfg parse: key='wifi_pass = xxxx', value='xxxx'
00001098 [INFO] cfg parse: key='wake_interval_ms = 60000.0', value='60000.0'
00001105 [INFO] cfg parse: key='timeout_ms = 20000.0', value='20000.0'
00001111 [INFO] CONFIG_TASK_STATE_READING_FILE => CONFIG_TASK_STATE_SUCCESS
00001117 [INFO] APP_STATE_AWAIT_CONFIG_TASK => APP_STATE_WARM_BOOT
00001123 [INFO] APP_STATE_WARM_BOOT => APP_STATE_AWAIT_WINC
00001128 [INFO] APP_STATE_AWAIT_WINC => APP_STATE_START_WINC_TASK
00001134 [INFO] APP_STATE_START_WINC_TASK => APP_STATE_AWAIT_WINC_TASK
00001140 [INFO] WINC_TASK_STATE_INIT => WINC_TASK_STATE_REQ_OPEN
00001146 [INFO] WINC_TASK_STATE_REQ_OPEN => WINC_TASK_STATE_PRINT_VERSION
00001152 [INFO] WINC1500 Info:
00001155 [INFO]   Chip ID: 1377184
00001158 [INFO]   Firmware Ver: 19.7.6 SVN Rev 19389
00001163 [INFO]   Firmware Built at Oct 29 2021 Time 11:07:37
00001168 [INFO]   Firmware Min Driver Ver: 19.3.0
00001172 [INFO] WINC_TASK_STATE_PRINT_VERSION => WINC_TASK_STATE_REQ_DHCP
00001179 [INFO] WINC_TASK_STATE_REQ_DHCP => WINC_TASK_STATE_CONFIGURING_STA
00001185 [INFO] WINC_TASK_STATE_CONFIGURING_STA => WINC_TASK_STATE_START_CONNECT
00001194 [INFO] WINC_TASK_STATE_START_CONNECT => WINC_TASK_STATE_AWAIT_CONNECT
00003176 [INFO] Connected to AP
00003178 [INFO] WINC_TASK_STATE_AWAIT_CONNECT => WINC_TASK_STATE_START_HTTP_TASK
00003185 [INFO] WINC_TASK_STATE_START_HTTP_TASK => WINC_TASK_STATE_AWAIT_HTTP_TASK
00003193 [INFO] HTTP_TASK_STATE_INIT => HTTP_TASK_STATE_AWAIT_IP_LINK
00003708 [INFO] DHCP address is 192.168.1.22
00003712 [INFO] HTTP_TASK_STATE_AWAIT_IP_LINK => HTTP_TASK_STATE_START_DNS
00003719 [INFO] HTTP_TASK_STATE_START_DNS => HTTP_TASK_STATE_AWAIT_DNS
00003748 [INFO] example.com resolved to 93.184.216.34
00003752 [INFO] HTTP_TASK_STATE_AWAIT_DNS => HTTP_TASK_STATE_START_SOCKET
00003759 [INFO] HTTP_TASK_STATE_START_SOCKET => HTTP_TASK_STATE_AWAIT_SOCKET
00003779 [INFO] Socket 0 connected
00003782 [INFO] HTTP_TASK_STATE_AWAIT_SOCKET => HTTP_TASK_STATE_START_SEND
00003789 [INFO] Sending
==>>>
GET /index.html HTTP/1.1
Host: example.com
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:95.0)
Accept: */*;q=0.8

==>>>
00003804 [INFO] HTTP_TASK_STATE_START_SEND => HTTP_TASK_STATE_AWAIT_SEND
00003810 [INFO] HTTP_TASK_STATE_AWAIT_SEND => HTTP_TASK_STATE_AWAIT_RESPONSE
00003819 [INFO] Received response:
==<<<
HTTP/1.1 200 OK
Accept-Ranges: bytes
Age: 216545
Cache-Control: max-age=604800
Content-Type: text/html; charset=UTF-8
Date: Sun, 15 May 2022 10:45:18 GMT
Etag: "3147526947"
Expires: Sun, 22 May...
==<<<
00003840 [INFO] HTTP_TASK_STATE_AWAIT_RESPONSE => HTTP_TASK_STATE_SUCCESS
00003847 [INFO] WINC_TASK_STATE_AWAIT_HTTP_TASK => WINC_TASK_STATE_START_DISCONNECT
00003855 [INFO] WINC_TASK_STATE_START_DISCONNECT => WINC_TASK_STATE_AWAIT_DISCONNECT
00003864 [INFO] Disconnected from AP
00003867 [INFO] WINC_TASK_STATE_AWAIT_DISCONNECT => WINC_TASK_STATE_SUCCESS
00003874 [INFO] APP_STATE_AWAIT_WINC_TASK => APP_STATE_START_HIBERNATION
00003880 [INFO] Success / Attempts = 1 / 1
00003884 [INFO] Preparing to hibernate
```


## dev notes for v0.1.0

### On cold boot:

Consult SD card for presence of `config.txt` file.  If present, parse the file
to get any application parameters:

* wifi_ssid: The SSID of the SP [default = none]
* wifi_pass: The WiFi password [default = none]
* host_url: The URL for a host server for a packet exchange.
* wake_interval_ms: How often the system wakes [60000]
* timeout_ms: How long the system will stay awake before timeout [15000]
* winc_image: The filename of the WINC image to flash, if present [none]
* log_level: The debug level for serial output

All configuration parameters except winc_image are saved to non-volatile RAM
and used on warm reboots.

If winc_iamge is present, and the named file exists, the WINC will be reflashed
with the named image.

### On warm boot (and after cold boot)

* Initialize the WINC
* Connect to the AP
* Open a socket to the host server
* Send a packet
* Verify response
* hibernate until wake_interval_ms has elapsed
* Reboot

If the above sequence does not complete within timeout_ms, the system will
stop what it is doing and (almost) immediately hibernate, shutting down any
peripherals prior to hibernations.

### Waking and hibernating

On cold boot the system writes the RTC count into app.prev_wake_at.  Before
hibernating, it increments prev_wake_at by wake_interval_ms and sets the RTC
to wake the processor at that time (assuming it is sufficiently far in the
future) and enters hibernate mote.

But before hibernating, it calls `<module>_will_sleep()` for relevant modules
in order to let them do any cleanup prior to hibernation.

### Module list (tentative)

#### tasks (with internal state)

* app - low-level chip and board initialization, marshals other modules.  On
  cold boot, opens filesystem on the sd card.
* config_task - reads filesystem, reads and processes config.txt
* http_task - opens socket, exchanges packet with remote host.
* imager_task - reads filesystem, reprograms WINC as needed.
* winc_task - manages the winc chip through its various states.

#### other modules

* mu_parse_cfg - parse a config.txt formatted file
* mu_parse_url - parse a URL to extract socket, etc.
* mu_strbuf, mu_str - "zero copy" string manipulation functions
* nv_data - non-volatile data storage and retrieval
* yb_log - logging support that honors log_level

## Yellowbird Requirements

## Useful links

* [ATSAME54 Data Sheet (Cortex-M4F)](http://ww1.microchip.com/downloads/en/DeviceDoc/SAM_D5xE5x_Family_Data_Sheet_DS60001507F.pdf)
* [ATSAME54 Errata](http://ww1.microchip.com/downloads/en/DeviceDoc/SAM-D5x-E5x-Family-Silicon-Errata-DS80000748K.pdf)
* [SAM E54 Xplained Pro User's Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/70005321A.pdf)
* [SAM E54 Xplained Pro Design Documentation](https://ww1.microchip.com/downloads/en/DeviceDoc/SAM-E54-Xplained-Pro-Design-Documentaion-rev9.zip)
* [WINC1500 Xplained Pro User's Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/50002616A.pdf)
* [WINC1500 Xplained Pro Design Files](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42388-ATWINC1500-Xplained-Pro_UserGuide.zip)
* [IO1 Xplained Pro User Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42078-IO1-Xplained-Pro_User-Guide.pdf)
