# Dummy WiFi Module

## Overview

The Dummy WiFi Module is an example Linux kernel module for a Wi-Fi FullMAC driver. This module demonstrates how to create a wireless device, perform scans, and connect to a Wi-Fi network. Please note that this module is a simplified example for educational purposes and does not provide real Wi-Fi functionality.

## Module Information

- **Author:** Dr. -Ing. Ahmad Kamal Nasir <dringakn@gmail.com>
- **Version:** 1.0
- **License:** GPL v2

## Description

The Dummy WiFi Module creates a wireless device with the following capabilities:

- STA (Station) mode only.
- Scanning for nearby networks (dummy network in this case).
- Connecting to a Wi-Fi network (dummy network).

## Module Structure

The module consists of the following key components:

1. **`dummy_wifi_context` Structure:** This structure holds various components and data related to the Dummy WiFi wireless network manager.

2. **Work Queue Items:**

   - `ws_connect`: Work queue item for connection handling.
   - `ws_disconnect`: Work queue item for disconnection handling.
   - `ws_scan`: Work queue item for wireless scanning.

3. **Private Context Structures:**

   - `dummy_wifi_wiphy_priv_context`: Structure to hold private context data related to a wireless PHY (wiphy) for the Dummy WiFi system.
   - `dummy_wifi_ndev_priv_context`: Structure to hold private context data related to a network device (ndev) for the Dummy WiFi system.

4. **cfg80211_ops:** Structure defining a set of operations for interacting with the cfg80211 subsystem, including scanning, connecting, and disconnecting.

## Functionality

### Scanning

The module provides a "scan" routine that informs the Linux kernel about a "dummy" Basic Service Set (BSS). When the scan is done, it calls `cfg80211_scan_done()` to inform the kernel that the scan is finished.

### Connecting

The module also offers a "connect" routine for the Dummy WiFi device. It checks if the connecting SSID is a dummy SSID and takes appropriate actions:

- If the SSID is not a dummy, it triggers a connection timeout.
- If the SSID is a dummy, it sends dummy BSS information to the kernel and notifies the kernel of a successful connection.

### Disconnecting

The module handles disconnection events through a "disconnect" routine. It informs the wireless stack that the device has disconnected and provides a reason code for the disconnection.

## Usage

To use the Dummy WiFi Module, follow these steps:

0. Download

   ```shell
   git clone https://github.com/dringakn/DummyWiFi.git
   cd DummyWiFi

   ```

1. Build and install the module:

   ```shell
   make
   sudo insmod dummywifi.ko

   ```

2. Check if the module is loaded:

   ```shell
   lsmod | grep dummywifi

   ```

   Sample output:

   ```
   dummywifi              16384  0
   cfg80211              970752  5 iwlmvm,mt7601u,iwlwifi,mac80211,dummywifi
   ```

3. To get wifi info and to perform a scan:

   ```shell
   iw dummy info
   iw dev dummy0 scan

   ```

   Sample dummy info output using `iw dummy info`:

   ```
   Wiphy dummy
   max # scan SSIDs: 69
   max scan IEs length: 0 bytes
   max # sched scan SSIDs: 0
   max # match sets: 0
   Retry short limit: 7
   Retry long limit: 4
   Coverage class: 0 (up to 0m)
   Available Antennas: TX 0 RX 0
   Supported interface modes:
   	 * managed
   Band 1:
   	Bitrates (non-HT):
   		* 1.0 Mbps
   		* 2.0 Mbps
   		* 5.5 Mbps
   		* 11.0 Mbps
   	Frequencies:
   		* 2437 MHz [6] (20.0 dBm)
   Supported commands:
   	 * connect
   	 * disconnect
   software interface modes (can always be added):
   interface combinations are not supported
   Device supports scan flush.
   max # scan plans: 1
   max scan plan interval: -1
   max scan plan iterations: 0
   Supported extended features:

   ```

   Sample scan output using `iw dev dummy0 scan`:

   ```
   BSS aa:bb:cc:dd:ee:ff(on dummy0)
   TSF: 0 usec (0d, 00:00:00)
   freq: 2437
   beacon interval: 100 TUs
   capability: ESS (0x0001)
   last seen: 0 ms ago
   SSID: MyAwesomeWiFi
   ```

4. To connect to the dummy network:

   ```shell
   iw dev dummy0 link
   iw dev dummy0 connect MyAwesomeWiFi [2437]

   ```

   The sample output of the dummy device before connection using `iw dev dummy0 link`:

   ```
   Not connected.
   ```

   The sample output of the `iwconfig dummy0` after connection:

   ```
   dummy0   IEEE 802.11  ESSID:"MyAwesomeWiFi"
            Mode:Managed  Frequency:2.437 GHz  Access Point: AA:BB:CC:DD:EE:FF
            Retry short limit:7   RTS thr:off   Fragment thr:off
            Power Management:on
   ```

   _Note_ Since only one channel (6, 2.437GHz) is implemented, therefore connection will not be established if diffrent frequency is specified.
   Error might states `command failed: Invalid argument (-22)`. If there is already a conneciton the error might states `command failed: Operation already in progress (-114)`
   The frequency or channel information can be obtained as follows `iwlist dummy0 freq`, here is the sample output:

   ```
   dummy0   1 channels in total; available frequencies :
            Channel 06 : 2.437 GHz
            Current Frequency:2.437 GHz (Channel 6)

   ```

5. To disconnect from the network:

   ```shell
   iw dev dummy0 disconnect

   ```

   The sample output of the `iwconfig dummy0` after disconnection:

   ```
   dummy0   IEEE 802.11  ESSID:off/any
            Mode:Managed  Access Point: Not-Associated
            Retry short limit:7   RTS thr:off   Fragment thr:off
            Power Management:on

   ```

6. Unload the module when done:

   ```shell
   sudo rmmod dummywifi

   ```

7. Cleanup: To remove the module and clean up the build artifacts:
   ```shell
   make clean
   ```

# Disclaimer

This module is a simplified example for educational purposes and does not provide real Wi-Fi functionality. It serves as a starting point for developing FullMAC Wi-Fi drivers for the Linux kernel.

# License

This module is released under the GPL v2 license. See the LICENSE file for details.
