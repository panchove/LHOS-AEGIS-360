#include "AsyncA7670.h"

#include "AsyncA7670Client.h"

#include <esp_task_wdt.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

static const TickType_t A7670_STATUS_MUTEX_TIMEOUT_TICKS = pdMS_TO_TICKS(50);

AsyncA7670::AsyncA7670(Stream &stream)
    : _stream(stream), _debugStream(nullptr), _modemTaskHandle(nullptr),
      _responseTaskHandle(nullptr), _commandQueue(nullptr),
      _responseMutex(nullptr), _statusMutex(nullptr),
      gprsStatus(GPRSStatus::DISCONNECTED), ntpStatus(NTPStatus::NOT_SYNCED),
      netRegStatus(false), modemRestarted(false), _responseIndex(0),
      _waitingForResponse(false), _responseTimeout(0), _responseStartTime(0),
      _signalQuality(-1), _signalStrength(-999), initialized(false),
      modemReady(false), _baudRate(115200) {
  // Initialize TCP connections
  for (int i = 0; i < 10; i++) {
    tcpStatus[i] = TCPStatus::CLOSED;
    tcpConnections[i].connected = false;
    tcpConnections[i].retryCount = 0;
    tcpConnections[i].lastRetryTime = 0;
    tcpConnections[i].got_data = false;
    tcpConnections[i].expectedDataLength = 0;
    tcpConnections[i].modemAvailableRequested = false;
    tcpConnections[i].modemAvailable = 0;

    // Initialize client pointers
    clients[i] = nullptr;
  }
}

AsyncA7670::~AsyncA7670() { end(); }

bool AsyncA7670::begin(uint32_t baudRate) {
  if (initialized) {
    return true;
  }

  // Try AT command up to 5 times to test modem responsiveness
  bool modemResponding = false;
  for (int attempt = 0; attempt < 5; attempt++) {
    debugPrint("AT test command attempt ");
    debugPrint(String(attempt + 1).c_str());
    debugPrintln("/5");

    // Flush before sending, but avoid getting stuck if the modem keeps
    // streaming data.
    uint32_t flushStart = millis();
    size_t flushedBytes = 0;
    while (_stream.available()) {
      _stream.read();
      flushedBytes++;
      if (flushedBytes >= 1024 || (millis() - flushStart) > 100) {
        break;
      }
    }

    // Send AT command directly
    _stream.print("AT\r\n");
    _stream.flush();

    // Wait for response
    unsigned long startTime = millis();
    String response = "";
    bool gotOK = false;

    while (millis() - startTime < 3000) { // 3 second timeout
      if (_stream.available()) {
        char c = _stream.read();
        if (c >= 32 && c <= 126) { // Only printable characters
          response += c;
        }

        // Check for OK response (case insensitive)
        if (response.indexOf("OK") >= 0) {
          gotOK = true;
          break;
        }
      }
      vTaskDelay(pdMS_TO_TICKS(1));
    }

    debugPrint("Raw response: '");
    debugPrint(response.c_str());
    debugPrintln("'");

    if (gotOK) {
      debugPrintln("✓ Modem responded to AT command");
      modemResponding = true;
      break;
    } else {
      debugPrintln("✗ No OK response detected");
      vTaskDelay(pdMS_TO_TICKS(10000)); // Wait before retry
    }
  }

  _sendConfirmReceived = false;
  _lastSendConfirmed = 0;
  _baudRate = baudRate;
  _isTcpDataCollection = false; // Initialize TCP collection flag

  // Create FreeRTOS objects
  _commandQueue = xQueueCreate(A7670_QUEUE_SIZE, sizeof(A7670Command));
  if (!_commandQueue) {
    debugPrintln("Failed to create command queue");
    return false;
  }

  _responseMutex = xSemaphoreCreateMutex();
  if (!_responseMutex) {
    debugPrintln("Failed to create response mutex");
    return false;
  }

  _statusMutex = xSemaphoreCreateMutex();
  if (!_statusMutex) {
    debugPrintln("Failed to create status mutex");
    return false;
  }

  _tcpCollectionMutex = xSemaphoreCreateMutex();
  if (!_tcpCollectionMutex) {
    debugPrintln("Failed to create TCP collection mutex");
    return false;
  }

  _connectionMutex = xSemaphoreCreateMutex();
  if (!_connectionMutex) {
    debugPrintln("Failed to create connection mutex");
    return false;
  }

  // Create tasks
  BaseType_t result = xTaskCreatePinnedToCore(
    modemTaskWrapper, "A7670_Modem", A7670_TASK_STACK_SIZE, this,
    A7670_TASK_PRIORITY, &_modemTaskHandle,
    1 // Pin to core 1
  );

  if (result != pdPASS) {
    debugPrintln("Failed to create modem task");
    return false;
  }

  result = xTaskCreatePinnedToCore(
    responseTaskWrapper, "A7670_Response", A7670_TASK_STACK_SIZE, this,
    A7670_TASK_PRIORITY + 2, // Higher priority for response handling
    &_responseTaskHandle,
    1 // Pin to core 1
  );

  if (result != pdPASS) {
    debugPrintln("Failed to create response task");
    return false;
  }

  // Initialize modem
  vTaskDelay(pdMS_TO_TICKS(500)); // Wait for tasks to be ready

  // Flush any pending data (bounded to avoid starvation if modem is chatty).
  {
    uint32_t flushStart = millis();
    size_t flushedBytes = 0;
    while (_stream.available()) {
      _stream.read();
      flushedBytes++;
      if (flushedBytes >= 2048 || (millis() - flushStart) > 200) {
        break;
      }
    }
  }

  if (!modemResponding) {
    debugPrintln("Modem not responding to AT command");
    return false;
  }

  // Now use the command queue for additional initialization
  sendATCommand("ATE0"); // Disable echo
  vTaskDelay(pdMS_TO_TICKS(300));
  sendATCommand("AT+CFUN=1");     // Full functionality
  vTaskDelay(pdMS_TO_TICKS(300)); // Wait for modem to stabilize
  sendATCommand("AT+CMEE=2");     // Enable verbose error messages
  vTaskDelay(pdMS_TO_TICKS(300));
  sendATCommand("AT+CREG=1"); // Enable network registration URC
  vTaskDelay(pdMS_TO_TICKS(300));
  sendATCommand("AT+CGREG=1"); // Enable GPRS registration URC
  vTaskDelay(pdMS_TO_TICKS(300));
  sendATCommand("AT+CLIP=1"); // Enable caller ID
  vTaskDelay(pdMS_TO_TICKS(300));
  sendATCommand("AT+CTZR=0"); // Disable time zone URC's
  vTaskDelay(pdMS_TO_TICKS(300));
  sendATCommand("AT+CTZU=1"); // Enable automatic time zone update
  vTaskDelay(pdMS_TO_TICKS(300));

  initialized = true;
  return true;
}

bool AsyncA7670::registerNetwork() {
  if (!initialized) {
    return false;
  }
  netRegStatus = false; // Reset network registration status
  time_t initTime = millis();
  while (!netRegStatus && (millis() - initTime < 20000)) {
    checkNetworkRegistration();
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
  if (!netRegStatus) {
    time_t now = millis();
    debugPrintln("Network registration failed. Restarting modem.");
    modemRestarted = false;
    restart();
    while (!modemRestarted && (millis() - now < 30000)) {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (!modemRestarted) {
      debugPrintln("Modem restart failed.");
      return false;
    }
    netRegStatus = false;
    time_t initTime = millis();
    while (!netRegStatus && (millis() - initTime < 20000)) {
      checkNetworkRegistration();
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
    if (!netRegStatus) {
      debugPrintln("Network registration failed after modem restart.");
      return false;
    }
  }
  debugPrintln("Network registered successfully.");
  modemReady = true;
  return true;
}

void AsyncA7670::ensureTcpCollectionMutex() {
  if (_tcpCollectionMutex == nullptr) {
    _tcpCollectionMutex = xSemaphoreCreateMutex();
  }
}

void AsyncA7670::end() {
  if (!initialized) {
    return;
  }

  // Stop tasks
  if (_modemTaskHandle) {
    vTaskDelete(_modemTaskHandle);
    _modemTaskHandle = nullptr;
  }

  if (_responseTaskHandle) {
    vTaskDelete(_responseTaskHandle);
    _responseTaskHandle = nullptr;
  }

  // Clean up FreeRTOS objects
  if (_commandQueue) {
    vQueueDelete(_commandQueue);
    _commandQueue = nullptr;
  }

  if (_responseMutex) {
    vSemaphoreDelete(_responseMutex);
    _responseMutex = nullptr;
  }

  if (_statusMutex) {
    vSemaphoreDelete(_statusMutex);
    _statusMutex = nullptr;
  }

  initialized = false;
  modemReady = false;
}

void AsyncA7670::modemTaskWrapper(void *parameter) {
  AsyncA7670 *instance = static_cast<AsyncA7670 *>(parameter);
  instance->modemTask();
}

void AsyncA7670::responseTaskWrapper(void *parameter) {
  AsyncA7670 *instance = static_cast<AsyncA7670 *>(parameter);
  instance->responseTask();
}

void AsyncA7670::modemTask() {
  A7670Command command;

  while (true) {
    // Process commands from queue
    if (xQueueReceive(_commandQueue, &command, pdMS_TO_TICKS(100)) == pdTRUE) {
      switch (command.type) {
      case A7670CommandType::AT_COMMAND:
        debugPrint("Sending AT command: ");
        debugPrintln(command.data);
        if (_responseMutex &&
            xSemaphoreTake(_responseMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
          _stream.print(command.data);
          _stream.print("\r\n");
          _stream.flush();
          xSemaphoreGive(_responseMutex);
        } else {
          debugPrintln("AT send skipped: UART lock timeout");
        }
        break;

      case A7670CommandType::GPRS_CONNECT:
        processGprsConnect();
        break;

      case A7670CommandType::GPRS_DISCONNECT:
        processGprsDisconnect();
        break;

      case A7670CommandType::TCP_CONNECT:
        processTcpConnect(command.mux);
        break;

      case A7670CommandType::TCP_DISCONNECT:
        processTcpDisconnect(command.mux);
        break;

        // case A7670CommandType::NTP_SYNC:
        //     processNtpSync();
        //     break;

      case A7670CommandType::GET_STATUS:
        // Status check is handled by periodic checks
        break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent task starvation
  }
}

void AsyncA7670::responseTask() {
  while (true) {
    processIncomingData();
    vTaskDelay(pdMS_TO_TICKS(1)); // Minimal delay for high responsiveness
  }
}

bool AsyncA7670::sendCommand(A7670CommandType type, const char *data,
                             uint8_t mux, size_t length, uint32_t timeout,
                             bool priority) {
  if (!_commandQueue) {
    return false;
  }

  A7670Command command;
  command.type = type;
  command.mux = mux;
  command.length = length;
  command.timeout = timeout;
  command.priority = priority;

  if (data) {
    strncpy(command.data, data, sizeof(command.data) - 1);
    command.data[sizeof(command.data) - 1] = '\0';
  } else {
    command.data[0] = '\0';
  }

  if (priority) {
    // For priority commands, try to send immediately
    return xQueueSendToFront(_commandQueue, &command, pdMS_TO_TICKS(1000)) ==
           pdTRUE;
  } else {
    return xQueueSend(_commandQueue, &command, pdMS_TO_TICKS(1000)) == pdTRUE;
  }
}

bool AsyncA7670::sendATCommand(const char *command, uint32_t timeout,
                               bool priority) {
  return sendCommand(A7670CommandType::AT_COMMAND, command, 0, 0, timeout,
                     priority);
}

// GPRS Functions
bool AsyncA7670::gprsConnect(const char *apn, const char *user,
                             const char *password) {
  if (!initialized) {
    return false;
  }
  // Check if connection is already in progress
  if (gprsStatus == GPRSStatus::CONNECTING) {
    debugPrintln("GPRS connection already in progress, skipping");
    // xSemaphoreGive(_statusMutex);
    return false; // Return false as connection is not yet complete
  }

  _apn = apn ? apn : "";
  _apnUser = user ? user : "";
  _apnPassword = password ? password : "";

  return sendCommand(A7670CommandType::GPRS_CONNECT);
}

bool AsyncA7670::gprsDisconnect() {
  if (!initialized) {
    return false;
  }

  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return false;
  }
  gprsStatus = GPRSStatus::DISCONNECTED;
  xSemaphoreGive(_statusMutex);

  return sendCommand(A7670CommandType::GPRS_DISCONNECT);
}

bool AsyncA7670::isGprsConnected() {
  if (!initialized) {
    return false;
  }

  // Simply return cached status - don't trigger additional AT commands
  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return false;
  }
  bool connected = (gprsStatus == GPRSStatus::CONNECTED &&
                    !_localIP.isEmpty() && _localIP != "0.0.0.0");
  xSemaphoreGive(_statusMutex);

  return connected;
}

String AsyncA7670::getLocalIP() {
  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return String("");
  }
  String ip = _localIP;
  xSemaphoreGive(_statusMutex);
  return ip;
}

void AsyncA7670::processGprsConnect() {
  debugPrintln("Starting GPRS connection process");

  // Prevent concurrent GPRS connection attempts
  if (xSemaphoreTake(_connectionMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    debugPrintln("GPRS connection already in progress, skipping");
    return;
  }

  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    xSemaphoreGive(_connectionMutex);
    return;
  }
  // Set status to connecting
  gprsStatus = GPRSStatus::CONNECTING;
  xSemaphoreGive(_statusMutex);

  // Close network only if needed
  debugPrintln("Closing GPRS network first...");
  sendATCommand("AT+NETCLOSE");
  vTaskDelay(pdMS_TO_TICKS(3000));

  // Configure APN
  if (!_apn.isEmpty()) {
    debugPrintln("Configuring APN...");
    String apnCmd = "AT+CGDCONT=1,\"IP\",\"" + _apn + "\",\"0.0.0.0\",0,0";
    sendATCommand(apnCmd.c_str());
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Set authentication
  if (!_apnUser.isEmpty() || !_apnPassword.isEmpty()) {
    debugPrintln("Setting APN authentication...");
    String authCmd =
      "AT+CGAUTH=1,1,\"" + _apnUser + "\",\"" + _apnPassword + "\"";
    sendATCommand(authCmd.c_str());
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Configure TCP settings
  debugPrintln("Configuring TCP settings...");
  sendATCommand("AT+CIPMODE=0");
  vTaskDelay(pdMS_TO_TICKS(1000));

  sendATCommand("AT+CIPSENDMODE=0");
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Enable multi-socket mode so mux 0 (main socket) and mux 3 (Zigbee bridge)
  // can coexist on cellular sessions.
  sendATCommand("AT+CIPMUX=1");
  vTaskDelay(pdMS_TO_TICKS(1000));

  sendATCommand("AT+CIPCCFG=10,0,0,0,1,0,75000");
  vTaskDelay(pdMS_TO_TICKS(1000));

  sendATCommand("AT+CIPTIMEOUT=7500,15000,15000");
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Ensure manual receive mode so data is buffered and pulled via CIPRXGET
  sendATCommand("AT+CIPRXGET=1");
  vTaskDelay(pdMS_TO_TICKS(300));

  // Open network
  debugPrintln("Opening network...");
  sendATCommand("AT+NETOPEN", 30000); // 30 second timeout

  // Wait for NETOPEN completion instead of fixed delay
  // The response will be processed in processResponse() and update gprsStatus
  // Don't immediately send IPADDR - let the state machine handle it

  xSemaphoreGive(_connectionMutex);
}

void AsyncA7670::processGprsDisconnect() {
  debugPrintln("Processing GPRS disconnect");
  sendATCommand("AT+NETCLOSE");

  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return;
  }
  gprsStatus = GPRSStatus::DISCONNECTED;
  _localIP = "";
  xSemaphoreGive(_statusMutex);
}

void AsyncA7670::checkGprsStatus() { sendATCommand("AT+IPADDR"); }

void AsyncA7670::checkNetworkRegistration() {
  sendATCommand("AT+CGREG?", 5000, true); // High priority command
}

// TCP Functions
bool AsyncA7670::modemConnect(const char *host, uint16_t port, uint8_t mux,
                              bool ssl, int timeout_s) {
  if (!initialized || mux >= 10) {
    return false;
  }

  if (ssl) {
    debugPrintln("SSL not supported in this version");
    return false;
  }

  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return false;
  }
  tcpConnections[mux].host = host;
  tcpConnections[mux].port = port;
  tcpConnections[mux].ssl = ssl;
  tcpConnections[mux].connected = false;
  tcpConnections[mux].retryCount = 0; // Reset retry counter for new connection
  tcpConnections[mux].lastRetryTime = 0;
  tcpStatus[mux] = TCPStatus::CONNECTING;
  xSemaphoreGive(_statusMutex);

  return sendCommand(A7670CommandType::TCP_CONNECT, nullptr, mux);
}

bool AsyncA7670::modemDisconnect(uint8_t mux) {
  if (!initialized || mux >= 10) {
    return false;
  }

  return sendCommand(A7670CommandType::TCP_DISCONNECT, nullptr, mux);
}

bool AsyncA7670::modemGetConnected(uint8_t mux) {
  if (mux >= 10) {
    return false;
  }

  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return false;
  }
  bool connected = tcpConnections[mux].connected;
  xSemaphoreGive(_statusMutex);
  return connected;
}

size_t AsyncA7670::modemSend(const void *buff, size_t len, uint8_t mux) {
  if (!initialized || mux >= 10 || !buff || len == 0) {
    return 0;
  }

  if (!modemGetConnected(mux)) {
    debugPrintln("Cannot send: socket not connected");
    return 0;
  }

  // Serialize modemSend calls so CIPSEND transactions do not overlap.
  if (!_responseMutex ||
      xSemaphoreTake(_responseMutex, pdMS_TO_TICKS(12000)) != pdTRUE) {
    debugPrintln("Cannot send: modem send lock timeout");
    return 0;
  }

  size_t totalSentBytes = 0;
  uint32_t lastWdtKick = millis();
  while (len > 0) {
    size_t chunkSize = len > A7670_MAX_SEND_CHUNK ? A7670_MAX_SEND_CHUNK : len;
    String sendCmd = "AT+CIPSEND=" + String(mux) + "," + String(chunkSize);
    _awaitingSendPrompt = true;
    _sendPromptReceived = false;
    _sendConfirmReceived = false;
    _lastSendConfirmed = 0;

    // Send directly to UART while holding the shared UART lock.
    _stream.print(sendCmd);
    _stream.print("\r\n");
    _stream.flush();
    long startTime = millis();

    // Wait for ">" prompt handled by responseTask.
    while (!_sendPromptReceived) {
      if (millis() - startTime > 10000) { // 10 second timeout
        debugPrintln("Timeout waiting for '>' prompt");
        _awaitingSendPrompt = false;
        _sendPromptReceived = false;
        xSemaphoreGive(_responseMutex);
        return totalSentBytes;
      }
      if (millis() - lastWdtKick > 500) {
        esp_task_wdt_reset();
        lastWdtKick = millis();
      }
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    _awaitingSendPrompt = false;

    // Send the actual data
    _stream.write(reinterpret_cast<const uint8_t *>(buff), chunkSize);
    _stream.flush();

    // Wait for +CIPSEND confirmation set by responseTask/parseResponse.
    startTime = millis();

    while (!_sendConfirmReceived) {
      if (millis() - startTime > 5000) { // 5 second timeout
        debugPrintln("Timeout waiting for '+CIPSEND:' confirmation");
        _sendPromptReceived = false;
        xSemaphoreGive(_responseMutex);
        return totalSentBytes;
      }
      if (millis() - lastWdtKick > 500) {
        esp_task_wdt_reset();
        lastWdtKick = millis();
      }
      vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (_lastSendConfirmed <= 0) {
      debugPrintln("Invalid '+CIPSEND' confirmation");
      xSemaphoreGive(_responseMutex);
      return totalSentBytes;
    }

    size_t confirmed = static_cast<size_t>(_lastSendConfirmed);
    totalSentBytes += confirmed;
    if (confirmed < chunkSize) {
      debugPrintln(
        "Partial '+CIPSEND' confirmation, aborting remaining payload");
      xSemaphoreGive(_responseMutex);
      return totalSentBytes;
    }

    len -= chunkSize;
    buff = static_cast<const uint8_t *>(buff) + chunkSize;
  }

  _awaitingSendPrompt = false;
  _sendPromptReceived = false;
  xSemaphoreGive(_responseMutex);
  return totalSentBytes;
}

size_t AsyncA7670::modemRead(size_t len, uint8_t mux) {
  if (!modemGetConnected(mux)) {
    debugPrintln("Cannot read: socket not connected");
    return 0;
  }

  size_t available = tcpConnections[mux].modemAvailable;
  if (available == 0) {
    return 0; // No data available
  }

  size_t toRead = (len < available) ? len : available;
  time_t startTime = millis();
  tcpConnections[mux].incomingData = false;
  String readCmd = "AT+CIPRXGET=2," + String(mux) + "," + String(toRead);
  sendATCommand(readCmd.c_str(), 10000, true); // High priority command
  while (!tcpConnections[mux].incomingData &&
         (millis() - startTime < 2000)) { // 2 second timeout
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  return tcpConnections[mux].incomingDataLength;
}

void AsyncA7670::processTcpConnect(uint8_t mux) {
  if (mux >= 10) {
    return;
  }

  debugPrint("Connecting TCP socket ");
  debugPrint(String(mux).c_str());
  debugPrint(" to ");
  debugPrint(tcpConnections[mux].host.c_str());
  debugPrint(":");
  debugPrint(String(tcpConnections[mux].port).c_str());
  debugPrintln("");
  // Connect to host
  String connectCmd = "AT+CIPOPEN=" + String(mux) + ",\"TCP\",\"" +
                      tcpConnections[mux].host + "\"," +
                      String(tcpConnections[mux].port);
  sendATCommand(connectCmd.c_str(), A7670_CONNECT_TIMEOUT_MS);
}

void AsyncA7670::processTcpDisconnect(uint8_t mux) {
  if (mux >= 10) {
    return;
  }

  String disconnectCmd = "AT+CIPCLOSE=" + String(mux);
  sendATCommand(disconnectCmd.c_str());
}

void AsyncA7670::checkTcpStatus() {
  String statusCmd = "AT+CIPCLOSE?";
  sendATCommand(statusCmd.c_str());
}

// NTP Functions
bool AsyncA7670::ntpServerSync(const char *server, int timeZone) {
  if (!initialized) {
    return false;
  }

  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return false;
  }
  _ntpServer = server ? server : "pool.ntp.org";
  _timeZone = timeZone;
  ntpStatus = NTPStatus::SYNCING;
  xSemaphoreGive(_statusMutex);
  String cmd = "AT+CNTP=\"" + _ntpServer + "\"," + String(_timeZone * 4);
  sendATCommand(cmd.c_str());
  vTaskDelay(pdMS_TO_TICKS(1000)); // Wait a bit before starting sync
  sendATCommand("AT+CNTP");        // Start sync
  time_t startTime = millis();
  while (ntpStatus == NTPStatus::SYNCING && (millis() - startTime < 10000)) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  return AsyncA7670::ntpStatus == NTPStatus::SYNCED;
}

bool AsyncA7670::isNtpSynced() {
  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return false;
  }
  bool synced = (ntpStatus == NTPStatus::SYNCED);
  xSemaphoreGive(_statusMutex);
  return synced;
}

// String AsyncA7670::getDateTime() {
//     xSemaphoreTake(_statusMutex, portMAX_DELAY);
//     String dateTime = _dateTime;
//     xSemaphoreGive(_statusMutex);
//     return dateTime;
// }

// int AsyncA7670::NTPServerSync(const String& server, int timeZone) {
//     // TinyGSM compatibility: return 0 on success, non-zero on failure
//     bool success = ntpServerSync(server.c_str(), timeZone);
//     return success ? 0 : 1;
// }

bool AsyncA7670::getNetworkTime(int *year, int *month, int *day, int *hour,
                                int *minute, int *second, float *timezone) {
  debugPrintln("Requesting current time");
  if (!initialized) {
    debugPrintln("getNetworkTime: Modem not initialized");
    return false;
  }
  if (!isNtpSynced()) {
    debugPrintln("getNetworkTime: NTP not synced");
    return false;
  }
  _dateTime = ""; // Clear previous datetime
  sendATCommand("AT+CCLK?");
  time_t start = millis();
  while (_dateTime.length() == 0 && (millis() - start < 5000)) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  if (_dateTime.length() == 0) {
    debugPrintln("getNetworkTime: No datetime available after AT+CCLK?");
    return false;
  }

  debugPrint("getNetworkTime: Parsing datetime: ");
  debugPrintln(_dateTime.c_str());

  // Parse datetime string (format: "YY/MM/DD,HH:MM:SS+ZZ" or
  // "YY/MM/DD,HH:MM:SS-ZZ") Example: "25/09/09,12:03:08+00"
  int yr, mon, dy, hr, min, sec;
  char tzSign;
  int tzValue;

  // Try parsing with timezone
  if (sscanf(_dateTime.c_str(), "%d/%d/%d,%d:%d:%d%c%d", &yr, &mon, &dy, &hr,
             &min, &sec, &tzSign, &tzValue) == 8) {
    if (year)
      *year = 2000 + yr; // Convert YY to YYYY
    if (month)
      *month = mon;
    if (day)
      *day = dy;
    if (hour)
      *hour = hr;
    if (minute)
      *minute = min;
    if (second)
      *second = sec;
    if (timezone) {
      float tz = tzValue / 4.0; // Convert quarters to hours
      if (tzSign == '-')
        tz = -tz;
      *timezone = tz;
    }
    debugPrint("getNetworkTime: Successfully parsed with timezone: ");
    debugPrint(String(tzSign).c_str());
    debugPrintln(String(tzValue).c_str());
    return true;
  }

  // Try parsing without timezone (fallback)
  if (sscanf(_dateTime.c_str(), "%d/%d/%d,%d:%d:%d", &yr, &mon, &dy, &hr, &min,
             &sec) == 6) {
    if (year)
      *year = 2000 + yr; // Convert YY to YYYY
    if (month)
      *month = mon;
    if (day)
      *day = dy;
    if (hour)
      *hour = hr;
    if (minute)
      *minute = min;
    if (second)
      *second = sec;
    if (timezone)
      *timezone = 0.0; // Default to UTC
    debugPrintln("getNetworkTime: Successfully parsed without timezone");
    return true;
  }

  debugPrintln("getNetworkTime: Failed to parse datetime");
  return false;
}

// void AsyncA7670::processNtpSync() {
//     debugPrintln("Processing NTP sync");

//     // Convert timezone from hours to quarters (A7670 expects quarters)
//     int timeZoneQuarters = _timeZone * 4;
//     String ntpCmd = "AT+CNTP=\"" + _ntpServer + "\"," +
//     String(timeZoneQuarters); sendATCommand(ntpCmd.c_str());
//     vTaskDelay(pdMS_TO_TICKS(1000));

//     sendATCommand("AT+CNTP");  // Start sync
// }

// // Fallback NTP sync with multiple servers
// bool AsyncA7670::tryNtpWithFallback(const char* primaryServer, const char*
// fallbackServer1, const char* fallbackServer2, int timeZone) {
//     // Try primary server
//     if (ntpServerSync(primaryServer, timeZone)) {
//         // Wait for result with timeout
//         uint32_t startTime = millis();
//         while (ntpStatus == NTPStatus::SYNCING && (millis() - startTime) <
//         15000) {
//             vTaskDelay(pdMS_TO_TICKS(100));
//         }
//         if (ntpStatus == NTPStatus::SYNCED) {
//             debugPrint("NTP sync successful with primary server: ");
//             debugPrintln(primaryServer);
//             return true;
//         }
//     }

//     // Try first fallback
//     debugPrint("Trying fallback server: ");
//     debugPrintln(fallbackServer1);
//     ntpStatus = NTPStatus::NOT_SYNCED; // Reset status
//     if (ntpServerSync(fallbackServer1, timeZone)) {
//         uint32_t startTime = millis();
//         while (ntpStatus == NTPStatus::SYNCING && (millis() - startTime) <
//         15000) {
//             vTaskDelay(pdMS_TO_TICKS(100));
//         }
//         if (ntpStatus == NTPStatus::SYNCED) {
//             debugPrint("NTP sync successful with fallback server: ");
//             debugPrintln(fallbackServer1);
//             return true;
//         }
//     }

//     // Try second fallback
//     debugPrint("Trying second fallback server: ");
//     debugPrintln(fallbackServer2);
//     ntpStatus = NTPStatus::NOT_SYNCED; // Reset status
//     if (ntpServerSync(fallbackServer2, timeZone)) {
//         uint32_t startTime = millis();
//         while (ntpStatus == NTPStatus::SYNCING && (millis() - startTime) <
//         15000) {
//             vTaskDelay(pdMS_TO_TICKS(100));
//         }
//         if (ntpStatus == NTPStatus::SYNCED) {
//             debugPrint("NTP sync successful with second fallback server: ");
//             debugPrintln(fallbackServer2);
//             return true;
//         }
//     }

//     debugPrintln("All NTP servers failed, will continue without time sync");
//     ntpStatus = NTPStatus::ERROR;
//     return false;
// }

// void AsyncA7670::checkNtpStatus() {
//     if (ntpStatus == NTPStatus::SYNCING) {
//         sendATCommand("AT+CCLK?");  // Get current time
//     }
// }

void AsyncA7670::processIncomingData() {
  if (!_stream.available())
    return;
  if (_drainingPayload)
    return;

  // Process a bounded chunk per call so this high-priority task cannot
  // monopolize the CPU.
  static constexpr size_t kMaxBytesPerCycle = 512;
  size_t processedBytes = 0;
  while (_stream.available() && processedBytes < kMaxBytesPerCycle) {
    char c = _stream.read();
    processedBytes++;

    // '>' prompt can arrive without CRLF after CIPSEND command.
    if (_awaitingSendPrompt && c == '>') {
      _sendPromptReceived = true;
      _awaitingSendPrompt = false;
      continue;
    }

    // Accumulate until '\n'; treat "\r\n" as single EOL.
    if (c == '\n') {
      // Strip a possible preceding '\r'
      if (_responseIndex > 0 && _responseBuffer[_responseIndex - 1] == '\r') {
        _responseIndex--;
      }
      _responseBuffer[_responseIndex] = '\0';

      // Ignore empty lines, else parse
      if (_responseIndex > 0) {
        parseResponse(_responseBuffer);
      }
      _responseIndex = 0;
      _responseBuffer[0] = '\0';
    } else {
      // regular byte into response line buffer (guard against overflow)
      if (_responseIndex < sizeof(_responseBuffer) - 1) {
        _responseBuffer[_responseIndex++] = c;
        _responseBuffer[_responseIndex] = '\0';
      } else {
        // overflow safety: reset line
        _responseIndex = 0;
        _responseBuffer[0] = '\0';
      }
    }
  }
}

/**
 * Parse a +RECEIVE URC of the form "+RECEIVE,<mux>,<len>"
 * Returns true if parse successful, fills mux and length.
 */
bool AsyncA7670::parseReceiveHeader(const String &line, int &mux, int &length) {
  mux = -1;
  length = 0;

  // Expected format: +RECEIVE,<mux>,<len>
  int firstComma = line.indexOf(',');
  int secondComma = (firstComma >= 0) ? line.indexOf(',', firstComma + 1) : -1;

  if (firstComma < 0 || secondComma < 0) {
    return false;
  }

  // Extract mux and length
  String muxStr = line.substring(firstComma + 1, secondComma);
  String lenStr = line.substring(secondComma + 1);

  mux = muxStr.toInt();
  length = lenStr.toInt();

  return (mux >= 0 && length > 0);
}

// Consume at most one CRLF (or single LF) that separates the header line and
// the payload. This must be called only when we have exclusive ownership of the
// UART stream (mutex held). Return how many bytes were consumed (0,1 or 2).
static inline size_t consumeOptionalSeparator(Stream &s,
                                              uint32_t timeoutMs = 50) {
  // Wait a little for data (modal, but short)
  uint32_t start = millis();
  while (s.available() == 0) {
    if (millis() - start > timeoutMs)
      return 0;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  int p = s.peek();
  if (p == '\r') {
    s.read(); // consume '\r'
    // try to consume following '\n' if present
    if (s.available()) {
      if (s.peek() == '\n') {
        s.read();
        return 2;
      }
    } else {
      // wait a tiny bit for the '\n'
      start = millis();
      while (millis() - start < 20 && s.available() == 0)
        vTaskDelay(pdMS_TO_TICKS(1));
      if (s.available() && s.peek() == '\n') {
        s.read();
        return 2;
      }
    }
    return 1;
  } else if (p == '\n') {
    s.read(); // consume lone '\n'
    return 1;
  }
  return 0;
}

void AsyncA7670::parseResponse(const char *response) {
  if (!response || strlen(response) == 0) {
    return;
  }

  // Remove leading/trailing whitespace
  String resp = String(response);
  resp.trim();

  // GNSS/NMEA streams can be very chatty; ignore raw NMEA lines to avoid
  // log/CPU saturation.
  if (resp.startsWith("$")) {
    return;
  }

  debugPrint("RX: ");
  debugPrintln(response);

  if (resp.equals("OK")) {
    // Command completed successfully
    return;
  } else if (resp.equals("ERROR")) {
    // Command failed
    return;
  } else if (resp.startsWith("+NETOPEN:")) {
    // Network status response
    // Format: +NETOPEN: <status>[,<err>]
    // status: 0=network closed, 1=network opened
    // err: error code if status is 0
    int status = -1, error = -1;
    if (sscanf(resp.c_str(), "+NETOPEN: %d,%d", &status, &error) >= 1) {
      if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
          pdTRUE)
        return;
      if (status == 1) {
        // Network is open - set to connected regardless of previous status
        gprsStatus = GPRSStatus::CONNECTED;
        debugPrintln("GPRS network is open");
        xSemaphoreGive(_statusMutex);

        // Now that network is open, get IP address
        debugPrintln("Getting IP address...");
        sendATCommand("AT+IPADDR");
        return; // Exit early since we already released mutex
      } else if (status == 0) {
        // Network reported as closed, but check if IP is actually available
        debugPrintln("NETOPEN reported status 0, checking IP address...");
        xSemaphoreGive(_statusMutex);

        // Avoid blocking in the response parser task; just query IP
        // immediately.
        sendATCommand("AT+IPADDR");
        return; // Don't set DISCONNECTED yet, let IPADDR response determine
                // status
      }
      xSemaphoreGive(_statusMutex);
    }
  } else if (resp.startsWith("+IPADDR:")) {
    // IP address received
    int colonIndex = resp.indexOf(':');
    if (colonIndex > 0 && colonIndex < resp.length() - 1) {
      String newIP = resp.substring(colonIndex + 1);
      newIP.trim();

      if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
          pdTRUE)
        return;

      // Validate IP address
      if (newIP.length() > 0 && newIP != "0.0.0.0" && newIP != "ERROR") {
        _localIP = newIP;
        // If we have a valid IP and we're connecting, mark as connected
        gprsStatus = GPRSStatus::CONNECTED;
        debugPrint("Valid IP address: ");
        debugPrintln(_localIP.c_str());
      } else {
        // Invalid IP, mark as disconnected
        _localIP = "";
        gprsStatus = GPRSStatus::DISCONNECTED;
        debugPrint("Invalid IP address received: ");
        debugPrintln(newIP.c_str());
      }
      xSemaphoreGive(_statusMutex);
    } else {
      // No IP address in response or malformed response
      if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
          pdTRUE)
        return;
      _localIP = "";
      gprsStatus = GPRSStatus::DISCONNECTED;
      debugPrintln("No IP address available");
      xSemaphoreGive(_statusMutex);
    }
  } else if (resp.startsWith("+CDNSGIP:")) {
    // DNS resolution result
    // Format: +CDNSGIP: <status>,<domain>,<IP1>[,<IP2>,...]
    // status: 1=success, 0=failed
    int status = -1;
    String domain, resolvedIP;

    int firstComma = resp.indexOf(',');
    int secondComma = resp.indexOf(',', firstComma + 1);

    if (firstComma > 0 && secondComma > firstComma) {
      status = resp.substring(resp.indexOf(':') + 1, firstComma).toInt();
      domain = resp.substring(firstComma + 1, secondComma);
      domain.replace("\"", ""); // Remove quotes

      int thirdComma = resp.indexOf(',', secondComma + 1);
      if (thirdComma > 0) {
        resolvedIP = resp.substring(secondComma + 1, thirdComma);
      } else {
        resolvedIP = resp.substring(secondComma + 1);
      }
      resolvedIP.replace("\"", ""); // Remove quotes
      resolvedIP.trim();

      debugPrint("DNS resolution for ");
      debugPrint(domain.c_str());
      if (status == 1 && resolvedIP.length() > 0) {
        debugPrint(" succeeded: ");
        debugPrintln(resolvedIP.c_str());
      } else {
        debugPrintln(" failed");
      }
      _resolvedHostIp = resolvedIP;
    }
  } else if (resp.startsWith("+CIPOPEN:")) {
    // TCP connection result
    int mux = -1, result = -1;
    if (sscanf(resp.c_str(), "+CIPOPEN: %d,%d", &mux, &result) == 2) {
      if (mux >= 0 && mux < 10) {
        if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
            pdTRUE)
          return;
        if (result == 0) {
          tcpStatus[mux] = TCPStatus::CONNECTED;
          tcpConnections[mux].connected = true;
          tcpConnections[mux].retryCount = 0; // Reset retry counter on success

          debugPrint("TCP connection ");
          debugPrint(String(mux).c_str());
          debugPrintln(" established");
        } else {
          tcpStatus[mux] = TCPStatus::ERROR;
          tcpConnections[mux].connected = false;
          debugPrint("TCP connection ");
          debugPrint(String(mux).c_str());
          debugPrint(" failed with error ");
          debugPrintln(String(result).c_str());

          // Special handling for error 4 (network error)
          if (result == 4) {
            debugPrintln(
              "Error 4 detected - will retry TCP connection after network "
              "verification");
            // Keep status as CONNECTING to allow retry
            tcpStatus[mux] = TCPStatus::CONNECTING;
            // The main loop will retry the connection automatically
          }
        }
        xSemaphoreGive(_statusMutex);
      }
    }
  } else if (resp.startsWith("+CIPSEND:")) {
    // Send operation result
    int mux = -1, requested = -1, confirmed = -1;
    if (sscanf(resp.c_str(), "+CIPSEND: %d,%d,%d", &mux, &requested,
               &confirmed) == 3) {
      _sendConfirmReceived = true;
      _lastSendConfirmed = confirmed;
    }
  } else if (resp.startsWith("+CIPCLOSE:")) {
    // Handle two different formats:
    // 1. URC format: +CIPCLOSE: <mux>,<cause> (connection closed notification)
    // 2. Query response: +CIPCLOSE: <status0>,<status1>,...,<status9> (all
    // connection statuses)

    // Check if this is a query response (contains multiple comma-separated
    // values)
    int commaCount = 0;
    for (int i = 0; i < resp.length(); i++) {
      if (resp.charAt(i) == ',')
        commaCount++;
    }

    if (commaCount >= 9) {
      // Query response format: +CIPCLOSE: <status0>,<status1>,...,<status9>
      // Parse all 10 connection statuses
      String statusPart = resp.substring(resp.indexOf(':') + 1);
      statusPart.trim();

      int statusValues[10];
      int parseIndex = 0;
      int valueIndex = 0;
      String currentValue = "";

      // Parse comma-separated values
      for (int i = 0; i <= statusPart.length() && valueIndex < 10; i++) {
        if (i == statusPart.length() || statusPart.charAt(i) == ',') {
          statusValues[valueIndex] = currentValue.toInt();
          valueIndex++;
          currentValue = "";
        } else {
          currentValue += statusPart.charAt(i);
        }
      }

      // Update all connection statuses
      if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
          pdTRUE)
        return;
      for (int mux = 0; mux < 10 && mux < valueIndex; mux++) {
        if (statusValues[mux] == 1) {
          // Connection is active
          tcpStatus[mux] = TCPStatus::CONNECTED;
          tcpConnections[mux].connected = true;
          // Don't reset bytesAvailable as there might be pending data
        } else {
          // Connection is closed
          tcpStatus[mux] = TCPStatus::CLOSED;
          tcpConnections[mux].connected = false;
        }
      }
      xSemaphoreGive(_statusMutex);
    } else {
      // URC format: +CIPCLOSE: <mux>,<cause> (existing code)
      int mux = -1, cause = -1;
      if (sscanf(resp.c_str(), "+CIPCLOSE: %d,%d", &mux, &cause) == 2) {
        if (mux >= 0 && mux < 10) {
          if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
              pdTRUE)
            return;
          tcpStatus[mux] = TCPStatus::CLOSED;
          tcpConnections[mux].connected = false;
          xSemaphoreGive(_statusMutex);

          debugPrint("TCP connection ");
          debugPrint(String(mux).c_str());
          debugPrint(" closed by remote (cause: ");
          debugPrint(String(cause).c_str());
          debugPrintln(")");
        }
      }
    }
  } else if (resp.startsWith("+RECEIVE")) {
    // URC: +RECEIVE,<mux>,<len>
    // In CIPRXGET=1 mode this is only a notification,
    // we should NOT directly collect data here.
    int mux = -1, length = 0;
    if (parseReceiveHeader(resp, mux, length)) {
      if (mux >= 0 && mux < 10) {
        if (_debugStream) {
          _debugStream->print("URC +RECEIVE: mux=");
          _debugStream->print(mux);
          _debugStream->print(" len=");
          _debugStream->println(length);
        }
        // Just mark data as pending, maintainClient() will pull via CIPRXGET
        tcpConnections[mux].got_data = true;
        tcpConnections[mux].expectedDataLength = length;
        tcpConnections[mux].got_data = true;
        tcpConnections[mux].expectedDataLength = length;

        // mark as pending in modem, not in local buffer
        tcpConnections[mux].modemAvailable += (size_t)length;
      }
    }
    return;
  } else if (resp.startsWith("+CNTP:")) {
    // NTP sync result
    int result = -1;
    if (sscanf(resp.c_str(), "+CNTP: %d", &result) == 1) {
      if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
          pdTRUE)
        return;
      if (result == 0) {
        ntpStatus = NTPStatus::SYNCED;
        debugPrintln("NTP sync successful");
        xSemaphoreGive(_statusMutex);
        // // Query the current time after successful sync
        // sendATCommand("AT+CCLK?");
        // // Auto-sync ESP32 system clock
        // vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for CCLK response
        // if (syncSystemClock()) {
        //     debugPrintln("ESP32 system clock automatically synced with
        //     network time");
        // }
      } else {
        ntpStatus = NTPStatus::ERROR;
        debugPrint("NTP sync failed with error ");
        debugPrintln(String(result).c_str());
        xSemaphoreGive(_statusMutex);
      }
    }
  } else if (resp.startsWith("+CCLK:")) {
    // Clock reading
    int quoteStart = resp.indexOf('"');
    int quoteEnd = resp.lastIndexOf('"');
    if (quoteStart >= 0 && quoteEnd > quoteStart) {
      if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
          pdTRUE)
        return;
      _dateTime = resp.substring(quoteStart + 1, quoteEnd);

      // // If we successfully read the time and NTP was syncing, mark as synced
      // if (ntpStatus == NTPStatus::SYNCING && _dateTime.length() > 0) {
      //     ntpStatus = NTPStatus::SYNCED;
      //     debugPrintln("NTP status updated to SYNCED after successful CCLK
      //     read");
      // }
      xSemaphoreGive(_statusMutex);
      debugPrint("Date/Time: ");
      debugPrintln(_dateTime.c_str());
    }
  } else if (resp.startsWith("+CGACT:")) {
    // PDP context activation status
    // Format: +CGACT: <cid>,<state>
    // cid: context identifier, state: 0=deactivated, 1=activated
    int cid = -1, state = -1;
    if (sscanf(resp.c_str(), "+CGACT: %d,%d", &cid, &state) == 2) {
      if (cid == 1) { // We use context 1 for GPRS
        if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
            pdTRUE)
          return;
        if (state == 0) {
          // PDP context deactivated
          gprsStatus = GPRSStatus::DISCONNECTED;
          _localIP = "";
          debugPrintln("PDP context deactivated");
        } else if (state == 1) {
          // PDP context activated
          debugPrintln("PDP context activated");
          // Don't automatically set to connected - wait for NETOPEN and IP
          // confirmation
        }
        xSemaphoreGive(_statusMutex);
      }
    }
  } else if (resp.startsWith("+CGREG:")) {
    // Network registration status
    // Format: +CGREG: <n>,<stat>[,<lac>,<ci>[,<AcT>]]
    // stat: 0=not searching, 1=registered home, 2=searching, 3=denied,
    // 5=registered roaming
    int n = -1, stat = -1;
    if (sscanf(resp.c_str(), "+CGREG: %d,%d", &n, &stat) == 2) {
      if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
          pdTRUE)
        return;
      if (stat == 0 || stat == 2 || stat == 3 || stat == 4 || stat == 6 ||
          stat == 11) {
        // Not registered or searching or denied or unknown or not registered
        // for other reasons
        netRegStatus = false;
        if (gprsStatus == GPRSStatus::CONNECTED ||
            gprsStatus == GPRSStatus::CONNECTING) {
          gprsStatus = GPRSStatus::DISCONNECTED;
          _localIP = "";
          debugPrint("Network registration lost, status: ");
          debugPrintln(String(stat).c_str());
        }
      } else if (stat == 1 || stat == 5) {
        // Registered (home or roaming)
        netRegStatus = true;
        debugPrint("Network registered, status: ");
        debugPrintln(String(stat).c_str());
        // Don't automatically set to connected - this is just registration
      }
      xSemaphoreGive(_statusMutex);
    }
  } else if (resp.startsWith("+CSQ:")) {
    // Signal quality response
    // Format: +CSQ: <rssi>,<ber>
    // rssi: 0-31 (signal strength), 99 = unknown/not detectable
    // ber: bit error rate (usually not used)
    int rssi = 0, ber = 0;
    if (sscanf(resp.c_str(), "+CSQ: %d,%d", &rssi, &ber) == 2) {
      if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
          pdTRUE)
        return;

      if (rssi == 99) {
        // Signal not detectable
        _signalQuality = 0;
        _signalStrength = -999;
        debugPrintln("Signal not detectable");
      } else if (rssi >= 0 && rssi <= 31) {
        // Convert RSSI to signal quality percentage
        // Using a more realistic mapping:
        // 0-1: 0-5%, 2-9: 5-25%, 10-14: 25-50%, 15-19: 50-75%, 20-31: 75-100%
        int quality;
        if (rssi <= 1) {
          quality = rssi * 5; // 0-5%
        } else if (rssi <= 9) {
          quality = 5 + ((rssi - 2) * 20 / 7); // 5-25%
        } else if (rssi <= 14) {
          quality = 25 + ((rssi - 10) * 25 / 4); // 25-50%
        } else if (rssi <= 19) {
          quality = 50 + ((rssi - 15) * 25 / 4); // 50-75%
        } else {
          quality = 75 + ((rssi - 20) * 25 / 11); // 75-100%
        }

        // Ensure within bounds
        if (quality > 100)
          quality = 100;
        if (quality < 0)
          quality = 0;

        // Convert RSSI to dBm: dBm = -113 + (2 * rssi)
        int dbm = -113 + (2 * rssi);

        _signalQuality = quality;
        _signalStrength = dbm;

        debugPrint("Signal update: RSSI=");
        debugPrint(String(rssi).c_str());
        debugPrint(", Quality=");
        debugPrint(String(quality).c_str());
        debugPrint("%, Strength=");
        debugPrint(String(dbm).c_str());
        debugPrintln(" dBm");
      } else {
        debugPrint("Invalid RSSI value: ");
        debugPrintln(String(rssi).c_str());
      }

      xSemaphoreGive(_statusMutex);
    }
  } else if (resp.startsWith("+CIPRXGET:")) {
    // +CIPRXGET: 1,<mux>              (URC: data arrived)
    // +CIPRXGET: 4,<mux>,<avail>      (available bytes in modem)
    // +CIPRXGET: 2,<mux>,<read>,<rest>(response to read; raw payload follows)

    int idx = resp.indexOf(':');
    int mode = resp.substring(idx + 1).toInt();

    if (mode == 1) {
      // URC: data arrived
      int comma = resp.indexOf(',', idx + 1);
      int mux = resp.substring(comma + 1).toInt();
      if (mux >= 0 && mux < 10) {
        tcpConnections[mux].got_data = true;
        if (_debugStream) {
          _debugStream->print("URC +CIPRXGET: 1 -> mux=");
          _debugStream->print(mux);
          _debugStream->println(" (got_data=true)");
        }
      }
      return;
    } else if (mode == 4) {
      // +CIPRXGET: 4,<mux>,<avail>
      int p1 = resp.indexOf(',', idx + 1);
      int p2 = resp.indexOf(',', p1 + 1);
      int mux = resp.substring(p1 + 1, p2).toInt();
      int avail = resp.substring(p2 + 1).toInt();

      if (mux >= 0 && mux < 10) {
        tcpConnections[mux].modemAvailableRequested =
          true; // Mark that we have queried available data
        tcpConnections[mux].modemAvailable = (size_t)avail;
      }
      return;
    } else if (mode == 2) {
      // +CIPRXGET: 2,<mux>,<read_len>,<rest_len>
      int p1 = resp.indexOf(',', idx + 1);
      int p2 = resp.indexOf(',', p1 + 1);
      int p3 = resp.indexOf(',', p2 + 1);

      int mux = resp.substring(p1 + 1, p2).toInt();
      int read_len = resp.substring(p2 + 1, p3).toInt();
      int rest_len = resp.substring(p3 + 1).toInt();

      if (mux >= 0 && mux < 10 && read_len > 0) {
        auto &conn = tcpConnections[mux];
        if (read_len > 0) {
          if (_debugStream) {
            _debugStream->print("parseResponse[");
            _debugStream->print(mux);
            _debugStream->print("]: CIPRXGET=2, read_len=");
            _debugStream->print(read_len);
            _debugStream->print(", rest_len=");
            _debugStream->println(rest_len);
          }
          // Prevent any other reader (e.g., response task) from touching the
          // UART during binary drain.
          _drainingPayload = true;
          collectTcpData((uint8_t)mux, (size_t)read_len, /*maxWaitMs=*/10000);
          _drainingPayload = false;
          // Marcar estado de data entrante
          if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
              pdTRUE)
            return;
          conn.incomingData = true;
          conn.incomingDataLength = (size_t)read_len;
          conn.modemAvailable = (size_t)rest_len;
          conn.got_data =
            rest_len > 0; // If there's rest_len, we expect more data later
          xSemaphoreGive(_statusMutex);
        }
      }
      return;
    }
  } else if (resp.equals("PB DONE")) {
    // Modem restart completed (response to AT+CRESET)
    debugPrintln("Modem restart completed");
    if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
        pdTRUE)
      return;
    modemRestarted = true;
    xSemaphoreGive(_statusMutex);
  } else if (resp.equals("+CGNSSPWR: READY!")) {
    debugPrintln("GNSS is ready");
    if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
        pdTRUE)
      return;
    gnssEnabled = true;
    xSemaphoreGive(_statusMutex);
  } else if (resp.startsWith("+CGNSSINFO:")) {
    int colonPos = resp.indexOf(':');
    String dataPart =
      (colonPos >= 0) ? resp.substring(colonPos + 1) : String("");
    dataPart.trim();

    gnssData.rawData = dataPart;
    gnssData.fixQuality = 0;
    _gnssInfoPending = false;

    if (dataPart.length() > 0) {
      int commaPos = dataPart.indexOf(',');
      String fixField =
        (commaPos >= 0) ? dataPart.substring(0, commaPos) : dataPart;
      fixField.trim();
      gnssData.fixQuality = static_cast<uint8_t>(fixField.toInt());
    }

    gnssDataReceived = true;
    return;
  }

  return;
}

void AsyncA7670::collectTcpData(uint8_t mux, size_t len, uint32_t maxWaitMs) {
  if (mux >= 10 || !clients[mux])
    return;

  const uint32_t deadline = millis() + maxWaitMs;
  size_t got = 0;
  if (_debugStream) {
    _debugStream->print("collectTcpData[");
    _debugStream->print(mux);
    _debugStream->print("]: collecting ");
    _debugStream->print(len);
    _debugStream->print(" bytes, stream.available()=");
    _debugStream->println(_stream.available());
  }

  while (got < len && (int32_t)(deadline - millis()) > 0) {
    int avail = _stream.available();
    if (avail <= 0) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }
    char c = _stream.read();
    clients[mux]->_rxFifo.put(c);
    got++;
  }
  if (got < len && _debugStream) {
    _debugStream->print("Timeout waiting for TCP data: expected ");
    _debugStream->print(len);
    _debugStream->print(" bytes, got ");
    _debugStream->print(got);
    _debugStream->print(" bytes in ");
    _debugStream->print(maxWaitMs - (int32_t)(deadline - millis()));
    _debugStream->println(" ms");
  } else if (_debugStream) {
    _debugStream->print("collectTcpData[");
    _debugStream->print(mux);
    _debugStream->print("]: collected ");
    _debugStream->print(got);
    _debugStream->println(" bytes");
  }
}

String AsyncA7670::getModemInfo() {
  // This would send AT+GMI, AT+GMM, AT+GMR commands and collect responses
  return "SIMCom A7670 Module";
}

int AsyncA7670::getSignalQuality() {
  // Return cached signal quality, request update if needed
  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return -1;
  }
  int cachedQuality = _signalQuality;
  xSemaphoreGive(_statusMutex);

  // If we don't have a valid cached value, request an update
  if (cachedQuality == -1) {
    // Send AT+CSQ command to trigger signal quality update
    sendATCommand("AT+CSQ", 3000);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for response processing

    // Return updated cached value
    if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
        pdTRUE) {
      return cachedQuality;
    }
    cachedQuality = _signalQuality;
    xSemaphoreGive(_statusMutex);
  }

  return cachedQuality;
}

int AsyncA7670::getSignalStrength() {
  // Return cached signal strength, request update if needed
  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return -999;
  }
  int cachedStrength = _signalStrength;
  xSemaphoreGive(_statusMutex);

  // If we don't have a valid cached value, request an update
  if (cachedStrength == -999) {
    // Send AT+CSQ command to trigger signal strength update
    sendATCommand("AT+CSQ", 3000);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for response processing

    // Return updated cached value
    if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
        pdTRUE) {
      return cachedStrength;
    }
    cachedStrength = _signalStrength;
    xSemaphoreGive(_statusMutex);
  }

  return cachedStrength;
}

bool AsyncA7670::isModemAlive() { return modemReady; }

void AsyncA7670::debugPrint(const char *message) {
  if (!_debugStream || !message)
    return;
  int writable = _debugStream->availableForWrite();
  if (writable <= 0)
    return;
  size_t len = strlen(message);
  size_t toWrite =
    (len < static_cast<size_t>(writable)) ? len : static_cast<size_t>(writable);
  if (toWrite > 0) {
    _debugStream->write(reinterpret_cast<const uint8_t *>(message), toWrite);
  }
}

void AsyncA7670::debugPrintln(const char *message) {
  if (!_debugStream)
    return;
  if (message && message[0] != '\0') {
    debugPrint(message);
  }
  int writable = _debugStream->availableForWrite();
  if (writable > 0) {
    _debugStream->write('\n');
  }
}

bool AsyncA7670::restart(const char *pin) {
  debugPrintln("Restarting modem...");

  // Reset internal state
  if (xSemaphoreTake(_statusMutex, A7670_STATUS_MUTEX_TIMEOUT_TICKS) !=
      pdTRUE) {
    return false;
  }
  gprsStatus = GPRSStatus::DISCONNECTED;
  ntpStatus = NTPStatus::NOT_SYNCED;
  netRegStatus = false;
  modemRestarted = false; // Reset restart status
  // Reset all TCP connections
  for (int i = 0; i < 10; i++) {
    tcpStatus[i] = TCPStatus::CLOSED;
  }
  xSemaphoreGive(_statusMutex);

  // Send AT command to restart
  bool success = sendATCommand("AT+CRESET");

  // Note: modemRestarted will be set to true when the modem responds to basic
  // AT commands The calling code should use
  // waitForAsyncStatus(modemAsync.modemRestarted, true, ...)

  return success;
}

bool AsyncA7670::waitForNetwork(uint32_t timeout_ms) {
  uint32_t startTime = millis();

  while ((millis() - startTime) < timeout_ms) {
    if (isNetworkConnected()) {
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  return false;
}

bool AsyncA7670::begin(const char *pin) {
  // Initialize the modem with PIN
  if (!initialized) {
    if (!begin()) {
      return false;
    }
  }

  if (pin && strlen(pin) > 0) {
    String pinCmd = "AT+CPIN=\"" + String(pin) + "\"";
    sendATCommand(pinCmd.c_str());
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  return true;
}

// // Helper function to sync ESP32 system clock with network time
// bool AsyncA7670::syncSystemClock() {
//     int year, month, day, hour, minute, second;
//     float timezone;

//     if (!getNetworkTime(&year, &month, &day, &hour, &minute, &second,
//     &timezone)) {
//         debugPrintln("syncSystemClock: Failed to get network time");
//         return false;
//     }

//     // Convert to Unix timestamp
//     struct tm timeinfo = {};
//     timeinfo.tm_year = year - 1900;  // tm_year is years since 1900
//     timeinfo.tm_mon = month - 1;     // tm_mon is 0-11
//     timeinfo.tm_mday = day;
//     timeinfo.tm_hour = hour;
//     timeinfo.tm_min = minute;
//     timeinfo.tm_sec = second;
//     timeinfo.tm_isdst = -1;          // Let system determine DST

//     time_t timestamp = mktime(&timeinfo);
//     if (timestamp == -1) {
//         debugPrintln("syncSystemClock: Failed to convert time to timestamp");
//         return false;
//     }

//     // Apply timezone offset (convert quarters to seconds)
//     int timezoneOffsetSeconds = (int)(timezone * 3600);  // Convert hours to
//     seconds timestamp -= timezoneOffsetSeconds;  // Adjust to UTC

//     // Set ESP32 system time
//     struct timeval tv;
//     tv.tv_sec = timestamp;
//     tv.tv_usec = 0;

//     if (settimeofday(&tv, NULL) == 0) {
//         debugPrint("syncSystemClock: Successfully synced ESP32 clock to ");
//         debugPrint(String(year).c_str());
//         debugPrint("-");
//         debugPrint(String(month).c_str());
//         debugPrint("-");
//         debugPrint(String(day).c_str());
//         debugPrint(" ");
//         debugPrint(String(hour).c_str());
//         debugPrint(":");
//         debugPrint(String(minute).c_str());
//         debugPrint(":");
//         debugPrint(String(second).c_str());
//         debugPrint(" (TZ: ");
//         debugPrint(String(timezone).c_str());
//         debugPrintln(")");

//         // Verify the sync worked
//         time_t now = time(nullptr);
//         debugPrint("syncSystemClock: ESP32 system time now: ");
//         debugPrintln(String((unsigned long)now).c_str());

//         return true;
//     } else {
//         debugPrintln("syncSystemClock: Failed to set system time");
//         return false;
//     }
// }

// GPS function placeholders (to be implemented later)
bool AsyncA7670::enableGnss() {
  debugPrintln("Powering GNSS On");
  sendATCommand("AT+CGNSSPWR=0"); // Power off first to reset
  vTaskDelay(pdMS_TO_TICKS(1000));
  return sendATCommand("AT+CGNSSPWR=1");
}

bool AsyncA7670::setGnssMode(int mode) {
  debugPrintln("Setting GNSS Mode");
  if (!gnssEnabled) {
    debugPrintln("GNSS not enabled, cannot set mode");
    return false;
  }
  String cmd = "AT+CGNSSMODE=" + String(mode);
  return sendATCommand(cmd.c_str());
}

bool AsyncA7670::getGnssData() {
  debugPrintln("Getting GNSS Data");
  if (!gnssEnabled) {
    debugPrintln("GNSS not enabled, cannot get data");
    return false;
  }
  // Prevent command bursts when multiple tasks request GNSS almost
  // simultaneously.
  uint32_t now = millis();
  if (_gnssInfoPending && (now - _gnssInfoSentMs) < 4000) {
    return true;
  }
  _gnssInfoPending = true;
  _gnssInfoSentMs = now;
  return sendATCommand("AT+CGNSSINFO", 9000, true);
}

// Client maintenance function
void AsyncA7670::maintainClient(uint8_t mux) {
  if (mux >= 10 || !clients[mux]) {
    if (_debugStream) {
      _debugStream->print("maintainClient[");
      _debugStream->print(mux);
      _debugStream->print("]: early exit - mux>=10:");
      _debugStream->print(mux >= 10);
      _debugStream->print(" !clients[mux]:");
      _debugStream->println(!clients[mux]);
    }
    return;
  }

  TCPConnection &conn = tcpConnections[mux];

  if (!conn.connected) {
    if (_debugStream) {
      _debugStream->print("maintainClient[");
      _debugStream->print(mux);
      _debugStream->println("]: early exit - not connected");
    }
    return;
  }

  if (conn.got_data) {
    conn.got_data = false;                // reset flag
    conn.modemAvailableRequested = false; // reset flag
    time_t start = millis();
    char cmd[24];
    snprintf(cmd, sizeof(cmd), "AT+CIPRXGET=4,%u", mux);
    sendATCommand(cmd, A7670_RESPONSE_TIMEOUT_MS / 2, true);
    while (!conn.modemAvailableRequested && (millis() - start) < 2000) {
      vTaskDelay(pdMS_TO_TICKS(1)); // Wait for data to arrive
    }
    debugPrint("Modem data available: ");
    debugPrint(String(conn.modemAvailable).c_str());
    debugPrintln(" bytes");
  }
}
