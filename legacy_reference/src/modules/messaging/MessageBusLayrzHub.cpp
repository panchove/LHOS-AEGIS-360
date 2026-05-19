// MessageBusLayrzHub.cpp - Implementation
#include "MessageBusLayrzHub.h"

#include <cstring>
#include <modules/layrz_protocol/LinkLayrzHub.h>
#include <modules/network/NetLayrzHub.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

// Static state definitions
QueueHandle_t MessageBusLayrzHub::_rtQueue = nullptr;
QueueHandle_t MessageBusLayrzHub::_riQueue = nullptr;
BlackBoxLayrzHub *MessageBusLayrzHub::_bb = nullptr;
BlackBoxLayrzHub *MessageBusLayrzHub::_riBb = nullptr;
MessageBusLayrzHub::Config MessageBusLayrzHub::_cfg = {};
TaskHandle_t MessageBusLayrzHub::_senderHandle = nullptr;
TaskHandle_t MessageBusLayrzHub::_riHandle = nullptr;

// --- Reliable send (TCP) state (file scope) ---
static BusMessage _pendingRtMsg{};  // Realtime pending message awaiting ACK
static bool _pendingHasMsg = false; // True if waiting for <Ao>/<Ar>
static uint32_t _pendingSendMs = 0; // millis() when we sent pending
static bool _pendingFromBacklog =
  false; // Pending payload originated from backlog
// Fixed-size backlog line buffer: avoids repeated heap alloc/free during ACK
// wait cycles
static char _pendingBacklogBuf[MESSAGE_BUFFER_SIZE]; // Backlog line content
                                                     // while awaiting ACK
static size_t _pendingBacklogBufLen = 0; // Current length of _pendingBacklogBuf
static const uint32_t ACK_TIMEOUT_MS = 5000; // Timeout window for ACK
static uint32_t _rtSinceLastBacklog =
  0; // Fairness counter to periodically allow backlog

static bool pcPayloadMatchesCmdId(const char *payload,
                                  const char *expectedCmdId) {
  if (!payload || !expectedCmdId || expectedCmdId[0] == '\0')
    return false;

  const char *start = strstr(payload, "<Pc>");
  start = start ? (start + 4) : payload;
  const char *endTag = strstr(start, "</Pc>");
  size_t len = endTag ? (size_t)(endTag - start) : strlen(start);
  if (len == 0)
    return false;

  const char *firstSep =
    (const char *)memchr(start, ';', len); // timestamp separator
  if (!firstSep)
    return false;
  size_t remaining = len - (size_t)(firstSep - start) - 1;
  const char *cmdStart = firstSep + 1;
  const char *secondSep =
    (const char *)memchr(cmdStart, ';', remaining); // cmd_id separator
  if (!secondSep)
    return false;

  const size_t cmdLen = (size_t)(secondSep - cmdStart);
  const size_t expectedLen = strlen(expectedCmdId);
  return (cmdLen == expectedLen) &&
         (strncmp(cmdStart, expectedCmdId, cmdLen) == 0);
}

static inline bool isCellularSessionUpFast() {
  if (hubSettings.net_mode == "wifi")
    return true;
#ifdef USE_ASYNC_A7670
  return modemAsync.gprsStatus == GPRSStatus::CONNECTED;
#else
  return modem.isGprsConnected();
#endif
}

static inline bool isCellularSocketUsableFast() {
  if (hubSettings.net_mode == "wifi")
    return true;
#ifdef USE_ASYNC_A7670
  return modemAsync.netRegStatus &&
         modemAsync.gprsStatus == GPRSStatus::CONNECTED &&
         modemAsync.tcpStatus[0] == TCPStatus::CONNECTED;
#else
  return modem.isGprsConnected();
#endif
}

static inline bool isRawSocketConnectedFast() {
  if (hubSettings.net_mode == "wifi") {
    return tcpNonSslClient && tcpNonSslClient->connected();
  }
#ifdef USE_ASYNC_A7670
  return isCellularSocketUsableFast();
#else
  return tcpNonSslClient && tcpNonSslClient->connected();
#endif
}

static void logSocketDiagnostics(const char *reason, size_t expected,
                                 size_t sent, size_t sentNl) {
  if (!hubSettings.sys_debug_en)
    return;
  const size_t freeInt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const size_t freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const size_t largestInt =
    heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  const size_t largestPsram =
    heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  const size_t minInt = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
  const size_t minPsram = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
  const uint32_t hwmTcpRx =
    tcpSocketReceptionHandle
      ? (uint32_t)uxTaskGetStackHighWaterMark(tcpSocketReceptionHandle)
      : 0U;
  const uint32_t hwmWifi =
    checkWifiNetworkHandle
      ? (uint32_t)uxTaskGetStackHighWaterMark(checkWifiNetworkHandle)
      : 0U;
  const uint32_t hwmGsm =
    checkGSMNetworkHandle
      ? (uint32_t)uxTaskGetStackHighWaterMark(checkGSMNetworkHandle)
      : 0U;
  const bool tcpConn = tcpNonSslClient && tcpNonSslClient->connected();
#ifdef USE_ASYNC_A7670
  debugPrint("[TCP-DIAG] %s exp=%u sent=%u nl=%u tcpConn=%d auth=%d gprs=%d "
             "tcp0=%d heapInt=%u "
             "maxInt=%u minInt=%u heapPs=%u maxPs=%u minPs=%u "
             "stk(tcpRx/wifi/gsm)=%u/%u/%u\n",
             reason, (unsigned)expected, (unsigned)sent, (unsigned)sentNl,
             (int)tcpConn, (int)isSocketAuth, (int)modemAsync.isGprsConnected(),
             (int)modemAsync.tcpStatus[0], (unsigned)freeInt,
             (unsigned)largestInt, (unsigned)minInt, (unsigned)freePsram,
             (unsigned)largestPsram, (unsigned)minPsram, (unsigned)hwmTcpRx,
             (unsigned)hwmWifi, (unsigned)hwmGsm);
#else
  debugPrint(
    "[TCP-DIAG] %s exp=%u sent=%u nl=%u tcpConn=%d auth=%d heapInt=%u "
    "maxInt=%u "
    "minInt=%u heapPs=%u maxPs=%u minPs=%u stk(tcpRx/wifi/gsm)=%u/%u/%u\n",
    reason, (unsigned)expected, (unsigned)sent, (unsigned)sentNl, (int)tcpConn,
    (int)isSocketAuth, (unsigned)freeInt, (unsigned)largestInt,
    (unsigned)minInt, (unsigned)freePsram, (unsigned)largestPsram,
    (unsigned)minPsram, (unsigned)hwmTcpRx, (unsigned)hwmWifi,
    (unsigned)hwmGsm);
#endif
}

static void markSocketDegradedAndClose() {
  isSocketAuth = false;
  if (tcpNonSslClient)
    tcpNonSslClient->stop();
#ifdef USE_ASYNC_A7670
  if (hubSettings.net_mode != "wifi") {
    modemAsync.tcpStatus[0] = TCPStatus::CLOSED;
    modemAsync.tcpConnections[0].connected = false;
  }
#endif
}

MessageBusLayrzHub::Config MessageBusLayrzHub::defaultConfig() {
  Config c{};
  c.realtimeQueueDepth = 64;
  c.backlogBurstMax = 12;
  c.realtimeBurstMax = 20;
  c.realtimeLowWater = 8;
  c.idleDelayMs = 40;
  return c;
}

char *MessageBusLayrzHub::allocAndCopy(const char *src, size_t *outLen) {
  if (!src)
    return nullptr;
  size_t len = strlen(src);
  char *buf =
    (char *)heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!buf)
    return nullptr;
  memcpy(buf, src, len + 1);
  if (outLen)
    *outLen = len;
  return buf;
}

bool MessageBusLayrzHub::begin(BlackBoxLayrzHub *blackbox, const Config &cfg) {
  if (_rtQueue)
    return true; // already initialized
  _bb = blackbox;
  debugPrint("Is black box enabled? %s\n", _bb ? "Yes" : "No");
  _cfg = cfg;
  _rtQueue = xQueueCreate(_cfg.realtimeQueueDepth, sizeof(BusMessage));
  if (!_rtQueue)
    return false;
  _riQueue = xQueueCreate(12, sizeof(BusMessage));
  BaseType_t ok = xTaskCreatePinnedToCore(senderTask, "msg_bus", 4096, nullptr,
                                          5, &_senderHandle, 1);
  if (_riQueue) {
    xTaskCreatePinnedToCore(riTicketTask, "ri_ticket", 4096, nullptr, 4,
                            &_riHandle, 1);
  }
  return ok == pdPASS;
}

bool MessageBusLayrzHub::publish(const BusMessage &msg) {
  if (!_rtQueue)
    return false;
  if (msg.kind == BusMsgKind::RiTicket) {
    if (!_riQueue) {
      if (msg.freeAfterSend && msg.data)
        heap_caps_free(msg.data);
      return false;
    }
    if (xQueueSend(_riQueue, &msg, 0) == pdPASS)
      return true;
    debugPrint("RI ticket queue full, enqueuing to blackbox\n");
    if (msg.data && msg.len) {
      enqueueRiTicketBlackbox(msg.data, msg.len);
    }
    if (msg.freeAfterSend && msg.data)
      heap_caps_free(msg.data);
    return false;
  }
  // Append to history for qualifying message kinds (PdSensor, BleSensor) if
  // enabled. Removed isValidTime gating so history always accumulates even
  // before clock sync.
  if (_bb && hubSettings.sys_hist_en && !msg.skipHistory) {
    if (msg.kind == BusMsgKind::PdSensor || msg.kind == BusMsgKind::BleSensor) {
      if (msg.data && msg.len) {
        _bb->appendToHistory(msg.data, msg.len);
      }
    }
  }
  if (xQueueSend(_rtQueue, &msg, 0) == pdPASS)
    return true;
  // Queue full -> persist if allowed
  if (_bb && msg.persistIfFail && msg.data && msg.len)
    _bb->enqueue(msg.data, msg.len);
  if (msg.freeAfterSend && msg.data)
    heap_caps_free(msg.data);
  return false;
}

void MessageBusLayrzHub::freeMsg(const BusMessage &m) {
  if (m.freeAfterSend && m.data)
    heap_caps_free(m.data);
}

void MessageBusLayrzHub::riTicketTask(void *arg) {
  esp_task_wdt_add(NULL);
  BusMessage msg;
  const TickType_t waitTicks = pdMS_TO_TICKS(1000);
  const int maxRetries = 3;
  const int backlogBurstMax = 3;
  static uint32_t riNextAttemptMs = 0;
  static uint8_t riFailCount = 0;
  for (;;) {
    // if (hubSettings.sys_debug_en) {
    // 	static uint32_t lastStackLogMs = 0;
    // 	uint32_t now = millis();
    // 	if (now - lastStackLogMs > 15000) {
    // 		UBaseType_t words = uxTaskGetStackHighWaterMark(NULL);
    // 		debugPrint("Stack HW ri_ticket: %u words (%u bytes)\n",
    // (unsigned)words, (unsigned)(words * sizeof(StackType_t)));
    // lastStackLogMs = now;
    // 	}
    // }
    if (!_riQueue) {
      vTaskDelay(pdMS_TO_TICKS(250));
      esp_task_wdt_reset();
      continue;
    }
    uint32_t nowMs = millis();
    if (xQueueReceive(_riQueue, &msg, waitTicks) == pdPASS) {
      bool requeueMsg = false;
      if (msg.data && msg.len) {
        if (!riEndpointConfigured()) {
          if (hubSettings.sys_debug_en)
            debugPrint("RI API URL not configured, ticket queued for later\n");
          if (!enqueueRiTicketBlackbox(msg.data, msg.len))
            requeueMsg = true;
        } else if (!riNetworkAvailable() || !riMemoryAvailable()) {
          if (hubSettings.sys_debug_en)
            debugPrint("RI ticket deferred (net down or low memory)\n");
          if (!enqueueRiTicketBlackbox(msg.data, msg.len))
            requeueMsg = true;
        } else {
          if (nowMs < riNextAttemptMs) {
            if (hubSettings.sys_debug_en)
              debugPrint("RI backoff active, deferring ticket\n");
            requeueMsg = true;
          }
          if (_pendingHasMsg || uxQueueMessagesWaiting(_rtQueue) > 0) {
            if (hubSettings.sys_debug_en) {
              debugPrint(
                "Deferring RI ticket (socket busy: pending=%d rtq=%u)\n",
                (int)_pendingHasMsg,
                (unsigned)uxQueueMessagesWaiting(_rtQueue));
            }
            requeueMsg = true;
            // Skip HTTP while socket has pending work.
          }
          if (!requeueMsg) {
            esp_task_wdt_delete(NULL);
            pauseSocketForHttp();
            bool ok = false;
            for (int attempt = 0; attempt < maxRetries &&
                                  riNetworkAvailable() && riMemoryAvailable();
                 ++attempt) {
              ok = sendRiTicketHttp(msg.data, msg.len);
              if (ok)
                break;
              esp_task_wdt_reset();
              vTaskDelay(pdMS_TO_TICKS(500));
            }
            if (!ok) {
              debugPrint("RI API ticket send failed, enqueuing\n");
              if (!enqueueRiTicketBlackbox(msg.data, msg.len))
                requeueMsg = true;
              riFailCount =
                riFailCount < 10 ? (uint8_t)(riFailCount + 1) : riFailCount;
              uint32_t backoffMs = 5000U * riFailCount;
              if (backoffMs > 60000U)
                backoffMs = 60000U;
              riNextAttemptMs = millis() + backoffMs;
            }
            if (ok) {
              riFailCount = 0;
              riNextAttemptMs = 0;
            }
            resumeSocketAfterHttp();
            esp_task_wdt_add(NULL);
          }
        }
      }
      if (requeueMsg) {
        if (xQueueSendToFront(_riQueue, &msg, 0) == pdPASS) {
          vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
          debugPrint("RI ticket queue full, dropping\n");
          freeMsg(msg);
        }
      } else {
        freeMsg(msg);
      }
    } else {
      if (riEndpointConfigured() && riNetworkAvailable() &&
          riMemoryAvailable() && initRiTicketBlackbox() && _riBb->hasData()) {
        if (nowMs < riNextAttemptMs) {
          esp_task_wdt_reset();
          continue;
        }
        if (_pendingHasMsg || uxQueueMessagesWaiting(_rtQueue) > 0) {
          if (hubSettings.sys_debug_en) {
            debugPrint(
              "Deferring RI backlog (socket busy: pending=%d rtq=%u)\n",
              (int)_pendingHasMsg, (unsigned)uxQueueMessagesWaiting(_rtQueue));
          }
          esp_task_wdt_reset();
          continue;
        }
        esp_task_wdt_delete(NULL);
        pauseSocketForHttp();
        int sent = 0;
        static String
          riBbLine; // static: keeps allocation between loop iterations
        while (sent < backlogBurstMax && _riBb->hasData() &&
               riNetworkAvailable() && riMemoryAvailable()) {
          String &line = riBbLine;
          if (!_riBb->readNext(line))
            break;
          if (line.isEmpty()) {
            _riBb->commitRead(1);
            continue;
          }
          bool ok = false;
          for (int attempt = 0; attempt < maxRetries && riNetworkAvailable() &&
                                riMemoryAvailable();
               ++attempt) {
            ok = sendRiTicketHttp(line.c_str(), line.length());
            if (ok)
              break;
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(500));
          }
          if (ok) {
            _riBb->commitRead(line.length() + 1);
            sent++;
          } else {
            riFailCount =
              riFailCount < 10 ? (uint8_t)(riFailCount + 1) : riFailCount;
            uint32_t backoffMs = 5000U * riFailCount;
            if (backoffMs > 60000U)
              backoffMs = 60000U;
            riNextAttemptMs = millis() + backoffMs;
            break;
          }
        }
        if (sent > 0) {
          riFailCount = 0;
          riNextAttemptMs = 0;
        }
        resumeSocketAfterHttp();
        if (_riBb)
          _riBb->closeFiles();
        esp_task_wdt_add(NULL);
      }
    }
    esp_task_wdt_reset();
  }
}

void MessageBusLayrzHub::senderTask(void *arg) {
  esp_task_wdt_add(NULL);
  const TickType_t idleDelay = pdMS_TO_TICKS(_cfg.idleDelayMs);
  BusMessage msg;
  for (;;) {
    // if (hubSettings.sys_debug_en) {
    // 	static uint32_t lastStackLogMs = 0;
    // 	uint32_t now = millis();
    // 	if (now - lastStackLogMs > 15000) {
    // 		UBaseType_t words = uxTaskGetStackHighWaterMark(NULL);
    // 		debugPrint("Stack HW msg_bus: %u words (%u bytes)\n", (unsigned)words,
    // (unsigned)(words * sizeof(StackType_t))); 		lastStackLogMs = now;
    // 	}
    // }
    // When using raw TCP (protocol 0) all send logic lives in
    // tcpSocketReception to avoid socket contention.
    if (hubSettings.net_protocol == 0) {
      vTaskDelay(idleDelay);
      esp_task_wdt_reset();
      continue;
    }

    // HTTPS / other protocols (non‑raw): send realtime messages directly.
    while (xQueueReceive(_rtQueue, &msg, 0) == pdPASS) {
      BaseClientLayrzHub *client = ClientFactoryLayrzHub::createClient();
      bool ok = false;
      if (client) {
        serverResponse resp =
          client->sendDataToServer(msg.data ? msg.data : "");
        ok = (resp.responseCode == 200);
        delete client;
      }
      if (!ok && _bb && msg.persistIfFail && msg.data)
        _bb->enqueue(msg.data, msg.len);
      freeMsg(msg);
      if (uxQueueMessagesWaiting(_rtQueue) == 0)
        break; // avoid long monopolization
    }

    // Limited backlog replay for HTTPS when queue is low
    if (_bb && networkReady() &&
        uxQueueMessagesWaiting(_rtQueue) <= _cfg.realtimeLowWater &&
        _bb->hasData()) {
      int replayed = 0;
      static String line; // static: keeps allocation between iterations
      while (replayed < (int)_cfg.backlogBurstMax && _bb->hasData() &&
             uxQueueMessagesWaiting(_rtQueue) <= _cfg.realtimeLowWater) {
        if (!_bb->readNext(line))
          break;
        if (line.isEmpty()) {
          _bb->commitRead(1);
          replayed++;
          continue;
        }
        BaseClientLayrzHub *client = ClientFactoryLayrzHub::createClient();
        bool ok = false;
        if (client) {
          serverResponse resp = client->sendDataToServer(line.c_str());
          ok = (resp.responseCode == 200);
          delete client;
        }
        if (ok) {
          _bb->commitRead(line.length() + 1);
          replayed++;
          if (hubSettings.sys_debug_en)
            debugPrint("HTTPS replay TX(%u)\n", (unsigned)line.length());
        } else {
          // leave line for future retry
          break;
        }
      }
    }

    vTaskDelay(idleDelay);
    esp_task_wdt_reset();
  }
}

void MessageBusLayrzHub::pauseSocketForHttp() {
  if (hubSettings.net_protocol != 0) {
    return;
  }
  httpBusy = true;
  httpBusySinceMs = millis();
  while (_pendingHasMsg) {
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_task_wdt_reset();
  }
  if (tcpNonSslClient && tcpNonSslClient->connected()) {
    tcpNonSslClient->stop();
#ifdef USE_ASYNC_A7670
    if (hubSettings.net_mode != "wifi") {
      modemAsync.tcpStatus[0] = TCPStatus::CONNECTED;
      waitForAsyncStatusArray(modemAsync.tcpStatus, 0, TCPStatus::CLOSED,
                              TCPStatus::ERROR, 5000);
    }
#endif
  }
  isSocketAuth = false;
}

void MessageBusLayrzHub::resumeSocketAfterHttp() {
  if (hubSettings.net_protocol == 0) {
    NetLayrzHub::checkSocketClient();
  }
  httpBusy = false;
  httpBusySinceMs = 0;
}

static bool readHttpLineWithTimeout(Client *client, String &out,
                                    uint32_t timeoutMs) {
  if (!client)
    return false;
  out = "";
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    while (client->available()) {
      int ch = client->read();
      if (ch < 0)
        continue;
      if (ch == '\n')
        return true;
      if (ch != '\r')
        out += (char)ch;
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  return false;
}

static int parseHttpStatusCode(const String &statusLine) {
  int firstSpace = statusLine.indexOf(' ');
  if (firstSpace < 0)
    return -1;
  int secondSpace = statusLine.indexOf(' ', firstSpace + 1);
  String codeStr = (secondSpace > firstSpace)
                     ? statusLine.substring(firstSpace + 1, secondSpace)
                     : statusLine.substring(firstSpace + 1);
  codeStr.trim();
  return codeStr.toInt();
}

bool MessageBusLayrzHub::sendRiTicketHttp(const char *payload, size_t len) {
  if (!payload || len == 0)
    return false;
  if (hubSettings.ri_api_url.empty()) {
    debugPrint("RI API URL empty\n");
    return false;
  }
  if (!riMemoryAvailable()) {
    debugPrint("RI API low memory, skipping\n");
    return false;
  }

  std::string url = hubSettings.ri_api_url;
  bool useHttps = false;
  const std::string httpsPrefix = "https://";
  const std::string httpPrefix = "http://";

  if (url.rfind(httpsPrefix, 0) == 0) {
    useHttps = true;
    url = url.substr(httpsPrefix.length());
  } else if (url.rfind(httpPrefix, 0) == 0) {
    useHttps = false;
    url = url.substr(httpPrefix.length());
  } else {
    debugPrint("RI API URL missing scheme: %s\n",
               hubSettings.ri_api_url.c_str());
    return false;
  }

  std::string hostPort = url;
  std::string path = "/";
  size_t slashPos = url.find('/');
  if (slashPos != std::string::npos) {
    hostPort = url.substr(0, slashPos);
    path = url.substr(slashPos);
  }

  std::string host = hostPort;
  int port = useHttps ? 443 : 80;
  size_t colonPos = hostPort.find(':');
  if (colonPos != std::string::npos) {
    host = hostPort.substr(0, colonPos);
    std::string portStr = hostPort.substr(colonPos + 1);
    if (!portStr.empty())
      port = atoi(portStr.c_str());
  }

  if (host.empty()) {
    debugPrint("RI API URL invalid: %s\n", hubSettings.ri_api_url.c_str());
    return false;
  }

  if (!riNetworkAvailable()) {
    return false;
  }

  const uint32_t responseTimeoutMs = 5000;
  WiFiClient localWifiClient;
  SSLClient localWifiSslClient(&localWifiClient);
  Client *client = nullptr;

  if (useHttps) {
    if (hubSettings.net_mode == "wifi") {
      localWifiSslClient.setInsecure();
      localWifiSslClient.setHandshakeTimeout(responseTimeoutMs);
      client = &localWifiSslClient;
    } else {
      GSMClient_SSL.setInsecure();
      GSMClient_SSL.setHandshakeTimeout(responseTimeoutMs);
      client = &GSMClient_SSL;
    }
  } else {
    if (hubSettings.net_mode == "wifi") {
      client = &localWifiClient;
    } else {
#ifdef USE_ASYNC_A7670
      client = &GSMFotaClient;
#else
      client = &GSMClient;
#endif
    }
  }

  if (!client)
    return false;
  client->setTimeout(responseTimeoutMs);
  debugPrint("RI API POST %s://%s:%d%s\n", useHttps ? "https" : "http",
             host.c_str(), port, path.c_str());
  if (!client->connect(host.c_str(), port)) {
    debugPrint("RI API connect failed\n");
    client->stop();
    return false;
  }

  String hostHeader = String(host.c_str());
  if ((useHttps && port != 443) || (!useHttps && port != 80)) {
    hostHeader += ":";
    hostHeader += String(port);
  }

  String request = "POST " + String(path.c_str()) + " HTTP/1.1\r\n";
  request += "Host: " + hostHeader + "\r\n";
  request += "User-Agent: layrzHub\r\n";
  request += "Connection: close\r\n";
  request += "Content-Type: application/json\r\n";
  if (!hubSettings.ri_api_token.empty()) {
    request += "Authorization: ";
    request += hubSettings.ri_api_token.c_str();
    request += "\r\n";
  }
  request += "Content-Length: ";
  request += String(len);
  request += "\r\n\r\n";

  client->print(request);
  if (len > 0) {
    client->write(reinterpret_cast<const uint8_t *>(payload), len);
  }
  client->flush();

  String statusLine;
  if (!readHttpLineWithTimeout(client, statusLine, responseTimeoutMs)) {
    debugPrint("RI API timeout waiting status line\n");
    client->stop();
    return false;
  }

  int responseCode = parseHttpStatusCode(statusLine);
  debugPrint("RI API response: %d\n", responseCode);

  if (hubSettings.sys_debug_en) {
    String line;
    while (readHttpLineWithTimeout(client, line, 250)) {
      if (line.length() == 0)
        break;
    }
  }

  client->stop();
  return responseCode >= 200 && responseCode < 300;
}

bool MessageBusLayrzHub::initRiTicketBlackbox() {
#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
  if (_riBb)
    return true;
  if (!sdCardInitialized) {
    // debugPrint("RI ticket blackbox unavailable (SD not mounted)\n");
    return false;
  }
  BlackBoxLayrzHub::Config cfg;
  cfg.baseDir = "/ri_tickets";
  cfg.metaPath = "/ri_tickets/meta.txt";
  cfg.historyPath = "/ri_tickets/history.txt";
  cfg.logOnEnqueue = false;
  _riBb = new BlackBoxLayrzHub(SD, cfg);
  if (!_riBb || !_riBb->begin()) {
    debugPrint("RI ticket blackbox init failed\n");
    delete _riBb;
    _riBb = nullptr;
    return false;
  }
  _riBb->closeFiles();
  return true;
#else
  return false;
#endif
}

bool MessageBusLayrzHub::enqueueRiTicketBlackbox(const char *payload,
                                                 size_t len) {
  if (!payload || len == 0)
    return false;
  if (!initRiTicketBlackbox())
    return false;
  bool ok = _riBb->enqueue(payload, len);
  _riBb->closeFiles();
  return ok;
}

bool MessageBusLayrzHub::riNetworkAvailable() {
  if (hubSettings.net_mode == "wifi") {
    return WiFi.status() == WL_CONNECTED;
  }
#ifdef USE_ASYNC_A7670
  return modemAsync.isGprsConnected();
#else
  return modem.isGprsConnected();
#endif
}

bool MessageBusLayrzHub::riEndpointConfigured() {
  const std::string &url = hubSettings.ri_api_url;
  if (url.empty())
    return false;
  if (url.rfind("https://", 0) == 0)
    return true;
  if (url.rfind("http://", 0) == 0)
    return true;
  return false;
}

bool MessageBusLayrzHub::riMemoryAvailable() {
  const size_t minInternal = 60000;
  return heap_caps_get_free_size(MALLOC_CAP_INTERNAL) >= minInternal;
}

// Unified tcpSocketReception with reliable ACK/timeout handling
void MessageBusLayrzHub::tcpSocketReception(void *pvParameters) {
  esp_task_wdt_add(NULL);
  for (;;) {
    esp_task_wdt_reset();
    // if (hubSettings.sys_debug_en) {
    // 	static uint32_t lastStackLogMs = 0;
    // 	uint32_t now = millis();
    // 	if (now - lastStackLogMs > 15000) {
    // 		UBaseType_t words = uxTaskGetStackHighWaterMark(NULL);
    // 		debugPrint("Stack HW tcpSocketReception: %u words (%u bytes)\n",
    // (unsigned)words, (unsigned)(words * sizeof(StackType_t)));
    // lastStackLogMs = now;
    // 	}
    // }

    if (hubSettings.net_protocol == 0) {
      const uint32_t httpBusyTimeoutMs = 6000;
      if (httpBusy && httpBusySinceMs > 0 &&
          (millis() - httpBusySinceMs > httpBusyTimeoutMs)) {
        debugPrint("HTTP busy timeout, resuming socket\n");
        resumeSocketAfterHttp();
      }
      if (!tcpNonSslClient) {
        tcpNonSslClient = (hubSettings.net_mode == "wifi")
                            ? static_cast<Client *>(&wifiClient)
                            : static_cast<Client *>(&GSMClient);
      }
      const bool cellularSessionUp = isCellularSessionUpFast();
      const bool socketUsable = isCellularSocketUsableFast();

      // 1. Inbound first - read with small delays to allow FIFO accumulation
      if (cellularSessionUp && socketUsable && isRawSocketConnectedFast() &&
          tcpNonSslClient->available()) {
        // Static buffer avoids repeated heap alloc/free churn on every inbound
        // frame
        static char rxBuf[8193];
        uint16_t rxLen = 0;
        uint16_t rxLoopCounter = 0;
        while (tcpNonSslClient->available() && rxLen < 8192) {
          int ch = tcpNonSslClient->read();
          if (ch < 0)
            break;
          rxBuf[rxLen++] = (char)ch;
          if (((++rxLoopCounter) & 0x1F) == 0)
            esp_task_wdt_reset();
          vTaskDelay(1 /
                     portTICK_PERIOD_MS); // Allow FIFO to accumulate more data
        }
        rxBuf[rxLen] = '\0';
        if (rxLen > 0) {
          std::unique_ptr<std::string> s(new std::string(rxBuf, rxLen));
          auto arr = splitStringManual(s);
          if (arr) {
            if (Print *console = debugConsole()) {
              debugPrint("RX: ");
              console->println(rxBuf);
            }
            for (const auto &element : *arr) {
              esp_task_wdt_reset();
              if (element.find("<As>") != std::string::npos) {
                isSocketAuth = true;
                if (hubSettings.sys_debug_en)
                  debugPrint("Authenticated\n");
                blinkRGBLed(200);
              } else if (element.find("<Ar>Device not authenticated") !=
                           std::string::npos ||
                         element.find("<Ar>Device not found") !=
                           std::string::npos) {
                debugPrint("Auth error / device not found\n");
                if (tcpNonSslClient)
                  tcpNonSslClient->stop();
                isSocketAuth = false;
              } else if (element.find("<Ac>") != std::string::npos) {
                LayrzProtocol::cmdWrapper(element.substr(4), cmdSource::NET);
              } else if (element.find("<Ao>") != std::string::npos) {
                bool shouldRebootAfterPcAck = false;
                if (rebootAfterPcAck && _pendingHasMsg) {
                  if (_pendingFromBacklog) {
                    shouldRebootAfterPcAck = pcPayloadMatchesCmdId(
                      _pendingBacklogBuf, rebootAfterPcCmdId);
                  } else if (_pendingRtMsg.kind == BusMsgKind::PcAck) {
                    shouldRebootAfterPcAck = pcPayloadMatchesCmdId(
                      _pendingRtMsg.data, rebootAfterPcCmdId);
                  }
                }
                if (_pendingHasMsg) {
                  if (_pendingFromBacklog) {
                    if (_bb)
                      _bb->commitRead(_pendingBacklogBufLen + 1);
                    _pendingBacklogBuf[0] = '\0';
                    _pendingBacklogBufLen = 0;
                  } else {
                    freeMsg(_pendingRtMsg);
                  }
                  _pendingHasMsg = false;
                  _pendingFromBacklog = false;
                  if (bootAfterCommand &&
                      _pendingRtMsg.kind == BusMsgKind::PsSettings) {
                    if (hubSettings.sys_debug_en)
                      debugPrint("PS acknowledged successfully. Rebooting\n");
                    ESP.restart(); // Restart to apply new settings
                  }
                }
                if (shouldRebootAfterPcAck) {
                  rebootAfterPcAck = false;
                  rebootAfterPcCmdId[0] = '\0';
                  if (hubSettings.sys_debug_en)
                    debugPrint(
                      "<Pc> acknowledged for deferred reboot. Rebooting\n");
                  ESP.restart();
                }
                socketSendSuccess = true;
                blinkRGBLed(200);
              } else if (element.find("<Ar>") != std::string::npos) {
                if (_pendingHasMsg) {
                  if (_pendingFromBacklog) {
                    _pendingBacklogBuf[0] = '\0';
                    _pendingBacklogBufLen = 0;
                  } else {
                    if (_bb && _pendingRtMsg.persistIfFail &&
                        _pendingRtMsg.data)
                      _bb->enqueue(_pendingRtMsg.data, _pendingRtMsg.len);
                    freeMsg(_pendingRtMsg);
                  }
                  _pendingHasMsg = false;
                  _pendingFromBacklog = false;
                }
                socketSendSuccess = false;
                blinkRGBLed(-1);
              } else if (element.find("<Au>") != std::string::npos) {
                std::string ident = getBluetoothMACAddress() + ";;";
                ident += calculateCRC16(ident.c_str(), ident.length());
                std::string message = "<Pa>" + ident + "</Pa>";
                if (hubSettings.sys_debug_en)
                  debugPrint("Sending identification: %s\n", message.c_str());
                if (isRawSocketConnectedFast())
                  tcpNonSslClient->println(message.c_str());
              }
            }
          }
        }
      }

      // 2. ACK timeout
      if (_pendingHasMsg) {
        uint32_t now = millis();
        if (now - _pendingSendMs > ACK_TIMEOUT_MS) {
          blinkRGBLed(-1);
          if (hubSettings.sys_debug_en)
            debugPrint("ACK timeout (%u ms)\n",
                       (unsigned)(now - _pendingSendMs));
          size_t pendingLen =
            _pendingFromBacklog ? _pendingBacklogBufLen : _pendingRtMsg.len;
          logSocketDiagnostics("ack-timeout", pendingLen, 0, 0);
          if (_pendingFromBacklog) {
            _pendingBacklogBuf[0] = '\0';
            _pendingBacklogBufLen = 0;
          } else {
            if (_bb && _pendingRtMsg.persistIfFail && _pendingRtMsg.data)
              _bb->enqueue(_pendingRtMsg.data, _pendingRtMsg.len);
            freeMsg(_pendingRtMsg);
          }
          _pendingHasMsg = false;
          _pendingFromBacklog = false;
        }
      }

      // 3. Realtime priority with fairness allowing backlog every K realtime
      // sends
      static bool lastNetReady = false;
      bool netUpNow = false;
      if (cellularSessionUp && socketUsable) {
        netUpNow = networkReady();
      }
      if (hubSettings.sys_debug_en && netUpNow && !lastNetReady) {
        if (_bb)
          debugPrint(
            "NET READY: rtQ=%u bbHasData=%d rSeg=%u wSeg=%u rOff=%u wSz=%u\n",
            (unsigned)uxQueueMessagesWaiting(_rtQueue), _bb->hasData(),
            (unsigned)_bb->readSeg(), (unsigned)_bb->writeSeg(),
            (unsigned)_bb->readOff(), (unsigned)_bb->writeSz());
      }
      lastNetReady = netUpNow;

      if (!_pendingHasMsg) {
        if (httpBusy) {
          esp_task_wdt_reset();
          vTaskDelay(25 / portTICK_PERIOD_MS);
          continue;
        }
        size_t rtq = uxQueueMessagesWaiting(_rtQueue);
        bool backlogAvailable = _bb && _bb->hasData();
        const uint32_t FAIRNESS_THRESHOLD =
          3; // attempt backlog after 3 RT sends
        bool allowBacklog =
          backlogAvailable &&
          (rtq == 0 || _rtSinceLastBacklog >= FAIRNESS_THRESHOLD);
        if (netUpNow && allowBacklog) {
          static uint32_t blAttempt = 0;
          static String line; // static: keeps allocation between iterations
          if (_bb->readNext(line)) {
            if (!line.isEmpty()) {
              // debugPrint("BL payload: ");
              // if (hubSettings.sys_debug_en) UARTSerial.println(line);
              // Write exact length + newline to avoid any library-specific
              // truncation in println
              size_t toSend = line.length();
              size_t sent =
                tcpNonSslClient->write((const uint8_t *)line.c_str(), toSend);
              size_t sentNl = tcpNonSslClient->write('\n');
              if (hubSettings.sys_debug_en) {
                debugPrint("BL SEND len=%u sent=%u\n", (unsigned)toSend,
                           (unsigned)sent);
                if (sent != toSend)
                  debugPrint(
                    "WARNING: backlog partial send (expected %u, got %u)\n",
                    (unsigned)toSend, (unsigned)sent);
              }
              if (sent != toSend || sentNl != 1) {
                logSocketDiagnostics("backlog-write-fail", toSend, sent,
                                     sentNl);
                markSocketDegradedAndClose();
                continue; // leave backlog pointer untouched; retry later after
                          // reconnect
              }
              // Copy backlog line into fixed buffer (avoids Arduino String
              // alloc/free churn)
              strncpy(_pendingBacklogBuf, line.c_str(),
                      MESSAGE_BUFFER_SIZE - 1);
              _pendingBacklogBuf[MESSAGE_BUFFER_SIZE - 1] = '\0';
              _pendingBacklogBufLen = line.length();
              _pendingHasMsg = true;
              _pendingFromBacklog = true;
              _pendingSendMs = millis();
              _rtSinceLastBacklog = 0;
              if (hubSettings.sys_debug_en) {
                // Infer type from leading tag
                const char *type = "?";
                if (line.startsWith("<Pd>"))
                  type = "Pd";
                else if (line.startsWith("<Pi>"))
                  type = "Pi";
                else if (line.startsWith("<Ps>"))
                  type = "Ps";
                else if (line.startsWith("<Pc>"))
                  type = "Pc";
                else if (line.startsWith("<Pb>"))
                  type = "Pb";
                else if (line.startsWith("<Pa>"))
                  type = "Pa";
                else if (line.startsWith("<Pm>"))
                  type = "Pm";
                debugPrint("BL TX(%u) awaiting ACK (fairness) q=%u type=%s\n",
                           (unsigned)line.length(), (unsigned)rtq, type);
              }
            } else {
              _bb->commitRead(1);
            }
          } else if (hubSettings.sys_debug_en) {
            bool hd = backlogAvailable;
            debugPrint(
              "BB readNext false. hasData=%d attempt=%u readSeg=%u "
              "writeSeg=%u readOff=%u readFileSz=%u writeSz=%u pending=%d\n",
              (int)hd, (unsigned)++blAttempt, (unsigned)_bb->readSeg(),
              (unsigned)_bb->writeSeg(), (unsigned)_bb->readOff(),
              (unsigned)_bb->currentReadFileSize(), (unsigned)_bb->writeSz(),
              (int)_pendingHasMsg);
          }
        } else if (netUpNow && rtq > 0) {
          BusMessage next;
          if (xQueueReceive(_rtQueue, &next, 0) == pdPASS) {
            bool writeOk = true;
            if (next.data && next.len) {
              size_t sent =
                tcpNonSslClient->write((const uint8_t *)next.data, next.len);
              size_t sentNl = tcpNonSslClient->write('\n');
              if (hubSettings.sys_debug_en) {
                // debugPrint("RT SEND len=%u sent=%u\n", (unsigned)next.len,
                // (unsigned)sent);
                if (sent != next.len)
                  debugPrint(
                    "WARNING: realtime partial send (expected %u, got %u)\n",
                    (unsigned)next.len, (unsigned)sent);
              }
              if (sent != next.len || sentNl != 1) {
                writeOk = false;
                logSocketDiagnostics("realtime-write-fail", next.len, sent,
                                     sentNl);
              }
            } else {
              size_t sentNl = tcpNonSslClient->write('\n');
              if (sentNl != 1) {
                writeOk = false;
                logSocketDiagnostics("realtime-newline-write-fail", 0, 0,
                                     sentNl);
              }
            }
            if (!writeOk) {
              if (_bb && next.persistIfFail && next.data)
                _bb->enqueue(next.data, next.len);
              freeMsg(next);
              markSocketDegradedAndClose();
              continue;
            }
            _pendingRtMsg = next;
            _pendingHasMsg = true;
            _pendingFromBacklog = false;
            _pendingSendMs = millis();
            _rtSinceLastBacklog++;
            if (hubSettings.sys_debug_en) {
              const char *type = "?";
              if (next.data) {
                if (strncmp(next.data, "<Pd>", 4) == 0)
                  type = "Pd";
                else if (strncmp(next.data, "<Pi>", 4) == 0)
                  type = "Pi";
                else if (strncmp(next.data, "<Ps>", 4) == 0)
                  type = "Ps";
                else if (strncmp(next.data, "<Pc>", 4) == 0)
                  type = "Pc";
                else if (strncmp(next.data, "<Pb>", 4) == 0)
                  type = "Pb";
                else if (strncmp(next.data, "<Pa>", 4) == 0)
                  type = "Pa";
                else if (strncmp(next.data, "<Pm>", 4) == 0)
                  type = "Pm";
              }
              if (next.data && type == "Pd") {
                // debugPrint("RT payload: ");
                // if (hubSettings.sys_debug_en) UARTSerial.println(next.data);
              }
              debugPrint("RT TX(%u) qRem=%u rtSinceBL=%u type=%s\n",
                         (unsigned)next.len,
                         (unsigned)uxQueueMessagesWaiting(_rtQueue),
                         (unsigned)_rtSinceLastBacklog, type);
            }
          }
        } else if (!netUpNow) {
          // Network down: drain realtime queue to backlog
          int drained = 0;
          int moved = 0;
          while (drained < (int)_cfg.realtimeBurstMax) {
            BusMessage offmsg;
            if (xQueueReceive(_rtQueue, &offmsg, 0) != pdPASS)
              break;
            if (_bb && offmsg.persistIfFail && offmsg.data) {
              _bb->enqueue(offmsg.data, offmsg.len);
              moved++;
            }
            freeMsg(offmsg);
            drained++;
            if ((drained & 0x03) == 0)
              esp_task_wdt_reset();
          }
          if (hubSettings.sys_debug_en && (drained > 0 || moved > 0))
            debugPrint(
              "Drained %d RT msgs to backlog (moved=%d) while netDown\n",
              drained, moved);
        }
      }

      // 5. Idle pacing
      bool hasIncoming = false;
      if (socketUsable && tcpNonSslClient && isRawSocketConnectedFast()) {
        hasIncoming = tcpNonSslClient->available() > 0;
      }
      if (!_pendingHasMsg && uxQueueMessagesWaiting(_rtQueue) == 0 &&
          !hasIncoming)
        vTaskDelay(25 / portTICK_PERIOD_MS);
    } else {
      // HTTPS reception path (retain original client-based polling)
      if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
        BaseClientLayrzHub *client = ClientFactoryLayrzHub::createClient();
        if (client) {
          serverResponse response = client->receiveDataFromServer();
          if (!response.responseStr->empty()) {
            if (hubSettings.sys_debug_en) {
              debugPrint("Response from socket (receptor): ");
              if (Print *console = debugConsole())
                console->println(response.responseStr->c_str());
            }
            auto arr = splitStringManual(response.responseStr);
            if (arr) {
              for (const auto &element : *arr) {
                if (element.find("<Ac>") != std::string::npos) {
                  LayrzProtocol::cmdWrapper(element.substr(4), cmdSource::NET);
                }
              }
            }
          }
          delete client;
        }
        xSemaphoreGive(xSemaphore);
      }
    }

    esp_task_wdt_reset();
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }
}

// Determine if we can attempt sending: authenticated (raw TCP) or general
// connectivity for HTTPS.
bool MessageBusLayrzHub::networkReady() {
  if (hubSettings.net_protocol == 0) {
    // For raw TCP ensure socket object exists, is connected, and authentication
    // succeeded.
    if (!tcpNonSslClient || !isSocketAuth)
      return false;
    return isCellularSessionUpFast() && isRawSocketConnectedFast();
  }
  // For HTTPS mode rely on underlying mode connectivity (wifi or cellular) plus
  // auth if required by other publishers.
  if (hubSettings.net_mode == "wifi") {
    return (WiFi.status() == WL_CONNECTED) &&
           isSocketAuth; // keep same gating so ordering preserved
  } else if (hubSettings.net_mode == "cellular") {
    return isSocketAuth;
  }
  return false;
}
