#include <modules/ble_devices/BleLayrzHub.h>

std::string lastPosition = ";;;;;;;";

void BleDevLayrzCallBack::onResult(
  const BLEAdvertisedDevice *advertisedDevice) {
  std::string mac_address = advertisedDevice->getAddress().toString();

  // debugPrint("Found device: %s\n", mac_address.c_str());
  // convert the mac address to uppercase
  std::transform(mac_address.begin(), mac_address.end(), mac_address.begin(),
                 ::toupper);
  bool foundDevice = false;
  int index = 0;
  for (index = 0; index < 50; index++) {
    if (bleDevices[index].address == mac_address) {
      // debugPrint("BLE device %s has been found\n", mac_address.c_str());
      foundDevice = true;
      break;
    }
  }
  if (!foundDevice) {
    return;
  }
  // remove the colon from the mac address
  mac_address.erase(std::remove(mac_address.begin(), mac_address.end(), ':'),
                    mac_address.end());

  // Use SPIRAM buffer for BLE packet construction instead of std::string
  char *blePacketBuffer = BleMemoryManager::getMediumPacketBuffer();
  if (!blePacketBuffer) {
    debugPrint("ERROR: Failed to get SPIRAM buffer for BLE packet\n");
    return;
  }
  BleMemoryManager::clearBuffer(blePacketBuffer,
                                BleMemoryManager::MEDIUM_PACKET_BUFFER_SIZE);

  // Fixed-size stack buffers replace std::string += loops to eliminate
  // repeated heap alloc/realloc on every BLE advertisement callback.
  const std::string &bleModel = bleDevices[index].model;
  char bleName[64] = {0};
  char bleRssi[8] = {0};
  char bleTxPower[8] = {0};
  char bleManufacturerData[640] = {
    0}; // max ~250 raw bytes -> 500 hex chars + separators
  char bleServiceData[640] = {0};

  if (advertisedDevice->haveName()) {
    strncpy(bleName, advertisedDevice->getName().c_str(), sizeof(bleName) - 1);
  }

  snprintf(bleRssi, sizeof(bleRssi), "%d", advertisedDevice->getRSSI());
  if (advertisedDevice->haveTXPower()) {
    snprintf(bleTxPower, sizeof(bleTxPower), "%d",
             advertisedDevice->getTXPower());
  }

  if (advertisedDevice->haveManufacturerData()) {
    uint8_t mfdCount = advertisedDevice->getManufacturerDataCount();
    for (uint8_t i = 0; i < mfdCount; i++) {
      std::string manufacturerData = advertisedDevice->getManufacturerData(i);
      const uint8_t *data = (const uint8_t *)manufacturerData.data();
      int dataSize = manufacturerData.size();
      size_t used = strlen(bleManufacturerData);
      char hexBuf[3];

      snprintf(hexBuf, sizeof(hexBuf), "%02X", data[1]);
      strncat(bleManufacturerData, hexBuf,
              sizeof(bleManufacturerData) - used - 1);
      used += 2;
      snprintf(hexBuf, sizeof(hexBuf), "%02X", data[0]);
      strncat(bleManufacturerData, hexBuf,
              sizeof(bleManufacturerData) - used - 1);
      used += 2;
      strncat(bleManufacturerData, ":", sizeof(bleManufacturerData) - used - 1);
      used += 1;

      for (int j = 0;
           j < dataSize - 2 && used + 2 < sizeof(bleManufacturerData); j++) {
        snprintf(hexBuf, sizeof(hexBuf), "%02X", data[j + 2]);
        strncat(bleManufacturerData, hexBuf,
                sizeof(bleManufacturerData) - used - 1);
        used += 2;
      }

      if (i < mfdCount - 1 && used + 1 < sizeof(bleManufacturerData)) {
        strncat(bleManufacturerData, ",",
                sizeof(bleManufacturerData) - used - 1);
        used += 1;
      }
    }
  }

  snprintf(bleRssi, sizeof(bleRssi), "%d", advertisedDevice->getRSSI());

  if (advertisedDevice->haveTXPower()) {
    snprintf(bleTxPower, sizeof(bleTxPower), "%d",
             advertisedDevice->getTXPower());
  }
  if (advertisedDevice->haveServiceData()) {
    uint8_t serviceDataCount = advertisedDevice->getServiceDataCount();
    for (uint8_t i = 0; i < serviceDataCount; i++) {
      NimBLEUUID uuid = advertisedDevice->getServiceDataUUID(i);
      std::string uuidStr = uuid.toString().substr(2);
      size_t used = strlen(bleServiceData);
      strncat(bleServiceData, uuidStr.c_str(),
              sizeof(bleServiceData) - used - 1);
      used += uuidStr.size();
      strncat(bleServiceData, ":", sizeof(bleServiceData) - used - 1);
      used += 1;

      std::string serviceData = advertisedDevice->getServiceData(i);
      const uint8_t *data = (const uint8_t *)serviceData.data();
      int dataSize = serviceData.size();
      char hexBuf[3];
      for (int j = 0; j < dataSize && used + 2 < sizeof(bleServiceData); j++) {
        snprintf(hexBuf, sizeof(hexBuf), "%02X", data[j]);
        strncat(bleServiceData, hexBuf, sizeof(bleServiceData) - used - 1);
        used += 2;
      }

      if (i < serviceDataCount - 1 && used + 1 < sizeof(bleServiceData)) {
        strncat(bleServiceData, ",", sizeof(bleServiceData) - used - 1);
        used += 1;
      }
    }
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);
  // Find the third semicolon
  size_t firstSep = lastPosition.find(';');
  size_t secondSep = lastPosition.find(';', firstSep + 1);
  size_t thirdSep = lastPosition.find(';', secondSep + 1);
  lastPosition = lastPosition.substr(0, thirdSep + 1);

  // Construct BLE packet directly in SPIRAM buffer
  snprintf(blePacketBuffer, BleMemoryManager::MEDIUM_PACKET_BUFFER_SIZE,
           "%s;%ld;%s%s;%s;%s;%s;%s;%s;", mac_address.c_str(), tv.tv_sec,
           lastPosition.c_str(), bleModel.c_str(), bleName, bleRssi, bleTxPower,
           bleManufacturerData, bleServiceData);

  // Add CRC to the packet
  std::string crc = calculateCRC16(blePacketBuffer);
  strncat(blePacketBuffer, crc.c_str(),
          BleMemoryManager::MEDIUM_PACKET_BUFFER_SIZE -
            strlen(blePacketBuffer) - 1);

  // debugPrint("BLE packet: %s\n", blePacketBuffer);

  // Store the packet in the SPIRAM array (replaces bleDevicesMap)
  if (!BleMemoryManager::storeBlePacket(mac_address,
                                        std::string(blePacketBuffer))) {
    debugPrint("ERROR: Failed to store BLE packet in SPIRAM array\n");
  }
  // UARTSerial.print("Accumulated BLE data: ");
  // UARTSerial.println(bleData);
}

// Helper function for concatenating BLE packets
static void concatenateBlePacket(const char *macAddress, const char *packetData,
                                 void *userData) {
  char *packetBuffer = (char *)userData;

  size_t currentLen = strlen(packetBuffer);
  size_t deviceDataLen = strlen(packetData);

  // Check if we have enough space (leave room for CRC and tags)
  if (currentLen + deviceDataLen + 50 >=
      BleMemoryManager::LARGE_PACKET_BUFFER_SIZE) {
    // This could be logged but we'll handle it silently to avoid spam
    return;
  }

  strncat(packetBuffer, packetData,
          BleMemoryManager::LARGE_PACKET_BUFFER_SIZE - currentLen - 1);
  strncat(packetBuffer, ";",
          BleMemoryManager::LARGE_PACKET_BUFFER_SIZE - strlen(packetBuffer) -
            1);
}

// Helper function for BLE decoding
static void decodeBlePacket(const char *macAddress, const char *packetData,
                            void *userData) {
  char *decodedBuffer = (char *)userData;

  std::string bleDataToDecode = std::string(packetData);
  std::string decodedData = BleDecoder::decode(bleDataToDecode.c_str());
  if (decodedData.length() > 0) {
    strncat(decodedBuffer, decodedData.c_str(),
            MESSAGE_BUFFER_SIZE - strlen(decodedBuffer) - 1);
    strncat(decodedBuffer, ",",
            MESSAGE_BUFFER_SIZE - strlen(decodedBuffer) - 1);
  }
}

void BleDevLayrzCallBack::onScanEnd(const NimBLEScanResults &scanResults,
                                    int reason) {
  debugPrint("Scan ended with reason: %d\n", reason);

  if (BleMemoryManager::getBleDeviceCount() == 0) {
    debugPrint("No BLE devices found during scan.\n");
    return;
  }

  // Use large SPIRAM buffer for packet concatenation (50 devices * ~150 bytes
  // each)
  char *packetBuffer = BleMemoryManager::getLargePacketBuffer();
  if (!packetBuffer) {
    debugPrint("ERROR: Failed to get SPIRAM buffer for large packet\n");
    return;
  }
  BleMemoryManager::clearBuffer(packetBuffer,
                                BleMemoryManager::LARGE_PACKET_BUFFER_SIZE);

  // Concatenate all BLE device packets using iterator
  BleMemoryManager::iterateBleDevices(concatenateBlePacket, packetBuffer);

  // Add CRC to the concatenated packet
  std::string crc16 = calculateCRC16(packetBuffer);
  strncat(packetBuffer, crc16.c_str(),
          BleMemoryManager::LARGE_PACKET_BUFFER_SIZE - strlen(packetBuffer) -
            1);

  // Add protocol tags in-place to the same large buffer
  size_t currentLen = strlen(packetBuffer);

  // Check if we have enough space for the tags "<Pb>" + data + "</Pb>" + null
  // terminator
  if (currentLen + 10 >= BleMemoryManager::LARGE_PACKET_BUFFER_SIZE) {
    debugPrint("ERROR: Not enough space in large buffer for protocol tags\n");
    return;
  }

  // Shift the content to the right to make room for "<Pb>" at the beginning
  memmove(packetBuffer + 4, packetBuffer,
          currentLen + 1); // +1 for null terminator

  // Add opening tag
  packetBuffer[0] = '<';
  packetBuffer[1] = 'P';
  packetBuffer[2] = 'b';
  packetBuffer[3] = '>';

  // Add closing tag at the end
  strncat(packetBuffer, "</Pb>",
          BleMemoryManager::LARGE_PACKET_BUFFER_SIZE - strlen(packetBuffer) -
            1);

  if (hubSettings.sys_debug_en) {
    debugPrint("BLE data (<Pb>): ");
    for (int i = 0; i < strlen(packetBuffer); i++) {
      if (Print *console = debugConsole())
        console->print(packetBuffer[i]);
    }
    if (Print *console = debugConsole())
      console->println("");
  }

  while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  BusMessage bm;
  bm.kind = BusMsgKind::BleSensor;
  bm.data = MessageBusLayrzHub::allocAndCopy(packetBuffer, &bm.len);
  // Release semaphore immediately after copying buffer to avoid blocking other
  // producers
  debugPrint("Publishing <Pb> to MessageBus\n");
  bm.persistIfFail = true;
  bm.freeAfterSend = true;
  if (bm.data && isValidTime) {
    MessageBusLayrzHub::publish(bm);
  } else {
    // Free the allocation when skipping publish to avoid a memory leak
    if (bm.data) {
      heap_caps_free(bm.data);
      bm.data = nullptr;
    }
    debugPrint("Failed to alloc for Pb publish or not NTP synced\n");
  }

  if (hubSettings.data_ble_dec_en) {
    char *decodedBuffer = (char *)heap_caps_malloc(
      MESSAGE_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!decodedBuffer) {
      debugPrint("ERROR: Failed to allocate decodedBuffer in PSRAM\n");
      xSemaphoreGive(xSemaphore);
      BleMemoryManager::clearBleDevicesArray();
      return;
    }
    // create a decodedBuffer in PSRAM
    memset(decodedBuffer, 0, MESSAGE_BUFFER_SIZE);
    std::string position = GNSS::getPosition();
    struct timeval tv;
    gettimeofday(&tv, NULL);
    strncat(decodedBuffer, std::to_string(tv.tv_sec).c_str(),
            MESSAGE_BUFFER_SIZE - strlen(decodedBuffer) - 1);
    strncat(decodedBuffer, ";",
            MESSAGE_BUFFER_SIZE - strlen(decodedBuffer) - 1);
    strncat(decodedBuffer, position.c_str(),
            MESSAGE_BUFFER_SIZE - strlen(decodedBuffer) - 1);
    strncat(decodedBuffer, "report.code:LKBLE,",
            MESSAGE_BUFFER_SIZE - strlen(decodedBuffer) - 1);

    // Decode all BLE packets using iterator
    BleMemoryManager::iterateBleDevices(decodeBlePacket, decodedBuffer);

    // Replace the last comma with a semicolon and add a null terminator
    decodedBuffer[strlen(decodedBuffer) - 1] = ';';
    decodedBuffer[strlen(decodedBuffer)] = '\0';
    // calculate the CRC16
    std::string crc16 = calculateCRC16(decodedBuffer);
    strncat(decodedBuffer, crc16.c_str(),
            MESSAGE_BUFFER_SIZE - strlen(decodedBuffer) - 1);
    // Add the <Pd> and </Pd> tags
    for (int i = strlen(decodedBuffer); i >= 0; i--) {
      decodedBuffer[i + 4] = decodedBuffer[i]; // Shift characters to the right
    }
    decodedBuffer[0] = '<';
    decodedBuffer[1] = 'P';
    decodedBuffer[2] = 'd';
    decodedBuffer[3] = '>';
    strncat(decodedBuffer, "</Pd>",
            MESSAGE_BUFFER_SIZE - strlen(decodedBuffer) - 1);
    decodedBuffer[strlen(decodedBuffer)] = '\0';
    if (hubSettings.sys_debug_en) {
      debugPrint("Decoded BLE data (<Pd>): ");
      for (int i = 0; i < strlen(decodedBuffer); i++) {
        UARTSerial.print(decodedBuffer[i]);
      }
      UARTSerial.println("");
    }
    BusMessage bm;
    bm.kind = BusMsgKind::PdSensor;
    bm.data = MessageBusLayrzHub::allocAndCopy(decodedBuffer, &bm.len);
    // Release semaphore immediately after copying buffer to avoid blocking
    // other producers
    debugPrint("Publishing <Pd> with BLE sensors data to MessageBus\n");
    bm.persistIfFail = true;
    bm.freeAfterSend = true;
    if (bm.data && isValidTime) {
      MessageBusLayrzHub::publish(bm);
    } else {
      // Free the allocation when skipping publish to avoid a memory leak
      if (bm.data) {
        heap_caps_free(bm.data);
        bm.data = nullptr;
      }
      debugPrint("Failed to alloc for Pd publish or not NTP synced\n");
    }
    heap_caps_free(decodedBuffer);
  }
  xSemaphoreGive(xSemaphore);               // Release the semaphore
  BleMemoryManager::clearBleDevicesArray(); // Clear the SPIRAM array after
                                            // sending data
}

int BleDevicesLayrzHub::init() {
  // Initialize SPIRAM memory manager for BLE operations
  if (!BleMemoryManager::init()) {
    debugPrint("ERROR: Failed to initialize BLE SPIRAM memory manager\n");
    return -1;
  }

  // Load all 50 BLE devices using the unified storage system
  for (int i = 0; i < 50; i++) {
    String addressKey = "ble_" + String(i) + "_address";
    String modelKey = "ble_" + String(i) + "_model";

    bleDevices[i].address =
      UnifiedSettingsStorage::getString(addressKey, "").c_str();
    bleDevices[i].model =
      UnifiedSettingsStorage::getString(modelKey, "").c_str();
  }

  int activeBleDevices = 0;
  for (int i = 0; i < 50; i++) {
    debugPrint("BLE%d - model: %s mac: %s\n", i, bleDevices[i].model.c_str(),
               bleDevices[i].address.c_str());
    if (bleDevices[i].model != "" && bleDevices[i].address != "") {
      activeBleDevices++;
    }
  }
  if (activeBleDevices == 0)
    debugPrint("No active BLE devices.\n");

  // Print memory status after BLE device loading
  BleMemoryManager::printMemoryStatus();

  return activeBleDevices;
}

void BleDevicesLayrzHub::bleScanTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  const TickType_t semTakeWait = pdMS_TO_TICKS(250);

  while (true) {
    esp_task_wdt_reset();

    // if (hubSettings.sys_debug_en) {
    //     static uint32_t lastStackLogMs = 0;
    //     uint32_t now = millis();
    //     if (now - lastStackLogMs > 15000) {
    //         UBaseType_t words = uxTaskGetStackHighWaterMark(NULL);
    //         debugPrint("Stack HW bleScanTask: %u words (%u bytes)\n",
    //         (unsigned)words, (unsigned)(words * sizeof(StackType_t)));
    //         lastStackLogMs = now;
    //     }
    // }
    // If BLE stack is not ready, wait and retry
    if (!gBleStackReady || bleConfigConnected) {
      esp_task_wdt_reset();
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // Add timeout for NTP sync wait (max 60 seconds)
    uint32_t ntpWaitStart = millis();
    const uint32_t ntpWaitTimeout = 60000; // 60 seconds

    bool haveSemaphore = false;
    while (!haveSemaphore) {
      esp_task_wdt_reset();

      if (!gBleStackReady || bleConfigConnected) {
        vTaskDelay(pdMS_TO_TICKS(200));
        break;
      }

      if (xSemaphoreTake(xSemaphore, semTakeWait) != pdTRUE) {
        vTaskDelay(pdMS_TO_TICKS(20));
        continue;
      }

      if (!isValidTime && (millis() - ntpWaitStart) < ntpWaitTimeout) {
        xSemaphoreGive(xSemaphore);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (!isValidTime) {
          debugPrint("Waiting for NTP sync to send Pb...\n");
        }
        continue;
      }

      haveSemaphore = true;
    }

    if (!haveSemaphore) {
      continue;
    }

    if (!isValidTime && (millis() - ntpWaitStart) >= ntpWaitTimeout) {
      debugPrint("NTP sync timeout reached, continuing without time sync...\n");
    }
    if (!gBleStackReady) {
      xSemaphoreGive(xSemaphore);
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // Resolve scan object only while holding semaphore to avoid races with BLE
    // deinit/reinit.
    if (pBLEScan == nullptr) {
      pBLEScan = NimBLEDevice::getScan();
      if (pBLEScan == nullptr) {
        xSemaphoreGive(xSemaphore);
        vTaskDelay(pdMS_TO_TICKS(200));
        continue;
      }
      pBLEScan->setScanCallbacks(new BleDevLayrzCallBack(), false);
      pBLEScan->setActiveScan(true);
      pBLEScan->setInterval(100);
      pBLEScan->setWindow(99);
      pBLEScan->setPhy(NimBLEScan::Phy::SCAN_ALL);
      pBLEScan->setMaxResults(0);
    }

    // if (httpBusy || heap_caps_get_free_size(MALLOC_CAP_INTERNAL) < 60000) {
    //     if (hubSettings.sys_debug_en) {
    //         debugPrint("Skipping BLE scan (http busy or low memory)\n");
    //     }
    //     xSemaphoreGive(xSemaphore);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     continue;
    // }
    lastPosition = GNSS::getPosition();
    debugPrint("Scanning BLE devices...\n");
    if (pBLEScan == nullptr) {
      xSemaphoreGive(xSemaphore);
      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    if (pBLEScan->isScanning()) {
      pBLEScan->stop();
      vTaskDelay(pdMS_TO_TICKS(20)); // Deja que el host procese el stop
    }
    pBLEScan->clearResults();

    if (!pBLEScan->start(hubSettings.data_ble_win * 1000, false, false)) {
      debugPrint("BLE scan start failed\n");
    }

    xSemaphoreGive(xSemaphore);
    if (hubSettings.data_pub_per > 10) {
      for (int i = 0; i < int(hubSettings.data_pub_per / 5); i++) {
        // debugPrint("bleScanTasks is Resetting WDT\n");
        esp_task_wdt_reset();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    } else {
      // debugPrint("bleScanTasks is Resetting WDT\n");
      esp_task_wdt_reset();
      vTaskDelay(hubSettings.data_pub_per * 1000 / portTICK_PERIOD_MS);
    }
  }
}
