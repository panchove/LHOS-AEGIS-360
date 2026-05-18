#include <modules/zigbee/ZigbeeLayrzHub.h>

static void uart_rx_task(void *arg);
static void net_tx_task(void *arg);
static void net_rx_task(void *arg);
static void conn_mgr_task(void *arg);
static void local_usb_to_zig_task(void *arg);
static void local_zig_to_usb_task(void *arg);
static void local_conn_mgr_task(void *arg);
static bool zigbeeNetworkReady();
static bool zigbeeLocalMode();
static bool zigbeeCellMode();
static void refreshZigbeeCellTcpStatus(bool force);
static bool zigbeeSocketConnected();
static void logZigbeeCellDiag(const char *tag);
static bool resolveZigbeeHub(bool force);
static void recycleZigbeeWifi(bool hardReset);
static void recycleLocalBridge();

static TaskHandle_t s_connMgrHandle = nullptr;
static TaskHandle_t s_uartRxHandle = nullptr;
static TaskHandle_t s_netTxHandle = nullptr;
static TaskHandle_t s_netRxHandle = nullptr;
static TaskHandle_t s_localUsbToZigHandle = nullptr;
static TaskHandle_t s_localZigToUsbHandle = nullptr;
static TaskHandle_t s_localConnMgrHandle = nullptr;
static volatile bool s_zigbeeInitDone = false;

static SemaphoreHandle_t s_localUsbWriteMutex = nullptr;
static SemaphoreHandle_t s_localZigWriteMutex = nullptr;
static volatile bool s_localRecycleInProgress = false;

static volatile uint32_t s_lastUartRxMs = 0;
static volatile uint32_t s_lastNetTxMs = 0;
static volatile uint32_t s_lastNetRxMs = 0;
static volatile uint32_t s_lastUartTxMs = 0;
static volatile uint32_t s_lastReconnectMs = 0;
static volatile uint32_t s_lastLocalUsbRxMs = 0;
static volatile uint32_t s_lastLocalUsbTxMs = 0;
static volatile uint32_t s_lastLocalZigRxMs = 0;
static volatile uint32_t s_lastLocalZigTxMs = 0;
static volatile uint32_t s_lastLocalRecycleMs = 0;
static IPAddress s_zigbeeCachedIp;
static uint32_t s_zigbeeLastResolveMs = 0;
static uint32_t s_zigbeeLastWifiRecycleMs = 0;
static uint32_t s_zigbeeLastCellStatusMs = 0;
static uint32_t s_zigbeeLastCellDiagMs = 0;
static int s_zigbeeConnectFailureCount = 0;
static int s_zigbeeWifiRecycleEscalation = 0;

static constexpr uint32_t ZB_STALL_MS = 15000;
static constexpr uint32_t ZB_IDLE_RECYCLE_MS = 180000;
static constexpr uint8_t ZB_CELL_MUX = 3;
static constexpr size_t ZB_LOCAL_USB_TO_ZIG_CHUNK = 256;
static constexpr size_t ZB_LOCAL_ZIG_TO_USB_CHUNK = 256;
static constexpr uint32_t ZB_LOCAL_USB_IDLE_MS = 2;
static constexpr uint32_t ZB_LOCAL_READ_TIMEOUT_MS = 10;
static constexpr uint32_t ZB_LOCAL_STALL_MS = 15000;
static constexpr uint32_t ZB_LOCAL_RECYCLE_GUARD_MS = 5000;
static constexpr uint32_t ZB_LOCAL_ACTIVITY_WINDOW_MS = 2000;
static constexpr uint32_t ZB_LOCAL_RECYCLE_PAUSE_MS = 25;
static constexpr uint32_t ZB_LOCAL_RESET_DELAY_MS = 100;

static uint32_t stackHwmWords(TaskHandle_t h) {
  return h ? (uint32_t)uxTaskGetStackHighWaterMark(h) : 0U;
}

static bool resolveZigbeeHub(bool force) {
  if (hubSettings.net_mode != "wifi") {
    return false;
  }
  const uint32_t now = millis();
  const uint32_t CACHE_MS = 1800UL * 1000UL; // 30 minutes
  if (!force && s_zigbeeCachedIp && (now - s_zigbeeLastResolveMs) < CACHE_MS) {
    return true;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  IPAddress ip;
  if (WiFi.hostByName(hubSettings.zigbee_hub_url.c_str(), ip)) {
    s_zigbeeCachedIp = ip;
    s_zigbeeLastResolveMs = now;
    if (hubSettings.sys_debug_en) {
      debugPrint("Zigbee DNS: %s -> %s\n", hubSettings.zigbee_hub_url.c_str(),
                 ip.toString().c_str());
    }
    return true;
  }
  return s_zigbeeCachedIp;
}

static bool zigbeeLocalMode() { return hubSettings.zigbee_local_en; }

static bool zigbeeCellMode() {
  return (hubSettings.net_mode == "cellular" || hubSettings.net_mode == "gprs");
}

static void refreshZigbeeCellTcpStatus(bool force) {
#ifdef USE_ASYNC_A7670
  if (!zigbeeCellMode()) {
    return;
  }
  const uint32_t now = millis();
  if (!force && (now - s_zigbeeLastCellStatusMs) < ZB_CELL_STATUS_REFRESH_MS) {
    return;
  }
  modemAsync.checkTcpStatus();
  s_zigbeeLastCellStatusMs = now;
#else
  (void)force;
#endif
}

static void logZigbeeCellDiag(const char *tag) {
#ifdef USE_ASYNC_A7670
  if (!hubSettings.sys_debug_en || !zigbeeCellMode()) {
    return;
  }
  const uint32_t now = millis();
  if ((now - s_zigbeeLastCellDiagMs) < 2000) {
    return;
  }
  s_zigbeeLastCellDiagMs = now;
  const size_t freeInt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const size_t maxInt = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  const size_t freePs = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const size_t maxPs = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  debugPrint("[ZB-CELL] %s gprs=%d tcp3=%d clientConn=%d heapInt=%u maxInt=%u "
             "heapPs=%u maxPs=%u\n",
             tag, (int)modemAsync.isGprsConnected(),
             (int)modemAsync.tcpStatus[ZB_CELL_MUX],
             (int)(zigbeeTcpClient && zigbeeTcpClient->connected()),
             (unsigned)freeInt, (unsigned)maxInt, (unsigned)freePs,
             (unsigned)maxPs);
#else
  (void)tag;
#endif
}

static bool zigbeeSocketConnected() {
  if (!zigbeeTcpClient) {
    return false;
  }
  bool baseConn = zigbeeTcpClient->connected();
  if (!zigbeeCellMode()) {
    return baseConn;
  }
#ifdef USE_ASYNC_A7670
  const bool gprsUp = modemAsync.isGprsConnected();
  if (!(baseConn && gprsUp)) {
    logZigbeeCellDiag("socket-not-usable");
    return false;
  }
#endif
  return baseConn;
}

static void recycleZigbeeWifi(bool hardReset) {
  if (hardReset) {
    debugPrint("Zigbee bridge recycling WiFi (hard)\n");
    WiFi.disconnect(true, false);
    vTaskDelay(pdMS_TO_TICKS(500));
    WiFi.mode(WIFI_STA);
    vTaskDelay(pdMS_TO_TICKS(200));
  } else {
    debugPrint("Zigbee bridge recycling WiFi (soft)\n");
    WiFi.disconnect(false, false);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  WiFi.begin(hubSettings.wifi_ssid.c_str(), hubSettings.wifi_pass.c_str());
}

static void recycleLocalBridge() {
  s_localRecycleInProgress = true;
  vTaskDelay(pdMS_TO_TICKS(ZB_LOCAL_RECYCLE_PAUSE_MS));

  if (s_localUsbWriteMutex)
    xSemaphoreTake(s_localUsbWriteMutex, portMAX_DELAY);
  if (s_localZigWriteMutex)
    xSemaphoreTake(s_localZigWriteMutex, portMAX_DELAY);

  ZigSerial.flush();
  ZigSerial.end();
  vTaskDelay(pdMS_TO_TICKS(ZB_LOCAL_RESET_DELAY_MS));
  ZigSerial.begin(115200, SERIAL_8N1, ZIGBEE_RX, ZIGBEE_TX);
  ZigSerial.setTimeout(ZB_LOCAL_READ_TIMEOUT_MS);

  while (ZigSerial.available() > 0) {
    ZigSerial.read();
  }

  const uint32_t now = millis();
  s_lastLocalRecycleMs = now;
  s_lastLocalUsbRxMs = now;
  s_lastLocalUsbTxMs = now;
  s_lastLocalZigRxMs = now;
  s_lastLocalZigTxMs = now;

  if (s_localZigWriteMutex)
    xSemaphoreGive(s_localZigWriteMutex);
  if (s_localUsbWriteMutex)
    xSemaphoreGive(s_localUsbWriteMutex);

  s_localRecycleInProgress = false;
}

static bool zigbeeNetworkReady() {
  if (hubSettings.net_mode == "wifi") {
    return WiFi.status() == WL_CONNECTED;
  }
  if (hubSettings.net_mode == "cellular" || hubSettings.net_mode == "gprs") {
#ifdef USE_ASYNC_A7670
    return modemAsync.isGprsConnected();
#else
    return modem.isGprsConnected();
#endif
  }
  return false;
}

void initZigbee() {
  if (s_zigbeeInitDone) {
    if (hubSettings.sys_debug_en)
      debugPrint("Zigbee bridge already initialized\n");
    return;
  }

  if (zigbeeLocalMode()) {
    hubSettings.sys_debug_en = false;
    UARTSerial.setDebugOutput(false);

    ZigSerial.begin(115200, SERIAL_8N1, ZIGBEE_RX, ZIGBEE_TX);
    ZigSerial.setTimeout(ZB_LOCAL_READ_TIMEOUT_MS);

    if (!s_localUsbWriteMutex)
      s_localUsbWriteMutex = xSemaphoreCreateMutex();
    if (!s_localZigWriteMutex)
      s_localZigWriteMutex = xSemaphoreCreateMutex();
    if (!s_localUsbWriteMutex || !s_localZigWriteMutex) {
      return;
    }

    const uint32_t now = millis();
    s_lastLocalUsbRxMs = now;
    s_lastLocalUsbTxMs = now;
    s_lastLocalZigRxMs = now;
    s_lastLocalZigTxMs = now;
    s_lastLocalRecycleMs = now;

    xTaskCreatePinnedToCore(local_usb_to_zig_task, "zb_l_u2z", 4096, nullptr, 8,
                            &s_localUsbToZigHandle, 0);
    xTaskCreatePinnedToCore(local_zig_to_usb_task, "zb_l_z2u", 4096, nullptr, 9,
                            &s_localZigToUsbHandle, 1);
    xTaskCreatePinnedToCore(local_conn_mgr_task, "zb_l_mgr", 3072, nullptr, 5,
                            &s_localConnMgrHandle, 1);
    s_zigbeeInitDone = true;
    return;
  }

  if (zigbeeTcpClient == nullptr) {

    if (hubSettings.net_mode == "wifi") {
      zigbeeTcpClient = new WiFiClient();
    } else if (hubSettings.net_mode == "cellular" ||
               hubSettings.net_mode == "gprs") {
#ifdef USE_ASYNC_A7670
      GSMZigbeeClient.setTimeout(ZB_CELL_CONNECT_TIMEOUT_MS);
      zigbeeTcpClient = &GSMZigbeeClient; // Reuse pre-initialized mux 3 client
#else
      debugPrint("Cellular Zigbee requires AsyncA7670 build\n");
      return;
#endif
    } else {
      debugPrint("Invalid network mode for Zigbee hub: %s\n",
                 hubSettings.net_mode.c_str());
      return;
    }
  }
  if (!zigbeeTcpClient) {
    debugPrint("Failed to allocate zigbeeTcpClient\n");
    return;
  }

  ZigSerial.begin(115200, SERIAL_8N1, ZIGBEE_RX, ZIGBEE_TX);
  ZigSerial.setTimeout(5);

  // PSRAM ring allocation
  size_t cap = UPLINK_RING_BYTES;

  uplink_mem =
    (uint8_t *)heap_caps_malloc(cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!uplink_mem) {
    debugPrint("PSRAM alloc failed, falling back to internal heap\n");
    uplink_mem = (uint8_t *)heap_caps_malloc(cap, MALLOC_CAP_8BIT);
  }
  if (!uplink_mem) {
    debugPrint("FATAL: ring buffer alloc failed\n");
    while (true)
      delay(1000);
  }
  uplink_ring = new ByteRing(uplink_mem, cap);
  if (!uplink_ring) {
    debugPrint("FATAL: ring object alloc failed\n");
    return;
  }

  ring_mutex = xSemaphoreCreateMutex();
  net_mutex = xSemaphoreCreateMutex();
  if (!ring_mutex || !net_mutex) {
    debugPrint("FATAL: zigbee mutex alloc failed\n");
    return;
  }

  uint32_t now = millis();
  s_lastUartRxMs = now;
  s_lastNetTxMs = now;
  s_lastNetRxMs = now;
  s_lastUartTxMs = now;
  s_lastReconnectMs = now;

  // Tasks: prioriza net_rx y uart_rx para no perder bytes
  xTaskCreatePinnedToCore(conn_mgr_task, "conn_mgr", 4096, nullptr, 5,
                          &s_connMgrHandle, 1);
  xTaskCreatePinnedToCore(uart_rx_task, "uart_rx", 4096, nullptr, 8,
                          &s_uartRxHandle, 1);
  xTaskCreatePinnedToCore(net_tx_task, "net_tx", 4096, nullptr, 7,
                          &s_netTxHandle, 0);
  xTaskCreatePinnedToCore(net_rx_task, "net_rx", 4096, nullptr, 9,
                          &s_netRxHandle, 0);

  s_zigbeeInitDone = true;
}

static bool read_line_timeout(Client &c, String &out, uint32_t timeout_ms) {
  out = "";
  uint32_t start = millis();
  while ((millis() - start) < timeout_ms) {
    while (c.available()) {
      char ch = (char)c.read();
      if (ch == '\n')
        return true;
      if (ch != '\r')
        out += ch;
    }
    if (!c.connected())
      return false;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  return false;
}

static bool connect_and_handshake() {
  if (!zigbeeNetworkReady()) {
    return false;
  }
  if (!zigbeeTcpClient) {
    return false;
  }

  debugPrint("Connecting to hub %s:%d\n", hubSettings.zigbee_hub_url.c_str(),
             ZIGBEE_HUB_PORT);
  xSemaphoreTake(net_mutex, portMAX_DELAY);
  if (zigbeeTcpClient->connected())
    zigbeeTcpClient->stop();
  bool connected = false;
  if (hubSettings.net_mode == "wifi") {
    if (resolveZigbeeHub(false) && s_zigbeeCachedIp) {
      connected = zigbeeTcpClient->connect(s_zigbeeCachedIp, ZIGBEE_HUB_PORT);
      if (!connected && hubSettings.sys_debug_en)
        debugPrint("Zigbee connect via cached IP failed\n");
    }
    if (!connected) {
      connected = zigbeeTcpClient->connect(hubSettings.zigbee_hub_url.c_str(),
                                           ZIGBEE_HUB_PORT);
    }
    if (!connected && resolveZigbeeHub(true) && s_zigbeeCachedIp) {
      connected = zigbeeTcpClient->connect(s_zigbeeCachedIp, ZIGBEE_HUB_PORT);
    }
  } else {
#ifdef USE_ASYNC_A7670
    GSMZigbeeClient.setTimeout(ZB_CELL_CONNECT_TIMEOUT_MS);
#endif
    connected = zigbeeTcpClient->connect(hubSettings.zigbee_hub_url.c_str(),
                                         ZIGBEE_HUB_PORT);
  }
  xSemaphoreGive(net_mutex);
  if (!connected) {
    if (zigbeeCellMode()) {
      logZigbeeCellDiag("connect-failed");
    }
    if (hubSettings.net_mode == "wifi") {
      s_zigbeeConnectFailureCount++;
      if (hubSettings.sys_debug_en) {
        debugPrint("Zigbee TCP connect failed (dns/connect streak=%d)\n",
                   s_zigbeeConnectFailureCount);
      }
      uint32_t now = millis();
      if (s_zigbeeConnectFailureCount >= 3 &&
          (now - s_zigbeeLastWifiRecycleMs) > 15000) {
        bool hardReset = (s_zigbeeWifiRecycleEscalation > 0);
        recycleZigbeeWifi(hardReset);
        s_zigbeeWifiRecycleEscalation =
          min(2, s_zigbeeWifiRecycleEscalation + 1);
        s_zigbeeLastWifiRecycleMs = now;
        s_zigbeeConnectFailureCount = 0;
      }
    }
    debugPrint("TCP connect failed\n");
    return false;
  }
  s_zigbeeConnectFailureCount = 0;
  s_zigbeeWifiRecycleEscalation = 0;
  if (hubSettings.net_mode == "wifi") {
    zigbeeTcpClient->setTimeout(1000);
  } else {
#ifdef USE_ASYNC_A7670
    GSMZigbeeClient.setTimeout(ZB_CELL_CONNECT_TIMEOUT_MS);
#endif
  }

  std::string hello = std::string("site=") + hubSettings.zigbee_site_id +
                      " token=" + hubSettings.zigbee_token + "\n";
  xSemaphoreTake(net_mutex, portMAX_DELAY);
  zigbeeTcpClient->write((const uint8_t *)hello.c_str(), hello.length());
  zigbeeTcpClient->flush();
  xSemaphoreGive(net_mutex);
  debugPrint("Sent hello\n");

  String line;
  if (!read_line_timeout(*zigbeeTcpClient, line, HELLO_TIMEOUT_MS)) {
    debugPrint("Handshake timeout/no response\n");
    xSemaphoreTake(net_mutex, portMAX_DELAY);
    zigbeeTcpClient->stop();
    xSemaphoreGive(net_mutex);
    return false;
  }
  debugPrint("Hub response: %s\n", line.c_str());
  if (line != "OK") {
    debugPrint("Hub rejected connection\n");
    xSemaphoreTake(net_mutex, portMAX_DELAY);
    zigbeeTcpClient->stop();
    xSemaphoreGive(net_mutex);
    return false;
  }
  uint32_t now = millis();
  s_lastReconnectMs = now;
  s_lastNetTxMs = now;
  s_lastNetRxMs = now;
  s_lastUartRxMs = now;
  s_lastUartTxMs = now;
  return true;
}

static void disconnect_bridge() {
  bridge_up = false;
  if (!zigbeeTcpClient)
    return;
  xSemaphoreTake(net_mutex, portMAX_DELAY);
  if (zigbeeTcpClient->connected())
    zigbeeTcpClient->stop();
  while (zigbeeTcpClient->available() > 0) {
    zigbeeTcpClient->read();
  }
  xSemaphoreGive(net_mutex);
#ifdef USE_ASYNC_A7670
  if (zigbeeCellMode()) {
    modemAsync.tcpStatus[ZB_CELL_MUX] = TCPStatus::CLOSED;
    modemAsync.tcpConnections[ZB_CELL_MUX].connected = false;
  }
#endif
}

// ------------ Tasks ------------

// UART0 (USB/Serial) -> Zigbee UART
static void local_usb_to_zig_task(void *arg) {
  uint8_t *buf =
    (uint8_t *)heap_caps_malloc(ZB_LOCAL_USB_TO_ZIG_CHUNK, MALLOC_CAP_8BIT);
  if (!buf) {
    vTaskDelete(nullptr);
    return;
  }

  for (;;) {
    if (s_localRecycleInProgress) {
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }

    int avail = UARTSerial.available();
    if (avail <= 0) {
      vTaskDelay(pdMS_TO_TICKS(ZB_LOCAL_USB_IDLE_MS));
      continue;
    }

    int toRead = (avail > (int)ZB_LOCAL_USB_TO_ZIG_CHUNK)
                   ? (int)ZB_LOCAL_USB_TO_ZIG_CHUNK
                   : avail;
    int n = UARTSerial.readBytes(buf, toRead);
    if (n <= 0) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    s_lastLocalUsbRxMs = millis();
    if (s_localZigWriteMutex)
      xSemaphoreTake(s_localZigWriteMutex, portMAX_DELAY);
    size_t w = ZigSerial.write(buf, (size_t)n);
    if (s_localZigWriteMutex)
      xSemaphoreGive(s_localZigWriteMutex);
    if (w > 0) {
      s_lastLocalZigTxMs = millis();
    }
  }
}

// Zigbee UART -> UART0 (USB/Serial)
static void local_zig_to_usb_task(void *arg) {
  uint8_t *buf =
    (uint8_t *)heap_caps_malloc(ZB_LOCAL_ZIG_TO_USB_CHUNK, MALLOC_CAP_8BIT);
  if (!buf) {
    vTaskDelete(nullptr);
    return;
  }

  for (;;) {
    if (s_localRecycleInProgress) {
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }

    int n = ZigSerial.readBytes(buf, ZB_LOCAL_ZIG_TO_USB_CHUNK);
    if (n <= 0) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    s_lastLocalZigRxMs = millis();
    if (s_localUsbWriteMutex)
      xSemaphoreTake(s_localUsbWriteMutex, portMAX_DELAY);
    size_t w = UARTSerial.write(buf, (size_t)n);
    if (s_localUsbWriteMutex)
      xSemaphoreGive(s_localUsbWriteMutex);
    if (w > 0) {
      s_lastLocalUsbTxMs = millis();
    }
  }
}

static void local_conn_mgr_task(void *arg) {
  for (;;) {
    if (s_localRecycleInProgress) {
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    const uint32_t now = millis();
    const bool hostTalking =
      (now - s_lastLocalUsbRxMs) < ZB_LOCAL_ACTIVITY_WINDOW_MS;
    const bool zigTalking =
      (now - s_lastLocalZigRxMs) < ZB_LOCAL_ACTIVITY_WINDOW_MS;
    const bool recycleAllowed =
      (now - s_lastLocalRecycleMs) > ZB_LOCAL_RECYCLE_GUARD_MS;

    const bool hostToZigStalled =
      hostTalking && (now - s_lastLocalZigRxMs) > ZB_LOCAL_STALL_MS &&
      (now - s_lastLocalZigTxMs) < ZB_LOCAL_ACTIVITY_WINDOW_MS;

    const bool zigToHostStalled =
      zigTalking && (now - s_lastLocalUsbRxMs) > ZB_LOCAL_STALL_MS &&
      (now - s_lastLocalUsbTxMs) < ZB_LOCAL_ACTIVITY_WINDOW_MS;

    if (recycleAllowed && (hostToZigStalled || zigToHostStalled)) {
      recycleLocalBridge();
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// UART -> ring
// static void uart_rx_task(void* arg) {
//   uint8_t* buf = (uint8_t*)heap_caps_malloc(UART_READ_CHUNK,
//   MALLOC_CAP_8BIT); if (!buf) {
//     debugPrint("uart_rx_task: malloc failed\n");
//     vTaskDelete(nullptr);
//   }

//   for (;;) {
//     int n = ZigSerial.readBytes(buf, UART_READ_CHUNK); // blocking per
//     timeout set below if (n > 0) {
//       uplink_rx_uart_bytes += (uint64_t)n;
//       s_lastUartRxMs = millis();

//       xSemaphoreTake(ring_mutex, portMAX_DELAY);
//       uplink_ring->push_drop_oldest(buf, (size_t)n);
//       xSemaphoreGive(ring_mutex);
//     } else {
//       // yield
//       vTaskDelay(pdMS_TO_TICKS(1));
//     }
//   }
// }
static void uart_rx_task(void *arg) {
  uint8_t *buf = (uint8_t *)heap_caps_malloc(UART_READ_CHUNK, MALLOC_CAP_8BIT);
  if (!buf) {
    debugPrint("uart_rx_task: malloc failed\n");
    vTaskDelete(nullptr);
  }

  for (;;) {
    int avail = ZigSerial.available();
    if (avail <= 0) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    int toRead = (avail > (int)UART_READ_CHUNK) ? (int)UART_READ_CHUNK : avail;
    int n = ZigSerial.readBytes(buf, toRead);
    if (n <= 0) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    uplink_rx_uart_bytes += (uint64_t)n;
    s_lastUartRxMs = millis();

    xSemaphoreTake(ring_mutex, portMAX_DELAY);
    uplink_ring->push_drop_oldest(buf, (size_t)n);
    xSemaphoreGive(ring_mutex);
  }
}

// ring -> TCP
// static void net_tx_task(void* arg) {
//   uint8_t* buf = (uint8_t*)heap_caps_malloc(NET_READ_CHUNK, MALLOC_CAP_8BIT);
//   if (!buf) {
//     debugPrint("net_tx_task: malloc failed\n");
//     vTaskDelete(nullptr);
//   }

//   for (;;) {
//     if (!bridge_up) {
//       vTaskDelay(pdMS_TO_TICKS(50));
//       continue;
//     }

//     size_t n = 0;
//     xSemaphoreTake(ring_mutex, portMAX_DELAY);
//     n = uplink_ring->pop(buf, NET_READ_CHUNK);
//     xSemaphoreGive(ring_mutex);

//     if (n == 0) {
//       vTaskDelay(pdMS_TO_TICKS(2));
//       continue;
//     }

//     if (!zigbeeSocketConnected()) {
//       debugPrint("net_tx_task: socket not connected\n");
//       disconnect_bridge();
//       continue;
//     }

//     xSemaphoreTake(net_mutex, portMAX_DELAY);
//     size_t w = zigbeeTcpClient->write(buf, n);
//     xSemaphoreGive(net_mutex);

//     if (w != n) {
//       debugPrint("net_tx_task: short write (%u/%u)\n", (unsigned)w,
//       (unsigned)n); disconnect_bridge(); continue;
//     }
//     uplink_tx_net_bytes += (uint64_t)n;
//     s_lastNetTxMs = millis();
//   }
// }

static void net_tx_task(void *arg) {
  uint8_t *buf = (uint8_t *)heap_caps_malloc(NET_READ_CHUNK, MALLOC_CAP_8BIT);
  if (!buf) {
    debugPrint("net_tx_task: malloc failed\n");
    vTaskDelete(nullptr);
  }

  for (;;) {
    // Toma net_mutex primero para evitar race con disconnect_bridge()
    xSemaphoreTake(net_mutex, portMAX_DELAY);

    if (!bridge_up || !zigbeeSocketConnected()) {
      xSemaphoreGive(net_mutex);
      if (bridge_up) {
        // Socket reportó caído mientras bridge_up=true — dispara disconnect
        debugPrint("net_tx_task: socket not connected\n");
        disconnect_bridge();
      }
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    // Lee del ring mientras tenemos net_mutex (el write viene inmediato)
    size_t n = 0;
    xSemaphoreTake(ring_mutex, portMAX_DELAY);
    n = uplink_ring->pop(buf, NET_READ_CHUNK);
    xSemaphoreGive(ring_mutex);

    if (n == 0) {
      xSemaphoreGive(net_mutex);
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }

    size_t w = zigbeeTcpClient->write(buf, n);
    xSemaphoreGive(net_mutex);

    if (w != n) {
      debugPrint("net_tx_task: short write (%u/%u)\n", (unsigned)w,
                 (unsigned)n);
      disconnect_bridge();
      continue;
    }

    uplink_tx_net_bytes += (uint64_t)n;
    s_lastNetTxMs = millis();
  }
}

// TCP -> UART
// static void net_rx_task(void* arg) {
//   uint8_t* buf = (uint8_t*)heap_caps_malloc(NET_READ_CHUNK, MALLOC_CAP_8BIT);
//   if (!buf) {
//     debugPrint("net_rx_task: malloc failed\n");
//     vTaskDelete(nullptr);
//   }

//   for (;;) {
//     if (!bridge_up) {
//       vTaskDelay(pdMS_TO_TICKS(50));
//       continue;
//     }
//     if (!zigbeeSocketConnected()) {
//       debugPrint("net_rx_task: socket closed\n");
//       disconnect_bridge();
//       continue;
//     }

//     int avail = zigbeeTcpClient->available();
//     if (avail <= 0) {
//       vTaskDelay(pdMS_TO_TICKS(2));
//       continue;
//     }

//     int toRead = (avail > (int)NET_READ_CHUNK) ? (int)NET_READ_CHUNK : avail;
//     int n = zigbeeTcpClient->read(buf, toRead);
//     if (n <= 0) {
//       debugPrint("net_rx_task: read error/closed\n");
//       disconnect_bridge();
//       continue;
//     }

//     downlink_rx_net_bytes += (uint64_t)n;
//     s_lastNetRxMs = millis();

//     // Write to UART (may block if TX buffer full)
//     size_t w = ZigSerial.write(buf, (size_t)n);
//     // Avoid forcing flush each chunk; it can stall the bridge under burst
//     traffic. downlink_tx_uart_bytes += (uint64_t)w; if (w > 0) s_lastUartTxMs
//     = millis();

//     if (w != (size_t)n) {
//       debugPrint("net_rx_task: UART short write (%u/%u)\n", (unsigned)w,
//       (unsigned)n);
//       // Si esto pasa, hay presión fuerte; sin RTS/CTS es mala señal.
//     }
//   }
// }

static void net_rx_task(void *arg) {
  uint8_t *buf = (uint8_t *)heap_caps_malloc(NET_READ_CHUNK, MALLOC_CAP_8BIT);
  if (!buf) {
    debugPrint("net_rx_task: malloc failed\n");
    vTaskDelete(nullptr);
  }

  for (;;) {
    if (!bridge_up) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }
    if (!zigbeeSocketConnected()) {
      debugPrint("net_rx_task: socket closed\n");
      disconnect_bridge();
      continue;
    }

    int avail = zigbeeTcpClient->available();
    if (avail <= 0) {
      vTaskDelay(pdMS_TO_TICKS(1)); // era 2ms
      continue;
    }

    int toRead = (avail > (int)NET_READ_CHUNK) ? (int)NET_READ_CHUNK : avail;
    int n = zigbeeTcpClient->read(buf, toRead);
    if (n <= 0) {
      debugPrint("net_rx_task: read error/closed\n");
      disconnect_bridge();
      continue;
    }

    downlink_rx_net_bytes += (uint64_t)n;
    s_lastNetRxMs = millis();

    size_t w = ZigSerial.write(buf, (size_t)n);
    ZigSerial.flush(); // ← fuerza TX inmediato hacia el cc2652p

    downlink_tx_uart_bytes += (uint64_t)w;
    if (w > 0)
      s_lastUartTxMs = millis();

    if (w != (size_t)n) {
      debugPrint("net_rx_task: UART short write (%u/%u)\n", (unsigned)w,
                 (unsigned)n);
    }
  }
}

// Connection manager / reconnection loop
static void conn_mgr_task(void *arg) {
  uint32_t backoff = RECONNECT_MIN_MS;

  for (;;) {
    if (!zigbeeNetworkReady()) {
      if (bridge_up)
        debugPrint("Zigbee bridge down: network unavailable\n");
      disconnect_bridge();
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    if (!bridge_up || !zigbeeSocketConnected()) {
      bridge_up = false;
      if (connect_and_handshake()) {
        bridge_up = true;
        backoff = RECONNECT_MIN_MS;
        debugPrint("Bridge active\n");
      } else {
        debugPrint("Reconnect in %u ms\n", (unsigned)backoff);
        vTaskDelay(pdMS_TO_TICKS(backoff));
        backoff = (uint32_t)min((uint64_t)RECONNECT_MAX_MS,
                                (uint64_t)(backoff * 17) / 10);
        continue;
      }
    }

    if (bridge_up && zigbeeCellMode()) {
      // Keep diagnostics without forcing a disconnect on modem status samples
      // that can be stale/transient while using multiple sockets.
      refreshZigbeeCellTcpStatus(false);
    }

    // Self-healing: if UART keeps producing data but it is not forwarded to
    // network, recycle connection.
    uint32_t now = millis();
    size_t ring_sz = 0;
    xSemaphoreTake(ring_mutex, portMAX_DELAY);
    ring_sz = uplink_ring->size();
    xSemaphoreGive(ring_mutex);
    if (bridge_up && ring_sz > 0 && (now - s_lastUartRxMs) < ZB_STALL_MS &&
        (now - s_lastNetTxMs) > ZB_STALL_MS) {
      debugPrint("Zigbee stall detected (ring=%u, lastNetTx=%ums). Recycling "
                 "TCP bridge\n",
                 (unsigned)ring_sz, (unsigned)(now - s_lastNetTxMs));
      disconnect_bridge();
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // Optional long-idle recycle to recover silent half-open sessions.
    uint32_t lastActivity = s_lastNetTxMs;
    if ((now - s_lastNetRxMs) < (now - lastActivity))
      lastActivity = s_lastNetRxMs;
    if ((now - s_lastUartRxMs) < (now - lastActivity))
      lastActivity = s_lastUartRxMs;
    if ((now - s_lastUartTxMs) < (now - lastActivity))
      lastActivity = s_lastUartTxMs;
    if (bridge_up && (now - lastActivity) > ZB_IDLE_RECYCLE_MS) {
      debugPrint("Zigbee long-idle recycle (%ums without activity)\n",
                 (unsigned)(now - lastActivity));
      disconnect_bridge();
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // periodic stats
    static uint32_t last = 0;
    if (now - last > 10000) {
      last = now;

      size_t ring_sz = 0, ring_drop = 0;
      xSemaphoreTake(ring_mutex, portMAX_DELAY);
      ring_sz = uplink_ring->size();
      ring_drop = uplink_ring->drops();
      xSemaphoreGive(ring_mutex);
      size_t freeInt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
      size_t freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
      size_t maxInt = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
      size_t maxPsram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
      size_t minInt = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
      size_t minPsram = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
      uint32_t hwmConn = stackHwmWords(s_connMgrHandle);
      uint32_t hwmUartRx = stackHwmWords(s_uartRxHandle);
      uint32_t hwmNetTx = stackHwmWords(s_netTxHandle);
      uint32_t hwmNetRx = stackHwmWords(s_netRxHandle);
      const int sockConn =
        zigbeeTcpClient ? (int)zigbeeTcpClient->connected() : 0;
      const int netReady = zigbeeNetworkReady() ? 1 : 0;

      debugPrint("stats: bridge=%d sock=%d net=%d uart_rx=%lluB net_tx=%lluB "
                 "net_rx=%lluB "
                 "uart_tx=%lluB ring=%uB drops=%uB heapInt=%u maxInt=%u "
                 "minInt=%u heapPs=%u "
                 "maxPs=%u minPs=%u ageUartRx=%ums ageNetTx=%ums ageNetRx=%ums "
                 "ageUartTx=%ums "
                 "stk(conn/uRx/nTx/nRx)=%u/%u/%u/%u\n",
                 (int)bridge_up, sockConn, netReady,
                 (unsigned long long)uplink_rx_uart_bytes,
                 (unsigned long long)uplink_tx_net_bytes,
                 (unsigned long long)downlink_rx_net_bytes,
                 (unsigned long long)downlink_tx_uart_bytes, (unsigned)ring_sz,
                 (unsigned)ring_drop, (unsigned)freeInt, (unsigned)maxInt,
                 (unsigned)minInt, (unsigned)freePsram, (unsigned)maxPsram,
                 (unsigned)minPsram, (unsigned)(now - s_lastUartRxMs),
                 (unsigned)(now - s_lastNetTxMs),
                 (unsigned)(now - s_lastNetRxMs),
                 (unsigned)(now - s_lastUartTxMs), (unsigned)hwmConn,
                 (unsigned)hwmUartRx, (unsigned)hwmNetTx, (unsigned)hwmNetRx);
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
