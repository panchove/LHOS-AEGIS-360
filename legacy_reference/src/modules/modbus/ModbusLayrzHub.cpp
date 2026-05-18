#include <modules/modbus/ModbusLayrzHub.h>

ModbusClientRTU
  ModbusLayrzHub::modbusClient(RS485_RE_DE); // DE/RE pin for RS485
SpiRamJsonDocument *ModbusLayrzHub::_jsonModbusMap = nullptr;
uint8_t modbusData[30] = {0};     // Buffer to hold Modbus data
uint8_t modbusAddresses[4] = {0}; // Buffer to hold Modbus addresses
uint8_t modbusDeviceIds[4] = {0}; // Buffer to hold Mod
bool dataReady = false;           // Flag to indicate if data is ready

bool ModbusLayrzHub::initModbus() {
  // Initialize the Modbus client
  SoftwareSerialConfig serialConfig;
  if (hubSettings.rs485_bits == 8) {
    if (hubSettings.rs485_par == "even") {
      serialConfig = SWSERIAL_8E1;
    } else if (hubSettings.rs485_par == "odd") {
      serialConfig = SWSERIAL_8O1;
    } else {
      serialConfig = SWSERIAL_8N1;
    }
  } else {
    if (hubSettings.rs485_par == "even") {
      serialConfig = SWSERIAL_7E1;
    } else if (hubSettings.rs485_par == "odd") {
      serialConfig = SWSERIAL_7O1;
    } else {
      serialConfig = SWSERIAL_7N1;
    }
  }
  RS485.begin(hubSettings.rs485_baud, serialConfig, RS485_RX,
              RS485_TX); // Start the serial port with specified parameters
  modbusClient.begin(
    RS485,
    hubSettings.rs485_baud);   // Stream variant requires baudrate for timing
  modbusClient.setTimeout(50); // Set timeout for Modbus requests
  modbusClient.onDataHandler(&_handleModbusData);
  modbusClient.onErrorHandler(&_handleModbusError);
  if (!_loadModbusMap()) {
    debugPrint("Failed to load Modbus map\n");
    return false;
  }
  modbusAddresses[0] = hubSettings.modbus_addr1;
  modbusAddresses[1] = hubSettings.modbus_addr2;
  modbusAddresses[2] = hubSettings.modbus_addr3;
  modbusAddresses[3] = hubSettings.modbus_addr4;
  modbusDeviceIds[0] = hubSettings.modbus_dev1;
  modbusDeviceIds[1] = hubSettings.modbus_dev2;
  modbusDeviceIds[2] = hubSettings.modbus_dev3;
  modbusDeviceIds[3] = hubSettings.modbus_dev4;
  dataReady = false; // Reset data ready flag

  // Start the Modbus polling task
  xTaskCreatePinnedToCore(_modbusPollTask, "Modbus Poll Task", 4096, NULL, 5,
                          NULL, 1);
  // Start the Modbus publish task
  xTaskCreatePinnedToCore(_modbusPublishTask, "Modbus Publish Task", 4096, NULL,
                          5, NULL, 1);
  return true;
}

void ModbusLayrzHub::_handleModbusData(ModbusMessage msg, uint32_t token) {
  memset(modbusData, 0, sizeof(modbusData)); // Clear the modbusData buffer
  // Process the message as needed
  // skip the first byte (server ID) and second byte (function code)
  if (msg.size() < 3) {
    debugPrint("Invalid Modbus message size: %d\n", msg.size());
    return;
  }
  // Optionally, you can also process the message further or store it
  // For example, you can store the data in a buffer or process it based on the
  // function code and server ID.
  // if (hubSettings.sys_debug_en)
  // {
  //     debugPrint("Modbus Data: ");
  //     for (size_t i = 3; i < msg.size(); ++i) {
  //         UARTSerial.printf("%02X ", msg[i]);
  //         modbusData[i-3] = msg[i]; // Store the data in modbusData buffer
  //     }
  //     UARTSerial.print("\n");
  // }
  // Set data ready flag
  dataReady = true; // Indicate that data is ready for processing
}

void ModbusLayrzHub::_handleModbusError(Error error, uint32_t token) {
  ModbusError me(error);
  debugPrint("Error code: %02X - %s\n", (int)me, (const char *)me);
}

void ModbusLayrzHub::_sendModbusRequest(uint32_t token, uint8_t serverId,
                                        uint8_t functionCode,
                                        uint16_t startAddress,
                                        uint16_t numRegisters) {
  // Create a Modbus message and send it
  Error err = modbusClient.addRequest(token, serverId, functionCode,
                                      startAddress, numRegisters);
  if (err != SUCCESS) {
    ModbusError e(err);
    debugPrint("Error creating request: %02X - %s\n", (int)e, (const char *)e);
  }
}

std::string ModbusLayrzHub::_decodeModbusMessage(uint8_t *data, size_t length,
                                                 uint16_t serverId,
                                                 uint8_t modbusAddress,
                                                 uint8_t functionCode,
                                                 uint16_t startAddress) {
  // Process the Modbus response
  // debugPrint("Decoding Modbus Response\n");
  // debugPrint("Data length: %d, Server ID: %d, Function Code: %d, Start
  // Address: %d\n", length, serverId, functionCode, startAddress);
  JsonArray devicesArray = (*_jsonModbusMap)["d"].as<JsonArray>();
  std::ostringstream oss;
  for (JsonObject device : devicesArray) {
    if (device["id"].as<uint16_t>() == serverId) { // Check if device ID matches
      if (functionCode ==
          3) { // Function code 3 is for reading holding registers

        JsonArray holdingRegisters = device["hr"].as<JsonArray>();
        for (JsonObject hr : holdingRegisters) {
          uint16_t regStartAddress = hr["ao"].as<uint16_t>();
          std::string label = hr["l"].as<std::string>();
          if (regStartAddress == startAddress) {
            if (hr.containsKey("st")) {
              JsonArray states = hr["st"].as<JsonArray>();
              for (JsonObject state : states) {
                uint16_t dataValue = data[0] << 8 | data[1];
                uint16_t stateValue = state["v"].as<uint16_t>();
                if (stateValue == dataValue) {
                  oss << "modbus." << std::to_string(modbusAddress) << "."
                      << label << ":" << state["n"].as<std::string>() << ",";
                  return oss.str(); // Return the decoded message as a string
                }
              }
              oss << "modbus." << std::to_string(modbusAddress) << "." << label
                  << ":unknown,";
              return oss.str(); // Return the decoded message as a string
            }
            uint16_t bytes = hr["bl"].as<uint16_t>() / 8;
            std::string regType = hr["dt"].as<std::string>();
            bool isSigned = hr["is"].as<bool>();
            float offset = hr["o"].as<float>();
            float multiplier = hr["m"].as<float>();

            if (regType == "int") {
              // int could be 16-bit, 32-bit, or 64-bit signed or unsigned
              int64_t value = 0;
              if (bytes == 2) {
                uint16_t raw = (static_cast<uint16_t>(data[0]) << 8) |
                               data[1]; // Big-endian 16-bit
                value = isSigned ? static_cast<int16_t>(raw) : raw;
              } else if (bytes == 4) {
                uint32_t raw =
                  (static_cast<uint32_t>(data[0]) << 24) |
                  (static_cast<uint32_t>(data[1]) << 16) |
                  (static_cast<uint32_t>(data[2]) << 8) |
                  static_cast<uint32_t>(data[3]); // Big-endian 32-bit
                value = isSigned ? static_cast<int32_t>(raw) : raw;
              } else if (bytes == 8) {
                uint64_t raw =
                  (static_cast<uint64_t>(data[0]) << 56) |
                  (static_cast<uint64_t>(data[1]) << 48) |
                  (static_cast<uint64_t>(data[2]) << 40) |
                  (static_cast<uint64_t>(data[3]) << 32) |
                  (static_cast<uint64_t>(data[4]) << 24) |
                  (static_cast<uint64_t>(data[5]) << 16) |
                  (static_cast<uint64_t>(data[6]) << 8) |
                  static_cast<uint64_t>(data[7]); // Big-endian 64-bit
                value = isSigned ? static_cast<int64_t>(raw) : raw;
              }
              float finalValue =
                (value * multiplier) + offset; // Apply multiplier and offset
              oss << "modbus." << std::to_string(modbusAddress) << "." << label
                  << ":" << std::fixed << std::setprecision(3) << finalValue
                  << ",";
            }
          }
        }
      } else if (functionCode ==
                 4) { // Function code 4 is for reading input registers

        JsonArray inputRegisters = device["ir"].as<JsonArray>();
        for (JsonObject ir : inputRegisters) {
          uint16_t regStartAddress = ir["ao"].as<uint16_t>();
          std::string label = ir["l"].as<std::string>();
          if (regStartAddress == startAddress) {
            if (ir.containsKey("st")) {
              JsonArray states = ir["st"].as<JsonArray>();
              for (JsonObject state : states) {
                uint16_t dataValue = data[0] << 8 | data[1];
                uint16_t stateValue = state["v"].as<uint16_t>();
                if (stateValue == dataValue) {
                  oss << "modbus." << std::to_string(modbusAddress) << "."
                      << label << ":" << state["n"].as<std::string>() << ",";
                  return oss.str(); // Return the decoded message as a string
                }
              }
              oss << "modbus." << std::to_string(modbusAddress) << "." << label
                  << ":unknown,";
              return oss.str(); // Return the decoded message as a string
            }
            uint16_t bytes = ir["bl"].as<uint16_t>() / 8;
            std::string regType = ir["dt"].as<std::string>();
            bool isSigned = ir["is"].as<bool>();
            float offset = ir["o"].as<float>();
            float multiplier = ir["m"].as<float>();

            if (regType == "int") {
              // int could be 16-bit, 32-bit, or 64-bit signed or unsigned
              int64_t value = 0;
              if (bytes == 2) {
                uint16_t raw = (static_cast<uint16_t>(data[0]) << 8) |
                               data[1]; // Big-endian 16-bit
                value = isSigned ? static_cast<int16_t>(raw) : raw;
              } else if (bytes == 4) {
                uint32_t raw =
                  (static_cast<uint32_t>(data[0]) << 24) |
                  (static_cast<uint32_t>(data[1]) << 16) |
                  (static_cast<uint32_t>(data[2]) << 8) |
                  static_cast<uint32_t>(data[3]); // Big-endian 32-bit
                value = isSigned ? static_cast<int32_t>(raw) : raw;
              } else if (bytes == 8) {
                uint64_t raw =
                  (static_cast<uint64_t>(data[0]) << 56) |
                  (static_cast<uint64_t>(data[1]) << 48) |
                  (static_cast<uint64_t>(data[2]) << 40) |
                  (static_cast<uint64_t>(data[3]) << 32) |
                  (static_cast<uint64_t>(data[4]) << 24) |
                  (static_cast<uint64_t>(data[5]) << 16) |
                  (static_cast<uint64_t>(data[6]) << 8) |
                  static_cast<uint64_t>(data[7]); // Big-endian 64-bit
                value = isSigned ? static_cast<int64_t>(raw) : raw;
              }
              float finalValue =
                (value * multiplier) + offset; // Apply multiplier and offset
              oss << "modbus." << std::to_string(modbusAddress) << "." << label
                  << ":" << std::fixed << std::setprecision(3) << finalValue
                  << ",";
            }
          }
        }
      } else if (functionCode == 1) { // Function code 1 is for reading coils
        JsonArray digitalOutputCoils = device["doc"].as<JsonArray>();
        for (JsonObject doc : digitalOutputCoils) {
          uint16_t regStartAddress = doc["ao"].as<uint16_t>();
          std::string label = doc["l"].as<std::string>();
          if (regStartAddress == startAddress) {
            uint8_t value = data[0];
            oss << "modbus." << std::to_string(modbusAddress) << "." << label
                << ":" << std::to_string(value == 1) << std::string(",");
            return oss.str(); // Return the decoded message as a string
          }
        }
      } else if (functionCode ==
                 2) { // Function code 2 is for reading discrete inputs
        JsonArray digitalInputCoils = device["dic"].as<JsonArray>();
        for (JsonObject dic : digitalInputCoils) {
          uint16_t regStartAddress = dic["ao"].as<uint16_t>();
          std::string label = dic["l"].as<std::string>();
          if (regStartAddress == startAddress) {
            uint8_t value = data[0];
            oss << "modbus." << std::to_string(modbusAddress) << "." << label
                << ":" << std::to_string(value == 1) << std::string(",");
            return oss.str(); // Return the decoded message as a string
          }
        }
      } else {
        debugPrint("Unsupported function code: %d\n", functionCode);
      }
    }
  }
  return oss.str(); // Return the decoded message as a string
}

void ModbusLayrzHub::_processModbusError(Error error) {
  // Process Modbus error
  ModbusError me(error);
  debugPrint("Error code: %02X - %s\n", (int)me, (const char *)me);
}

bool ModbusLayrzHub::_loadModbusMap() {
  // Load the Modbus map from JSON or other sources
  if (!SPIFFS.begin(true)) {
    debugPrint("Failed to mount SPIFFS\n");
    return false;
  }
  File file = SPIFFS.open("/modbusMaps.json", "r");
  if (!file) {
    debugPrint("Failed to open modbus_map.json\n");
    return false;
  }
  size_t fileSize = file.size();
  if (fileSize == 0) {
    debugPrint("modbusMaps.json is empty\n");
    return false;
  }
  size_t jsonSize =
    fileSize * 2.1; // Adjust multiplier based on your JSON structure

  if (_jsonModbusMap != nullptr) {
    delete _jsonModbusMap; // Clean up previous instance
  }
  _jsonModbusMap = new SpiRamJsonDocument(jsonSize);
  DeserializationError error = deserializeJson(*_jsonModbusMap, file);
  file.close();
  if (error) {
    debugPrint("Deserialization error: %s\n", error.c_str());
    delete _jsonModbusMap;
    _jsonModbusMap = nullptr;
    return false;
  }
  debugPrint("Modbus map loaded successfully\n");
  return true;
}

void ModbusLayrzHub::_modbusPollTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  uint16_t timeout = 100; // Polling timeout in milliseconds
  while (true) {
    while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE) {
      vTaskDelay(pdMS_TO_TICKS(10)); // Wait for the mutex to be available
      esp_task_wdt_reset();          // Reset the watchdog timer
    }
    memset(modbusMsgBuffer, 0,
           MODBUS_MSG_BUFFER_SIZE); // Clear the modbus message buffer
    std::string position = GNSS::getPosition();
    strncat(modbusMsgBuffer, std::to_string(time(NULL)).c_str(),
            MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);
    strncat(modbusMsgBuffer, ";",
            MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);
    strncat(modbusMsgBuffer, position.c_str(),
            MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);
    strncat(modbusMsgBuffer, "report.code:LKMODBUS,",
            MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);
    uint16_t initialSize = strlen(modbusMsgBuffer);
    if (_jsonModbusMap) {
      // serializeJson(*jsonPgnDoc, UARTSerial);
      JsonArray devicesArray = (*_jsonModbusMap)["d"].as<JsonArray>();
      for (int i = 0; i < 3; i++) {
        if (modbusAddresses[i] == 0 || modbusDeviceIds[i] == 0)
          continue; // Skip if address is 0 or device ID is 0
        for (JsonObject device : devicesArray) {
          if (device["id"].as<int>() ==
              modbusDeviceIds[i]) { // Check if device ID matches
            uint16_t modbusDeviceId = device["id"].as<uint16_t>();
            std::string deviceModel = device["md"].as<std::string>();
            if (device.containsKey(
                  "hr")) { // Check if device has holding registers
              JsonArray holdingRegisters = device["hr"].as<JsonArray>();
              for (JsonObject hr : holdingRegisters) {
                uint16_t startAddress = hr["ao"].as<uint16_t>();
                uint16_t numRegisters = static_cast<uint16_t>(
                  hr["bl"].as<uint16_t>() / 16); // Assuming 16-bit registers
                //   debugPrint("device: %s, 485 address: %d, requesting %d
                //   holding registers from address %d\n", deviceModel.c_str(),
                //   modbusAddresses[i], numRegisters, startAddress);
                uint32_t startTime = millis(); // Use current time as token
                dataReady = false;             // Reset data ready flag
                _sendModbusRequest(startTime, modbusAddresses[i], 3,
                                   startAddress, numRegisters);
                while (!dataReady && (millis() - startTime < timeout)) {
                  vTaskDelay(pdMS_TO_TICKS(1)); // Delay to avoid busy-waiting
                }
                if (!dataReady) {
                  debugPrint("Timeout waiting for Modbus data for device ID %d "
                             "at address %d\n",
                             modbusDeviceId, startAddress);
                  continue; // Skip to the next register if timeout occurs
                }
                std::string msg = _decodeModbusMessage(
                  modbusData, sizeof(modbusData), modbusDeviceId,
                  modbusAddresses[i], 3, startAddress);
                if (!msg.empty()) {
                  strncat(modbusMsgBuffer, msg.c_str(),
                          MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);
                } else {
                  debugPrint(
                    "No data received for device ID %d at address %d\n",
                    modbusDeviceId, startAddress);
                }
                vTaskDelay(pdMS_TO_TICKS(10)); // Delay to avoid busy-waiting
              }
            }
            if (device.containsKey(
                  "ir")) { // Check if device has input registers
              JsonArray inputRegisters = device["ir"].as<JsonArray>();
              for (JsonObject ir : inputRegisters) {
                uint16_t startAddress = ir["ao"].as<uint16_t>();
                uint16_t numRegisters = static_cast<uint16_t>(
                  ir["bl"].as<uint16_t>() / 16); // Assuming 16-bit registers
                debugPrint("device: %s, 485 address: %d, requesting %d input "
                           "registers from address %d\n",
                           deviceModel.c_str(), modbusAddresses[i],
                           numRegisters, startAddress);
                uint32_t startTime = millis(); // Use current time as token
                dataReady = false;             // Reset data ready flag
                _sendModbusRequest(startTime, modbusAddresses[i], 4,
                                   startAddress, numRegisters);
                while (!dataReady && (millis() - startTime < timeout)) {
                  vTaskDelay(pdMS_TO_TICKS(1)); // Delay to avoid busy-waiting
                }
                if (!dataReady) {
                  debugPrint("Timeout waiting for Modbus data for device ID %d "
                             "at address %d\n",
                             modbusDeviceId, startAddress);
                  continue; // Skip to the next register if timeout occurs
                }
                std::string msg = _decodeModbusMessage(
                  modbusData, sizeof(modbusData), modbusDeviceId,
                  modbusAddresses[i], 4, startAddress);
                if (!msg.empty()) {
                  strncat(modbusMsgBuffer, msg.c_str(),
                          MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);
                } else {
                  debugPrint(
                    "No data received for device ID %d at address %d\n",
                    modbusDeviceId, startAddress);
                }
                vTaskDelay(pdMS_TO_TICKS(10)); // Delay to avoid busy-waiting
              }
            }
            if (device.containsKey(
                  "doc")) { // Check if device has discrete output coils
              JsonArray discreteOutputCoils = device["doc"].as<JsonArray>();
              for (JsonObject doc : discreteOutputCoils) {
                uint16_t startAddress = doc["ao"].as<uint16_t>();
                debugPrint(
                  "device: %s, 485 address: %d, requesting coil status "
                  "from address %d\n",
                  deviceModel.c_str(), modbusAddresses[i], startAddress);
                uint32_t startTime = millis(); // Use current time as token
                dataReady = false;             // Reset data ready flag
                _sendModbusRequest(startTime, modbusAddresses[i], 1,
                                   startAddress, 1);
                while (!dataReady && (millis() - startTime < timeout)) {
                  vTaskDelay(pdMS_TO_TICKS(1)); // Delay to avoid busy-waiting
                }
                if (!dataReady) {
                  debugPrint("Timeout waiting for Modbus data for device ID %d "
                             "at address %d\n",
                             modbusDeviceId, startAddress);
                  continue; // Skip to the next register if timeout occurs
                }
                std::string msg = _decodeModbusMessage(
                  modbusData, sizeof(modbusData), modbusDeviceId,
                  modbusAddresses[i], 1, startAddress);
                if (!msg.empty()) {
                  strncat(modbusMsgBuffer, msg.c_str(),
                          MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);
                } else {
                  debugPrint(
                    "No data received for device ID %d at address %d\n",
                    modbusDeviceId, startAddress);
                }
                vTaskDelay(pdMS_TO_TICKS(10)); // Delay to avoid busy-waiting
              }
            }
            if (device.containsKey(
                  "dic")) { // Check if device has discrete input contacts
              JsonArray discreteInputContacts = device["dic"].as<JsonArray>();
              for (JsonObject dic : discreteInputContacts) {
                uint16_t startAddress = dic["ao"].as<uint16_t>();
                debugPrint(
                  "device: %s, 485 address: %d, requesting discrete input "
                  "from address %d\n",
                  deviceModel.c_str(), modbusAddresses[i], startAddress);
                uint32_t startTime = millis(); // Use current time as token
                dataReady = false;             // Reset data ready flag
                _sendModbusRequest(startTime, modbusAddresses[i], 2,
                                   startAddress, 1);
                while (!dataReady && (millis() - startTime < timeout)) {
                  vTaskDelay(pdMS_TO_TICKS(1)); // Delay to avoid busy-waiting
                }
                if (!dataReady) {
                  debugPrint("Timeout waiting for Modbus data for device ID %d "
                             "at address %d\n",
                             modbusDeviceId, startAddress);
                  continue; // Skip to the next register if timeout occurs
                }
                std::string msg = _decodeModbusMessage(
                  modbusData, sizeof(modbusData), modbusDeviceId,
                  modbusAddresses[i], 2, startAddress);
                if (!msg.empty()) {
                  strncat(modbusMsgBuffer, msg.c_str(),
                          MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);
                } else {
                  debugPrint(
                    "No data received for device ID %d at address %d\n",
                    modbusDeviceId, startAddress);
                }
                vTaskDelay(pdMS_TO_TICKS(10)); // Delay to avoid busy-waiting
              }
            }
          }
        }
      }
    }

    if (strlen(modbusMsgBuffer) > initialSize) {
      // replace the last comma with a semi-colon and null-terminate the string
      modbusMsgBuffer[strlen(modbusMsgBuffer) - 1] = ';';
      modbusMsgBuffer[strlen(modbusMsgBuffer)] = '\0';

      std::string crc = calculateCRC16(modbusMsgBuffer);
      strncat(modbusMsgBuffer, crc.c_str(),
              MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);

      // Add <Pd> at the beginning and </Pd> at the end

      for (int i = strlen(modbusMsgBuffer); i >= 0; i--) {
        modbusMsgBuffer[i + 4] =
          modbusMsgBuffer[i]; // Shift characters to the right
      }
      modbusMsgBuffer[0] = '<';
      modbusMsgBuffer[1] = 'P';
      modbusMsgBuffer[2] = 'd';
      modbusMsgBuffer[3] = '>';
      strncat(modbusMsgBuffer, "</Pd>",
              MODBUS_MSG_BUFFER_SIZE - strlen(modbusMsgBuffer) - 1);
      modbusMsgBuffer[strlen(modbusMsgBuffer)] = '\0';
    }
    xSemaphoreGive(xSemaphore);
    if (hubSettings.modbus_poll_int > 10) {
      for (int i = 0; i < int(hubSettings.modbus_poll_int / 5); i++) {
        esp_task_wdt_reset();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    } else {
      esp_task_wdt_reset();
      vTaskDelay(hubSettings.modbus_poll_int * 1000 / portTICK_PERIOD_MS);
    }
  }
}

void ModbusLayrzHub::_modbusPublishTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  while (true) {
    while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE) {
      vTaskDelay(pdMS_TO_TICKS(10)); // Wait for the mutex to be available
      esp_task_wdt_reset();          // Reset the watchdog timer
    }
    if (strlen(modbusMsgBuffer) == 0) {
      xSemaphoreGive(xSemaphore);
      vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to avoid busy-waiting
      continue;                        // Skip if there's no message to publish
    }
    BusMessage bm;
    bm.kind = BusMsgKind::PdSensor; // proper classification for <Pd>
    bm.data = MessageBusLayrzHub::allocAndCopy(modbusMsgBuffer, &bm.len);
    // Release semaphore immediately after copying buffer to avoid blocking
    // other producers
    xSemaphoreGive(xSemaphore);
    debugPrint("Publishing <Pd> from Modbus to MessageBus (len=%u)\n",
               (unsigned)bm.len);
    bm.persistIfFail = true;
    bm.freeAfterSend = true;
    if (bm.data && isValidTime) {
      MessageBusLayrzHub::publish(bm);
    } else {
      debugPrint("Failed to alloc for Pd publish or not NTP synced\n");
    }
    if (hubSettings.data_pub_per > 10) {
      for (int i = 0; i < int(hubSettings.data_pub_per / 5); i++) {
        esp_task_wdt_reset();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    } else {
      esp_task_wdt_reset();
      vTaskDelay(hubSettings.data_pub_per * 1000 / portTICK_PERIOD_MS);
    }
  }
}
