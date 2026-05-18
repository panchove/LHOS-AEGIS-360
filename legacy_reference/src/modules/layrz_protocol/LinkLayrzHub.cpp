#include <cstring>
#include <modules/gpio/GpioLayrzHub.h>
#include <modules/layrz_protocol/LinkLayrzHub.h>
#include <modules/settings/UnifiedSettingsStorage.h>

void LayrzProtocol::sendBleConfiotMsg(const std::string &data) {
  if (bleConfigConnected) {
    int dataLength = data.length();
    int maxSize = mtuNegotiatedSize - 10; // safe value
    int totalChunks =
      (dataLength + maxSize - 1) / maxSize; // Calculate the number of chunks

    debugPrint("Sending data, # chunks: %d, maxSize: %d\n", totalChunks,
               maxSize);

    // Create and send the header
    std::string header =
      "##" + std::to_string(dataLength) + ";" + std::to_string(totalChunks);
    debugPrint("Header: %s\n", header.c_str());
    pCharacteristicTX->notify(reinterpret_cast<const uint8_t *>(header.c_str()),
                              header.length());
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Short delay to give the client
                                           // time to process the header

    // Send each chunk sequentially
    for (int i = 0; i < totalChunks; i++) {
      int chunkStart = i * maxSize;
      int chunkEnd = min(chunkStart + maxSize, dataLength);

      // Extract the current chunk
      std::string chunk = data.substr(chunkStart, chunkEnd - chunkStart);
      // debugPrint("Sending chunk %d\n", i);

      // Send the chunk over the TX characteristic
      pCharacteristicTX->notify(
        reinterpret_cast<const uint8_t *>(chunk.c_str()), chunk.length());
      debugPrint("Sending chunk %d of %d: ", i, totalChunks);
      if (Print *console = debugConsole()) {
        for (int j = 0; j < chunk.length(); j++) {
          console->print(chunk[j]);
        }
        console->println();
      }

      // Add a small delay to avoid overwhelming the client
      vTaskDelay(BLE_DELAY_PER_CHUNK / portTICK_PERIOD_MS);
    }
  } else {
    debugPrint("BLE Config not connected, cannot send data\n");
  }
}

void LayrzProtocol::cmdWrapper(std::string cmd, int cmdSource) {
  if (Print *console = debugConsole()) {
    debugPrint("Command received: ");
    console->println(cmd.c_str());
  }
  std::vector<std::string> cmdArray = splitString(cmd, ";");
  for (int i = 0; i < cmdArray.size() - 1; i += 4) {
    executeCmd(cmdArray[i + 1].c_str(), cmdArray[i].c_str(),
               cmdArray[i + 2].c_str(), cmdArray[i + 3].c_str(), cmdSource);
  }
}

setKey LayrzProtocol::parseKeyValue(std::string input) {
  setKey defaultKey{false, "", ""};
  size_t pos = input.find(":");
  if (pos == -1) {
    return defaultKey;
  }
  std::string key = input.substr(0, pos);
  std::string value = input.substr(pos + 1);
  // replace dot with underscore in key
  std::replace(key.begin(), key.end(), '.', '_');

  if (key.length() == 0) {
    return defaultKey;
  }

  return {true, key, value};
}

void LayrzProtocol::executeCmd(const char *cmd, const char *cmd_id,
                               const char *args, const char *crc,
                               int cmdSource) {
  std::string command = std::string(cmd_id) + ';' + std::string(cmd) + ';' +
                        std::string(args) + ';';
  std::string calcCRC = calculateCRC16(command.c_str());
  if (strcmp(crc, calcCRC.c_str()) != 0) {
    debugPrint("CRC error\n");
    sendCommandAck(cmd_id, "CRC error", cmdSource);
    return;
  }

  bool isExecuted = false;
  bool shouldSendPcOverBle = false;

  if (strcmp(cmd, "reboot") == 0) {
    reboot_Handler(cmd_id, cmdSource);
    return;
  } else if (strcmp(cmd, "get_config") == 0) {
    isExecuted = get_config_Handler(cmdSource);
    shouldSendPcOverBle = false;
  } else if (strcmp(cmd, "set_config") == 0) {
    set_config_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "get_info") == 0) {
    isExecuted = get_info_Handler(cmdSource);
    shouldSendPcOverBle = false;
  } else if (strcmp(cmd, "get_msg") == 0) {
    get_msg_Handler(cmd_id, cmdSource);
    return;
  } else if (strcmp(cmd, "set_do") == 0) {
    set_do_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "set_pwm_output") == 0) {
    set_pwm_output_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "set_serial") == 0) {
    set_serial_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "set_rgb") == 0) {
    set_rgb_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "ping") == 0) {
    ping_Handler(cmd_id, cmdSource);
    return;
  } else if (strcmp(cmd, "start_fota") == 0) {
    start_fota_Handler(cmd_id, cmdSource);
    shouldSendPcOverBle = true;
  } else if (strcmp(cmd, "factory_reset") == 0) {
    factory_reset_Handler(cmd_id, cmdSource);
    shouldSendPcOverBle = true;
  } else if (strcmp(cmd, "hpc168_reset") == 0) {
    hpc168_reset_Handler(cmd_id, cmdSource);
    return;
  } else if (strcmp(cmd, "set_ext_do") == 0) {
    set_ext_do_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "set_ext_ao") == 0) {
    set_ext_ao_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "ext_io_reboot") == 0) {
    ext_io_reboot_Handler(cmd_id, cmdSource);
    return;
  } else if (strcmp(cmd, "get_obd2_dtcs") == 0) {
    get_obd2_dtcs_Handler(cmd_id, cmdSource);
    // sendCommandAck(cmd_id,"ok", cmdSource);
    return;
  } else if (strcmp(cmd, "set_dn23e08_do") == 0) {
    set_dn23e08_do_Handler(cmd_id, args, cmdSource);
    return;
    ;
  } else if (strcmp(cmd, "set_dn23e08_multi_do") == 0) {
    set_dn23e08_multi_do_Handler(cmd_id, args, cmdSource);
    return;
    ;
  } else if (strcmp(cmd, "reset_dn23e08_do") == 0) {
    reset_dn23e08_do_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "dn23e08_reboot") == 0) {
    dn23e08_reboot_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "reset_dn23e08_counter") == 0) {
    reset_dn23e08_counter_Handler(cmd_id, args, cmdSource);
    return;
  } else if (strcmp(cmd, "take_photo") == 0) {
    take_photo_Handler(cmd_id, cmdSource);
    return;
  } else if (strcmp(cmd, "delete_blackbox") == 0) {
    delete_blackbox_Handler(cmd_id, cmdSource);
    return;
  } else if (strcmp(cmd, "delete_history") == 0) {
    delete_history_Handler(cmd_id, cmdSource);
    return;
  } else if (strcmp(cmd, "delete_images") == 0) {
    delete_images_Handler(cmd_id, cmdSource);
    return;
  } else if (strcmp(cmd, "format_sd") == 0) {
    format_sd_Handler(cmd_id, cmdSource);
    return;
    // removed low-level format command per Option A
  } else {
    debugPrint("Command not implemented\n");
    sendCommandAck(cmd_id, "command not implemented", cmdSource);
    return;
  }

  std::string msg =
    "Command " + std::string(cmd) +
    std::string(isExecuted ? " executed succesfully" : " failed");

  if ((cmdSource == cmdSource::BLE && shouldSendPcOverBle) ||
      cmdSource == cmdSource::NET) {
    sendCommandAck(cmd_id, msg.c_str(), cmdSource);
  }
}

void LayrzProtocol::getCommands(void *pvParameters) {
  esp_task_wdt_add(NULL);

  for (;;) {
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
      if (hubSettings.net_protocol == 1) {
        BaseClientLayrzHub *client = ClientFactoryLayrzHub::createClient();

        if (!client) {
          debugPrint("Error: Failed to create TCP Socket Client!\n");
          // vTaskDelete(NULL);
          xSemaphoreGive(xSemaphore);
          continue;
          ;
        }
        serverResponse response = client->receiveDataFromServer();
        if (response.responseCode == 200 &&
            response.responseStr->find("<Ac>") != -1) {
          cmdWrapper(response.responseStr->substr(4), cmdSource::NET);
        }
        delete client;
      }
    }
    xSemaphoreGive(xSemaphore);
    if (hubSettings.sys_cmd_per > 10) {
      for (int i = 0; i < int(hubSettings.sys_cmd_per / 5); i++) {
        esp_task_wdt_reset();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    } else {
      esp_task_wdt_reset();
      vTaskDelay(hubSettings.sys_cmd_per * 1000 / portTICK_PERIOD_MS);
    }
  }
}

//************************************** */
//***********Command Handlers ********** */
//************************************** */

void LayrzProtocol::reboot_Handler(const char *cmd_id, int cmdSource) {
  if (cmdSource == cmdSource::NET) {
    memset(rebootAfterPcCmdId, 0, sizeof(rebootAfterPcCmdId));
    if (cmd_id) {
      strncpy(rebootAfterPcCmdId, cmd_id, sizeof(rebootAfterPcCmdId) - 1);
    }
    rebootAfterPcAck = true;
    sendCommandAck(cmd_id, "Rebooting device...", cmdSource);
    debugPrint(
      "Reboot requested, waiting for <Pc> ACK before restart (cmd_id=%s)\n",
      rebootAfterPcCmdId);
    return;
  }
  sendCommandAck(cmd_id, "Rebooting device...", cmdSource);
  debugPrint("Rebooting device...\n");
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  ESP.restart();
}

bool LayrzProtocol::get_config_Handler(int cmdSource) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  // Construct device configuration payload
  std::string settingsPayload =
    std::to_string(tv.tv_sec) + ";" + deviceSettings::getDeviceSettings() + ";";
  settingsPayload += std::string(calculateCRC16(settingsPayload.c_str()));
  settingsPayload = "<Ps>" + settingsPayload + "</Ps>";

  // Debug output. It is too large for debug print
  debugPrint("Device settings: ");
  if (Print *console = debugConsole())
    console->println(settingsPayload.c_str());

  // Send response via appropriate channel
  bool success = true;
  if (cmdSource == cmdSource::NET) {
    // debugPrint("Sending device settings via NET\n");
    // BaseClientLayrzHub* client = ClientFactoryLayrzHub::createClient();

    // if (!client) {
    //     debugPrint("Error: Failed to create a Transport Client!\n");
    //     // vTaskDelete(NULL); No debería estar aquí
    //     return false;
    // }
    // if (isValidTime && isSocketAuth) {
    //     serverResponse response =
    //     client->sendDataToServer(settingsBuffer->c_str()); debugPrint("<Ps>
    //     sent with response code: %d\n", response.responseCode); success =
    //     response.responseCode==200;
    // }
    // delete client;  //revent memory leaks
    while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    BusMessage bm;
    bm.kind = BusMsgKind::PsSettings;
    bm.data =
      MessageBusLayrzHub::allocAndCopy(settingsPayload.c_str(), &bm.len);
    // Release semaphore immediately after copying buffer to avoid blocking
    // other producers
    xSemaphoreGive(xSemaphore);
    debugPrint("Publishing <Ps> to MessageBus\n");
    bm.persistIfFail = false;
    bm.freeAfterSend = true;
    if (bm.data && isValidTime) {
      MessageBusLayrzHub::publish(bm);
      success = true;
    } else {
      debugPrint("Failed to alloc for Ps publish or not NTP synced\n");
      success = false;
    }
  } else if (cmdSource == cmdSource::BLE) {
    debugPrint("Sending device info via BLE\n");
    sendBleConfiotMsg(settingsPayload);
  }
  return success;
}

void LayrzProtocol::set_config_Handler(const char *cmd_id, const char *args,
                                       int cmdSource) {
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "ARGS error", cmdSource);
    return;
  }

  // Allocate std::string in PSRAM
  void *psramBuffer = heap_caps_malloc(sizeof(std::string), MALLOC_CAP_SPIRAM);
  if (!psramBuffer) {
    debugPrint("PSRAM allocation failed!\n");
    sendCommandAck(cmd_id, "Internal error", cmdSource);
    return;
  }

  if (cmdSource != cmdSource::BLE) {
    sendCommandAck(cmd_id, "Updating settings...", cmdSource);
  }

  std::string *settingsBuffer =
    new (psramBuffer) std::string(); // Placement new in PSRAM
  *settingsBuffer = args;
  std::vector<std::string> keyArray = splitString(*settingsBuffer, ",");
  debugPrint("Processing %d settings keys\n", keyArray.size());

  int processedCount = 0;
  int failedCount = 0;

  for (const auto &keyElement : keyArray) {
    setKey key = parseKeyValue(keyElement);
    if (!key.isValid) {
      debugPrint("Invalid key format: %s\n", keyElement.c_str());
      failedCount++;
      continue; // Skip invalid keys but continue processing others
    }

    debugPrint("Setting key: %s = %s\n", key.key.c_str(), key.value.c_str());
    if (!deviceSettings::setDeviceSetting(key.key.c_str(), key.value.c_str())) {
      debugPrint("Failed to set key %s\n", key.key.c_str());
      failedCount++;
      continue;
    }
    processedCount++;

    // Add a small delay every 20 keys to prevent watchdog timeout
    if (processedCount % 20 == 0) {
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }
  }

  debugPrint("Settings processing complete: %d successful, %d failed\n",
             processedCount, failedCount);

  // Configure UART GPIO pins if UART is enabled (do this before saving)
  if (hubSettings.uart_en) // Set UART pins to UART mode
  {
    std::string rxPinKey =
      "gpio_" + std::to_string(hubSettings.uart_rx_pin) + "_mode";
    std::string txPinKey =
      "gpio_" + std::to_string(hubSettings.uart_tx_pin) + "_mode";
    if (deviceSettings::setDeviceSetting(rxPinKey.c_str(), "6")) {
      processedCount++;
      debugPrint("Set UART RX pin %d to mode 6\n", hubSettings.uart_rx_pin);
    }
    if (deviceSettings::setDeviceSetting(txPinKey.c_str(), "7")) {
      processedCount++;
      debugPrint("Set UART TX pin %d to mode 7\n", hubSettings.uart_tx_pin);
    }
  }

  // Save all settings to SPIFFS in one batch operation
  if (processedCount > 0) {
    debugPrint("Saving settings to SPIFFS...\n");
    UnifiedSettingsStorage::saveAllSettings();
    debugPrint("Settings saved successfully\n");
  }
  memset(settingsBuffer->data(), 0, settingsBuffer->length());
  *settingsBuffer = std::to_string(time(NULL)) + ";" + args + ";";
  *settingsBuffer += std::string(calculateCRC16(settingsBuffer->c_str()));
  *settingsBuffer = "<Ps>" + *settingsBuffer + "</Ps>";
  // Debug output
  debugPrint("New settings payload: ");
  if (Print *console = debugConsole()) {
    for (int i = 0; i < settingsBuffer->length(); i++) {
      console->print(settingsBuffer->at(i));
    }
    console->println("");
  }
  bool success = true;
  if (cmdSource == cmdSource::BLE) {
    debugPrint("Sending new settings via BLE\n");
    sendBleConfiotMsg(*settingsBuffer);
  } else if (cmdSource == cmdSource::NET) {
    while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    BusMessage bm;
    bm.kind = BusMsgKind::PsSettings;
    bm.data =
      MessageBusLayrzHub::allocAndCopy(settingsBuffer->c_str(), &bm.len);
    debugPrint("Publishing <Ps> to MessageBus\n");
    bm.persistIfFail = true;
    bm.freeAfterSend = true;
    if (bm.data && isValidTime) {
      MessageBusLayrzHub::publish(bm);
      bootAfterCommand = true;
    } else {
      debugPrint("Failed to alloc for Ps publish or not NTP synced\n");
    }
  }

  settingsBuffer->~basic_string();
  heap_caps_free(psramBuffer);
  xSemaphoreGive(xSemaphore);
}

bool LayrzProtocol::get_info_Handler(int cmdSource) {

  // Construct device info payload
  std::string corePayload = getBluetoothMACAddress() + ";";
  corePayload += hubSettings.fota_fw_id + ";";
  corePayload += std::to_string(BUILD_NUMBER) + ";";
  corePayload += std::to_string(hubSettings.sys_dev_id) + ";";
  corePayload += std::to_string(hubSettings.sys_dev_hw_id) + ";";
  corePayload += std::to_string(hubSettings.sys_dev_md_id) + ";";
  corePayload += std::to_string(hubSettings.fota_fw_branch) + ";";
  corePayload += std::string(hubSettings.fota_en ? "true" : "false") + ";";

  corePayload += calculateCRC16(
    corePayload.c_str()); // Compute CRC16 and add to core payload

  // Wrap with <Pi> and </Pi>
  std::string infoBuffer = "<Pi>" + corePayload + "</Pi>";

  // Debug output
  debugPrint("Device info: %s\n", infoBuffer.c_str());

  // Send response via appropriate channel
  if (cmdSource == cmdSource::NET) {
    // // debugPrint("Sending device info via NET\n");
    // // debugPrint("Device info: %s\n", infoBuffer.c_str());
    // BaseClientLayrzHub* client = ClientFactoryLayrzHub::createClient();
    // bool success = false;
    // if (!client) {
    //     debugPrint("Error: Failed to create a Transport Client!\n");
    //     // vTaskDelete(NULL);
    //     return false;
    // }
    // if (isValidTime && isSocketAuth) {
    //     serverResponse response =
    //     client->sendDataToServer(infoBuffer.c_str()); debugPrint("<Pi> sent
    //     with response code: %d\n", response.responseCode); success =
    //     response.responseCode==200;
    // }
    // delete client;  //revent memory leaks
    // return success;
    while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    BusMessage bm;
    bm.kind = BusMsgKind::PiInfo;
    bm.data = MessageBusLayrzHub::allocAndCopy(infoBuffer.c_str(), &bm.len);
    // Release semaphore immediately after copying buffer to avoid blocking
    // other producers
    xSemaphoreGive(xSemaphore);
    debugPrint("Publishing <Pi> to MessageBus\n");
    bm.persistIfFail = false;
    bm.freeAfterSend = true;
    if (bm.data && isValidTime) {
      MessageBusLayrzHub::publish(bm);
      return true;
    } else {
      debugPrint("Failed to alloc for Pi publish or not NTP synced\n");
      return false;
    }
  } else if (cmdSource == cmdSource::BLE) {
    debugPrint("Sending device info via BLE\n");
    sendBleConfiotMsg(infoBuffer);
  }
  return true;
}

void LayrzProtocol::get_msg_Handler(const char *cmd_id, int cmdSource) {
  memset(msgBuffer, 0, MESSAGE_BUFFER_SIZE);
  std::string position = GNSS::getPosition();
  SensorPublisherLayrzHub::buildSensorPayload(hubSettings.data_gpio_en,
                                              position);
  // debugPrint("Sensor data to send: ");
  // if (hubSettings.sys_debug_en)
  // {
  //     for (int i = 0; i < strlen(msgBuffer); i++)
  //     {
  //         UARTSerial.print(msgBuffer[i]);
  //     }
  //     UARTSerial.println("");
  // }
  if (strlen(msgBuffer) > 0) {
    if (cmdSource == cmdSource::NET) {
      // if (isValidTime && isSocketAuth) {
      //     BaseClientLayrzHub* client = ClientFactoryLayrzHub::createClient();

      //     if (!client) {
      //         sendCommandAck(cmd_id, "Transport client error", cmdSource);
      //         return;
      //     }
      //     serverResponse response = client->sendDataToServer(msgBuffer);
      //     debugPrint("<Pd> sent\n");
      //     if (response.responseCode == 200) {
      //         sendCommandAck(cmd_id, "Sensor data sent and received",
      //         cmdSource);
      //     }
      //     else {
      //         sendCommandAck(cmd_id, "Sensor data sent but not received",
      //         cmdSource);
      //     }
      //     delete client;
      // }
      while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      BusMessage bm;
      bm.kind = BusMsgKind::PsSettings;
      bm.data = MessageBusLayrzHub::allocAndCopy(msgBuffer, &bm.len);
      // Release semaphore immediately after copying buffer to avoid blocking
      // other producers
      xSemaphoreGive(xSemaphore);
      debugPrint("Publishing <Pd> to MessageBus\n");
      bm.persistIfFail = false;
      bm.freeAfterSend = true;
      if (bm.data && isValidTime) {
        MessageBusLayrzHub::publish(bm);
        sendCommandAck(cmd_id, "Sensor data sent", cmdSource);
      } else {
        // Free the allocation when skipping publish to avoid a memory leak
        if (bm.data) {
          heap_caps_free(bm.data);
          bm.data = nullptr;
        }
        debugPrint("Failed to alloc for Pd publish or not NTP synced\n");
        sendCommandAck(cmd_id, "Failed to publish sensor data", cmdSource);
      }
    } else if (cmdSource == cmdSource::BLE) {
      debugPrint("Sending sensor data via BLE\n");
      sendBleConfiotMsg(msgBuffer);
    }
  } else {
    sendCommandAck(cmd_id, "No sensor data to send", cmdSource);
  }
}

void LayrzProtocol::set_do_Handler(const char *cmd_id, const char *args,
                                   int cmdSource) {
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> doArray = splitString(args, ",");
  byte doByte = 0;
  byte doEnabled = 0;
  uint8_t doDuration[6] = {0, 0, 0, 0, 0, 0};
  for (const auto &doElement : doArray) {

    setKey digiOut = parseKeyValue(doElement);
    // debugPrint("DO: %s, value: %s\n", digiOut.key.c_str(),
    // digiOut.value.c_str());
    if (!digiOut.isValid) {
      sendCommandAck(cmd_id, "ARGS error", cmdSource);
      return;
    }
    if (digiOut.key == "do1_value" && digiOut.value == "true") {
      bitSet(doByte, 0);
      continue;
    }
    if (digiOut.key == "do2_value" && digiOut.value == "true") {
      bitSet(doByte, 1);
      continue;
    }
    if (digiOut.key == "do5_value" && digiOut.value == "true") {
      bitSet(doByte, 2);
      continue;
    }
    if (digiOut.key == "do6_value" && digiOut.value == "true") {
      bitSet(doByte, 3);
      continue;
    }
    if (digiOut.key == "do7_value" && digiOut.value == "true") {
      bitSet(doByte, 4);
      continue;
    }
    if (digiOut.key == "do14_value" && digiOut.value == "true") {
      bitSet(doByte, 5);
      continue;
    }
    if (digiOut.key == "do1_enable" && digiOut.value == "true") {
      bitSet(doEnabled, 0);
      continue;
    }
    if (digiOut.key == "do2_enable" && digiOut.value == "true") {
      bitSet(doEnabled, 1);
      continue;
    }
    if (digiOut.key == "do5_enable" && digiOut.value == "true") {
      bitSet(doEnabled, 2);
      continue;
    }
    if (digiOut.key == "do6_enable" && digiOut.value == "true") {
      bitSet(doEnabled, 3);
      continue;
    }
    if (digiOut.key == "do7_enable" && digiOut.value == "true") {
      bitSet(doEnabled, 4);
      continue;
    }
    if (digiOut.key == "do14_enable" && digiOut.value == "true") {
      bitSet(doEnabled, 5);
      continue;
    }
    if (digiOut.key == "do1_duration") {
      doDuration[0] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do2_duration") {
      doDuration[1] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do5_duration") {
      doDuration[2] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do6_duration") {
      doDuration[3] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do7_duration") {
      doDuration[4] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do14_duration") {
      doDuration[5] = std::stoi(digiOut.value);
      continue;
    }
  }
  int setDoStatus = GpioLayrzHub::setMultipleDO(doEnabled, doByte, doDuration);
  if (setDoStatus == 1) {
    sendCommandAck(cmd_id, "Digital outputs set with errors", cmdSource);
    return;
  }

  sendCommandAck(cmd_id, "Digital outputs successfully set", cmdSource);
}

void LayrzProtocol::set_pwm_output_Handler(const char *cmd_id, const char *args,
                                           int cmdSource) {
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> pwmoArray = splitString(args, ",");

  for (const auto &pwmoElement : pwmoArray) {
    debugPrint("PWM Output: %s\n", pwmoElement.c_str());
    setKey pwmOut = parseKeyValue(pwmoElement);
    if (!pwmOut.isValid) {
      sendCommandAck(cmd_id, "PWM Output ARGS error", cmdSource);
      return;
    }
    debugPrint("PWM Output: %s, value: %s\n", pwmOut.key.c_str(),
               pwmOut.value.c_str());
    int setPwmoStatus = GpioLayrzHub::setPwmOutput(std::stoi(pwmOut.key),
                                                   std::stoi(pwmOut.value));

    if (setPwmoStatus == 1) {
      sendCommandAck(cmd_id, "invalid GPIO Mode", cmdSource);
      return;
    } else if (setPwmoStatus == 2) {
      sendCommandAck(cmd_id, "Invalid GPIO value", cmdSource);
      return;
    } else if (setPwmoStatus == 3) {
      sendCommandAck(cmd_id, "GPIO not available", cmdSource);
      return;
    }
  }
  sendCommandAck(cmd_id, "PWM output successfully set", cmdSource);
}

void LayrzProtocol::set_serial_Handler(const char *cmd_id, const char *args,
                                       int cmdSource) {
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "Set Serial ARGS error", cmdSource);
    return;
  }

  std::vector<std::string> serialArray = splitString(args, ",");
  for (const auto &serialElement : serialArray) {
    debugPrint("Serial: %s\n", serialElement.c_str());
    setKey serial = parseKeyValue(serialElement);
    if (!serial.isValid) {
      sendCommandAck(cmd_id, "Serial ARGS error", cmdSource);
      return;
    }
    if (serial.key == "232_1") {
      if (hubSettings.rs232_1_en)
        RS232_1.print(serial.value.c_str());
      else {
        sendCommandAck(cmd_id, "RS232_1 not enabled", cmdSource);
        return;
      }
    } else if (serial.key == "232_2") {
      if (hubSettings.rs232_2_en)
        RS232_2.print(serial.value.c_str());
      else {
        sendCommandAck(cmd_id, "RS232_2 not enabled", cmdSource);
        return;
      }
    } else if (serial.key == "UART") {
      if (hubSettings.uart_en)
        UART_IO.print(serial.value.c_str());
      else {
        sendCommandAck(cmd_id, "UART_IO not enabled", cmdSource);
        return;
      }
    } else if (serial.key == "485") {
      if (hubSettings.rs485_en) {
        digitalWrite(RS485_RE_DE, HIGH);
        vTaskDelay(100 / portTICK_PERIOD_MS);

        for (int i = 0; i < serial.value.length(); i++) {
          RS485.print(serial.value[i]);
        }
        vTaskDelay(hubSettings.rs485_delay / portTICK_PERIOD_MS);
        digitalWrite(RS485_RE_DE, LOW);
      } else {
        sendCommandAck(cmd_id, "RS485 not enabled", cmdSource);
        return;
      }
    } else {
      sendCommandAck(cmd_id, "Invalid Serial Port", cmdSource);
      return;
    }
  }
  sendCommandAck(cmd_id, "Serial message succesfully sent", cmdSource);
}

void LayrzProtocol::set_rgb_Handler(const char *cmd_id, const char *args,
                                    int cmdSource) {
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "set_rgb command ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> rgbArray = splitString(args, ",");
  for (const auto &rgbElement : rgbArray) {
    debugPrint("RGB: %s\n", rgbElement.c_str());
    setKey rgb = parseKeyValue(rgbElement);
    if (!rgb.isValid) {
      sendCommandAck(cmd_id, "ARGS error", cmdSource);
      return;
    }
    if (std::stoi(rgb.key) < 1 || std::stoi(rgb.key) > 5) {
      sendCommandAck(cmd_id, "Invalid RGB key", cmdSource);
      return;
    }
    rgbColor color = hexToRGB(rgb.value.c_str());
    debugPrint("RGB Color: R:%d G:%d B:%d\n", color.red, color.green,
               color.blue);
    rgbLed.setBrightness(rgbBrightness);
    if (rgb.key == "1")
      rgbLed.setPixelColor(std::stoi(rgb.key) - 1,
                           rgbLed.Color(color.red, color.green, color.blue));
    else
      rgbLed.setPixelColor(std::stoi(rgb.key) - 1,
                           rgbLed.Color(color.green, color.red, color.blue));
  }
  rgbLed.show();
  sendCommandAck(cmd_id, "RGB LED successfully driven", cmdSource);
  delay(3000);
}

bool LayrzProtocol::ping_Handler(const char *cmd_id, int cmdSource) {
  sendCommandAck(cmd_id, "Pong", cmdSource);
  return true;
}

void LayrzProtocol::start_fota_Handler(const char *cmd_id, int cmdSource) {
  deviceSettings::setDeviceSetting("fota_force", "true");

  // CRITICAL: Save settings to SPIFFS immediately to persist the change
  if (!UnifiedSettingsStorage::saveAllSettings()) {
    debugPrint("ERROR: Failed to save fota_force to SPIFFS\n");
    sendCommandAck(cmd_id, "Error saving FOTA setting", cmdSource);
    return;
  }

  debugPrint("fota_force saved to SPIFFS successfully\n");
  if (cmdSource == cmdSource::NET) {
    memset(rebootAfterPcCmdId, 0, sizeof(rebootAfterPcCmdId));
    if (cmd_id) {
      strncpy(rebootAfterPcCmdId, cmd_id, sizeof(rebootAfterPcCmdId) - 1);
    }
    rebootAfterPcAck = true;
  }
  sendCommandAck(cmd_id, "Starting FOTA....", cmdSource);
  debugPrint("Starting fota on next boot\n");
  if (cmdSource != cmdSource::NET) {
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    ESP.restart();
  }
}

void LayrzProtocol::factory_reset_Handler(const char *cmd_id, int cmdSource) {
  sendCommandAck(cmd_id, "Factory reset...", cmdSource);
  debugPrint("Factory reset...rebooting\n");
  MorseCode morse(LED, hubSettings.rgb_count);
  deviceSettings::setFactorySettings();
  morse.display(0, "05", 0xFF6600, 0xFF6600);
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  ESP.restart();
}

void LayrzProtocol::hpc168_reset_Handler(const char *cmd_id, int cmdSource) {
  sendCommandAck(cmd_id, "Resetting HPC168...", cmdSource);
  if (!hubSettings.rs485_en) {
    sendCommandAck(cmd_id, "RS485 not enabled", cmdSource);
    return;
  }
  if (hubSettings.rs485_mode != "HPC168") {
    sendCommandAck(cmd_id, "RS485 mode not HPC168", cmdSource);
    return;
  }
  int trial = 0;
  bool success = false;
  while (trial < 3 && !success) {
    success = SerialMonitor::resetHpc168(hubSettings.hpc_front_addr,
                                         hubSettings.hpc_back_addr,
                                         hubSettings.hpc_back_en);
    trial++;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  if (success) {
    sendCommandAck(cmd_id, "HPC168 successfully reset", cmdSource);
  } else {
    sendCommandAck(cmd_id, "Failed to reset HPC168", cmdSource);
  }
}

void LayrzProtocol::set_ext_do_Handler(const char *cmd_id, const char *args,
                                       int cmdSource) {

  if (!hubSettings.data_serial_en || !hubSettings.uart_en ||
      hubSettings.uart_mode != "ioExtender") {
    sendCommandAck(cmd_id, "IO extender not available", cmdSource);
    return;
  }
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> doArray = splitString(args, ",");
  int responsesSum = 0;
  for (const auto &doElement : doArray) {
    debugPrint("External DO: %s\n", doElement.c_str());
    setKey digiOut = parseKeyValue(doElement);
    if (!digiOut.isValid) {
      sendCommandAck(cmd_id, "ARGS error", cmdSource);
      return;
    }

    int trial = 1;
    int resultCode = 1;
    while (trial < 6) {
      resultCode = SerialMonitor::setExternalOutput(
        std::stoi(digiOut.key), std::stoi(digiOut.value), 1);
      if (resultCode == 0)
        break;
      trial++;
      debugPrint("Trial: %d\n", trial);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    responsesSum += resultCode;
  }
  if (responsesSum == 0) {
    sendCommandAck(cmd_id, "external digital outputs successfully set",
                   cmdSource);
  } else {
    sendCommandAck(cmd_id, "Failed to set external digital outputs", cmdSource);
  }
}

void LayrzProtocol::set_ext_ao_Handler(const char *cmd_id, const char *args,
                                       int cmdSource) {
  if (!hubSettings.data_serial_en || !hubSettings.uart_en ||
      hubSettings.uart_mode != "ioExtender") {
    sendCommandAck(cmd_id, "IO extender not available", cmdSource);
    return;
  }
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "set_ext_ao ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> aoArray = splitString(args, ",");
  int responsesSum = 0;
  for (const auto &aoElement : aoArray) {
    debugPrint("External AO: %s\n", aoElement.c_str());
    setKey analogOut = parseKeyValue(aoElement);
    if (!analogOut.isValid) {
      sendCommandAck(cmd_id, "ARGS error", cmdSource);
      return;
    }

    int trial = 1;
    int resultCode = 1;
    while (trial < 6) {
      resultCode = SerialMonitor::setExternalOutput(
        std::stoi(analogOut.key), std::stoi(analogOut.value), 2);
      if (resultCode == 0)
        break;
      trial++;
      debugPrint("Trial: %d\n", trial);
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    responsesSum += resultCode;
  }
  if (responsesSum == 0) {
    sendCommandAck(cmd_id, "external analog outputs successfully set",
                   cmdSource);
  } else {
    sendCommandAck(cmd_id, "Failed to set external analog outputs", cmdSource);
  }
}

void LayrzProtocol::ext_io_reboot_Handler(const char *cmd_id, int cmdSource) {
  if (!hubSettings.data_serial_en || !hubSettings.uart_en ||
      hubSettings.uart_mode != "ioExtender") {
    sendCommandAck(cmd_id, "IO extender not available", cmdSource);
    return;
  }
  int trial = 1;
  bool success = false;
  while (trial < 6 && !success) {
    success = SerialMonitor::rebootIOExtender();
    trial++;
    debugPrint("Trial: %d\n", trial);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  if (success) {
    sendCommandAck(cmd_id, "IO extender successfully rebooted", cmdSource);
  } else {
    sendCommandAck(cmd_id, "Failed to reboot IO extender", cmdSource);
  }
}

void LayrzProtocol::get_obd2_dtcs_Handler(const char *cmd_id, int cmdSource) {
  bool obd2ModeEn = hubSettings.can_mode == 0 || hubSettings.can_mode == 1;
  if (!hubSettings.can_en || !obd2ModeEn) {
    sendCommandAck(cmd_id, "OBD2 not available", cmdSource);
    return;
  }
  if (Obd2LayrzHub::getObd2Dtcs() == 0 || obd2DtcBuffer == nullptr) {
    sendCommandAck(cmd_id, "No DTCs found", cmdSource);
    return;
  }

  // BaseClientLayrzHub* client = ClientFactoryLayrzHub::createClient();

  // if (!client) {
  //     debugPrint("Error: Failed to create a Transport Client!\n");
  //     return;
  // }
  // bool success = false;
  // if (isValidTime && isSocketAuth) {
  //     serverResponse response = client->sendDataToServer(obd2DtcBuffer);
  //     debugPrint("<Pd> with OBD2 DTCs sent - response code: %d\n",
  //     response.responseCode); success = response.responseCode==200;
  // }
  // delete client;  //Prevent memory leaks
  // if (success) {
  //     sendCommandAck(cmd_id, "OBD2 DTCs sent", cmdSource);
  // }
  // else {
  //     sendCommandAck(cmd_id, "Failed to send OBD2 DTCs", cmdSource);
  // }
  while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  BusMessage bm;
  bm.kind = BusMsgKind::PdSensor;
  bm.data = MessageBusLayrzHub::allocAndCopy(obd2DtcBuffer, &bm.len);
  // Release semaphore immediately after copying buffer to avoid blocking other
  // producers
  xSemaphoreGive(xSemaphore);
  debugPrint("Publishing <Pd> with OBD2 DTC's to MessageBus\n");
  bm.persistIfFail = false;
  bm.freeAfterSend = true;
  if (bm.data && isValidTime) {
    MessageBusLayrzHub::publish(bm);
    sendCommandAck(cmd_id, "OBD2 DTC's data sent", cmdSource);
  } else {
    debugPrint("Failed to alloc for Pd publish or not NTP synced\n");
    sendCommandAck(cmd_id, "Failed to publish OBD2 DTC's data", cmdSource);
  }
}

void LayrzProtocol::sendCommandAck(const char *cmd_id, const char *msg,
                                   int cmdSource) {
  // Build the payload (excluding <Pc>)
  struct timeval tv;
  gettimeofday(&tv, NULL);

  std::string corePayload =
    std::to_string(tv.tv_sec) + ";" + cmd_id + ";" + msg + ";";
  corePayload +=
    calculateCRC16(corePayload.c_str()); // Compute CRC16 only for core payload

  // Wrap with <Pc> and </Pc>
  std::string cmdResponseBuff = "<Pc>" + corePayload + "</Pc>";

  // Debug output
  if (hubSettings.sys_debug_en) {
    debugPrint("Command response: %s\n", cmdResponseBuff.c_str());
  }

  // Send response via appropriate channel
  if (cmdSource == cmdSource::NET) {
    // if (time(nullptr) < SECS_YR_2000) {
    //     debugPrint("Time not synchronized\n");
    //     return;
    // }

    // // Use Client Factory to send data
    // BaseClientLayrzHub* client = ClientFactoryLayrzHub::createClient();

    // if (!client) {
    //     debugPrint("Error: Failed to create TCP Socket Client!\n");
    //     // vTaskDelete(NULL);
    // return;
    // }
    // bool success = false;
    // if (isValidTime && isSocketAuth) {
    //     serverResponse response =
    //     client->sendDataToServer(cmdResponseBuff.c_str()); debugPrint("<Pc>
    //     sent with response code: %d\n", response.responseCode); success =
    //     response.responseCode==200;
    // }
    // delete client;  //Prevent memory leaks

    // if (success) {
    //     debugPrint("Command response sent successfully\n");
    // } else {
    //     debugPrint("Failed to send command response\n");
    // }
    while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    BusMessage bm;
    bm.kind = BusMsgKind::PcAck;
    bm.data =
      MessageBusLayrzHub::allocAndCopy(cmdResponseBuff.c_str(), &bm.len);
    // Release semaphore immediately after copying buffer to avoid blocking
    // other producers
    xSemaphoreGive(xSemaphore);
    debugPrint("Publishing <Pc> to MessageBus\n");
    bm.persistIfFail = false;
    bm.freeAfterSend = true;
    if (bm.data && isValidTime) {
      MessageBusLayrzHub::publish(bm);
    } else {
      debugPrint("Failed to alloc for Pc publish or not NTP synced\n");
    }
  } else if (cmdSource == cmdSource::BLE) {
    sendBleConfiotMsg(cmdResponseBuff);
  }
}

void LayrzProtocol::set_dn23e08_do_Handler(const char *cmd_id, const char *args,
                                           int cmdSource) {
  if (!hubSettings.data_serial_en || !hubSettings.rs485_en ||
      hubSettings.rs485_mode != "DN23E08") {
    sendCommandAck(cmd_id, "DN23E08 IO extender not available", cmdSource);
    return;
  }
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> doArray = splitString(args, ",");
  uint8_t rs485Addr = 0;
  byte doIndex = 0;
  uint8_t doValue = 0;
  uint8_t doDuration = 0;
  for (const auto &doElement : doArray) {
    setKey digiOut = parseKeyValue(doElement);
    if (!digiOut.isValid) {
      sendCommandAck(cmd_id, "ARGS error", cmdSource);
      return;
    }
    if (digiOut.key == "rs485_address") {
      rs485Addr = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do_index") {
      doIndex = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do_value") {
      doValue = digiOut.value == "true" ? 1 : 0;
      continue;
    }
    if (digiOut.key == "do_duration") {
      doDuration = std::stoi(digiOut.value);
      continue;
    }
  }

  SerialMonitor::setDN23E08DigitalOutput(rs485Addr, doIndex, doValue,
                                         doDuration);
  std::string msg = "DN23E08 digital output " + std::to_string(doIndex + 1) +
                    " set to " + std::to_string(doValue);
  sendCommandAck(cmd_id, msg.c_str(), cmdSource);
}

void LayrzProtocol::set_dn23e08_multi_do_Handler(const char *cmd_id,
                                                 const char *args,
                                                 int cmdSource) {
  if (!hubSettings.data_serial_en || !hubSettings.rs485_en ||
      hubSettings.rs485_mode != "DN23E08") {
    sendCommandAck(cmd_id, "DN23E08 IO extender not available", cmdSource);
    return;
  }
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> doArray = splitString(args, ",");
  int startAddr = 0;
  int endAddr = 0;
  byte doByte = 0;
  uint8_t doDuration[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (const auto &doElement : doArray) {
    setKey digiOut = parseKeyValue(doElement);
    if (!digiOut.isValid) {
      sendCommandAck(cmd_id, "ARGS error", cmdSource);
      return;
    }
    if (digiOut.key == "start_address") {
      startAddr = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "end_address") {
      endAddr = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do1_value" && digiOut.value == "true") {
      bitSet(doByte, 0);
      continue;
    }
    if (digiOut.key == "do2_value" && digiOut.value == "true") {
      bitSet(doByte, 1);
      continue;
    }
    if (digiOut.key == "do3_value" && digiOut.value == "true") {
      bitSet(doByte, 2);
      continue;
    }
    if (digiOut.key == "do4_value" && digiOut.value == "true") {
      bitSet(doByte, 3);
      continue;
    }
    if (digiOut.key == "do5_value" && digiOut.value == "true") {
      bitSet(doByte, 4);
      continue;
    }
    if (digiOut.key == "do6_value" && digiOut.value == "true") {
      bitSet(doByte, 5);
      continue;
    }
    if (digiOut.key == "do7_value" && digiOut.value == "true") {
      bitSet(doByte, 6);
      continue;
    }
    if (digiOut.key == "do8_value" && digiOut.value == "true") {
      bitSet(doByte, 7);
      continue;
    }
    if (digiOut.key == "do1_duration") {
      doDuration[0] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do2_duration") {
      doDuration[1] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do3_duration") {
      doDuration[2] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do4_duration") {
      doDuration[3] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do5_duration") {
      doDuration[4] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do6_duration") {
      doDuration[5] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do7_duration") {
      doDuration[6] = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "do8_duration") {
      doDuration[7] = std::stoi(digiOut.value);
      continue;
    }
  }
  // debugPrint("DO Byte: %d\n", doByte);
  // debugPrint("Start Address: %d\n", startAddr);
  // debugPrint("End Address: %d\n", endAddr);
  // debugPrint("DO Duration: %d %d %d %d %d %d %d %d\n", doDuration[0],
  // doDuration[1], doDuration[2], doDuration[3], doDuration[4], doDuration[5],
  // doDuration[6], doDuration[7]);
  SerialMonitor::setDN23E08DigitalOutputs(startAddr, endAddr, doByte,
                                          doDuration);
  sendCommandAck(cmd_id, "DN23E08 digital outputs set", cmdSource);
}

void LayrzProtocol::reset_dn23e08_do_Handler(const char *cmd_id,
                                             const char *args, int cmdSource) {
  if (!hubSettings.data_serial_en || !hubSettings.rs485_en ||
      hubSettings.rs485_mode != "DN23E08") {
    sendCommandAck(cmd_id, "DN23E08 IO extender not available", cmdSource);
    return;
  }
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> doArray = splitString(args, ",");
  int startAddr = 0;
  int endAddr = 0;
  byte doByte = 0;
  uint8_t doDuration[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (const auto &doElement : doArray) {
    setKey digiOut = parseKeyValue(doElement);
    if (!digiOut.isValid) {
      sendCommandAck(cmd_id, "ARGS error", cmdSource);
      return;
    }
    if (digiOut.key == "start_address") {
      startAddr = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "end_address") {
      endAddr = std::stoi(digiOut.value);
      continue;
    }
  }
  SerialMonitor::setDN23E08DigitalOutputs(startAddr, endAddr, doByte,
                                          doDuration);
  sendCommandAck(cmd_id, "DN23E08 digital outputs reset", cmdSource);
}

void LayrzProtocol::dn23e08_reboot_Handler(const char *cmd_id, const char *args,
                                           int cmdSource) {

  if (!hubSettings.data_serial_en || !hubSettings.rs485_en ||
      hubSettings.rs485_mode != "DN23E08") {
    sendCommandAck(cmd_id, "DN23E08 IO extender not available", cmdSource);
    return;
  }
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> doArray = splitString(args, ",");
  int startAddr = 0;
  int endAddr = 0;
  for (const auto &doElement : doArray) {
    setKey digiOut = parseKeyValue(doElement);
    if (!digiOut.isValid) {
      sendCommandAck(cmd_id, "ARGS error", cmdSource);
      return;
    }
    if (digiOut.key == "start_address") {
      startAddr = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "end_address") {
      endAddr = std::stoi(digiOut.value);
      continue;
    }
  }
  SerialMonitor::rebootDN23E08(startAddr, endAddr);
  sendCommandAck(cmd_id, "DN23E08 successfully rebooted", cmdSource);
}

void LayrzProtocol::reset_dn23e08_counter_Handler(const char *cmd_id,
                                                  const char *args,
                                                  int cmdSource) {
  if (!hubSettings.data_serial_en || !hubSettings.rs485_en ||
      hubSettings.rs485_mode != "DN23E08") {
    sendCommandAck(cmd_id, "DN23E08 IO extender not available", cmdSource);
    return;
  }
  if (args == nullptr || args[0] == '\0') {
    sendCommandAck(cmd_id, "ARGS error", cmdSource);
    return;
  }
  std::vector<std::string> doArray = splitString(args, ",");
  int startAddr = 0;
  int endAddr = 0;
  byte doByte = 0;
  uint8_t doDuration[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (const auto &doElement : doArray) {
    setKey digiOut = parseKeyValue(doElement);
    if (!digiOut.isValid) {
      sendCommandAck(cmd_id, "ARGS error", cmdSource);
      return;
    }
    if (digiOut.key == "start_address") {
      startAddr = std::stoi(digiOut.value);
      continue;
    }
    if (digiOut.key == "end_address") {
      endAddr = std::stoi(digiOut.value);
      continue;
    }
  }
  SerialMonitor::resetDN23E08TriggerCounter(startAddr, endAddr);
  sendCommandAck(cmd_id, "DN23E08 trigger counter reset", cmdSource);
}

void LayrzProtocol::delete_blackbox_Handler(const char *cmd_id, int cmdSource) {
  // Offload deletion to its own task to avoid long blocking / high stack use
  // inside BLE (nimble_host) task.
  static bool s_delInProgress = false;
  if (s_delInProgress) {
    sendCommandAck(cmd_id, "delete already running", cmdSource);
    return;
  }
  auto bb = MessageBusLayrzHub::blackbox();
  if (!bb) {
    sendCommandAck(cmd_id, "blackbox not enabled", cmdSource);
    return;
  }
  struct DelParams {
    char id[24];
    int source;
  };
  DelParams *p =
    (DelParams *)heap_caps_malloc(sizeof(DelParams), MALLOC_CAP_DEFAULT);
  if (!p) {
    sendCommandAck(cmd_id, "no mem", cmdSource);
    return;
  }
  memset(p, 0, sizeof(*p));
  strncpy(p->id, cmd_id ? cmd_id : "", sizeof(p->id) - 1);
  p->source = cmdSource;
  s_delInProgress = true;
  sendCommandAck(cmd_id, "delete backlog started", cmdSource);
  auto taskFn = [](void *arg) {
    DelParams *dp = (DelParams *)arg;
    bool okRes = false;
    auto bbLocal = MessageBusLayrzHub::blackbox();
    if (bbLocal) {
      okRes = bbLocal->deleteBacklog();
    }
    if (dp->source == cmdSource::NET) {
      LayrzProtocol::sendCommandAck(
        dp->id, okRes ? "blackbox backlog cleared" : "failed to clear backlog",
        dp->source);
    } else {
      debugPrint("blackbox delete %s (BLE origin)\n",
                 okRes ? "done" : "failed");
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    heap_caps_free(dp);
    s_delInProgress = false;
    vTaskDelete(NULL);
  };
  xTaskCreatePinnedToCore(taskFn, "delBB", 4096, p, 3, nullptr, 1);
}

void LayrzProtocol::delete_history_Handler(const char *cmd_id, int cmdSource) {
  static bool s_histDelInProgress = false;
  if (s_histDelInProgress) {
    sendCommandAck(cmd_id, "delete history running", cmdSource);
    return;
  }
  auto bb = MessageBusLayrzHub::blackbox();
  if (!bb) {
    sendCommandAck(cmd_id, "blackbox not enabled", cmdSource);
    return;
  }
  struct HistDelParams {
    char id[24];
    int source;
  };
  HistDelParams *p = (HistDelParams *)heap_caps_malloc(sizeof(HistDelParams),
                                                       MALLOC_CAP_DEFAULT);
  if (!p) {
    sendCommandAck(cmd_id, "no mem", cmdSource);
    return;
  }
  memset(p, 0, sizeof(*p));
  strncpy(p->id, cmd_id ? cmd_id : "", sizeof(p->id) - 1);
  p->source = cmdSource;
  s_histDelInProgress = true;
  sendCommandAck(cmd_id, "history delete started", cmdSource);
  auto taskFn = [](void *arg) {
    HistDelParams *hp = (HistDelParams *)arg;
    bool okRes = false;
    auto bbLocal = MessageBusLayrzHub::blackbox();
    if (bbLocal)
      okRes = bbLocal->deleteHistory();
    if (hp->source == cmdSource::NET) {
      LayrzProtocol::sendCommandAck(
        hp->id, okRes ? "history cleared" : "failed to clear history",
        hp->source);
    } else {
      debugPrint("history delete %s (BLE origin)\n", okRes ? "done" : "failed");
    }
    vTaskDelay(pdMS_TO_TICKS(30));
    heap_caps_free(hp);
    s_histDelInProgress = false;
    vTaskDelete(NULL);
  };
  xTaskCreatePinnedToCore(taskFn, "delHist", 3072, p, 3, nullptr, 1);
}

void LayrzProtocol::delete_images_Handler(const char *cmd_id, int cmdSource) {
  // As images live under /media; perform recursive delete similar to blackbox
  // format behavior.
  static bool s_imgDelInProgress = false;
  if (s_imgDelInProgress) {
    sendCommandAck(cmd_id, "delete images running", cmdSource);
    return;
  }
  struct ImgDelParams {
    char id[24];
    int source;
  };
  ImgDelParams *p =
    (ImgDelParams *)heap_caps_malloc(sizeof(ImgDelParams), MALLOC_CAP_DEFAULT);
  if (!p) {
    sendCommandAck(cmd_id, "no mem", cmdSource);
    return;
  }
  memset(p, 0, sizeof(*p));
  strncpy(p->id, cmd_id ? cmd_id : "", sizeof(p->id) - 1);
  p->source = cmdSource;
  s_imgDelInProgress = true;
  sendCommandAck(cmd_id, "images delete started", cmdSource);
  auto taskFn = [](void *arg) {
    ImgDelParams *ip = (ImgDelParams *)arg;
    bool okRes = false;
    // Simple recursive delete of /media
    File dir = SD.open("/media");
    if (dir) {
      dir.close();
      // Walk entries
      File root = SD.open("/media");
      if (root) {
        for (File f = root.openNextFile(); f; f = root.openNextFile()) {
          String name = f.name();
          bool isDir = f.isDirectory();
          f.close();
          if (!name.startsWith("/"))
            name = String("/media/") + name; // ensure path
          if (isDir) {
            // skip nested dirs for now (none expected)
          } else {
            SD.remove(name);
          }
        }
        root.close();
      }
      SD.rmdir("/media");
      okRes = true;
    } else {
      okRes = true; // treat as success if folder absent
    }
    if (ip->source == cmdSource::NET) {
      LayrzProtocol::sendCommandAck(
        ip->id, okRes ? "images cleared" : "failed to clear images",
        ip->source);
    } else {
      debugPrint("images delete %s (BLE origin)\n", okRes ? "done" : "failed");
    }
    vTaskDelay(pdMS_TO_TICKS(30));
    heap_caps_free(ip);
    s_imgDelInProgress = false;
    vTaskDelete(NULL);
  };
  xTaskCreatePinnedToCore(taskFn, "delImg", 4096, p, 3, nullptr, 1);
}

void LayrzProtocol::format_sd_Handler(const char *cmd_id, int cmdSource) {
  // Run format asynchronously to avoid long blocking inside BLE/NimBLE host
  // task (previously caused stack canary).
  static bool s_formatInProgress = false;
  if (s_formatInProgress) {
    sendCommandAck(cmd_id, "format already running", cmdSource);
    return;
  }
  auto bb = MessageBusLayrzHub::blackbox();
  if (!bb) {
    sendCommandAck(cmd_id, "blackbox not enabled", cmdSource);
    return;
  }
  // Copy command id for possible final ack in async task
  struct FormatParams {
    char id[24];
    int source;
  }; // id length small; adjust if needed
  FormatParams *p =
    (FormatParams *)heap_caps_malloc(sizeof(FormatParams), MALLOC_CAP_DEFAULT);
  if (!p) {
    sendCommandAck(cmd_id, "no mem", cmdSource);
    return;
  }
  memset(p, 0, sizeof(*p));
  strncpy(p->id, cmd_id ? cmd_id : "", sizeof(p->id) - 1);
  p->source = cmdSource;
  s_formatInProgress = true;
  sendCommandAck(cmd_id, "format started", cmdSource);
  auto taskFn = [](void *arg) {
    FormatParams *fp = (FormatParams *)arg;
    auto bbLocal = MessageBusLayrzHub::blackbox();
    bool okRes = false;
    if (bbLocal) {
      okRes = bbLocal->formatMediaFAT32();
    }
    // For NET we can send a completion ack; for BLE avoid NimBLE calls from
    // this task (risky); rely on started ack.
    if (fp->source == cmdSource::NET) {
      LayrzProtocol::sendCommandAck(
        fp->id, okRes ? "format done" : "format failed", fp->source);
    } else {
      debugPrint("SD format %s (BLE origin)\n", okRes ? "done" : "failed");
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    heap_caps_free(fp);
    s_formatInProgress = false;
    vTaskDelete(NULL);
  };
  xTaskCreatePinnedToCore(taskFn, "fmtSD", 4096, p, 3, nullptr, 1);
}

void LayrzProtocol::take_photo_Handler(const char *cmd_id, int cmdSource) {

  if (!hubSettings.acam_en) {
    sendCommandAck(cmd_id, "Camera not available", cmdSource);
    return;
  } else {
    sendCommandAck(cmd_id,
                   ArducamLayrzHub::takePicture() ? "Photo taken"
                                                  : "Failed to take photo",
                   cmdSource);
  }
}
