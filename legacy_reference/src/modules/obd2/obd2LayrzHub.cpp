#include <modules/obd2/obd2LayrzHub.h>

OBD2Class OBD2;

uint8_t Obd2LayrzHub::fillSupportedPidsList() {
  OBD2.initializeSupportedPIDs();
  uint8_t pidsCount = 0;
  for (uint32_t pid = 0; pid < 200; pid++) {
    if (OBD2.pidSupported(pid)) {
      supportedPids[pidsCount] = pid;
      pidsCount++;
    }
  }
  return pidsCount;
}

bool Obd2LayrzHub::initObd2() {
  long initTime = millis();
  uint32_t baudRate = hubSettings.can_mode == 0 ? 250000 : 500000;
  debugPrint("Initializing OBD2 stack..\n");
  while (millis() - initTime < 10000) {
    if (OBD2.begin(baudRate)) {
      debugPrint("OBD2 stack initialized successfully\n");
      isObd2Initialized = true;
      supportedPidsCount = fillSupportedPidsList();
      for (int i = 0; i < supportedPidsCount; i++) {
        debugPrint("Supported PID: 0x%02X - %s\n", supportedPids[i],
                   OBD2.pidName(supportedPids[i]).c_str());
      }
      return true;
    }
  }
  debugPrint("Failed to initialize OBD2 stack\n");
  return false;
}

void Obd2LayrzHub::obd2MonitorTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  uint8_t pidData[4];
  obd2Decoder.begin();

  while (true) {
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
      supportedPidsCount = fillSupportedPidsList();

      if (supportedPidsCount < 2) {
        xSemaphoreGive(xSemaphore);
        esp_task_wdt_reset();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      }

      memset(canDataBuffer, 0, CANBUS_BUFFER_SIZE); // Initialize with zeros
      debugPrint("Requesting OBD2 PIDs\n");
      std::string position = GNSS::getPosition();
      strncat(canDataBuffer, std::to_string(time(NULL)).c_str(),
              CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
      strncat(canDataBuffer, ";",
              CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
      strncat(canDataBuffer, position.c_str(),
              CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
      strncat(canDataBuffer, "report.code:LKCAN,",
              CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
      bool pidReadSuccess = true;
      for (int i = 0; i < supportedPidsCount; i++) {
        if (OBD2.pidDataRead(0x01, supportedPids[i], &pidData,
                             sizeof(pidData)) > 0) {
          // debugPrint("PID Data: ");
          // for (int i = 0; i < sizeof(pidData); i++) {
          //     UARTSerial.print(pidData[i], HEX);
          //     UARTSerial.print(" ");
          // }
          // UARTSerial.println();
          std::string decodedStr;
          if (hubSettings.can_decode_en)
            decodedStr = obd2Decoder.decodePIDs(supportedPids[i], pidData,
                                                sizeof(pidData));
          else {
            std::stringstream ss;
            ss << "obd2.raw.data." << supportedPids[i] << ":";
            for (int i = 0; i < sizeof(pidData); i++) {
              ss << std::uppercase << std::setfill('0') << std::setw(2)
                 << std::hex << (int)pidData[i];
            }
            decodedStr = ss.str();
          }
          // debugPrint("Decoded Message: ");
          // if (hubSettings.sys_debug_en) {
          //     UARTSerial.println(decodedStr.c_str());
          //     UARTSerial.println();
          // }
          strncat(canDataBuffer, decodedStr.c_str(),
                  CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
          strncat(canDataBuffer, ",",
                  CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);
        } else {
          debugPrint("Failed to read PID 0x%02X\n", supportedPids[i]);
          pidReadSuccess = false;
          break;
        }
      }
      if (!pidReadSuccess) {
        xSemaphoreGive(xSemaphore);
        esp_task_wdt_reset();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      }

      // replace the last comma with a semi-colon and null-terminate the string
      canDataBuffer[strlen(canDataBuffer) - 1] = ';';
      canDataBuffer[strlen(canDataBuffer)] = '\0';

      std::string crc = calculateCRC16(canDataBuffer);
      strncat(canDataBuffer, crc.c_str(),
              CANBUS_BUFFER_SIZE - strlen(canDataBuffer) - 1);

      // Add <Pd> at the beginning and </Pd> at the end

      for (int i = strlen(canDataBuffer); i >= 0; i--) {
        canDataBuffer[i + 4] =
          canDataBuffer[i]; // Shift characters to the right
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
        if (Print *console = debugConsole())
          console->println(canDataBuffer);
      }
      // BaseClientLayrzHub* client = ClientFactoryLayrzHub::createClient();

      // if (!client) {
      //     debugPrint("Error: Failed to create a Transport Client!\n");
      //     xSemaphoreGive(xSemaphore);
      //     esp_task_wdt_reset();
      //     vTaskDelay(hubSettings.can_pub_per * 1000 / portTICK_PERIOD_MS);
      //     continue;
      // }
      // serverResponse response = client->sendDataToServer(canDataBuffer);
      // debugPrint("<Pd> with CAN data sent - Response code: %d\n",
      // response.responseCode); if (requestObd2Dtcs) {
      //     int dtcCount = getObd2Dtcs();
      //     if (dtcCount > 0) {
      //         serverResponse dtcResponse =
      //         client->sendDataToServer(obd2DtcBuffer); debugPrint("<Pd> with
      //         DTC data sent - Response code: %d\n",
      //         dtcResponse.responseCode);
      //     }
      // }
      // delete client;  //Prevent memory leaks

      BusMessage bm;
      bm.kind = BusMsgKind::PdSensor;
      bm.data = MessageBusLayrzHub::allocAndCopy(canDataBuffer, &bm.len);
      // Release semaphore immediately after copying buffer to avoid blocking
      // other producers
      debugPrint("Publishing <Pd> from OBD2 to MessageBus\n");
      bm.persistIfFail = true;
      bm.freeAfterSend = true;
      if (bm.data && isValidTime) {
        MessageBusLayrzHub::publish(bm);
      } else {
        debugPrint("Failed to alloc for Pd publish or not NTP synced\n");
      }
      if (requestObd2Dtcs) {
        int dtcCount = getObd2Dtcs();
        if (dtcCount > 0) {
          bm.data = MessageBusLayrzHub::allocAndCopy(obd2DtcBuffer, &bm.len);
          if (bm.data && isValidTime) {
            MessageBusLayrzHub::publish(bm);
          } else {
            debugPrint("Failed to alloc for DTC publish or not NTP synced\n");
          }
        }
      }
    }

    xSemaphoreGive(xSemaphore);

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

int Obd2LayrzHub::getObd2Dtcs() {
  bool success = false;
  memset(obd2DtcBuffer, 0, OBD2_DTC_BUFFER_SIZE); // Initialize with zeros
  debugPrint("Requesting OBD2 PIDs\n");
  std::string position = GNSS::getPosition();
  strncat(obd2DtcBuffer, std::to_string(time(NULL)).c_str(),
          OBD2_DTC_BUFFER_SIZE - strlen(obd2DtcBuffer) - 1);
  strncat(obd2DtcBuffer, ";", OBD2_DTC_BUFFER_SIZE - strlen(obd2DtcBuffer) - 1);
  strncat(obd2DtcBuffer, position.c_str(),
          OBD2_DTC_BUFFER_SIZE - strlen(obd2DtcBuffer) - 1);
  strncat(obd2DtcBuffer, "report.code:LKDTC,",
          OBD2_DTC_BUFFER_SIZE - strlen(obd2DtcBuffer) - 1);
  std::string dtcs[40]; // Buffer for up to 40 DTCs
  int dtcCount = OBD2.dtcRead(dtcs, 40);

  if (dtcCount > 0) {
    std::string decodedStr = "";
    for (int i = 0; i < dtcCount; i++) {
      debugPrint("DTC: %s\n", dtcs[i].c_str());
      if (hubSettings.can_decode_en)
        decodedStr += obd2Decoder.decodeDTC(dtcs[i].c_str());

      else {
        std::stringstream ss;
        ss << "obd2.dtc." << i << ":" << dtcs[i];
        decodedStr += ss.str();
      }
    }

    strncat(obd2DtcBuffer, decodedStr.c_str(),
            OBD2_DTC_BUFFER_SIZE - strlen(obd2DtcBuffer) - 1);
    // replace the last comma with a semi-colon and null-terminate the string
    obd2DtcBuffer[strlen(obd2DtcBuffer) - 1] = ';';
    obd2DtcBuffer[strlen(obd2DtcBuffer)] = '\0';

    std::string crc = calculateCRC16(obd2DtcBuffer);
    strncat(obd2DtcBuffer, crc.c_str(),
            OBD2_DTC_BUFFER_SIZE - strlen(obd2DtcBuffer) - 1);

    // Add <Pd> at the beginning and </Pd> at the end

    for (int i = strlen(obd2DtcBuffer); i >= 0; i--) {
      obd2DtcBuffer[i + 4] = obd2DtcBuffer[i]; // Shift characters to the right
    }
    obd2DtcBuffer[0] = '<';
    obd2DtcBuffer[1] = 'P';
    obd2DtcBuffer[2] = 'd';
    obd2DtcBuffer[3] = '>';
    strncat(obd2DtcBuffer, "</Pd>",
            OBD2_DTC_BUFFER_SIZE - strlen(obd2DtcBuffer) - 1);
    canDataBuffer[strlen(obd2DtcBuffer)] = '\0';

    if (hubSettings.sys_debug_en) {
      debugPrint("OBD2 DTC Data: ");
      if (Print *console = debugConsole())
        console->println(obd2DtcBuffer);
    }
  }
  return dtcCount;
}
