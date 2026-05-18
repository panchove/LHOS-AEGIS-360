#include <modules/network/NetLayrzHub.h>

// Static members
IPAddress NetLayrzHub::s_cachedIP;
uint32_t NetLayrzHub::s_lastResolveMs = 0;
static uint32_t s_connectivityLostSinceMs = 0;

static bool connectivityEstablished() {
  bool transportUp = false;
  if (hubSettings.net_mode == "wifi") {
    transportUp = (WiFi.status() == WL_CONNECTED);
  } else {
#ifdef USE_ASYNC_A7670
    transportUp = modemAsync.isGprsConnected();
#else
    transportUp = modem.isGprsConnected();
#endif
  }

  if (!transportUp)
    return false;

  if (hubSettings.net_protocol == 0) {
    return tcpNonSslClient && tcpNonSslClient->connected() && isSocketAuth;
  }

  return isSocketAuth;
}

static void serviceConnectivityRebootTimer(const char *sourceTag) {
#if NET_CONNECTIVITY_REBOOT_TIMEOUT_MS > 0
  const uint32_t now = millis();
  const bool connected = connectivityEstablished();

  if (connected) {
    if (s_connectivityLostSinceMs != 0 && hubSettings.sys_debug_en) {
      debugPrint("Connectivity restored after %u ms (%s)\n",
                 (unsigned)(now - s_connectivityLostSinceMs), sourceTag);
    }
    s_connectivityLostSinceMs = 0;
    return;
  }

  if (s_connectivityLostSinceMs == 0) {
    s_connectivityLostSinceMs = now;
    if (hubSettings.sys_debug_en) {
      debugPrint("Connectivity lost (%s). Reboot timeout armed for %u ms\n",
                 sourceTag, (unsigned)NET_CONNECTIVITY_REBOOT_TIMEOUT_MS);
    }
    return;
  }

  if ((now - s_connectivityLostSinceMs) >= NET_CONNECTIVITY_REBOOT_TIMEOUT_MS) {
    debugPrint("Connectivity timeout exceeded (%s, %u ms). Rebooting\n",
               sourceTag, (unsigned)(now - s_connectivityLostSinceMs));
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP.restart();
  }
#endif
}

// Consider either WiFi (got IP) or active GPRS session as having network
static bool haveIp() {
  bool wifiOk = (WiFi.status() == WL_CONNECTED && WiFi.localIP()[0] != 0);
  bool gprsOk = false;
  // modem object is declared in global objects; guard in case of init order
#ifdef USE_ASYNC_A7670
  gprsOk = modemAsync.isGprsConnected();
#else
#ifdef TINY_GSM_MODEM_HAS_GPRS
  gprsOk = modem.isGprsConnected();
#else
  // If library doesn't define the macro just try the call (most TinyGSM
  // variants support it)
  gprsOk = modem.isGprsConnected();
#endif
#endif
  return wifiOk || gprsOk;
}

static void recycleWifiInterface(bool hardReset) {
  if (hardReset) {
    debugPrint("Recycling WiFi interface (hard reset)\n");
    WiFi.disconnect(true, false);
    vTaskDelay(pdMS_TO_TICKS(500));
    WiFi.mode(WIFI_STA);
    vTaskDelay(pdMS_TO_TICKS(200));
  } else {
    debugPrint("Recycling WiFi interface (soft reset)\n");
    WiFi.disconnect(false, false);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  WiFi.begin(hubSettings.wifi_ssid.c_str(), hubSettings.wifi_pass.c_str());
}

static uint32_t stackHwmWords(TaskHandle_t h) {
  return h ? (uint32_t)uxTaskGetStackHighWaterMark(h) : 0U;
}

static void logNetworkResourceDiag(const char *tag) {
  if (!hubSettings.sys_debug_en)
    return;

  const size_t freeInt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const size_t maxInt = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  const size_t minInt = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
  const size_t freePs = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const size_t maxPs = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  const size_t minPs = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
  const uint8_t fragInt = memoryFragmentationPercent(freeInt, maxInt);
  const uint32_t hwmWifi = stackHwmWords(checkWifiNetworkHandle);
  const uint32_t hwmGsm = stackHwmWords(checkGSMNetworkHandle);
  const uint32_t hwmTcpRx = stackHwmWords(tcpSocketReceptionHandle);
  const bool tcpConn = tcpNonSslClient && tcpNonSslClient->connected();

  debugPrint(
    "[NET-RES] %s heapInt=%u maxInt=%u minInt=%u fragInt=%u%% heapPs=%u "
    "maxPs=%u "
    "minPs=%u fragPs=%u%% stk(wifi/gsm/tcpRx)=%u/%u/%u wifiSt=%d tcpConn=%d\n",
    tag, (unsigned)freeInt, (unsigned)maxInt, (unsigned)minInt,
    (unsigned)memoryFragmentationPercent(freeInt, maxInt), (unsigned)freePs,
    (unsigned)maxPs, (unsigned)minPs,
    (unsigned)memoryFragmentationPercent(freePs, maxPs), (unsigned)hwmWifi,
    (unsigned)hwmGsm, (unsigned)hwmTcpRx, (int)WiFi.status(), (int)tcpConn);
}

static void checkMemoryCircuitBreaker(const char *sourceTag) {
#if MEM_CB_ENABLE
  static uint32_t lowMemSamples = 0;
  static uint32_t firstLowMemMs = 0;

  const uint32_t now = millis();
  if (now < MEM_CB_BOOT_GRACE_MS)
    return;

  const size_t freeInt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const size_t maxInt = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  const size_t minInt = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
  const size_t freePs = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const size_t maxPs = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  const size_t minPs = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
  const uint8_t fragInt = memoryFragmentationPercent(freeInt, maxInt);

  const bool lowInt = freeInt <= MEM_CB_HEAP_INT_LOW_BYTES;
  const bool lowMaxInt = maxInt <= MEM_CB_MAX_INT_LOW_BYTES;
  const bool critInt = freeInt <= MEM_CB_HEAP_INT_CRIT_BYTES;
  const bool critMaxInt = maxInt <= MEM_CB_MAX_INT_CRIT_BYTES;
  const bool highFragInt = fragInt >= MEM_CB_FRAG_INT_LOW_PCT;
  const bool lowPs = freePs <= MEM_CB_HEAP_PS_LOW_BYTES;
  const bool critical =
    critInt || critMaxInt || (lowInt && (lowMaxInt || highFragInt)) || lowPs;

  if (!critical) {
    lowMemSamples = 0;
    firstLowMemMs = 0;
    return;
  }

  if (lowMemSamples == 0) {
    firstLowMemMs = now;
  }
  lowMemSamples++;

  if (hubSettings.sys_debug_en) {
    debugPrint("[MEM-CB] src=%s sample=%u/%u heapInt=%u maxInt=%u minInt=%u "
               "fragInt=%u%% heapPs=%u "
               "maxPs=%u minPs=%u fragPs=%u%% lowInt=%d lowMaxInt=%d "
               "highFragInt=%d lowPs=%d\n",
               sourceTag, (unsigned)lowMemSamples,
               (unsigned)MEM_CB_CONSECUTIVE_SAMPLES, (unsigned)freeInt,
               (unsigned)maxInt, (unsigned)minInt, (unsigned)fragInt,
               (unsigned)freePs, (unsigned)maxPs, (unsigned)minPs,
               (unsigned)memoryFragmentationPercent(freePs, maxPs), (int)lowInt,
               (int)lowMaxInt, (int)highFragInt, (int)lowPs);
  }

  if (lowMemSamples >= MEM_CB_CONSECUTIVE_SAMPLES &&
      (now - firstLowMemMs) >= MEM_CB_MIN_HOLD_MS) {
    debugPrint("[MEM-CB] Threshold reached: src=%s heapInt=%u maxInt=%u "
               "minInt=%u fragInt=%u%% "
               "heapPs=%u maxPs=%u minPs=%u fragPs=%u%% autoRestart=%d\n",
               sourceTag, (unsigned)freeInt, (unsigned)maxInt, (unsigned)minInt,
               (unsigned)fragInt, (unsigned)freePs, (unsigned)maxPs,
               (unsigned)minPs,
               (unsigned)memoryFragmentationPercent(freePs, maxPs),
               (int)MEM_CB_AUTO_RESTART);
#if MEM_CB_AUTO_RESTART
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP.restart();
#else
    lowMemSamples = 0;
    firstLowMemMs = 0;
#endif
  }
#else
  (void)sourceTag;
#endif
}

bool NetLayrzHub::resolveServer(bool force) {
  const uint32_t now = millis();
  const uint32_t CACHE_MS = 3600UL * 1000UL; // 1 hour
  if (!force && s_cachedIP && (now - s_lastResolveMs) < CACHE_MS)
    return true; // still fresh
  if (!haveIp())
    return false; // no network at all
  const char *host = hubSettings.net_server.c_str();
  IPAddress ip;
  const int MAX_ATTEMPTS = 3;
  // Only attempt WiFi based DNS resolution when WiFi is actually connected; for
  // GPRS we'll rely on the client.connect(host, port) path performing modem DNS
  // internally.
  if (WiFi.status() == WL_CONNECTED) {
    for (int i = 0; i < MAX_ATTEMPTS; i++) {
      if (WiFi.hostByName(host, ip)) {
        s_cachedIP = ip;
        s_lastResolveMs = now;
        if (hubSettings.sys_debug_en)
          debugPrint("Resolved %s -> %s\n", host, ip.toString().c_str());
        return true;
      }
      vTaskDelay(pdMS_TO_TICKS(250 + (i * 100)));
    }
  }
  if (hubSettings.sys_debug_en)
    debugPrint("DNS resolve failed for %s (attempts=%d)%s\n", host,
               MAX_ATTEMPTS,
               s_cachedIP ? " using cached IP fallback" : " no cache");
  return s_cachedIP; // fallback if previous cache exists
}

IPAddress NetLayrzHub::cachedServerIP() { return s_cachedIP; }

bool NetLayrzHub::checkSocketClient(void) {
  if (tcpNonSslClient == nullptr) {
    debugPrint("Socket client not initialized\n");
    return false;
  }
  if (netConnectFirstTime) {
    debugPrint("Stopping TCP socket client\n");
    tcpNonSslClient->stop();
    if (hubSettings.net_mode != "wifi") {
      modemAsync.tcpStatus[0] =
        TCPStatus::CONNECTED; // force status to connected to allow proper close
      if (waitForAsyncStatusArray(modemAsync.tcpStatus, 0, TCPStatus::CLOSED,
                                  TCPStatus::ERROR, 30000)) {
        debugPrint("Socket client stopped\n");
      } else {
        debugPrint("Socket client stop timed out\n");
      }
    }
    netConnectFirstTime = false;
  }

  // Static counters for consecutive failures - persistent across calls
  static int tcpFailureCount = 0;
  static int wifiDnsFailureCount = 0;
  static int wifiRecycleEscalation = 0;
  static uint32_t wifiLastRecycleMs = 0;

  long initTime = millis();
  bool haveTcpConnection = false;
  if (hubSettings.net_mode == "cellular") {
    // Prefer client state first to avoid frequent global CIPCLOSE? polling,
    // which can interfere with additional mux sockets (e.g., Zigbee on mux 3).
    haveTcpConnection = tcpNonSslClient->connected();
    if (!haveTcpConnection) {
      modemAsync.checkTcpStatus();
      haveTcpConnection = !waitForAsyncStatusArray(
        modemAsync.tcpStatus, 0, TCPStatus::CLOSED, TCPStatus::CONNECTED, 5000);
    }
    debugPrint("Cellular TCP socket status: %s\n",
               haveTcpConnection ? "Connected" : "Not connected");
  } else {
    haveTcpConnection = tcpNonSslClient->connected();
  }

  if (!haveTcpConnection) {
    debugPrint("⚠️ TCP Socket not connected\n");

    // If we're on cellular and have multiple consecutive failures, force GPRS
    // restart
    if (hubSettings.net_mode == "cellular" && tcpFailureCount >= 3) {
      debugPrint("⚠️ Multiple TCP failures (%d) - forcing GPRS restart\n",
                 tcpFailureCount);
      tcpFailureCount = 0; // Reset counter

      // Force GPRS disconnect and reconnect to clear corrupted IP stack
      debugPrint("Disconnecting GPRS to clear corrupted state...\n");
      modemAsync.gprsDisconnect();
      if (waitForAsyncStatus(modemAsync.gprsStatus, GPRSStatus::DISCONNECTED,
                             GPRSStatus::ERROR, 10000)) {
        debugPrint("GPRS disconnected successfully\n");
      }

      // Reset status to DISCONNECTED before checking, like in connectGprs()
      modemAsync.gprsStatus = GPRSStatus::DISCONNECTED;

      // Reconnect GPRS
      debugPrint("Reconnecting GPRS...\n");
      modemAsync.gprsConnect(hubSettings.gprs_apn.c_str(),
                             hubSettings.gprs_apn_user.c_str(),
                             hubSettings.gprs_apn_pass.c_str());
      if (!waitForAsyncStatus(modemAsync.gprsStatus, GPRSStatus::CONNECTED,
                              GPRSStatus::ERROR, 20000)) {
        debugPrint("Failed to restore GPRS after restart\n");
        return false;
      }
      debugPrint("GPRS restarted successfully, IP: %s\n",
                 String(modemAsync.getLocalIP()).c_str());
    }

    uint16_t port = hubSettings.net_server == "link.layrz.network"
                      ? STABLE_SOCKET_PORT
                      : TESTING_SOCKET_PORT;
    if (hubSettings.net_mode == "wifi") {
      haveTcpConnection = false;
      // Try cached IP first to survive transient DNS outages.
      if (resolveServer(false) && s_cachedIP) {
        haveTcpConnection = tcpNonSslClient->connect(s_cachedIP, port);
        if (hubSettings.sys_debug_en && !haveTcpConnection)
          debugPrint("Connect via cached IP failed\n");
      }
      // Fallback to hostname connect (DNS).
      if (!haveTcpConnection) {
        haveTcpConnection =
          tcpNonSslClient->connect(hubSettings.net_server.c_str(), port);
      }
      // Force re-resolve + retry IP once more.
      if (!haveTcpConnection && resolveServer(true) && s_cachedIP) {
        haveTcpConnection = tcpNonSslClient->connect(s_cachedIP, port);
      }

      if (!haveTcpConnection) {
        wifiDnsFailureCount++;
        if (hubSettings.sys_debug_en)
          debugPrint("WiFi socket connect failed (dns/connect streak=%d)\n",
                     wifiDnsFailureCount);

        const uint32_t now = millis();
        if (wifiDnsFailureCount >= 2 && (now - wifiLastRecycleMs) > 15000) {
          // Escalate to hard reset when repeated soft resets did not recover
          // DNS/socket.
          bool hardReset = (wifiRecycleEscalation > 0);
          recycleWifiInterface(hardReset);
          wifiRecycleEscalation = min(2, wifiRecycleEscalation + 1);
          wifiLastRecycleMs = now;
          wifiDnsFailureCount = 0;
        }
      } else {
        wifiDnsFailureCount = 0;
        wifiRecycleEscalation = 0;
      }
    } else {
      haveTcpConnection =
        tcpNonSslClient->connect(hubSettings.net_server.c_str(), port);
      if (hubSettings.net_mode == "cellular") {
        haveTcpConnection =
          waitForAsyncStatusArray(modemAsync.tcpStatus, 0, TCPStatus::CONNECTED,
                                  TCPStatus::ERROR, 30000);
      }
    }
    if (!haveTcpConnection) {
      // Increment failure counter for cellular mode
      if (hubSettings.net_mode == "cellular") {
        tcpFailureCount++;
        debugPrint("Failed to connect to socket server (failure #%d)\n",
                   tcpFailureCount);
      } else {
        debugPrint("Failed to connect to socket server\n");
      }
      return false;
    } else {
      // Reset failure counter on successful connection
      tcpFailureCount = 0;
      std::string ident = getBluetoothMACAddress() + ";;";
      ident += calculateCRC16(ident.c_str(), ident.length());
      std::string message = "<Pa>" + ident + "</Pa>\r\n";
      if (hubSettings.sys_debug_en)
        debugPrint("Sending identification: %s\n", message.c_str());
      tcpNonSslClient->print(message.c_str());
    }
  } else {
    // Reset failure counter when connection is already established
    tcpFailureCount = 0;
    // debugPrint("Socket client keeps alive\n");
  }
  return true;
}

void NetLayrzHub::connectWiFi(const char *ssid, const char *password,
                              const char *ntpserver1, const char *ntpserver2) {
  WiFi.enableIpV6();
  isValidTime = (time(nullptr) >= SECS_YR_2000);
  if (WiFi.status() != WL_CONNECTED) {
    if (hubSettings.rgb_en) {
      rgbLed.setBrightness(rgbBrightness);
      if (!bleConfigConnected)
        rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
      else
        rgbLed.setPixelColor(0, rgbLed.Color(255, 255, 0));
      rgbLed.show();
    }
    long startAttemptTime = millis();
    debugPrint("Connecting to WiFi network: %s\n", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttemptTime < 15000) {
      esp_task_wdt_reset();
      vTaskDelay(500 / portTICK_PERIOD_MS);
      if (Print *console = debugConsole())
        console->print(".");
    }
    if (Print *console = debugConsole())
      console->println(".");
    if (WiFi.status() != WL_CONNECTED) {
      debugPrint("Failed to connect to WiFi network: %s\n", ssid);
    } else {
      rgbLed.setBrightness(rgbBrightness);
      if (hubSettings.rgb_en) {
        if (bleConfigConnected)
          rgbLed.setPixelColor(0, rgbLed.Color(255, 255, 0));
        else if (isSocketAuth)
          rgbLed.setPixelColor(0, rgbLed.Color(0, 255, 0));
        else
          rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
        rgbLed.show();
      }
      if (hubSettings.net_protocol == 0) // TCP socket
      {
        tcpNonSslClient = &wifiClient;
        if (!checkSocketClient()) {
          isSocketAuth = false;
          if (hubSettings.rgb_en) {
            rgbLed.setBrightness(rgbBrightness);
            rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
            rgbLed.show();
          }
        }
      } else if (hubSettings.net_protocol == 1) // HTTPS
      {
        tcpSslClient = &WifiClient_SSL;
        isSocketAuth = true;
        if (hubSettings.rgb_en) {
          rgbLed.setBrightness(rgbBrightness);
          rgbLed.setPixelColor(0, rgbLed.Color(0, 255, 0));
          rgbLed.show();
        }
      }

      debugPrint("Connected to WiFi network: %s\n", ssid);
      debugPrint("IP address: %s\n", WiFi.localIP().toString().c_str());
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (hubSettings.net_protocol == 0) // TCP socket
    {
      tcpNonSslClient = &wifiClient;
      if (!checkSocketClient()) {
        debugPrint("Failed to connect to socket server\n");
        isSocketAuth = false;
        if (hubSettings.rgb_en) {
          rgbLed.setBrightness(rgbBrightness);
          rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
          rgbLed.show();
        }
      }
    } else if (hubSettings.net_protocol == 1) // HTTPS
    {
      tcpSslClient = &WifiClient_SSL;
      isSocketAuth = true;
      if (hubSettings.rgb_en) {
        rgbLed.setBrightness(rgbBrightness);
        rgbLed.setPixelColor(0, rgbLed.Color(0, 255, 0));
        rgbLed.show();
      }
    }
    // debugPrint("WiFi connected. IP address: %s\n",
    // WiFi.localIP().toString().c_str());
    // // debugPrint ("Time now: %d\n", time_now);
    int trials = 0;
    // struct timeval tv;
    // gettimeofday(&tv, NULL);

    while (time(nullptr) < SECS_YR_2000 && trials < 3) {
      debugPrint("Syncronizing time\n");
      sincroTime("wifi", ntpserver1, ntpserver2);
      esp_task_wdt_reset();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      trials++;
    }
    isValidTime = (time(nullptr) >= SECS_YR_2000);
  }
}

void NetLayrzHub::connectGprs(const char *apn, const char *user,
                              const char *pass, const char *pin,
                              const char *ntpserver1, const char *ntpserver2) {
  isValidTime = (time(nullptr) >= SECS_YR_2000);
#ifdef USE_ASYNC_A7670
  if (modemAsync.isGprsConnected()) {
    if (hubSettings.sys_debug_en)
      debugPrint("GPRS already connected. IP address: %s\n",
                 String(modemAsync.getLocalIP()).c_str());
    // Already connected, handle socket setup
    if (hubSettings.net_protocol == 0) {
      tcpNonSslClient = &GSMClient;
      if (tcpNonSslClient && tcpNonSslClient->connected()) {
        return;
      }
      if (!checkSocketClient()) {
        isSocketAuth = false;
        if (hubSettings.rgb_en) {
          rgbLed.setBrightness(rgbBrightness);
          rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
          rgbLed.show();
        }
      } else {
        debugPrint("Connected to socket server\n");
      }
    } else if (hubSettings.net_protocol == 1) {
      tcpSslClient = &GSMClient_SSL;
      isSocketAuth = true;
      if (hubSettings.rgb_en) {
        rgbLed.setBrightness(rgbBrightness);
        rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 255));
        rgbLed.show();
      }
      debugPrint("Connected to socket SSL server\n");
    }
  } else {
    debugPrint("GPRS is disconnected. Attempting to reconnect...\n");
    isSocketAuth = false;
    modemAsync.tcpStatus[0] = TCPStatus::CLOSED;
    modemAsync.tcpConnections[0].connected = false;
    rgbLed.setBrightness(rgbBrightness);
    if (hubSettings.rgb_en) {
      if (!bleConfigConnected)
        rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
      else
        rgbLed.setPixelColor(0, rgbLed.Color(255, 255, 0));
      rgbLed.show();
    }

    long initTime = millis();
    modemAsync.netRegStatus = false;       // Reset network registration status
    modemAsync.checkNetworkRegistration(); // Start network registration check
    waitForAsyncBool(modemAsync.netRegStatus, true,
                     60000); // Wait up to 60s for registration
    if (!modemAsync.isNetworkConnected()) {
      debugPrint("Cellular network not registered. Trying to reconnect...\n");
      modemAsync.tcpStatus[0] = TCPStatus::CLOSED;
      modemAsync.tcpConnections[0].connected = false;

      // Reset status and restart modem
      modemAsync.modemRestarted = false;
      modemAsync.restart(hubSettings.gprs_pin.c_str());

      // Wait for modem restart completion
      if (!waitForAsyncBool(modemAsync.modemRestarted, true, 60000)) {
        debugPrint("Failed to restart modem within timeout\n");
        return;
      }
      debugPrint("Modem restart completed\n");

      // Check network registration after restart
      modemAsync.netRegStatus = false;
      modemAsync.checkNetworkRegistration();

      if (!waitForAsyncBool(modemAsync.netRegStatus, true, 60000)) {
        debugPrint("Failed to register on cellular network\n");
        return;
      }
      debugPrint("Network registration successful\n");
    }

    if (modemAsync.isNetworkConnected()) {
      debugPrint("Cellular network registered. Connecting to APN: %s\n", apn);
      // Start GPRS connection (non-blocking)
      modemAsync.gprsConnect(apn, user, pass);

      // Use utility function for non-blocking wait
      if (waitForAsyncStatus(modemAsync.gprsStatus, GPRSStatus::CONNECTED,
                             GPRSStatus::ERROR, 30000)) {
        debugPrint("GPRS connected with IP address: %s\n",
                   String(modemAsync.getLocalIP()).c_str());
        if (hubSettings.net_protocol == 0) {
          tcpNonSslClient = &GSMClient;
          if (!checkSocketClient()) {
            isSocketAuth = false;
            if (hubSettings.rgb_en) {
              rgbLed.setBrightness(rgbBrightness);
              rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
              rgbLed.show();
            }
          } else {
            debugPrint("Connected to socket server\n");
          }
        } else if (hubSettings.net_protocol == 1) {
          tcpSslClient = &GSMClient_SSL;
          isSocketAuth = true;
          if (hubSettings.rgb_en) {
            rgbLed.setBrightness(rgbBrightness);
            rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 255));
            rgbLed.show();
          }
        }
      } else {
        debugPrint("Failed to connect to GPRS within timeout\n");
      }
    }
  }
  int trials = 0;
  while (time(nullptr) < SECS_YR_2000 && trials < 3) {
    debugPrint("Syncronizing NTP time\n");
    sincroTime("cellular", ntpserver1, ntpserver2);
    esp_task_wdt_reset();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    trials++;
  }
  isValidTime = (time(nullptr) >= SECS_YR_2000);

#else
  if (modem.isGprsConnected()) {
    // Already connected, handle socket setup
    if (hubSettings.net_protocol == 0) {
      tcpNonSslClient = &GSMClient;
      if (!checkSocketClient()) {
        isSocketAuth = false;
        if (hubSettings.rgb_en) {
          rgbLed.setBrightness(rgbBrightness);
          rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
          rgbLed.show();
        }
      } else {
        debugPrint("Connected to socket server\n");
      }
    } else if (hubSettings.net_protocol == 1) {
      tcpSslClient = &GSMClient_SSL;
      isSocketAuth = true;
      if (hubSettings.rgb_en) {
        rgbLed.setBrightness(rgbBrightness);
        rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 255));
        rgbLed.show();
      }
      debugPrint("Connected to socket SSL server\n");
    }
    return;
  }

  // Not connected, attempt connection
  rgbLed.setBrightness(rgbBrightness);
  if (hubSettings.rgb_en) {
    if (!bleConfigConnected)
      rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
    else
      rgbLed.setPixelColor(0, rgbLed.Color(255, 255, 0));
    rgbLed.show();
  }

  debugPrint("GPRS disconnected\n");
  if (!modem.isNetworkConnected()) {
    debugPrint("Network GSM disconnected. Trying to reconnect...\n");
    modem.restart(hubSettings.gprs_pin.c_str());
    if (!modem.waitForNetwork())
      debugPrint("Failed to connect to the network\n");
  }

  if (modem.isNetworkConnected()) {
    debugPrint("Modem: %s\n", modem.getModemName().c_str());
    debugPrint("Network GSM connected\nConnecting to APN: %s\n", apn);
    if (!modem.gprsConnect(apn, user, pass))
      debugPrint("Fail to connect to APN\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    if (modem.isGprsConnected()) {
      debugPrint("GPRS connected\nSignal quality: %s\n",
                 String(modem.getSignalQuality()).c_str());
      if (hubSettings.net_protocol == 0) {
        tcpNonSslClient = &GSMClient;
        if (!checkSocketClient()) {
          isSocketAuth = false;
          if (hubSettings.rgb_en) {
            rgbLed.setBrightness(rgbBrightness);
            rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
            rgbLed.show();
          }
        } else {
          debugPrint("Connected to socket server\n");
        }
      } else if (hubSettings.net_protocol == 1) {
        tcpSslClient = &GSMClient_SSL;
        isSocketAuth = true;
        if (hubSettings.rgb_en) {
          rgbLed.setBrightness(rgbBrightness);
          rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 255));
          rgbLed.show();
        }
      }
      int trials = 0;
      while (time(nullptr) < SECS_YR_2000 && trials < 3) {
        debugPrint("Syncronizing time\n");
        sincroTime("cellular", ntpserver1, ntpserver2);
        esp_task_wdt_reset();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        trials++;
      }
      isValidTime = (time(nullptr) >= SECS_YR_2000);
    }
  }
#endif
}

void NetLayrzHub::checkWifiNetwork(void *pvParameters) {
  // Add current task to watchdog timer
  esp_task_wdt_add(NULL);
  uint32_t lastDiagMs = 0;
  for (;;) {
    esp_task_wdt_reset();
    connectWiFi(hubSettings.wifi_ssid.c_str(), hubSettings.wifi_pass.c_str(),
                hubSettings.sys_ntpserver1.c_str(),
                hubSettings.sys_ntpserver2.c_str());
    serviceConnectivityRebootTimer("wifi");
    checkMemoryCircuitBreaker("wifi");
    // Note: connectWiFi() already handles socket connection checking, no need
    // for additional check here
    const uint32_t now = millis();
    if ((now - lastDiagMs) > 30000) {
      lastDiagMs = now;
      logNetworkResourceDiag("wifi-keepalive");
    }

    esp_task_wdt_reset();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

void NetLayrzHub::checkGSMNetwork(void *pvParameters) {
  // Add current task to watchdog timer
  esp_task_wdt_add(NULL);
  if (hubSettings.gprs_pin.length() == 0) {
    hubSettings.gprs_pin == "0000";
  }
  if (hubSettings.gprs_pin.length() < 4) {
    hubSettings.gprs_pin.insert(0, "000");
  }
  uint32_t lastDiagMs = 0;
  for (;;) {
    esp_task_wdt_reset();
    bool needReconnect = true;
#ifdef USE_ASYNC_A7670
    const bool gprsUp = modemAsync.isGprsConnected();
    bool socketUp = true;
    if (hubSettings.net_protocol == 0) {
      socketUp = (tcpNonSslClient && tcpNonSslClient->connected());
    }
    needReconnect = (!gprsUp || !socketUp);
#else
    const bool gprsUp = modem.isGprsConnected();
    bool socketUp = true;
    if (hubSettings.net_protocol == 0) {
      socketUp = (tcpNonSslClient && tcpNonSslClient->connected());
    }
    needReconnect = (!gprsUp || !socketUp);
#endif

    if (needReconnect) {
      logNetworkResourceDiag("gsm-before-connect");
      // Do not hold the global app semaphore during cellular reconnect:
      // connectGprs can block for long periods and starve unrelated tasks
      // watched by WDT.
      connectGprs(
        hubSettings.gprs_apn.c_str(), hubSettings.gprs_apn_user.c_str(),
        hubSettings.gprs_apn_pass.c_str(), hubSettings.gprs_pin.c_str(),
        hubSettings.sys_ntpserver1.c_str(), hubSettings.sys_ntpserver2.c_str());
      esp_task_wdt_reset();
      serviceConnectivityRebootTimer("cellular");
      checkMemoryCircuitBreaker("gsm-after-connect");
      logNetworkResourceDiag("gsm-after-connect");
    } else {
      serviceConnectivityRebootTimer("cellular");
      checkMemoryCircuitBreaker("gsm-healthy");
      const uint32_t now = millis();
      if ((now - lastDiagMs) > 30000) {
        lastDiagMs = now;
        logNetworkResourceDiag("gsm-keepalive");
      }
    }

    esp_task_wdt_reset();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

void NetLayrzHub::synchroNTP(void *pvParameters) {
  for (;;) {
    vTaskDelay(hubSettings.sys_ntp_period * 3600000 / portTICK_PERIOD_MS);
    isValidTime = (time(nullptr) >= SECS_YR_2000);
    isNtpSynced = false; // Reset sync status (does not invalidate current time)
    int trials = 0;
    while (trials < 3) {
      debugPrint("Periodic NTP time sync attempt %d\n", trials + 1);
      sincroTime(hubSettings.net_mode.c_str(),
                 hubSettings.sys_ntpserver1.c_str(),
                 hubSettings.sys_ntpserver2.c_str());
      if (isNtpSynced) {
        break;
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      trials++;
    }
    if (!isNtpSynced) {
      debugPrint("Periodic NTP sync failed (time_valid=%s)\n",
                 isValidTime ? "true" : "false");
    }
  }
}

void NetLayrzHub::socketKeepAlive(void *pvParameters) {
  // DISABLED: TCP socket monitoring now handled by checkWifiNetwork (10s) and
  // checkGSMNetwork (60s) These tasks already monitor network connectivity and
  // will call checkSocketClient() when appropriate
  for (;;) {
    vTaskDelay(300000 /
               portTICK_PERIOD_MS); // Sleep indefinitely - task disabled
  }
}
