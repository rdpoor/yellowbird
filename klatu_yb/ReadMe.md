# Klatu Networks "Yellowbird" Project
_rdpoor@gmail.com, Feb 2022_

## Context
Klatu develops and sells remote monitoring systems, with a primary focus on
performance and predictive monitoring for ultra-low refrigeration equipment.

An key offering in the Klatu product line is "Blackbird", built around the
Microchip SAME54 microcontroller and the WINC1500 network processor.

At some customer sites, the Blackbird has trouble communicating: it goes offline
sporadically, and sometimes fails to reconnect altogether.

The key goal of this Yellowbird project is to develop a simple application, also
based on the SAMD54 and WINC1500, that communicates periodically with the Klatu
servers in order to identify the root cause behind the communication failures:

* Is there a problem with the firmware in the Blackbird?
* Is there a problem with version v 19.5.4 WINC1500 firmware used by the
Blackbird?  (Note that the newest version is v 19.7.7)
* Is there a problem with the configuration of the customers' WIFI networks?

## Yellowbird Requirements

### Yellowbird Bill of Materials

The Yellowbird system comprises:
* A SAME54 Xplained Pro development board
* A WINC1500 Xplained Pro extension board
* An IO1 XPlained Pro extension board
* A FATFS-formatted microSD card
* A battery pack with an appropriate 3.3V voltage regulator.

### Software Requirements

The system shall be developed using Microchip MPLAB.X with the XC32 compiler,
the Harmony3 software framework, and the FreeRTOS operating environment.  The
SD card shall be in the FATFS format.

In order to get acceptable battery life, the system shall complete its main loop
in under under 5 seconds, and its sleep current shall be under 200 microamperes.

### Yellowbird Operation

* Upon restarting, if this is a "cold" boot due to system reset or other causes,
if will perform the following steps.  Otherwise, it will immediately enter the
main loop (see below):
  * Upon a "cold" reboot, the system will read the micro SD card in the I/O1
  Xplained Pro extension processor.
  * It will look for a file named `config.txt` in the root directory and will
  will read the following attributes from it:
    * `SSID` (mandatory): the SSID of the WIFI network to connect to
    * `Password` (mandatory): the password of the WIFI network to connect to
    * `Interval` (optional): the number of seconds to hibernate between reboots.
    Defaults to 60 seconds.
    * `WINCImage` (optional): the file name of a WINC "all in one" image to load
    into the WINC1500.
  * If the `WINCImage` attribute is present, the contents of the named file is
  compared against the WINC1500 flash memory and the WINC1500 is updated if they
  differ.
* The firmware then enters the main loop as follows:
  * The system initializes the WINC1500, connects to the AP, performs NTP and
  DNS lookups and opens a socket to the Klatu servers
  * The system sends a JSON formatted packet of data to the servers.  The packet
  contains various bits of telemetry information, including RSSI and the current
  time.
  * The system shuts down drivers and peripherals in order to conserve battery
  power.
  * The system enters hibernation mode, with an interrupt set to wake it at the
  interval specified in the config.txt file.
  * Waking from hibernation will be considered a "warm" reboot, and the system
  will repeat the main loop.

## Useful links

* [ATSAME54 Data Sheet (Cortex-M4F)](http://ww1.microchip.com/downloads/en/DeviceDoc/SAM_D5xE5x_Family_Data_Sheet_DS60001507F.pdf)
* [ATSAME54 Errata](http://ww1.microchip.com/downloads/en/DeviceDoc/SAM-D5x-E5x-Family-Silicon-Errata-DS80000748K.pdf)
* [SAM E54 Xplained Pro User's Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/70005321A.pdf)
* [SAM E54 Xplained Pro Design Documentation](https://ww1.microchip.com/downloads/en/DeviceDoc/SAM-E54-Xplained-Pro-Design-Documentaion-rev9.zip)
* [WINC1500 Xplained Pro User's Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/50002616A.pdf)
* [WINC1500 Xplained Pro Design Files](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42388-ATWINC1500-Xplained-Pro_UserGuide.zip)
* [IO1 Xplained Pro User Guide](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-42078-IO1-Xplained-Pro_User-Guide.pdf)
* [Harmony 3 Wireless application examples for WINC1500](https://microchip-mplab-harmony.github.io/wireless_apps_winc1500/)
* [Create MPLAB WINC Project - Getting Started](https://microchip-mplab-harmony.github.io/wireless_apps_winc1500/apps/getting_started/create_winc_project_from_scratch.html)
* [WINC Socket mode project with SAME54 (from scratch)](https://microchip-mplab-harmony.github.io/wireless_apps_winc1500/apps/wifi_socket_demos/winc_same54_socketmode_from_scratch.html)

## Design Notes

### The tasks

* `app_task` orchestrates the sub-tasks and provides linkage of shared data.
  * Open serial port
  * If cold boot, run the `config_task`
  * If cold boot, if winc_image_filename non-null, run winc_imager.
  * Run the `winc_task` to connect to the access point
  * Run the `http_task` to exchange a packet with the server
  * Hibernate for 60 - epsilon seconds
* `config_task` handles opening the config.txt file and extracting parameters.
  * Runs within the `app_task` (only on cold boot)
  * open the config.txt file
  * process each line to extract parameters
  * write parameters to nv storage
  * if `winc_image` parameter is present, run `winc_imager`
  * close the file
* `winc_imager` handles re-flashing the WINC chip
  * Runs withn the `app_task` (only if winc_image parameter is specified)
  * Opens the WINC in exclusive mode
  * Block by block, updates WINC firmware to match the winc_image file.
  * Closes the WINC
* `winc_task` handles opening the winc and connecting to an access point.
  * Runs within the `app_task`
  * If cold boot, lists the available access points
  * Opens a connection to the specified access point
  * Performs DHCP, DNS, NTP exchanges
* `http_task` opens a socket and exchanges a packet with the server.
  * Runs within the `winc_task` (since it requires the driver handle)
  * opens socket
  * constructs a request packet
  * sends the packet
  * waits for response
  * closes the socket

### Other modules

* `nv_data` exposes a struct for data that needs preserving across reboots
* `config_parser` parses a config.txt file
* `yb_utils` provides generally useful utilities.

## Functionality Checklist

### Done

* Boots, prints on the console
* Initializes the WINC driver
* Detects reboot cause (cold boot / warm boot)
* Initializes and mounts microSD / FATFS on cold boot
* Hibernates for 60 seconds
* Stores data in Backup RAM across hibernations, de-activates main RAM
* Open config.txt file on cold boot
* Connect to AP with SSI and Password specified in config.txt

### On Deck

* If `WINCImage` is specified in config.txt and is readable, then reflash WINC
* Parse contents of config.txt and save in preservedRam structure
* Get WINC mac address
* Perform DNS lookup
* Capture NTP timestamp
* Open socket to server
* Compose header and body for request
* Send request
* Capture response
* [Under Consideration] Log responses from server to preservedRam
* [Under Consideration] On cold boot, write preservedRam to microSD card

### Notes

I was doing an AP scan on each reboot, but that takes ~875 mSec, so now I only
do an AP scan on cold boot.  (It could be useful to factor that into its own
task so it could be invoked as needed.)
