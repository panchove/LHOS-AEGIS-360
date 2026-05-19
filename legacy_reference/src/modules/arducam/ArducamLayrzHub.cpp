#include <modules/arducam/ArducamLayrzHub.h>

extern "C" void arducamSpiBlockTransfer(uint8_t *tx_buf, uint8_t *rx_buf,
                                        uint32_t len) {
  // Unified HSPI bus transfer helper for Arducam library
  if (rx_buf == NULL) {
    SPI.transfer(tx_buf, len);
  } else {
    SPI.transferBytes(tx_buf, rx_buf, len);
  }
}

Arducam_Mega HubCam(ARDUCAM_CS);

static bool _arducamQuickProbe(uint32_t tries = 16) {
  if (!spiSharedInitialized) {
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    pinMode(ARDUCAM_CS, OUTPUT);
    digitalWrite(ARDUCAM_CS, HIGH);
    spiSharedInitialized = true;
    if (!spiBusMutex)
      spiBusMutex = xSemaphoreCreateMutex();
  }

  if (spiBusMutex)
    xSemaphoreTake(spiBusMutex, portMAX_DELAY);
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(SD_CS, HIGH);
  digitalWrite(ARDUCAM_CS, LOW);

  uint8_t first = SPI.transfer(0x00);
  bool allFF = (first == 0xFF);
  bool all00 = (first == 0x00);
  for (uint32_t i = 1; i < tries; ++i) {
    uint8_t v = SPI.transfer(0x00);
    allFF &= (v == 0xFF);
    all00 &= (v == 0x00);
  }

  digitalWrite(ARDUCAM_CS, HIGH);
  SPI.endTransaction();
  if (spiBusMutex)
    xSemaphoreGive(spiBusMutex);

  bool present = !(allFF || all00);
  if (!present && hubSettings.sys_debug_en) {
    debugPrint("ArduCam quick probe: No device (all %s)\n",
               allFF ? "0xFF" : "0x00");
  }
  return present;
}

bool ArducamLayrzHub::initArducamMega() {
  debugPrint("Initializing Arducam Mega...\n");

  // Ensure shared HSPI bus initialized (camera + SD share it)
  if (!spiSharedInitialized) {
    debugPrint("SPI not yet initialized, starting...\n");
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    pinMode(ARDUCAM_CS, OUTPUT);
    digitalWrite(ARDUCAM_CS, HIGH);
    spiSharedInitialized = true;
    if (!spiBusMutex)
      spiBusMutex = xSemaphoreCreateMutex();
  }
  // Test if Arducam is wired to SPI first
  if (!_arducamQuickProbe()) {
    debugPrint("Arducam not detected.\n");
    return false;
  }

  if (spiBusMutex)
    xSemaphoreTake(spiBusMutex, portMAX_DELAY);
  // Deselect all devices before init
  digitalWrite(SD_CS, HIGH);
  digitalWrite(ARDUCAM_CS, HIGH);

  int rc = HubCam.begin();
  // Immediately release bus; BlackBox/SD may still be mounting.
  if (spiBusMutex)
    xSemaphoreGive(spiBusMutex);
  // Ensure camera CS high after init to prevent bus hold during early SD
  // operations.
  digitalWrite(ARDUCAM_CS, HIGH);
  if (rc != 0) {
    debugPrint("Arducam Mega initialization failed rc=%d\n", rc);
    return false;
  }
  switch (hubSettings.acam_wb_mode) // Check the white balance mode
  {
  case 0: // Auto
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_DEFAULT); // Set white balance
                                                          // mode to auto
    break;
  case 1: // Sunny
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_SUNNY); // Set white balance
                                                        // mode to sunny
    break;
  case 2: // Office
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_OFFICE); // Set white balance
                                                         // mode to office
    break;
  case 3: // Cloudy
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_CLOUDY); // Set white balance
                                                         // mode to cloudy
    break;
  case 4: // Home
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_HOME); // Set white balance mode
                                                       // to home
    break;
  default:
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_DEFAULT); // Set white balance
                                                          // mode to auto
    debugPrint("Invalid white balance mode, defaulting to auto\n");
    break;
  }
  HubCam.setAutoExposure(1);        // Enable auto exposure
  HubCam.setAutoISOSensitive(1);    // Enable auto ISO sensitivity
  switch (hubSettings.acam_wb_mode) // Check the white balance mode
  {
  case 0: // Auto
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_DEFAULT); // Set white balance
                                                          // mode to auto
    break;
  case 1: // Sunny
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_SUNNY); // Set white balance
                                                        // mode to sunny
    break;
  case 2: // Office
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_OFFICE); // Set white balance
                                                         // mode to office
    break;
  case 3: // Cloudy
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_CLOUDY); // Set white balance
                                                         // mode to cloudy
    break;
  case 4: // Home
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_HOME); // Set white balance mode
                                                       // to home
    break;
  default:
    HubCam.setAutoWhiteBalanceMode(
      CAM_WHITE_BALANCE::CAM_WHITE_BALANCE_MODE_DEFAULT); // Set white balance
                                                          // mode to auto
    debugPrint("Invalid white balance mode, defaulting to auto\n");
    break;
  }
  HubCam.setAutoWhiteBalance(1);     // Enable auto white balance
  HubCam.setAutoFocus(0x02);         // Enable auto focus
  switch (hubSettings.acam_color_fx) // Check the color effect
  {
  case 0: // None
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_NONE); // Set color effect to none
    break;
  case 1: // Blueish
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_BLUEISH); // Set color effect to blueish
    break;
  case 2: // Redish
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_REDISH); // Set color effect to redish
    break;
  case 3: // Black/white
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_BW); // Set color effect to black/white
    break;
  case 4: // Sepia
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_SEPIA); // Set color effect to sepia
    break;
  case 5: // Positive/negative inversion
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_NEGATIVE); // Set color effect to
                                            // positive/negative inversion
    break;
  case 6: // Grass green
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_GRASS_GREEN); // Set color effect to grass
                                               // green
    break;
  case 7: // Over exposure
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_OVER_EXPOSURE); // Set color effect to over
                                                 // exposure
    break;
  case 8: // Solarize
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_SOLARIZE); // Set color effect to solarize
    break;
  default:
    HubCam.setColorEffect(
      CAM_COLOR_FX::CAM_COLOR_FX_NONE); // Set color effect to none
    debugPrint("Invalid color effect, defaulting to none\n");
    break;
  }
  debugPrint("Arducam Mega initialized successfully\n");
  // Small delay to let any pending SD card power-up finish before camera task
  // starts captures.
  vTaskDelay(10 / portTICK_PERIOD_MS);
  return true;
}

bool ArducamLayrzHub::takePicture() {
  CAM_IMAGE_MODE resolution;    // Declare resolution variable
  switch (hubSettings.acam_res) // Check the resolution
  {
  case 0:                                              // QQVGA
    resolution = CAM_IMAGE_MODE::CAM_IMAGE_MODE_QQVGA; // Set the resolution to
                                                       // QQVGA (160x120)
    break;
  case 1:                                             // QVGA
    resolution = CAM_IMAGE_MODE::CAM_IMAGE_MODE_QVGA; // Set the resolution to
                                                      // QVGA (320x240)
    break;
  case 2: // VGA
    resolution =
      CAM_IMAGE_MODE::CAM_IMAGE_MODE_VGA; // Set the resolution to VGA (640x480)
    break;
  case 3:                                             // SVGA
    resolution = CAM_IMAGE_MODE::CAM_IMAGE_MODE_SVGA; // Set the resolution to
                                                      // SVGA (800x600)
    break;
  case 4: // HD
    resolution =
      CAM_IMAGE_MODE::CAM_IMAGE_MODE_HD; // Set the resolution to SVGA (800x600)
    break;
  default:
    resolution = CAM_IMAGE_MODE::CAM_IMAGE_MODE_VGA; // Set the resolution to
                                                     // SVGA (800x600)
    debugPrint("Invalid resolution, defaulting to VGA\n");
    break;
  }
  time_t now = millis();
  // Safety: ensure HSPI initialized in case initArducamMega wasn't called or
  // failed previously
  if (!spiSharedInitialized) {
    debugPrint("[acam] SPI not init at capture, initializing ad-hoc.\n");
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    pinMode(ARDUCAM_CS, OUTPUT);
    digitalWrite(ARDUCAM_CS, HIGH);
    spiSharedInitialized = true;
    if (!spiBusMutex)
      spiBusMutex = xSemaphoreCreateMutex();
  }
  if (spiBusMutex)
    xSemaphoreTake(spiBusMutex, portMAX_DELAY);
  // Make sure SD is deselected
  digitalWrite(SD_CS, HIGH);
  // digitalWrite(ARDUCAM_CS, HIGH);
  int picRc =
    HubCam.takePicture(resolution, CAM_IMAGE_PIX_FMT::CAM_IMAGE_PIX_FMT_JPG);
  if (picRc == 0) {
    debugPrint("Image captured\n");
    uint32_t len = HubCam.getTotalLength();
    // Prepare optional persistent storage (integrate into single read loop
    // below)
    File imgFile;
    char fname[40] = {0};
    bool saveImage = hubSettings.acam_storage_en;
#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
    if (saveImage) {
      if (!SD.exists("/media"))
        SD.mkdir("/media");
      time_t tnow = time(nullptr);
      struct tm tmnow;
      bool haveTime = false;
      if (tnow > 1600000000) {
        localtime_r(&tnow, &tmnow);
        haveTime = true;
      }
      if (haveTime)
        snprintf(fname, sizeof(fname), "/media/%04d%02d%02d_%02d%02d%02d.jpg",
                 tmnow.tm_year + 1900, tmnow.tm_mon + 1, tmnow.tm_mday,
                 tmnow.tm_hour, tmnow.tm_min, tmnow.tm_sec);
      else
        snprintf(fname, sizeof(fname), "/media/%lu.jpg",
                 (unsigned long)millis());
      imgFile = SD.open(fname, FILE_WRITE);
      if (!imgFile) {
        debugPrint("Failed to open image file %s\n", fname);
        saveImage = false;
      }
    }
#else
    (void)imgFile;
    (void)fname;       // suppress unused warnings
    saveImage = false; // disable SD saving on non-HUB2 builds
#endif

    const uint16_t CHUNK = 255;
    uint32_t remaining = len;
    // create a buffer to hold the image data chunk allocated in PSRAM
    uint8_t *buf = (uint8_t *)heap_caps_malloc(256, MALLOC_CAP_SPIRAM);
    if (buf == nullptr) {
      debugPrint("Failed to allocate memory for image buffer\n");
      return false;
    }
    debugPrint("Buffer allocated in PSRAM\n");
    // read the image data into the buffer

    memset(mediaMsgBuffer, 0, MEDIA_MSG_BUFFER_SIZE); // initialize with zeros
    snprintf(mediaMsgBuffer, MEDIA_MSG_BUFFER_SIZE, "%s;image/jpeg;",
             hubSettings.acam_name.c_str());
    while (remaining > 0) {
      uint16_t toRead = (remaining > CHUNK) ? CHUNK : remaining;
      HubCam.readBuff(buf, toRead);
      // Write raw bytes to file if enabled
      if (saveImage && imgFile) {
#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
        imgFile.write(buf, toRead);
#endif
      }
      strncat(mediaMsgBuffer, base64::encode(buf, toRead).c_str(),
              MEDIA_MSG_BUFFER_SIZE - strlen(mediaMsgBuffer) - 1);
      remaining -= toRead;
    }
    if (saveImage && imgFile) {
#if defined(LAYRZ_HUB2_BUILD) || defined(LAYRZ_HUB25_BUILD)
      imgFile.flush();
      imgFile.close();
      debugPrint("Saved image to %s\n", fname);
#endif
    }
    strncat(mediaMsgBuffer, ";",
            MEDIA_MSG_BUFFER_SIZE - strlen(mediaMsgBuffer) - 1);
    // calculate CRC16
    std::string crc = calculateCRC16(mediaMsgBuffer);
    strncat(mediaMsgBuffer, crc.c_str(),
            MEDIA_MSG_BUFFER_SIZE - strlen(mediaMsgBuffer) - 1);
    // Append the end tag to the media message buffer
    strncat(mediaMsgBuffer, "</Pm>",
            MEDIA_MSG_BUFFER_SIZE - strlen(mediaMsgBuffer) - 1);
    // Add <Pm> tag at the beginning of the media message buffer
    memmove(mediaMsgBuffer + 4, mediaMsgBuffer,
            strlen(mediaMsgBuffer) +
              1); // Move the buffer content to the right by 4 bytes
    memcpy(mediaMsgBuffer, "<Pm>",
           4); // Copy <Pm> tag at the beginning of the buffer
    debugPrint("Media message buffer length: %d\n", strlen(mediaMsgBuffer));
    debugPrint("Capture time: %d ms\n", millis() - now);
    // print the media message buffer for debugging
    // if (hubSettings.sys_debug_en) {
    //     debugPrint("Media message buffer: ");
    //     UARTSerial.println(mediaMsgBuffer);
    // }
    // Free the allocated buffer after use
    free(buf);
    if (spiBusMutex)
      xSemaphoreGive(spiBusMutex);
    now = millis();
    if (!isSocketAuth || mediaMsgBuffer == nullptr) {
      return false;
    }
    if (strlen(mediaMsgBuffer) > 0) {
      while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE)
        vTaskDelay(100 / portTICK_PERIOD_MS);
      BusMessage bm;
      bm.kind = BusMsgKind::Media;
      bm.data = MessageBusLayrzHub::allocAndCopy(mediaMsgBuffer, &bm.len);
      // Release semaphore immediately after copying buffer to avoid blocking
      // other producers
      xSemaphoreGive(xSemaphore);
      debugPrint("Publishing <Pm> to MessageBus\n");
      bm.persistIfFail = true;
      bm.freeAfterSend = true;
      if (bm.data) {
        MessageBusLayrzHub::publish(bm);
      } else {
        debugPrint("Failed to alloc for Pm publish\n");
      }
    }
  } else {
    if (spiBusMutex)
      xSemaphoreGive(spiBusMutex);
    debugPrint("Image capture failed rc=%d\n", picRc);
    return false;
  }
  return true;
}

void ArducamLayrzHub::cyclicArducamTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  while (true) {
    // if (hubSettings.net_protocol == 0 && hubSettings.acam_cyclic_int > 240) {
    //   tcpNonSslClient->stop();    // Close the connection
    //   isSocketAuth = false;
    //   uint16_t port = hubSettings.net_server == "link.layrz.network" ?
    //   STABLE_SOCKET_PORT : TESTING_SOCKET_PORT;
    //   tcpNonSslClient->connect(hubSettings.net_server.c_str(), port);
    //   debugPrint("Reconnecting to TCP server\n");
    // }
    // while (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdFALSE ||
    // !isSocketAuth) {
    //   esp_task_wdt_reset();
    //   xSemaphoreGive(xSemaphore);
    //   vTaskDelay(1000 / portTICK_PERIOD_MS);
    //   debugPrint("Waiting for NTP sync and socket auth to send Pm...\n");
    // }
    if (takePicture()) {
      debugPrint("Image captured and sent\n");
    } else {
      debugPrint("Image capture failed\n");
    }
    // xSemaphoreGive(xSemaphore);
    if (hubSettings.acam_cyclic_int > 10) {
      for (int i = 0; i < int(hubSettings.acam_cyclic_int / 5); i++) {
        esp_task_wdt_reset();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
      }
    } else {
      esp_task_wdt_reset();
      vTaskDelay(hubSettings.acam_cyclic_int * 1000 / portTICK_PERIOD_MS);
    }
  }
}