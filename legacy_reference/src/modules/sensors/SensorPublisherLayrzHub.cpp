#include "SensorPublisherLayrzHub.h"

void SensorPublisherLayrzHub::buildSensorPayload(bool gpioEn,
                                                 const std::string &position) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  strncat(msgBuffer, std::to_string(tv.tv_sec).c_str(),
          MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  strncat(msgBuffer, ";", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  strncat(msgBuffer, position.c_str(),
          MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  strncat(msgBuffer, "report.code:LKSEN,fw.build:",
          MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  strncat(msgBuffer, std::to_string(BUILD_NUMBER).c_str(),
          MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  strncat(msgBuffer, ",fw.id:", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  strncat(msgBuffer, hubSettings.fota_fw_id.c_str(),
          MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  strncat(msgBuffer,
          ",fw.branch:", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  std::string fwBranch =
    hubSettings.fota_fw_branch == 0 ? "stable" : "development";
  strncat(msgBuffer, fwBranch.c_str(),
          MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  int esp32S3Temp = temperatureRead();
  if (hubSettings.net_mode == "wifi") {
    strncat(msgBuffer,
            ",wifi.rssi:", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    strncat(msgBuffer, std::to_string(WiFi.RSSI()).c_str(),
            MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  } else if (hubSettings.net_mode == "cellular") {
    strncat(msgBuffer,
            ",cell.strength:", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
#ifdef USE_ASYNC_A7670
    strncat(msgBuffer, std::to_string(modemAsync.getSignalQuality()).c_str(),
            MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
#else
    strncat(msgBuffer, std::to_string(modem.getSignalQuality()).c_str(),
            MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
#endif
  }

  strncat(msgBuffer,
          ",cpu.temperature:", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  strncat(msgBuffer, std::to_string(esp32S3Temp).c_str(),
          MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  if (gpioEn) {
    std::string gpioData = GpioLayrzHub::getGPIOPayload();
    if (gpioData != "") {
      strncat(msgBuffer, ",", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
      strncat(msgBuffer, gpioData.c_str(),
              MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    }
  }

  if (strlen(uartIoDataBuffer) > 0) {
    strncat(msgBuffer, ",", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    strncat(msgBuffer, uartIoDataBuffer,
            MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    memset(uartIoDataBuffer, 0, UART_IO_BUFFER_SIZE);
  }
  // if (strlen(rs485DataBuffer) > 0 && RS485Mode != "transparent")
  if (strlen(rs485DataBuffer) > 0) {
    strncat(msgBuffer, ",", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    strncat(msgBuffer, rs485DataBuffer,
            MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    memset(rs485DataBuffer, 0, RS485_BUFFER_SIZE);
  }
  // if (strlen(rs232_1DataBuffer) > 0 && RS232_1Mode != "transparent")
  if (strlen(rs232_1DataBuffer) > 0) {
    strncat(msgBuffer, ",", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    strncat(msgBuffer, rs232_1DataBuffer,
            MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    memset(rs232_1DataBuffer, 0, RS232_1_BUFFER_SIZE);
  }
  // if (strlen(rs232_2DataBuffer) > 0 && RS232_1Mode != "transparent")
  if (strlen(rs232_2DataBuffer) > 0) {
    strncat(msgBuffer, ",", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    strncat(msgBuffer, rs232_2DataBuffer,
            MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    memset(rs232_2DataBuffer, 0, RS232_2_BUFFER_SIZE);
  }
  if (strlen(extIoDataBuffer) > 0) {
    strncat(msgBuffer, ",", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    strncat(msgBuffer, extIoDataBuffer,
            MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    memset(extIoDataBuffer, 0, EXT_IO_BUFFER_SIZE);
  }

  // --- BlackBox / SD metrics (only for HUB2 builds) ---
#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
  {
    BlackBoxLayrzHub *bb = MessageBusLayrzHub::blackbox();
    if (bb && sdCardInitialized) {
      size_t backlog = bb->backlogBytes(); // bytes
      size_t history = bb->historyBytes(); // bytes
      uint64_t imgBytes = 0;
      uint32_t imgCount = 0; // image stats (bytes & count)
      if (SD.exists("/media")) {
        File dir = SD.open("/media");
        if (dir) {
          for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
            if (!f.isDirectory()) {
              imgBytes += f.size();
              imgCount++;
            }
            f.close();
          }
          dir.close();
        }
      }
      uint64_t capacityKB = bb->mediaTotalBytes() / 1024ULL;
      uint64_t usedKB =
        (backlog + history + imgBytes) / 1024ULL; // include images
      uint64_t freeKB = (capacityKB > usedKB) ? (capacityKB - usedKB) : 0ULL;
      uint32_t usedPct =
        (capacityKB > 0) ? (uint32_t)((usedKB * 100ULL) / capacityKB) : 0;
      char metrics[200];
      metrics[0] = 0;
      snprintf(metrics, sizeof(metrics),
               ",sd.backlog.kb:%u,sd.history.kb:%u,sd.images.kb:%u,sd.images."
               "count:%u,sd."
               "used.kb:%u,sd.free.kb:%u,sd.capacity.kb:%u,sd.used.pct:%u",
               (unsigned)(backlog / 1024U), (unsigned)(history / 1024U),
               (unsigned)(imgBytes / 1024U), (unsigned)imgCount,
               (unsigned)usedKB, (unsigned)freeKB, (unsigned)capacityKB,
               usedPct);
      strncat(msgBuffer, metrics, MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
    }
    // Always include sd.mounted (1 if previously initialized and still
    // accessible, else 0)
    extern volatile bool
      sdCardInitialized; // declared in GlobalObjectsLayrzHub.cpp
    char mnt[24];
    mnt[0] = 0;
    snprintf(mnt, sizeof(mnt), ",sd.mounted:%d", sdCardInitialized ? 1 : 0);
    strncat(msgBuffer, mnt, MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  }
#endif
  strncat(msgBuffer, ";", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  std::string crc = calculateCRC16(msgBuffer);
  strncat(msgBuffer, crc.c_str(), MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  for (int i = strlen(msgBuffer); i >= 0; i--) {
    msgBuffer[i + 4] = msgBuffer[i]; // Shift characters to the right
  }
  msgBuffer[0] = '<';
  msgBuffer[1] = 'P';
  msgBuffer[2] = 'd';
  msgBuffer[3] = '>';
  strncat(msgBuffer, "</Pd>", MESSAGE_BUFFER_SIZE - strlen(msgBuffer) - 1);
  msgBuffer[strlen(msgBuffer)] = '\0';
}

void SensorPublisherLayrzHub::updateSensors(void *pvParameters) {
  // Add current task to watchdog timer
  esp_task_wdt_add(NULL);
  bool firstBuildDone = false;

  for (;;) {
    std::string position = GNSS::getPosition();
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
      if (isValidTime) {
        memset(msgBuffer, 0, MESSAGE_BUFFER_SIZE); // Clear the message buffer
        buildSensorPayload(hubSettings.data_gpio_en, position);
        firstBuildDone = true;
      }
    }
    xSemaphoreGive(xSemaphore); // Release the semaphore after task is done
    if (!firstBuildDone) {
      // NTP not yet synced — retry quickly instead of blocking for the full
      // period
      esp_task_wdt_reset();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else if (hubSettings.data_upd_per > 10) {
      for (int i = 0; i < int(hubSettings.data_upd_per / 5); i++) {
        esp_task_wdt_reset();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    } else {
      esp_task_wdt_reset();
      vTaskDelay(hubSettings.data_upd_per * 1000 / portTICK_PERIOD_MS);
    }
  }
}

void SensorPublisherLayrzHub::sendSensorData(void *pvParameters) {
  esp_task_wdt_add(NULL);
  bool firstPublishDone = false;

  for (;;) {
    while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE ||
           msgBuffer == nullptr) {
      esp_task_wdt_reset();
      xSemaphoreGive(xSemaphore);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    BusMessage bm;
    bm.kind = BusMsgKind::PdSensor;
    bm.data = MessageBusLayrzHub::allocAndCopy(msgBuffer, &bm.len);
    // Release semaphore immediately after copying buffer to avoid blocking
    // other producers
    xSemaphoreGive(xSemaphore);
    debugPrint("Publishing <Pd> from Sensors to MessageBus\n");
    if (hubSettings.sys_debug_en) {
      for (int i = 0; i < bm.len; i++) {
        if (Print *console = debugConsole())
          console->print(msgBuffer[i]);
      }
      if (Print *console = debugConsole())
        console->println();
    }

    bm.persistIfFail = true;
    if (bm.data && bm.len > 0 && isValidTime) {
      notePdHeartbeat();
      MessageBusLayrzHub::publish(bm);
      firstPublishDone = true;
    } else {
      // Free the allocation when skipping publish to avoid a memory leak
      if (bm.data) {
        heap_caps_free(bm.data);
        bm.data = nullptr;
      }
      if (!isValidTime)
        debugPrint("Skipping Pd publish: NTP not synced\n");
      else
        debugPrint("Skipping Pd publish: buffer not ready yet\n");
    }
    if (!firstPublishDone) {
      // Not yet connected/synced — retry quickly to send the first message as
      // soon as possible
      esp_task_wdt_reset();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else if (hubSettings.data_pub_per > 10) {
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

void SensorPublisherLayrzHub::pingTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  for (;;) {
    for (int i = 0; i < (int)(PING_PERIOD_SECS / 5); i++) {
      esp_task_wdt_reset();
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    if (!isValidTime) {
      continue;
    }
    char pingBuf[128];
    memset(pingBuf, 0, sizeof(pingBuf));
    struct timeval tv;
    gettimeofday(&tv, NULL);
    snprintf(pingBuf, sizeof(pingBuf), "%ld;;;;;;;;report.code:PING;",
             (long)tv.tv_sec);
    std::string crc = calculateCRC16(pingBuf);
    strncat(pingBuf, crc.c_str(), sizeof(pingBuf) - strlen(pingBuf) - 1);
    size_t bodyLen = strlen(pingBuf);
    for (int i = (int)bodyLen; i >= 0; i--) {
      pingBuf[i + 4] = pingBuf[i];
    }
    pingBuf[0] = '<';
    pingBuf[1] = 'P';
    pingBuf[2] = 'd';
    pingBuf[3] = '>';
    strncat(pingBuf, "</Pd>", sizeof(pingBuf) - strlen(pingBuf) - 1);
    BusMessage bm;
    bm.kind = BusMsgKind::PdSensor;
    bm.persistIfFail = false;
    bm.skipHistory = true;
    bm.data = MessageBusLayrzHub::allocAndCopy(pingBuf, &bm.len);
    if (bm.data) {
      notePdHeartbeat();
      MessageBusLayrzHub::publish(bm);
      debugPrint("Ping <Pd> published\n");
    }
  }
}
