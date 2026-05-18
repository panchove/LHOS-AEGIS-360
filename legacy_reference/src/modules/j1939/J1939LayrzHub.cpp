#include <modules/j1939/J1939LayrzHub.h>

ARD1939ESP32S3 J1939LayrzHub::j1939;
J1939Decoder J1939LayrzHub::decoder;

bool J1939LayrzHub::initJ1939() {
  int baudRate;
  if (hubSettings.can_mode == SAEJ1939_STD_250K ||
      hubSettings.can_mode == SAEJ1939_FMS_250K) {
    baudRate = 250000;
  } else if (hubSettings.can_mode == SAEJ1939_STD_500K ||
             hubSettings.can_mode == SAEJ1939_FMS_500K) {
    baudRate = 500000;
  } else {
    baudRate = 250000;
  }
  // Initialize J1939 stack
  if (j1939.Init(SYSTEM_TIME, baudRate) == 0) {
    debugPrint("J1939 Canbus initialized successfully.\n");
  } else {
    debugPrint("J1939 Canbus initialization failed.\n");
    return false;
  }
  // Initialize the J1939 decoder
  if (!decoder.begin()) {
    debugPrint("Decoder initialization failed!\n");
    return false;
  }

  // Set preferred source address and address range
  j1939.SetPreferredAddress(SA_PREFERRED);
  j1939.SetAddressRange(ADDRESSRANGEBOTTOM, ADDRESSRANGETOP);

  // Configure the device NAME
  j1939.SetNAME(NAME_IDENTITY_NUMBER, NAME_MANUFACTURER_CODE,
                NAME_FUNCTION_INSTANCE, NAME_ECU_INSTANCE, NAME_FUNCTION,
                NAME_VEHICLE_SYSTEM, NAME_VEHICLE_SYSTEM_INSTANCE,
                NAME_INDUSTRY_GROUP, NAME_ARBITRARY_ADDRESS_CAPABLE);
  return true;
}

void J1939LayrzHub::j1939MonitorTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  byte msgType;
  uint32_t pgn;
  byte msgData[1785];
  int msgLen;
  byte destAddr;
  byte srcAddr;
  byte priority;
  std::unordered_map<uint32_t, char *>
    uniqueMessages; // unordered_map to hold PGN and pointers to decoded
                    // messages

  while (true) {
    // if (hubSettings.net_protocol == 0 && hubSettings.can_pub_per > 240) {
    //     tcpNonSslClient->stop();    // Close the connection
    //     isSocketAuth = false;
    //     uint16_t port = hubSettings.net_server == "link.layrz.network" ?
    //     STABLE_SOCKET_PORT : TESTING_SOCKET_PORT;
    //     tcpNonSslClient->connect(hubSettings.net_server.c_str(), port);
    //     debugPrint("Reconnecting to TCP server\n");
    // }
    while (true) {
      if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
        if (isValidTime && isSocketAuth) {
          break;
        }
        xSemaphoreGive(xSemaphore);
      }
      vTaskDelay(pdMS_TO_TICKS(1000)); // Wait before retrying
      esp_task_wdt_reset();            // Reset the watchdog timer
    }
    memset(canDataBuffer, 0, CANBUS_BUFFER_SIZE); // Initialize with zeros
    debugPrint("Reading J1939 messages\n");
    std::string position = GNSS::getPosition();
    strncat(canDataBuffer, std::to_string(time(NULL)).c_str(),
            CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
    strncat(canDataBuffer, ";", CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
    strncat(canDataBuffer, position.c_str(),
            CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
    strncat(canDataBuffer, "report.code:LKCAN,",
            CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);

    long time = millis();
    while (millis() - time < hubSettings.can_poll_win * 1000) {
      byte response = j1939.Operate(&msgType, &pgn, msgData, &msgLen, &destAddr,
                                    &srcAddr, &priority);
      if (response == J1939_MSG_APP) {
        debugPrint("PGN: %u\n", pgn);
        debugPrint("Data: ");
        if (hubSettings.sys_debug_en) {
          for (int i = 0; i < msgLen; i++) {
            if (Print *console = debugConsole()) {
              if (msgData[i] < 0x10)
                console->print("0");
              console->print(msgData[i], HEX);
              console->print(" ");
            }
          }
          if (Print *console = debugConsole())
            console->println();
        }
        std::string decodedStr;
        if (hubSettings.can_decode_en)
          decodedStr = decoder.decode(pgn, msgData, msgLen);
        else {
          std::stringstream ss;
          ss << "j1939.raw.data." << pgn << ":";
          for (int i = 0; i < msgLen; i++) {
            ss << std::uppercase << std::setfill('0') << std::setw(2)
               << std::hex << (int)msgData[i];
          }
          decodedStr = ss.str();
        }
        debugPrint("Decoded Message: ");
        if (hubSettings.sys_debug_en) {
          if (Print *console = debugConsole()) {
            console->println(decodedStr.c_str());
            console->println();
          }
        }

        // Allocate decoded message in PSRAM
        char *decoded =
          (char *)heap_caps_malloc(decodedStr.length() + 1, MALLOC_CAP_SPIRAM);
        if (decoded == NULL) {
          debugPrint("PSRAM Allocation Failed for decoded message\n");
          continue;
        }
        strcpy(decoded, decodedStr.c_str());

        // Replace existing message for the PGN if already exists
        if (uniqueMessages.find(pgn) != uniqueMessages.end()) {
          free(uniqueMessages[pgn]);
        }
        uniqueMessages[pgn] = decoded;
      }

      esp_task_wdt_reset(); // Reset watchdog
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    if (uniqueMessages.size() == 0) {
      xSemaphoreGive(xSemaphore);
      esp_task_wdt_reset();
      for (auto &entry : uniqueMessages) {
        free(entry.second);
      }
      uniqueMessages.clear();
      vTaskDelay(hubSettings.can_pub_per * 1000 / portTICK_PERIOD_MS);
      continue;
    }

    // Concatenate unique messages into canDataBuffer
    for (const auto &entry : uniqueMessages) {
      strncat(canDataBuffer, entry.second,
              CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
      strncat(canDataBuffer, ",",
              CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
    }

    // replace the last comma with a semi-colon and null-terminate the string
    canDataBuffer[strlen(canDataBuffer) - 1] = ';';
    canDataBuffer[strlen(canDataBuffer)] = '\0';

    std::string crc = calculateCRC16(canDataBuffer);
    strncat(canDataBuffer, crc.c_str(),
            CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);

    // Add <Pd> at the beginning and </Pd> at the end

    for (int i = strlen(canDataBuffer); i >= 0; i--) {
      canDataBuffer[i + 4] = canDataBuffer[i]; // Shift characters to the right
    }
    canDataBuffer[0] = '<';
    canDataBuffer[1] = 'P';
    canDataBuffer[2] = 'd';
    canDataBuffer[3] = '>';
    strncat(canDataBuffer, "</Pd>",
            CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
    canDataBuffer[strlen(canDataBuffer)] = '\0';

    if (hubSettings.sys_debug_en) {
      debugPrint("CANBUS Data: ");
      if (hubSettings.sys_debug_en) {
        if (Print *console = debugConsole())
          console->println(canDataBuffer);
      }
    }
    // BaseClientLayrzHub* client = ClientFactoryLayrzHub::createClient();

    // if (!client) {
    //     debugPrint("Error: Failed to create a Transport Client!\n");
    //     xSemaphoreGive(xSemaphore);
    //     esp_task_wdt_reset();
    //     // Free allocated memory for decoded messages
    //     for (auto& entry : uniqueMessages) {
    //         free(entry.second);
    //     }
    //     uniqueMessages.clear();
    //     vTaskDelay(hubSettings.can_pub_per * 1000 / portTICK_PERIOD_MS);
    //     continue;
    // }
    // if (isSocketAuth && isValidTime) {
    //     serverResponse response = client->sendDataToServer(canDataBuffer);
    //     debugPrint("<Pd> with J1939 data sent - Response code: %d\n",
    //     response.responseCode);
    // }
    // delete client;  //Prevent memory leaks

    BusMessage bm;
    bm.kind = BusMsgKind::PdSensor;
    bm.data = MessageBusLayrzHub::allocAndCopy(canDataBuffer, &bm.len);
    // Release semaphore immediately after copying buffer to avoid blocking
    // other producers
    xSemaphoreGive(xSemaphore);
    debugPrint("Publishing <Pd> from J1939 to MessageBus\n");
    bm.persistIfFail = true;
    bm.freeAfterSend = true;
    if (bm.data && isValidTime) {
      MessageBusLayrzHub::publish(bm);
    } else {
      debugPrint("Failed to alloc for Pd publish or not NTP synced\n");
    }

    // Free allocated memory for decoded messages
    for (auto &entry : uniqueMessages) {
      free(entry.second);
    }
    uniqueMessages.clear();

    if (hubSettings.can_pub_per > 10) {
      for (int i = 0; i < int(hubSettings.can_pub_per / 5); i++) {
        esp_task_wdt_reset();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    } else {
      esp_task_wdt_reset();
      vTaskDelay(hubSettings.can_pub_per * 1000 / portTICK_PERIOD_MS);
    }
  }
}
