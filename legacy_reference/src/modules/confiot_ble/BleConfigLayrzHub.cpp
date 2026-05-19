#include <modules/confiot_ble/BleConfigLayrzHub.h>
#include <new>

namespace {
constexpr uint8_t BLE_CMD_QUEUE_LEN = 6;
constexpr TickType_t BLE_CMD_QUEUE_PUSH_TIMEOUT = pdMS_TO_TICKS(5);
constexpr TickType_t BLE_CFG_SEM_TIMEOUT = pdMS_TO_TICKS(1000);
constexpr uint32_t BLE_SUPERVISOR_PERIOD_MS = 2000;
constexpr uint32_t BLE_RECOVERY_COOLDOWN_MS = 15000;
constexpr uint8_t BLE_MAX_CONSECUTIVE_FAILURES = 3;

TaskHandle_t bleCommandWorkerHandle = nullptr;
volatile bool bleCommandInProgress = false;
volatile bool bleAdvRefreshRequested = true;
uint8_t bleConsecutiveFailures = 0;
uint32_t bleNextRecoveryAllowedMs = 0;
bool bleAdvConfigured = false;

BleConnectionCallbacks bleConnectionCallbacks;
WriteCharacteristicCallbacks writeCharacteristicCallbacks;
ReadCharacteristicCallbacks readCharacteristicCallbacks;

void resetBleRxState() {
  rxMsg = "";
  rxChunkCount = 1;
  rxPayload = "";
  rxPayloadSize = 0;
}

void bleCommandWorkerTask(void *pvParameters) {
  for (;;) {
    std::string *queuedCommand = nullptr;
    if (xQueueReceive(xQueueCommand, &queuedCommand, portMAX_DELAY) == pdTRUE &&
        queuedCommand != nullptr) {
      bleCommandInProgress = true;
      if (bleConfigTimer != NULL) {
        xTimerStop(bleConfigTimer, 0);
      }
      LayrzProtocol::cmdWrapper(*queuedCommand, cmdSource::BLE);
      if (bleConfigConnected && bleConfigTimer != NULL) {
        xTimerReset(bleConfigTimer, 0);
        xTimerStart(bleConfigTimer, 0);
      }
      // Commands can change identifier/config fields used in advertising
      // payload.
      bleAdvRefreshRequested = true;
      bleCommandInProgress = false;
      delete queuedCommand;
    }
  }
}

void resetBleRuntimeGlobals() {
  pServer = nullptr;
  pService = nullptr;
  pCharacteristicRX = nullptr;
  pCharacteristicTX = nullptr;
  pAdvertising = nullptr;
  pBLEScan = nullptr;
  bleConfigConnected = false;
  gBleStackReady = false;
  bleAdvConfigured = false;
  bleAdvRefreshRequested = true;
}

bool configureAdvertisingDataLocked() {
  if (!pAdvertising) {
    debugPrint("Failed to set instance data: advertising object is null\n");
    return false;
  }

  NimBLEExtAdvertisement advData;
#ifdef LAYRZ_BLE_5_ADV
  advData.setLegacyAdvertising(false);
  advData.setPrimaryPhy(BLE_HCI_LE_PHY_1M);
  advData.setSecondaryPhy(BLE_HCI_LE_PHY_2M);
#else
  advData.setLegacyAdvertising(true);
#endif
  advData.setConnectable(true);
  advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  advData.setName(Utils::BleGetDeviceName());

  std::string mfdata = "";
  byte mac[6];
  esp_read_mac(mac, ESP_MAC_BT);

  byte sysHwModelId[3] = {0};
  memcpy(sysHwModelId, &hubSettings.sys_dev_hw_id,
         sizeof(hubSettings.sys_dev_hw_id));

  byte sysDeviceId[9] = {0};
  memcpy(sysDeviceId, &hubSettings.sys_dev_id, sizeof(hubSettings.sys_dev_id));

  mfdata += (char)0x17;
  mfdata += (char)0x0F; // Company reserved ID = 0x0F17

#if defined(LAYRZ_HUB1_BUILD)
  mfdata += (char)sysDeviceId[7];
  mfdata += (char)sysDeviceId[6];
  mfdata += (char)sysDeviceId[5];
  mfdata += (char)sysDeviceId[4];
  mfdata += (char)sysDeviceId[3];
  mfdata += (char)sysDeviceId[2];
  mfdata += (char)sysDeviceId[1];
  mfdata += (char)sysDeviceId[0];
  mfdata += (char)sysHwModelId[1];
  mfdata += (char)sysHwModelId[0];
#endif

  mfdata += (char)mac[0];
  mfdata += (char)mac[1];
  mfdata += (char)mac[2];
  mfdata += (char)mac[3];
  mfdata += (char)mac[4];
  mfdata += (char)mac[5];

#ifdef LAYRZ_BLE_5_ADV
  if (!advData.setManufacturerData(mfdata)) {
    debugPrint("Failed to set manufacturer data\n");
    return false;
  }
#endif

  if (!pAdvertising->setInstanceData(BLE_INSTANCE_ID, advData)) {
    debugPrint("Failed to set instance data\n");
    return false;
  }

#ifndef LAYRZ_BLE_5_ADV
  NimBLEExtAdvertisement scanResult;
  scanResult.setManufacturerData(mfdata);

#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
  std::string modelData = "";
  modelData += (char)sysHwModelId[1];
  modelData += (char)sysHwModelId[0];
  uint16_t modelUuid = 0x1516;
  scanResult.setServiceData(NimBLEUUID(modelUuid), modelData);

  std::string ownerData = "";
  ownerData += (char)sysDeviceId[7];
  ownerData += (char)sysDeviceId[6];
  ownerData += (char)sysDeviceId[5];
  ownerData += (char)sysDeviceId[4];
  ownerData += (char)sysDeviceId[3];
  ownerData += (char)sysDeviceId[2];
  ownerData += (char)sysDeviceId[1];
  ownerData += (char)sysDeviceId[0];
  uint16_t ownerUuid = 0x1517;
  scanResult.setServiceData(NimBLEUUID(ownerUuid), ownerData);
#endif

  if (!pAdvertising->setScanResponseData(BLE_INSTANCE_ID, scanResult)) {
    debugPrint("Failed to set scan response data\n");
    return false;
  }
#endif

  bleAdvConfigured = true;
  bleAdvRefreshRequested = false;
  return true;
}

bool ensureBleServiceAndAdvertisingLocked() {
  if (!NimBLEDevice::isInitialized() || !gBleStackReady) {
    return false;
  }

  if (!pServer) {
    // ble_gatts_reset() and ble_gatts_add_svcs() (called inside createServer)
    // require no active GAP procedures. The scanner releases xSemaphore right
    // after starting a scan, so a scan can be in progress here. Stop it so
    // ble_gatts_mutable() returns true; otherwise ble_gatts_reset() silently
    // fails and the subsequent ble_svc_gap_init() asserts on BLE_HS_EBUSY.
    if (pBLEScan && pBLEScan->isScanning()) {
      pBLEScan->stop();
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    pServer = NimBLEDevice::createServer();
    if (!pServer) {
      debugPrint("Failed to create BLE server\n");
      return false;
    }
    pServer->setCallbacks(&bleConnectionCallbacks, false);
  }

  if (!pService) {
    pService = pServer->createService(BLEUUID(NUS_UUID));
    if (!pService) {
      debugPrint("Failed to create BLE service\n");
      return false;
    }

    pCharacteristicRX =
      pService->createCharacteristic(BLEUUID(RX_UUID), NIMBLE_PROPERTY::WRITE);
    if (!pCharacteristicRX) {
      debugPrint("Failed to create RX characteristic\n");
      return false;
    }
    pCharacteristicRX->setCallbacks(&writeCharacteristicCallbacks);

    pCharacteristicTX =
      pService->createCharacteristic(BLEUUID(TX_UUID), NIMBLE_PROPERTY::NOTIFY);
    if (!pCharacteristicTX) {
      debugPrint("Failed to create TX characteristic\n");
      return false;
    }
    pCharacteristicTX->createDescriptor(
      "2902", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, 2);
    pCharacteristicTX->setCallbacks(&readCharacteristicCallbacks);
  }

  if (!pService->isStarted()) {
    debugPrint("Starting BLE service\n");
    pService->start();
    debugPrint("BLE service started (re)build\n");
  }

  if (!pAdvertising) {
    pAdvertising = pServer->getAdvertising();
    if (!pAdvertising) {
      debugPrint("Failed to get BLE advertising object\n");
      return false;
    }
    bleAdvConfigured = false;
  }

  if (bleAdvRefreshRequested) {
    bleAdvConfigured = false;
  }

  if (!bleAdvConfigured) {
    if (pAdvertising->isAdvertising()) {
      pAdvertising->stop(BLE_INSTANCE_ID);
    }
    if (!configureAdvertisingDataLocked()) {
      return false;
    }
  }

  if (!pAdvertising->isAdvertising()) {
    if (!pAdvertising->start(BLE_INSTANCE_ID)) {
      debugPrint("Failed to start advertising\n");
      return false;
    }
  }

  return true;
}

bool recoverBleStack() {
  if (xSemaphoreTake(xSemaphore, BLE_CFG_SEM_TIMEOUT) != pdTRUE) {
    // debugPrint("BLE recovery skipped: semaphore timeout\n");
    return false;
  }

  NimBLEDevice::deinit(true);
  resetBleRuntimeGlobals();
  xSemaphoreGive(xSemaphore);

  vTaskDelay(pdMS_TO_TICKS(300));
  StartupLayrzHub::InitNimBLE();
  if (!NimBLEDevice::isInitialized() || !gBleStackReady) {
    // debugPrint("BLE recovery failed: init not ready\n");
    return false;
  }

  return true;
}
} // namespace

void bleConfigTimerCB(TimerHandle_t xTimer) {
  if (bleCommandInProgress) {
    debugPrint("BLE timer deferred: command in progress\n");
    xTimerReset(xTimer, 0);
    return;
  }
  debugPrint("BLE Configurator Timer expired\n");
  bleConfigConnected = false;
  if (pServer != NULL) {
    pServer->disconnect(connHandle);
  }
}

void BleConnectionCallbacks::onConnect(NimBLEServer *pServer,
                                       NimBLEConnInfo &connInfo) {
  debugPrint("BLE Config Server Connected\n");
  if (bleScanHandle != NULL) {
    // Prevent TWDT false positives while scan task is intentionally suspended.
    esp_task_wdt_delete(bleScanHandle);
    vTaskSuspend(
      bleScanHandle); // Suspend the BLE scanning task when a client connects
  }
  bleConfigConnected = true;
  if (hubSettings.rgb_en) {
    rgbLed.setBrightness(rgbBrightness);
    rgbLed.setPixelColor(0, rgbLed.Color(255, 255, 0));
    rgbLed.show();
  }

  connHandle = connInfo.getConnHandle();
  debugPrint("Connection handle: %d\n", connHandle);

  // Start the 2-minute timeout timer when client connects
  if (bleConfigTimer != NULL) {
    xTimerStart(bleConfigTimer, 0);
  }
  resetBleRxState();
}

void BleConnectionCallbacks::onDisconnect(NimBLEServer *pServer,
                                          NimBLEConnInfo &connInfo,
                                          int reason) {
  debugPrint("BLE Config Server Disconnected\n");
  bleConfigConnected = false;
  bleCommandInProgress = false;

  connHandle = 0; // Reset the connection handle

  // Stop the timer when client disconnects
  if (bleConfigTimer != NULL) {
    xTimerStop(bleConfigTimer, 0);
  }

  if (hubSettings.rgb_en) {
    if (hubSettings.net_mode == "wifi" && isSocketAuth) {
      rgbLed.setBrightness(rgbBrightness);
      rgbLed.setPixelColor(0, rgbLed.Color(0, 255, 0));
      rgbLed.show();
    } else if (hubSettings.net_mode == "cellular" && isSocketAuth) {
      rgbLed.setBrightness(rgbBrightness);
      rgbLed.setPixelColor(0, rgbLed.Color(0, 0, 255));
      rgbLed.show();
    } else {
      rgbLed.setBrightness(rgbBrightness);
      rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0));
      rgbLed.show();
    }
  }
  if (bleScanHandle != NULL) {
    vTaskResume(
      bleScanHandle); // Resume the BLE scanning task when a client disconnects
    esp_task_wdt_add(bleScanHandle);
  }
}

void BleConnectionCallbacks::onMTUChange(uint16_t MTU,
                                         NimBLEConnInfo &connInfo) {
  debugPrint("BLE Config MTU changed to %d\n", MTU);
  mtuNegotiatedSize = MTU;
  pServer->updateConnParams(connHandle, 7.5, 30, 0,
                            600); // Set connection parameters
}

void WriteCharacteristicCallbacks::onWrite(
  NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
  rxMsg = pCharacteristic->getValue();
  if (rxMsg.length() > 0) {
    // Reset the timer since we received data
    if (bleConfigTimer != NULL) {
      xTimerReset(bleConfigTimer, 0);
    }
    debugPrint("Received Value: %s\n", rxMsg.c_str());
    if (rxMsg.find("##") != -1) {
      rxMsg = rxMsg.substr(2, rxMsg.length() - 2);
      debugPrint("Header received: %s\n", rxMsg.c_str());

      std::vector<std::string> keyArray = splitString(rxMsg, ";");
      if (keyArray.size() != 2) {
        debugPrint("Invalid keyArray size\n");
        return;
      }
      try {
        rxPayloadSize = std::stoi(keyArray[0]);
        rxChunkCount = std::stoi(keyArray[1]);
      } catch (...) {
        debugPrint("Invalid payload header values\n");
        resetBleRxState();
        return;
      }

      if (rxPayloadSize < 0 || rxChunkCount <= 0) {
        debugPrint("Invalid payload header range\n");
        resetBleRxState();
        return;
      }

      rxPayload.reserve(static_cast<size_t>(rxPayloadSize));
      debugPrint("Payload Size: %d, Chunk Count: %d\n", rxPayloadSize,
                 rxChunkCount);
    } else {
      rxPayload += rxMsg;
      rxChunkCount--;
      // debugPrint("Chunk Count: %d\n", rxChunkCount);
    }

    if (rxChunkCount == 0) {
      if (rxPayload.length() != rxPayloadSize) {
        debugPrint("Payload size mismatch\n");
        resetBleRxState();
        return;
      }
      debugPrint("");
      if (Print *console = debugConsole()) {
        console->println(("Received from Confiot BLE: " + rxPayload).c_str());
      }

      // debugPrint("Payload Size: %d\n", rxPayload.length());
      if (rxPayload.find("<Ac>") != -1 && rxPayload.find("</Ac>") != -1) {
        debugPrint("A command has been received\n");
        const size_t acStart = rxPayload.find("<Ac>");
        const size_t acEnd = rxPayload.find("</Ac>", acStart + 4);

        if (acStart != std::string::npos && acEnd != std::string::npos &&
            acEnd > (acStart + 4)) {
          std::string command =
            rxPayload.substr(acStart + 4, acEnd - (acStart + 4));
          std::string *queuedCommand =
            new (std::nothrow) std::string(std::move(command));

          if (queuedCommand == nullptr) {
            debugPrint("BLE command enqueue allocation failed\n");
          } else if (!xQueueCommand ||
                     xQueueSend(xQueueCommand, &queuedCommand,
                                BLE_CMD_QUEUE_PUSH_TIMEOUT) != pdTRUE) {
            debugPrint("BLE command queue full/dropped\n");
            delete queuedCommand;
          } else {
            debugPrint("BLE command enqueued\n");
          }
        } else {
          debugPrint("Invalid <Ac> command frame\n");
        }
      } else {
        debugPrint("No command received\n");
      }
      resetBleRxState();
    }
  }
}

void ReadCharacteristicCallbacks::onRead(NimBLECharacteristic *pCharacteristic,
                                         NimBLEConnInfo &connInfo) {
  debugPrint("bleConfig onRead request\n");
}

void ReadCharacteristicCallbacks::onSubscribe(
  NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo,
  uint16_t subValue) {
  debugPrint("bleConfig onSubscribe request\n");
  if (subValue == 0) {
    debugPrint("Unsubscribed from TX characteristic\n");
  } else {
    debugPrint("Subscribed to TX characteristic\n");
  }
}

/**
 * Initializes the BLE device and sets up the BLE server and service.
 *
 * @throws ErrorType description of error
 */
void confiotOverBLE(void *pvParameters) {
  debugPrint("BLE Configurator started\n");

  if (xQueueCommand == NULL) {
    xQueueCommand = xQueueCreate(BLE_CMD_QUEUE_LEN, sizeof(std::string *));
    if (xQueueCommand == NULL) {
      debugPrint("Failed to create BLE command queue\n");
    }
  }

  if (xQueueCommand != NULL && bleCommandWorkerHandle == nullptr) {
    BaseType_t created =
      xTaskCreatePinnedToCore(bleCommandWorkerTask, "bleCmdWorker", 10240, NULL,
                              4, &bleCommandWorkerHandle, 1);
    if (created != pdPASS) {
      debugPrint("Failed to create BLE command worker task\n");
      bleCommandWorkerHandle = nullptr;
    }
  }

  uint16_t timeout_sec = hubSettings.sys_ble_timeout;
  if (timeout_sec == 0)
    timeout_sec = 120; // Valor mínimo de seguridad

  if (bleConfigTimer == NULL) {
    bleConfigTimer =
      xTimerCreate("bleConfigTimer", pdMS_TO_TICKS(1000 * timeout_sec), pdTRUE,
                   (void *)0, bleConfigTimerCB);
  }

  resetBleRxState();
  bleAdvRefreshRequested = true;
  if (!NimBLEDevice::isInitialized() || !gBleStackReady) {
    StartupLayrzHub::InitNimBLE();
  }

  for (;;) {
    bool healthy = false;

    if (!bleConfigConnected && !bleCommandInProgress) {
      if (!NimBLEDevice::isInitialized() || !gBleStackReady) {
        // debugPrint("BLE supervisor: stack not ready, trying init\n");
        bleAdvConfigured = false;
        bleAdvRefreshRequested = true;
        bleConsecutiveFailures = 0;
        StartupLayrzHub::InitNimBLE();
      } else if (xSemaphoreTake(xSemaphore, BLE_CFG_SEM_TIMEOUT) == pdTRUE) {
        healthy = ensureBleServiceAndAdvertisingLocked();
        xSemaphoreGive(xSemaphore);
      } else {
        // debugPrint("BLE supervisor: semaphore timeout\n");
      }
    } else {
      healthy = true;
    }

    if (healthy) {
      noteBleSupervisorHeartbeat(true);
      bleConsecutiveFailures = 0;
    } else if (NimBLEDevice::isInitialized() && gBleStackReady &&
               !bleConfigConnected) {
      if (bleConsecutiveFailures < 0xFF) {
        bleConsecutiveFailures++;
      }
      // debugPrint("BLE supervisor failed (%u/%u)\n",
      //            static_cast<unsigned>(bleConsecutiveFailures),
      //            static_cast<unsigned>(BLE_MAX_CONSECUTIVE_FAILURES));
    }

    uint32_t now = millis();
    if (!healthy && !bleConfigConnected && !bleCommandInProgress &&
        bleConsecutiveFailures >= BLE_MAX_CONSECUTIVE_FAILURES &&
        static_cast<int32_t>(now - bleNextRecoveryAllowedMs) >= 0) {
      bleNextRecoveryAllowedMs = now + BLE_RECOVERY_COOLDOWN_MS;
      // debugPrint("BLE supervisor performing controlled recovery\n");
      if (recoverBleStack()) {
        bleConsecutiveFailures = 0;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(BLE_SUPERVISOR_PERIOD_MS));
  }
}
