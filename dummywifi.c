/**
 * @file dummy_wifi_module.c
 * @brief Example Linux kernel module for a Wi-Fi FullMAC driver
 */

#include <linux/module.h>    // Linux module support
#include <linux/semaphore.h> // Semaphore support
#include <linux/skbuff.h>    // Network packet manipulation
#include <linux/workqueue.h> // Workqueue support
#include <net/cfg80211.h>    // Configuration 802.11 framework

#define WIPHY_NAME "dummy"         // Name of the Wi-Fi device
#define NDEV_NAME "dummy%d"        // Name template for network devices
#define SSID_DUMMY "MyAwesomeWiFi" // Default SSID for the Wi-Fi network
#define SSID_DUMMY_SIZE (sizeof("MyAwesomeWiFi") - 1) // Size of the SSID

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Ahmad Kamal Nasir <dringakn@gmail.com>");
MODULE_VERSION("1.0"); // Module version
MODULE_DESCRIPTION(
    "Dumb example for cfg80211(aka FullMAC) driver."
    "Module creates wireless device with network."
    "The device can work as station(STA mode) only."
    "The device can perform scan that \"scans\" only dummy network."
    "Also it performs \"connect\" to the dummy network.");

/**
 * @struct dummy_wifi_context
 * @brief Context structure for the DummyWiFi wireless network manager.
 *
 * This structure holds various components and data related to the DummyWiFi
 * wireless network manager.
 */
struct dummy_wifi_context {
  struct wiphy *wiphy;     /**< Pointer to the wireless PHY device. */
  struct net_device *ndev; /**< Pointer to the network device. */
  struct semaphore sem;    /**< Semaphore for synchronization. */

  struct work_struct
      ws_connect; /**< Work queue item for connection handling. */
  char connecting_ssid[sizeof(
      SSID_DUMMY)]; /**< SSID of the currently connecting network. */

  struct work_struct
      ws_disconnect; /**< Work queue item for disconnection handling. */
  u16 disconnect_reason_code; /**< Reason code for the disconnection. */

  struct work_struct ws_scan; /**< Work queue item for wireless scanning. */
  struct cfg80211_scan_request
      *scan_request; /**< Scan request configuration. */
};

/**
 * @struct dummy_wifi_context
 * @brief Represents the context structure for the DummyWiFi application.
 * @brief g_ctx:Global context variable for the DummyWiFi application.
 *
 * This variable holds a pointer to an instance of the `dummy_wifi_context`
 * structure, which is used to maintain the application's state and
 * configuration. It is initialized as NULL and should be allocated memory and
 * initialized before use.
 */
static struct dummy_wifi_context *g_ctx = NULL;

/**
 * @struct dummy_wifi_wiphy_priv_context
 * @brief Structure to hold private context data related to a wireless PHY
 * (wiphy) for the DummyWiFi system.
 *
 * This structure is used to associate specific context data with a wireless PHY
 * (wiphy) in the DummyWiFi system.
 */
struct dummy_wifi_wiphy_priv_context {
  struct dummy_wifi_context
      *navi; ///< Pointer to the DummyWiFi context associated with this wiphy.
};

/**
 * @struct dummy_wifi_ndev_priv_context
 * @brief Structure to hold private context data related to a network device
 * (ndev) for the DummyWiFi system.
 *
 * This structure is used to associate specific context data with a network
 * device (ndev) in the DummyWiFi system.
 */
struct dummy_wifi_ndev_priv_context {
  struct dummy_wifi_context
      *navi; ///< Pointer to the DummyWiFi context associated with this ndev.
  struct wireless_dev
      wdev; ///< Wireless device structure associated with this ndev.
};

/**
 * @brief Get the private context associated with a wireless PHY (wiphy).
 *
 * This function retrieves a pointer to a custom private context structure
 * specific to the DummyWiFi wireless PHY, which is associated with the given
 * wiphy structure.
 *
 * @param wiphy A pointer to the wiphy structure representing the wireless PHY.
 *
 * @return A pointer to the dummy_wifi_wiphy_priv_context structure associated
 *         with the wiphy, cast to the appropriate type.
 */
static struct dummy_wifi_wiphy_priv_context *
wiphy_get_navi_context(struct wiphy *wiphy) {
  // Cast the private data pointer associated with the wiphy to the
  // dummy_wifi_wiphy_priv_context type and return it.
  return (struct dummy_wifi_wiphy_priv_context *)wiphy_priv(wiphy);
}

/**
 * @brief Get the private context associated with a network device.
 *
 * This function retrieves the private context structure associated with a
 * network device. It assumes that the network device has a private context
 * structure set using the `netdev_priv` macro.
 *
 * @param ndev Pointer to the network device for which the private context
 *             is to be retrieved.
 *
 * @return A pointer to the private context structure associated with the
 *         network device.
 */
static struct dummy_wifi_ndev_priv_context *
ndev_get_navi_context(struct net_device *ndev) {
  // Cast the result of `netdev_priv(ndev)` to a pointer of the desired type.
  return (struct dummy_wifi_ndev_priv_context *)netdev_priv(ndev);
}

/**
 * @brief Inform the kernel about a dummy BSS (Basic Service Set).
 *
 * This function informs the Linux kernel about the presence of a dummy BSS
 * (Basic Service Set) for a wireless network interface. It provides information
 * about the BSS, such as its channel, signal strength, BSSID, and other
 * details.
 *
 * @param navi Pointer to the navigation context structure.
 */
static void inform_dummy_bss(struct dummy_wifi_context *navi) {
  struct cfg80211_bss *bss = NULL;

  // Define the information about the BSS.
  struct cfg80211_inform_bss data = {
      .chan = &navi->wiphy->bands[NL80211_BAND_2GHZ]
                   ->channels[0], /* the only channel for this demo */
      .scan_width = NL80211_BSS_CHAN_WIDTH_20,
      /* signal "type" not specified in this DEMO so it's basically unused,
         it can be some kind of percentage from 0 to 100 or mBm value */
      /* signal "type" may be specified before wiphy registration by setting
         wiphy->signal_type */
      .signal = 1337,
  };

  // Define the BSSID (Basic Service Set Identifier).
  char bssid[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

  /* ie - array of tags that are usually retrieved from the beacon frame or
     probe response. */
  char ie[SSID_DUMMY_SIZE + 2] = {WLAN_EID_SSID, SSID_DUMMY_SIZE};
  // Copy the SSID (Service Set Identifier) into the IE (Information Element)
  // array.
  memcpy(ie + 2, SSID_DUMMY, SSID_DUMMY_SIZE);

  /* It is also possible to use cfg80211_inform_bss() instead of
     cfg80211_inform_bss_data() */
  // Inform the kernel about the BSS with the provided data.
  bss = cfg80211_inform_bss_data(navi->wiphy, &data, CFG80211_BSS_FTYPE_UNKNOWN,
                                 bssid, 0, WLAN_CAPABILITY_ESS, 100, ie,
                                 sizeof(ie), GFP_KERNEL);

  /* Free the cfg80211_bss structure. The refcounter of the structure
     should be decremented if it's not used. */
  cfg80211_put_bss(navi->wiphy, bss);
}

/**
 * @brief This function is a routine for performing a Wi-Fi scan in the dummy
 * context.
 * "Scan" routine for DEMO. It just inform the kernel about "dummy" BSS and
 * "finishs" scan. When scan is done it should call cfg80211_scan_done() to
 * inform the kernel that scan is finished. This routine called through
 * workqueue, when the kernel asks about scan through cfg80211_ops.
 *
 * @param w A pointer to a work_struct representing the work to be done.
 */
static void dummy_wifi_scan_routine(struct work_struct *w) {
  // Retrieve the dummy context from the work_struct using container_of
  struct dummy_wifi_context *navi =
      container_of(w, struct dummy_wifi_context, ws_scan);

  // Create a structure to hold scan information, initialize 'aborted' to false
  struct cfg80211_scan_info info = {
      /* if scan was aborted by user (calling cfg80211_ops->abort_scan) or by
         any driver/hardware issue - field should be set to "true" */
      .aborted = false,
  };

  /* Pretend some work, also, you can't call cfg80211_scan_done right away after
   * cfg80211_ops->scan(). It's unclear why, but the netlink client may not
   * receive the "scan done" message. It could be due to potential concurrency
   * between "scan_routine" and cfg80211_ops->scan(), causing
   * cfg80211_scan_done() to be called before cfg80211_ops->scan() returns. */
  msleep(100);

  /* Inform with dummy BSS */
  inform_dummy_bss(navi);

  // Check if the semaphore can be acquired, return if interrupted
  if (down_interruptible(&navi->sem)) {
    return;
  }

  /* Finish the scan by calling cfg80211_scan_done() with the scan request and
   * info. It marks the scan as complete and provides information about the scan
   * status. */
  cfg80211_scan_done(navi->scan_request, &info);

  // Reset the scan_request pointer to NULL
  navi->scan_request = NULL;

  // Release the semaphore to allow other processes to access the dummy
  // context
  up(&navi->sem);
}

/**
 * @brief Connect routine for the DummyWiFi device.
 * This function is responsible for handling the connection routine of the
 * DummyWiFi device. It checks if the connecting SSID is a dummy SSID and takes
 * appropriate actions.
 *
 * It just checks SSID of the ESS to connect and informs the kernel that connect
 * is finished. It should call cfg80211_connect_bss() when connect is finished
 * or cfg80211_connect_timeout() when connect is failed. This "demo" can connect
 * only to ESS with SSID equal to SSID_DUMMY value. This routine called through
 * workqueue, when the kernel asks about connect through cfg80211_ops.
 *
 *
 * @param w A pointer to the work_struct associated with the connection routine.
 */
static void dummy_wifi_connect_routine(struct work_struct *w) {
  // Retrieve the DummyWiFi context from the work_struct.
  struct dummy_wifi_context *navi =
      container_of(w, struct dummy_wifi_context, ws_connect);

  // Attempt to acquire a semaphore lock. If interrupted, return.
  if (down_interruptible(&navi->sem)) {
    return;
  }

  // Check if the connecting SSID is not a dummy SSID.
  if (memcmp(navi->connecting_ssid, SSID_DUMMY, sizeof(SSID_DUMMY)) != 0) {
    // The SSID is not a dummy, trigger a connection timeout.
    cfg80211_connect_timeout(navi->ndev, NULL, NULL, 0, GFP_KERNEL,
                             NL80211_TIMEOUT_SCAN);
  } else {
    // The SSID is a dummy SSID.

    // Send dummy BSS information to the kernel.
    inform_dummy_bss(navi);

    // Notify the kernel of a successful connection to a known ESS.
    // It's also possible to use cfg80211_connect_result() or
    // cfg80211_connect_done().
    cfg80211_connect_bss(navi->ndev, NULL, NULL, NULL, 0, NULL, 0,
                         WLAN_STATUS_SUCCESS, GFP_KERNEL,
                         NL80211_TIMEOUT_UNSPECIFIED);
  }

  // Clear the connecting SSID to indicate that the connection is complete.
  navi->connecting_ssid[0] = 0;

  // Release the semaphore lock.
  up(&navi->sem);
}

/**
 * @brief Disconnect routine for the DummyWiFi driver.
 *
 * This function is responsible for handling disconnection events in the
 * DummyWiFi driver. It gets invoked as part of a work queue.
 * This routine called through workqueue, when the kernel asks about
 * disconnect through cfg80211_ops.
 *
 * @param w Pointer to the work structure associated with the disconnection.
 */
static void dummy_wifi_disconnect_routine(struct work_struct *w) {

  // Extract the DummyWiFi context structure from the work structure.
  struct dummy_wifi_context *navi =
      container_of(w, struct dummy_wifi_context, ws_disconnect);

  // Attempt to acquire a semaphore, which is a synchronization primitive.
  // If interrupted by a signal (e.g., Ctrl+C), return without further action.
  if (down_interruptible(&navi->sem)) {
    return;
  }

  // This function informs the wireless stack that the device has disconnected.
  // Notify the wireless networking stack about the disconnection event.
  // Parameters:
  // - navi->ndev: The DummyWiFi network device.
  // - navi->disconnect_reason_code: The reason code for the disconnection.
  // - NULL: No IEs (Information Elements) to include in the disconnect frame.
  // - 0: Length of the IEs (0 because there are no IEs).
  // - true: Indicate that the disconnection is initiated by the local device.
  // - GFP_KERNEL: Memory allocation flags (Kernel memory allocation).
  cfg80211_disconnected(navi->ndev, navi->disconnect_reason_code, NULL, 0, true,
                        GFP_KERNEL);

  // Reset the disconnect reason code to 0 to indicate a clean disconnection.
  navi->disconnect_reason_code = 0;

  // Release the previously acquired semaphore to allow other threads to access
  // shared resources.
  up(&navi->sem);
}

/**
 * @brief Initiates a scan operation on a wireless device.
 *
 * This function is responsible for initiating a scan operation on a given
 * wireless device. It acquires the necessary synchronization semaphore and
 * schedules a work item to perform the scan.
 *
 * @param wiphy The wireless PHY device for which the scan is to be initiated.
 * @param request The scan request configuration.
 * @return 0 on success, -ERESTARTSYS if interrupted by a signal, -EBUSY if
 *         another scan operation is already in progress.
 */
static int nvf_scan(struct wiphy *wiphy,
                    struct cfg80211_scan_request *request) {
  // Obtain the DummyWiFi context associated with the wireless PHY device.
  struct dummy_wifi_context *navi = wiphy_get_navi_context(wiphy)->navi;

  // Attempt to acquire the synchronization semaphore; this prevents concurrent
  // access to DummyWiFi context.
  if (down_interruptible(&navi->sem)) {
    // The semaphore acquisition was interrupted by a signal, so return an
    // error.
    return -ERESTARTSYS;
  }

  // Check if there's already a scan request in progress; if so, release the
  // semaphore and return an error indicating that the device is busy.
  if (navi->scan_request != NULL) {
    up(&navi->sem); // Release the semaphore.
    return -EBUSY;
  }

  // Set the scan request in the DummyWiFi context to the provided request.
  navi->scan_request = request;

  // Release the semaphore, allowing other operations to proceed.
  up(&navi->sem);

  // Schedule a work item to perform the scan operation.
  if (!schedule_work(&navi->ws_scan)) {
    // If scheduling the work item fails (e.g., due to lack of resources),
    // return an error.
    return -EBUSY;
  }

  // Return 0 to indicate that the scan operation was successfully initiated.
  return 0; /* OK */
}

/**
 * @brief Connects a wireless device to a network.
 *
 * This function connects a wireless device to a network using the provided
 * parameters.
 *
 * @param wiphy The wireless PHY (physical layer) structure.
 * @param dev The network device structure.
 * @param sme The connection parameters including SSID.
 *
 * @return 0 on success, -ERESTARTSYS if interrupted by a signal,
 *         or -EBUSY if the connection work is already scheduled.
 */
static int nvf_connect(struct wiphy *wiphy, struct net_device *dev,
                       struct cfg80211_connect_params *sme) {
  // Get the DummyWiFi context associated with the wireless PHY.
  struct dummy_wifi_context *navi = wiphy_get_navi_context(wiphy)->navi;

  // Determine the length of the SSID, limited to 15 characters.
  size_t ssid_len = sme->ssid_len > 15 ? 15 : sme->ssid_len;

  // Attempt to acquire the semaphore with interruptible wait.
  if (down_interruptible(&navi->sem)) {
    // The semaphore acquisition was interrupted by a signal.
    return -ERESTARTSYS;
  }

  // Copy the SSID from the connection parameters to the DummyWiFi context.
  // Ensure it is null-terminated.
  memcpy(navi->connecting_ssid, sme->ssid, ssid_len);
  navi->connecting_ssid[ssid_len] = 0;

  // Release the semaphore.
  up(&navi->sem);

  // Schedule the connection work to be performed asynchronously.
  if (!schedule_work(&navi->ws_connect)) {
    // The work is already scheduled or the scheduling failed.
    return -EBUSY;
  }

  // The connection request was successfully initiated.
  return 0;
}

/**
 * @brief Disconnect a wireless device from a Wi-Fi network.
 *
 * This function is used to disconnect a wireless device from a Wi-Fi network
 * by providing a reason code for the disconnection.
 *
 * @param wiphy Pointer to the wireless hardware (WiPHY) structure.
 * @param dev Pointer to the network device structure.
 * @param reason_code The reason code for the disconnection.
 * @return 0 on success, -ERESTARTSYS if interrupted by a signal, -EBUSY if work
 *         scheduling fails.
 */
static int nvf_disconnect(struct wiphy *wiphy, struct net_device *dev,
                          u16 reason_code) {
  // Retrieve the DummyWiFi context associated with the provided wiphy
  // structure.
  struct dummy_wifi_context *navi = wiphy_get_navi_context(wiphy)->navi;

  // Attempt to acquire a semaphore lock, allowing interruption by signals.
  if (down_interruptible(&navi->sem)) {
    return -ERESTARTSYS; // Return with error code if interrupted.
  }

  // Set the disconnect reason code in the DummyWiFi context.
  navi->disconnect_reason_code = reason_code;

  // Release the semaphore lock.
  up(&navi->sem);

  // Schedule the disconnection work to be executed asynchronously.
  if (!schedule_work(&navi->ws_disconnect)) {
    return -EBUSY; // Return with an error code if scheduling fails.
  }
  return 0; // Return success status.
}

/**
 * @brief Structure for storing operations related to the cfg80211 subsystem.
 *
 * This structure defines a set of operations that can be performed on the
 * cfg80211 subsystem. These operations are typically implemented by a driver
 * or a module to interact with the wireless networking stack.
 *
 * Structure of functions for FullMAC 80211 drivers.
 * Functions that implemented along with fields/flags in wiphy structure would
 * represent drivers features. This DEMO can only perform "scan" and "connect".
 * Some functions cant be implemented alone, for example: with "connect" there
 * is should be function "disconnect".
 */
static struct cfg80211_ops nvf_cfg_ops = {
    /**
     * @brief Function pointer to the scan operation.
     *
     * This function is called when the cfg80211 subsystem needs to perform a
     * Wi-Fi network scan. It should be implemented to initiate and handle the
     * scanning process.
     */
    .scan = nvf_scan,

    /**
     * @brief Function pointer to the connect operation.
     *
     * This function is called when the cfg80211 subsystem needs to establish
     * a connection to a Wi-Fi network. It should be implemented to handle the
     * connection setup and management.
     */
    .connect = nvf_connect,

    /**
     * @brief Function pointer to the disconnect operation.
     *
     * This function is called when the cfg80211 subsystem needs to disconnect
     * from a currently connected Wi-Fi network. It should be implemented to
     * perform the necessary cleanup and disconnection steps.
     */
    .disconnect = nvf_disconnect,
};

/**
 * @brief This function is the network device driver's start_xmit callback.
 *
 * It is called when the network stack wants to transmit a packet using
 * the specified network device. In this implementation, the function
 * simply frees the provided skb (socket buffer) since there is no
 * actual packet transmission happening. Note that the skb ownership
 * is transferred to this callback, so it is responsible for cleanup.
 *
 * @param skb Pointer to the socket buffer containing the packet to be
 * transmitted.
 * @param dev Pointer to the network device structure.
 *
 * @return Returns NETDEV_TX_OK to indicate that the packet was successfully
 *         processed (even though no actual transmission occurred).
 */
static netdev_tx_t nvf_ndo_start_xmit(struct sk_buff *skb,
                                      struct net_device *dev) {
  /* Free the skb as its ownership has moved to the xmit callback. */
  kfree_skb(skb);

  /* Indicate that the packet was successfully processed. */
  return NETDEV_TX_OK;
}

/**
 * @brief Network device operations structure for NVF driver
 *
 * This structure defines the network device operations for the NVF driver.
 * It specifies functions that should be implemented to handle various network
 * device tasks.
 * Structure of functions for network devices. It should have at least
 * ndo_start_xmit functions that called for packet to be sent.
 */
static struct net_device_ops nvf_ndev_ops = {
    /**
     * @brief Start transmission of a network packet
     *
     * This function is responsible for initiating the transmission of a network
     * packet. It is part of the network device operations and is called when
     * the network device needs to send a packet.
     *
     * @param skb Pointer to the network packet to be transmitted.
     * @param dev Pointer to the network device.
     * @return 0 on success, an error code on failure.
     */
    .ndo_start_xmit = nvf_ndo_start_xmit,
};

/**
 * @brief Structure to represent a supported 2.4 GHz Wi-Fi channel.
 *
 * This structure holds information about a 2.4 GHz Wi-Fi channel that is
 * supported by a device.
 */
static struct ieee80211_channel nvf_supported_channels_2ghz[] = {{
    /**
     * @brief The band to which the channel belongs.
     *
     * This field specifies that the channel belongs to the 2.4 GHz band.
     * NL80211_BAND_2GHZ is a constant representing the 2.4 GHz band.
     */
    .band = NL80211_BAND_2GHZ,

    /**
     * @brief Hardware-specific value for the channel.
     *
     * This field represents a hardware-specific value associated with the
     * channel. In this case, the channel is assigned the value 6.
     */
    .hw_value = 6,

    /**
     * @brief The center frequency of the channel in megahertz (MHz).
     *
     * This field specifies the center frequency of the channel in megahertz.
     * For this channel, it is set to 2437 MHz, which corresponds to channel 6
     * in the 2.4 GHz band.
     */
    .center_freq = 2437,
}};

/**
 * @brief An array of supported IEEE 802.11 rates for 2.4GHz frequency band.
 *
 * This array defines the supported rates for the 2.4GHz frequency band in a
 * structured format using a C array of structures.
 */
static struct ieee80211_rate nvf_supported_rates_2ghz[] = {
    {
        .bitrate = 10,   /**< Bitrate in Mbps for the rate (10 Mbps). */
        .hw_value = 0x1, /**< Hardware-specific value (0x1). */
    },
    {
        .bitrate = 20,   /**< Bitrate in Mbps for the rate (20 Mbps). */
        .hw_value = 0x2, /**< Hardware-specific value (0x2). */
    },
    {
        .bitrate = 55,   /**< Bitrate in Mbps for the rate (55 Mbps). */
        .hw_value = 0x4, /**< Hardware-specific value (0x4). */
    },
    {
        .bitrate = 110,  /**< Bitrate in Mbps for the rate (110 Mbps). */
        .hw_value = 0x8, /**< Hardware-specific value (0x8). */
    }};

/**
 * @brief Data structure for representing the IEEE 802.11 supported band for
 * the 2.4 GHz frequency range.
 *
 * This structure defines the capabilities and characteristics of the 2.4 GHz
 * band for IEEE 802.11 wireless networking.
 */
static struct ieee80211_supported_band nf_band_2ghz = {
    .ht_cap.cap = IEEE80211_HT_CAP_SGI_20, /* Set the Short Guard Interval (SGI)
                                              capability to 20 MHz width. */
    .ht_cap.ht_supported =
        false, /* Indicate that HT (High Throughput) is not supported. */

    .channels = nvf_supported_channels_2ghz, /* Store the array of supported
                                                channels for 2.4 GHz band. */
    .n_channels =
        ARRAY_SIZE(nvf_supported_channels_2ghz), /* Store the number of
                                                    supported channels. */

    .bitrates = nvf_supported_rates_2ghz, /* Store the array of supported bit
                                             rates for 2.4 GHz band. */
    .n_bitrates =
        ARRAY_SIZE(nvf_supported_rates_2ghz), /* Store the number of supported
                                                 bit rates. */
};

/**
 * @brief Create a new dummy context.
 *
 * This function creates a new dummy context, which represents a wireless
 * device.
 * Function that creates wiphy context and net_device with wireless_dev.
 * wiphy/net_device/wireless_dev is basic interfaces for the kernel to interact
 * with driver as wireless one. It returns driver's main "dummy" context.
 *
 * @return A pointer to the newly created dummy context, or NULL on failure.
 */
static struct dummy_wifi_context *dummy_wifi_create_context(void) {
  struct dummy_wifi_context *ret = NULL;
  struct dummy_wifi_wiphy_priv_context *wiphy_data = NULL;
  struct dummy_wifi_ndev_priv_context *ndev_data = NULL;

  /* Allocate memory for the dummy context */
  ret = kmalloc(sizeof(*ret), GFP_KERNEL);
  if (!ret) {
    goto l_error;
  }

  /* Allocate memory for the wiphy context, representing a wireless device.
   * This context is used for communication with the wireless subsystem. */
  ret->wiphy = wiphy_new_nm(
      &nvf_cfg_ops, sizeof(struct dummy_wifi_wiphy_priv_context), WIPHY_NAME);
  if (ret->wiphy == NULL) {
    goto l_error_wiphy;
  }

  /* Save the dummy context in wiphy private data. */
  wiphy_data = wiphy_get_navi_context(ret->wiphy);
  wiphy_data->navi = ret;

  /* Set the device object as wiphy "parent." This is typically a physical
   * device. Uncomment this line if you have a device object available. */
  /* set_wiphy_dev(ret->wiphy, dev); */

  /* Set the supported interface mode for the wiphy context. In this case, it's
   * NL80211_IFTYPE_STATION (station mode). */
  ret->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION);

  /* Define the supported frequency bands for the wireless device.
   * In this example, we use only the 2.4 GHz band (NL80211_BAND_2GHZ).
   * You can add other bands as needed. */
  ret->wiphy->bands[NL80211_BAND_2GHZ] = &nf_band_2ghz;

  /* Set the maximum number of SSIDs that can be scanned for. */
  ret->wiphy->max_scan_ssids = 69;

  /* Register the wiphy context. After this, a new wireless device should be
   * visible in the system. You can check with: $ iw list Wiphy dummy */
  if (wiphy_register(ret->wiphy) < 0) {
    goto l_error_wiphy_register;
  }

  /* Allocate network device context. */
  ret->ndev =
      alloc_netdev(sizeof(*ndev_data), NDEV_NAME, NET_NAME_ENUM, ether_setup);
  if (ret->ndev == NULL) {
    goto l_error_alloc_ndev;
  }

  /* Fill the private data of the network context. */
  ndev_data = ndev_get_navi_context(ret->ndev);
  ndev_data->navi = ret;

  /* Fill wireless_dev context. This context combines a network device and
   * wireless information. */
  ndev_data->wdev.wiphy = ret->wiphy;
  ndev_data->wdev.netdev = ret->ndev;
  ndev_data->wdev.iftype = NL80211_IFTYPE_STATION;
  ret->ndev->ieee80211_ptr = &ndev_data->wdev;

  /* Set the device object for the net_device. Uncomment this line if you have
   * a device object available. */
  /* SET_NETDEV_DEV(ret->ndev, wiphy_dev(ret->wiphy)); */

  /* Set network device hooks, such as ndo_start_xmit(). */
  ret->ndev->netdev_ops = &nvf_ndev_ops;

  /* Add proper net_device initialization here. */

  /* Register the network device. After this, a new network device should be
   * visible with: $ ip a */
  if (register_netdev(ret->ndev)) {
    goto l_error_ndev_register;
  }

  return ret;

l_error_ndev_register:
  free_netdev(ret->ndev);
l_error_alloc_ndev:
  wiphy_unregister(ret->wiphy);
l_error_wiphy_register:
  wiphy_free(ret->wiphy);
l_error_wiphy:
  kfree(ret);
l_error:
  return NULL;
}

/**
 * @brief Free the resources associated with a dummy context.
 *
 * This function is responsible for releasing the memory and resources
 * associated with a dummy context. It performs the following actions:
 *
 * 1. Checks if the context pointer is not NULL to avoid dereferencing a
 *    null pointer.
 * 2. Unregisters the network device (netdev) associated with the context.
 * 3. Frees the network device.
 * 4. Unregisters the wireless PHY (wiphy) associated with the context.
 * 5. Frees the wireless PHY.
 * 6. Finally, deallocates the memory used by the dummy context itself.
 *
 * @param ctx Pointer to the dummy context to be freed.
 */
static void dummy_wifi_free(struct dummy_wifi_context *ctx) {
  // Check if the context pointer is NULL to avoid dereferencing a null pointer.
  if (ctx == NULL) {
    return;
  }

  // Unregister the network device (netdev) associated with the context.
  unregister_netdev(ctx->ndev);

  // Free the network device.
  free_netdev(ctx->ndev);

  // Unregister the wireless PHY (wiphy) associated with the context.
  wiphy_unregister(ctx->wiphy);

  // Free the wireless PHY.
  wiphy_free(ctx->wiphy);

  // Deallocate the memory used by the dummy context itself.
  kfree(ctx);
}

/**
 * @brief Module initialization function.
 *
 * This function initializes the virtual Wi-Fi module.
 * - It creates a context structure and initializes it with default values.
 * - Initializes the synchronization semaphore.
 * - Initializes workqueue items for connection, disconnection, and scanning
 * routines.
 *
 * @return 0 if initialization is successful, -1 otherwise.
 */
static int __init virtual_wifi_init(void) {
  g_ctx = dummy_wifi_create_context();

  if (g_ctx != NULL) {
    /* Initialize the synchronization semaphore with an initial value of 1. */
    sema_init(&g_ctx->sem, 1);

    /* Initialize workqueue items for various Wi-Fi routines. */
    INIT_WORK(&g_ctx->ws_connect, dummy_wifi_connect_routine);
    g_ctx->connecting_ssid[0] = 0; // Clear the connecting SSID.

    INIT_WORK(&g_ctx->ws_disconnect, dummy_wifi_disconnect_routine);
    g_ctx->disconnect_reason_code = 0; // Clear the disconnect reason code.

    INIT_WORK(&g_ctx->ws_scan, dummy_wifi_scan_routine);
    g_ctx->scan_request = NULL; // Clear the scan request pointer.
  }

  return g_ctx == NULL; // Return 1 if g_ctx is NULL (initialization failed).
}

/**
 * @brief Module exit function.
 *
 * This function cleans up and exits the virtual Wi-Fi module.
 * - Cancels any pending workqueue items for connection, disconnection, and
 * scanning.
 * - Frees the memory associated with the context structure.
 */
static void __exit virtual_wifi_exit(void) {
  /* Make sure that no work is queued for the specified workqueue items. */
  cancel_work_sync(&g_ctx->ws_connect);
  cancel_work_sync(&g_ctx->ws_disconnect);
  cancel_work_sync(&g_ctx->ws_scan);

  /* Free the memory associated with the context structure. */
  dummy_wifi_free(g_ctx);
}

module_init(virtual_wifi_init);
module_exit(virtual_wifi_exit);
