#include <modules/startup/StartupLayrzHub.h>

static bool mountSharedSdCard(uint32_t &mountedFreqHz) {
  static const uint32_t kSdMountFreqs[] = {4000000UL, 10000000UL, 20000000UL};
  static const uint8_t kSdMaxOpenFiles = 10;
  mountedFreqHz = 0;

  for (uint32_t freq : kSdMountFreqs) {
    digitalWrite(ARDUCAM_CS, HIGH);
    digitalWrite(SD_CS, HIGH);
    vTaskDelay(pdMS_TO_TICKS(2));

    SPI.end();
    vTaskDelay(pdMS_TO_TICKS(2));
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
    pinMode(ARDUCAM_CS, OUTPUT);
    digitalWrite(ARDUCAM_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    pinMode(SPI_MISO, INPUT_PULLUP);
    vTaskDelay(pdMS_TO_TICKS(20));

    if (SD.begin(SD_CS, SPI, freq, "/sd", kSdMaxOpenFiles)) {
      mountedFreqHz = freq;
      return true;
    }

    SD.end();
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  return false;
}

void StartupLayrzHub::InitUsbCommunications() {
  UARTSerial.begin(115200);
  while (!UARTSerial) {
    delay(10);
  }
  UARTSerial.setDebugOutput(true);
  UARTSerial.printf("Device ident: %s\n", getBluetoothMACAddress().c_str());
}

void StartupLayrzHub::checkFactoryResetOnBoot() {
  UARTSerial.println("Checking for factory reset condition...");
  pinMode(0, INPUT_PULLUP);
  delay(1000); // Debounce delay
  if (digitalRead(0) == LOW) {
    UARTSerial.println("Factory reset...");
    MorseCode morse(LED, hubSettings.rgb_count);
    deviceSettings::setFactorySettings();
    morse.display(0, "05", 0xFF6600, 0xFF6600);
    delay(1000);
    ESP.restart();
  }
}

void StartupLayrzHub::initDeviceSettings() {
  deviceSettings::initSettingsBuffer();
  deviceSettings::initDeviceSettings();
  if (hubSettings.zigbee_local_en) {
    hubSettings.sys_debug_en = false;
    UARTSerial.setDebugOutput(false);
  } else {
    UARTSerial.setDebugOutput(hubSettings.sys_debug_en);
  }
  // Now print to console the current settings if debug is enabled
  if (hubSettings.sys_debug_en) {
    std::string currentSettings = deviceSettings::getDeviceSettings();
    std::vector<std::string> settingsArray = splitString(currentSettings, ",");
    if (Print *console = debugConsole()) {
      console->println("Current settings: ");
      for (const auto &setting : settingsArray) {
        console->printf("%s\n", setting.c_str());
      }
    }
  }
}

void StartupLayrzHub::resetTime() {
  struct timeval tv;
  tv.tv_sec = 0;  // Reset seconds to 0
  tv.tv_usec = 0; // Reset microseconds to 0
  settimeofday(&tv, NULL);
}

void StartupLayrzHub::initRgbLed(const char *greeting) {
  const int rgbLevels[6] = {0, 1, 5, 40, 100, 255};
  rgbBrightness = rgbLevels[hubSettings.rgb_brightness];
  if (hubSettings.rgb_en) {
    rgbLed.begin();
    rgbLed.setBrightness(rgbBrightness);
    MorseCode morse(LED, hubSettings.rgb_count);
    delay(1000);
    morse.display(0, greeting);
    rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
    rgbLed.show();
  }
}

void StartupLayrzHub::initWatchdogTimer(int timeout) {
  esp_task_wdt_init(timeout,
                    true); // The 'true' parameter enables panic handling
}

bool StartupLayrzHub::createBinarySemaphore() {
  // Create a binary semaphore
  xSemaphore = xSemaphoreCreateBinary();
  if (xSemaphore == NULL) {
    debugPrint("Semaphore creation failed!\n");
    return false;
  }
  xSemaphoreGive(xSemaphore);
  return true;
}

void StartupLayrzHub::startSystemHealthMonitor() {
  if (systemHealthMonitorHandle != nullptr) {
    return;
  }
  xTaskCreatePinnedToCore(systemHealthMonitorTask, "sys_health", 4096, nullptr,
                          4, &systemHealthMonitorHandle, 1);
}

void StartupLayrzHub::startConfiotOverBLE() {
  xTaskCreatePinnedToCore(confiotOverBLE, "confiotOverBLE", 5120, NULL, 0,
                          &confiotOverBleHandle, 1);
}

void StartupLayrzHub::initA7670Module() {
  if (hubSettings.net_mode == "cellular" || hubSettings.gnss_en) {
    debugPrint("Initializing A7670 GPS & 4G module \n");
    UART_COMM.setRxBufferSize(8192);
    UART_COMM.setTxBufferSize(512);

    UART_COMM.begin(115200, SERIAL_8N1, COMM_RX, COMM_TX, false);
    delay(1000);

    // Initialize AsyncA7670 library (new async implementation)
    debugPrint("Initializing AsyncA7670 library...\n");
    // modemAsync.setDebugStream(&UARTSerial);  // Set debug output to
    // UARTSerial (USB-UART)
    modemAsync.setDebugStream(nullptr);
    if (modemAsync.begin()) {
      debugPrint("AsyncA7670 initialized successfully\n");
    } else {
      debugPrint("AsyncA7670 initialization failed\n");
    }
  }
  if (hubSettings.net_mode == "cellular") {
    debugPrint("Registering to cellular network...\n");
    if (modemAsync.registerNetwork()) {
      debugPrint("Registered to cellular network successfully\n");
    } else {
      debugPrint("Cellular network registration failed\n");
    }
  }
}

void StartupLayrzHub::initGnss() {
  if (hubSettings.gnss_en) {
    GNSS::initGNSS(hubSettings.gnss_mode);
  }
}

void StartupLayrzHub::startTcpSocketListener() {
  if (hubSettings.net_protocol == 0) {
    xTaskCreatePinnedToCore(MessageBusLayrzHub::tcpSocketReception,
                            "tcpSocketReception", 4096, NULL, 20,
                            &tcpSocketReceptionHandle, 0);
  }
}

void StartupLayrzHub::connectToNetwork(std::string netmode) {
  if (netmode == "wifi") {

    WiFi.setHostname(hubSettings.sys_dev_name.c_str());
    int wifiTrials = 0;
    while (WiFi.status() != WL_CONNECTED && wifiTrials < 3) {
      NetLayrzHub::connectWiFi(
        hubSettings.wifi_ssid.c_str(), hubSettings.wifi_pass.c_str(),
        hubSettings.sys_ntpserver1.c_str(), hubSettings.sys_ntpserver2.c_str());
      delay(1000);
      wifiTrials++;
    }
  } else {
    int gsmTrials = 0;
#ifdef USE_ASYNC_A7670
    while (!modemAsync.isGprsConnected() && gsmTrials < 3)
#else
    while (!modem.isGprsConnected() && gsmTrials < 3)
#endif
    {
      NetLayrzHub::connectGprs(
        hubSettings.gprs_apn.c_str(), hubSettings.gprs_apn_user.c_str(),
        hubSettings.gprs_apn_pass.c_str(), hubSettings.gprs_pin.c_str(),
        hubSettings.sys_ntpserver1.c_str(), hubSettings.sys_ntpserver2.c_str());
      gsmTrials++;
      delay(1000);
    }
  }
}

void StartupLayrzHub::startSynchroNtpTask() {
  xTaskCreatePinnedToCore(NetLayrzHub::synchroNTP, "synchroNTP", 4096, NULL, 1,
                          &synchroNTPHandle, 0);
}

void StartupLayrzHub::checkForFirmwareUpdate() {
  debugPrint("DEBUG: Checking firmware update - fota_en: %s, fota_force: %s\n",
             hubSettings.fota_en ? "true" : "false",
             hubSettings.fota_force ? "true" : "false");

  if (hubSettings.fota_en || hubSettings.fota_force) {
    debugPrint("Starting firmware check task...\n");
    xTaskCreatePinnedToCore(checkFirmware, "checkFirmware", 12288, NULL, 5,
                            &checkFirmwareHandle, 0);
    while (!isFWChecked) {
      delay(1000);
    }
  } else {
    debugPrint("Firmware update check skipped - both fota_en and fota_force "
               "are false\n");
  }
}

void StartupLayrzHub::startNetworkKeepAlive() {
  if (hubSettings.net_mode == "wifi") {
    xTaskCreatePinnedToCore(NetLayrzHub::checkWifiNetwork, "checkWifiNetwork",
                            4096, NULL, 5, &checkWifiNetworkHandle, 0);
  } else {
    xTaskCreatePinnedToCore(NetLayrzHub::checkGSMNetwork, "checkGSMNetwork",
                            4096, NULL, 3, &checkGSMNetworkHandle, 0);
  }
  if (hubSettings.net_protocol == 0) {
    xTaskCreatePinnedToCore(NetLayrzHub::socketKeepAlive, "socketKeepAlive",
                            4096, NULL, 1, NULL, 0);
  }
}

bool StartupLayrzHub::initDataBuffers() {
  if (!_initMsgBuffer())
    return false;
  if (!_initSettingsBuffer())
    return false;
  if (!_initRs485Buffer())
    return false;
  if (!_initRs232_1Buffer())
    return false;
  if (!_initRs232_2Buffer())
    return false;
  if (!_initUartIoBuffer())
    return false;
  if (!_initExtIoBuffer())
    return false;
  if (!_initCanbusBuffer())
    return false;
  if (!_initObd2DtcBuffer())
    return false;
  if (!_initMediaBuffer())
    return false;
  if (!_initModbusBuffer())
    return false;
  return true;
}

bool StartupLayrzHub::_initMsgBuffer() {
  msgBuffer = (char *)heap_caps_malloc(MESSAGE_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (msgBuffer == nullptr) {
    debugPrint("Failed to allocate message buffer size\n");
    return false;
  }
  debugPrint("Message buffer allocated\n");
  memset(msgBuffer, 0, MESSAGE_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initSettingsBuffer() {
  settingsBuffer =
    (char *)heap_caps_malloc(SETTINGS_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (settingsBuffer == nullptr) {
    debugPrint("Failed to allocate settings buffer size\n");
    return false;
  }
  debugPrint("Settings buffer allocated\n");
  memset(settingsBuffer, 0, SETTINGS_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initUartIoBuffer() {
  uartIoDataBuffer =
    (char *)heap_caps_malloc(UART_IO_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (uartIoDataBuffer == nullptr) {
    debugPrint("Failed to allocate UART IO buffer size\n");
    return false;
  }
  debugPrint("UART IO buffer allocated\n");
  memset(uartIoDataBuffer, 0, UART_IO_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initRs485Buffer() {
  rs485DataBuffer =
    (char *)heap_caps_malloc(RS485_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (rs485DataBuffer == nullptr) {
    debugPrint("Failed to allocate RS485 buffer size\n");
    return false;
  }
  debugPrint("RS485 buffer allocated\n");
  memset(rs485DataBuffer, 0, RS485_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initRs232_1Buffer() {
  rs232_1DataBuffer =
    (char *)heap_caps_malloc(RS232_1_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (rs232_1DataBuffer == nullptr) {
    debugPrint("Failed to allocate RS232_1 buffer size\n");
    return false;
  }
  debugPrint("RS232_1 buffer allocated\n");
  memset(rs232_1DataBuffer, 0, RS232_1_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initRs232_2Buffer() {
  rs232_2DataBuffer =
    (char *)heap_caps_malloc(RS232_2_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (rs232_2DataBuffer == nullptr) {
    debugPrint("Failed to allocate RS232_2 buffer size\n");
    return false;
  }
  debugPrint("RS232_2 buffer allocated\n");
  memset(rs232_2DataBuffer, 0, RS232_2_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initExtIoBuffer() {
  extIoDataBuffer =
    (char *)heap_caps_malloc(EXT_IO_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (extIoDataBuffer == nullptr) {
    debugPrint("Failed to allocate EXT IO buffer size\n");
    return false;
  }
  debugPrint("EXT IO buffer allocated\n");
  memset(extIoDataBuffer, 0, EXT_IO_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initCanbusBuffer() {
  canDataBuffer =
    (char *)heap_caps_malloc(CANBUS_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (canDataBuffer == nullptr) {
    debugPrint("Failed to allocate Canbus data buffer in PSRAM\n");
    return false;
  }
  debugPrint("Canbus data buffer allocated\n");
  memset(canDataBuffer, 0, CANBUS_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initObd2DtcBuffer() {
  obd2DtcBuffer =
    (char *)heap_caps_malloc(OBD2_DTC_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (obd2DtcBuffer == nullptr) {
    debugPrint("Failed to allocate Obd2 DTCs buffer in PSRAM\n");
    return false;
  }
  debugPrint("OBD2 DTCs buffer allocated\n");
  memset(obd2DtcBuffer, 0, OBD2_DTC_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initMediaBuffer() {
  mediaMsgBuffer =
    (char *)heap_caps_malloc(MEDIA_MSG_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (mediaMsgBuffer == nullptr) {
    debugPrint("Failed to allocate media message buffer size\n");
    return false;
  }
  debugPrint("Media message buffer allocated\n");
  memset(mediaMsgBuffer, 0, MEDIA_MSG_BUFFER_SIZE); // initialize with zeros
  return true;
}

bool StartupLayrzHub::_initModbusBuffer() {
  modbusMsgBuffer =
    (char *)heap_caps_malloc(MODBUS_MSG_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
  if (modbusMsgBuffer == nullptr) {
    debugPrint("Failed to allocate Modbus message buffer size\n");
    return false;
  }
  debugPrint("Modbus message buffer allocated\n");
  memset(modbusMsgBuffer, 0, MODBUS_MSG_BUFFER_SIZE); // initialize with zeros
  return true;
}

void StartupLayrzHub::initGpio() {
  GpioLayrzHub::initGPIO();
  if (hubSettings.sys_debug_en)
    GpioLayrzHub::printGPIOConfigs();
  if (!GpioLayrzHub::createDebTimers())
    debugPrint("Failed to create debounce timers\n");
}

void StartupLayrzHub::initBleSensors() {
  if (BleDevicesLayrzHub::init() > 0 && hubSettings.data_ble_en) {
    xTaskCreatePinnedToCore(BleDevicesLayrzHub::bleScanTask, "bleScanTask",
                            4096, NULL, 3, &bleScanHandle, 0);
  }
}

void StartupLayrzHub::initSensorsMessaging() {
  xTaskCreatePinnedToCore(SensorPublisherLayrzHub::updateSensors,
                          "updateSensors", 4096, NULL, 2, &updateSensorsHandle,
                          0);
  xTaskCreatePinnedToCore(SensorPublisherLayrzHub::sendSensorData,
                          "sendSensorData", 4096, NULL, 5,
                          &sendSensorDataHandle, 0);
  xTaskCreatePinnedToCore(SensorPublisherLayrzHub::pingTask, "pingTask", 3072,
                          NULL, 2, &pingTaskHandle, 0);
}

void StartupLayrzHub::initSerialCommunications() {
  if (hubSettings.data_serial_en) {
    if (hubSettings.rs485_en || hubSettings.rs232_1_en ||
        hubSettings.rs232_2_en || hubSettings.uart_en) {
      if (hubSettings.rs485_mode != "modbus") {
        xTaskCreatePinnedToCore(SerialMonitor::serialMonitorTask,
                                "serialMonitorTask", 7168, NULL, 1,
                                &serialMonitorHandle, 0);
      }
    }
  }
}

void StartupLayrzHub::initCanBus() {
  if (hubSettings.can_en) {
    if (hubSettings.can_mode == SAEJ1939_STD_250K ||
        hubSettings.can_mode == SAEJ1939_FMS_250K ||
        hubSettings.can_mode == SAEJ1939_STD_500K ||
        hubSettings.can_mode == SAEJ1939_FMS_500K) {
      if (J1939LayrzHub::initJ1939()) {
        xTaskCreatePinnedToCore(J1939LayrzHub::j1939MonitorTask, "j1939Task",
                                8192, NULL, 1, &j1939MonitorTaskHandle, 0);
      }
    } else if (hubSettings.can_mode == OBD2_STD_250K ||
               hubSettings.can_mode == OBD2_STD_500K) {
      Obd2LayrzHub::initObd2();
      xTaskCreatePinnedToCore(Obd2LayrzHub::obd2MonitorTask, "obd2Task", 8192,
                              NULL, 1, &obd2MonitorTaskHandle, 0);
    }
  }
}

void StartupLayrzHub::startModbus() {
  uint8_t modbusDevConnected =
    hubSettings.modbus_dev1 + hubSettings.modbus_dev2 +
    hubSettings.modbus_dev3 + hubSettings.modbus_dev4;
  uint8_t modbusAddressUsed =
    hubSettings.modbus_addr1 + hubSettings.modbus_addr2 +
    hubSettings.modbus_addr3 + hubSettings.modbus_addr4;
  if (hubSettings.rs485_en && hubSettings.rs485_mode == "modbus" &&
      modbusDevConnected > 0 && modbusAddressUsed > 0) {
    ModbusLayrzHub::initModbus();
  }
}

void StartupLayrzHub::initSPI() {
  if (!spiSharedInitialized) {
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
    pinMode(ARDUCAM_CS, OUTPUT);
    digitalWrite(ARDUCAM_CS, HIGH);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    pinMode(SPI_MISO, INPUT_PULLUP);
    spiSharedInitialized = true;
  }
  if (!spiBusMutex) {
    spiBusMutex = xSemaphoreCreateMutex();
  }
}

void StartupLayrzHub::initArducamMega() {
  if (hubSettings.acam_en && ArducamLayrzHub::initArducamMega())
    arducamInitialized = true;
}

void StartupLayrzHub::startArducamCyclicCaptures() {
  if (arducamInitialized && hubSettings.acam_cyclic_en) {
    xTaskCreatePinnedToCore(ArducamLayrzHub::cyclicArducamTask, "arducamTask",
                            8192, NULL, 1, &arducamTaskHandle, 0);
  }
}

void StartupLayrzHub::startLayrzProtocol() {
  if (hubSettings.net_protocol == 1 && hubSettings.data_pub_per > 15) {
    xTaskCreatePinnedToCore(LayrzProtocol::getCommands, "getCommands", 8192,
                            NULL, 4, &getCommandsHandle, 0);
  }
}

void StartupLayrzHub::InitNimBLE() {
  if (NimBLEDevice::isInitialized()) {
    gBleStackReady = true;
    return;
  }

  NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
  NimBLEDevice::setScanDuplicateCacheSize(200);
  bool initOk = NimBLEDevice::init(Utils::GetDeviceName());
  gBleStackReady = initOk && NimBLEDevice::isInitialized();

  if (!gBleStackReady) {
    debugPrint("InitNimBLE failed\n");
  }
}

void StartupLayrzHub::initBlackBox() {
#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
  // Ensure mutex exists before any SPI/SD interaction.
  if (!spiBusMutex) {
    spiBusMutex = xSemaphoreCreateMutex();
  }

  // Take the shared SPI bus while mounting SD and constructing initial blackbox
  // files.
  if (spiBusMutex)
    xSemaphoreTake(spiBusMutex, portMAX_DELAY);

  // Deselect camera while touching SD to avoid CS contention.
  digitalWrite(ARDUCAM_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  uint32_t mountedFreqHz = 0;
  bool sdOk = mountSharedSdCard(mountedFreqHz);
  if (!sdOk) {
    debugPrint("SD mount failed!\n");
  } else if (hubSettings.sys_debug_en) {
    debugPrint("SD mount ok at %u Hz, cardType=%u, size=%lluMB\n",
               (unsigned)mountedFreqHz, (unsigned)SD.cardType(),
               (unsigned long long)(SD.cardSize() / (1024ULL * 1024ULL)));
  }

  bool bbOk = false;
  if (sdOk) {
    bbOk = blackboxSDCARD.begin();
  }

  if (!bbOk) {
    if (hubSettings.sys_debug_en)
      debugPrint("Failed to initialize BlackBox on SDCARD (sdOk=%d)\n",
                 (int)sdOk);
    // Ensure message bus is initialized early even if SD card mount or blackbox
    // init fails.
    auto cfg = MessageBusLayrzHub::defaultConfig();
    if (!MessageBusLayrzHub::begin(nullptr, cfg)) {
      debugPrint("MessageBus early initialization failed\n");
    } else if (hubSettings.sys_debug_en) {
      debugPrint("MessageBus early init (no blackbox)\n");
    }
  } else {
    debugPrint("BlackBox initialized on SDCARD\n");
    sdCardInitialized =
      true; // Set the flag to true if initialization is successful
    // Attach blackbox to already running bus (or initialize if not yet)
    auto cfg = MessageBusLayrzHub::defaultConfig();
    if (!MessageBusLayrzHub::begin(&blackboxSDCARD, cfg)) {
      debugPrint("MessageBus (attach) failed\n");
    } else if (hubSettings.sys_debug_en) {
      debugPrint("MessageBus attached to blackbox (queue=%u)\n",
                 (unsigned)cfg.realtimeQueueDepth);
    }
  }

  if (spiBusMutex)
    xSemaphoreGive(spiBusMutex);
#else
  debugPrint("BlackBox not supported on this hardware\n");
  sdCardInitialized = false;
  auto cfg = MessageBusLayrzHub::defaultConfig();
  if (!MessageBusLayrzHub::begin(nullptr, cfg)) {
    debugPrint("MessageBus early initialization failed\n");
  } else if (hubSettings.sys_debug_en) {
    debugPrint("MessageBus early init (no blackbox)\n");
  }
#endif
}

void StartupLayrzHub::initZigbeeCoordinator() {
  if (hubSettings.zigbee_en) {
    if (!hubSettings.zigbee_local_en && !hubSettings.net_en)
      return;
    hubSettings.uart_en = false;
    initZigbee();
  }
}
