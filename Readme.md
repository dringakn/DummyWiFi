# Dummy WiFi Module

## Overview

The Dummy WiFi Module is an example Linux kernel module for a Wi-Fi FullMAC driver. This module demonstrates how to create a wireless device, perform scans, and connect to a Wi-Fi network. Please note that this module is a simplified example for educational purposes and does not provide real Wi-Fi functionality.

## Module Information

- **Author:** Ahmad Kamal Nasir <dringakn@gmail.com>
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

3. To get wifi info and to perform a scan:

   ```shell
   iw dummy info
   iw dev dummy0 scan

   ```

4. To connect to the dummy network:

   ```shell
   iw dev dummy0 connect MyAwesomeWiFi

   ```

5. To disconnect from the network:

   ```shell
   iw dev dummy0 disconnect

   ```

6. Unload the module when done:

   ```shell
   sudo rmmod dummy_wifi_module

   ```

7. Cleanup: To remove the module and clean up the build artifacts:
   ```shell
   make clean
   ```

# Disclaimer

This module is a simplified example for educational purposes and does not provide real Wi-Fi functionality. It serves as a starting point for developing FullMAC Wi-Fi drivers for the Linux kernel.

# License

This module is released under the GPL v2 license. See the LICENSE file for details.
