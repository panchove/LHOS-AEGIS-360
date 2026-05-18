#include <modules/fota/FotaLayrzHub.h>
#include <modules/settings/UnifiedSettingsStorage.h>
/**
 * Executes OTA firmware update using single SSL connection (TinyGSM-style).
 *
 * @return true if OTA update is successful, false otherwise.
 *
 * @throws None
 */
bool fotaUpgrade(const char *url) {
  debugPrint("Starting FOTA upgrade using direct SSL client\n");

  // Parse URL
  String urlString = String(url);
  int serverStart = urlString.indexOf("://") + 3;
  int pathStart = urlString.indexOf("/", serverStart);
  String server = urlString.substring(serverStart, pathStart);
  String path = urlString.substring(pathStart);

  debugPrint("Server: %s\n", server.c_str());
  debugPrint("Path: %s\n", path.c_str());

  Client *client = nullptr;
  if (hubSettings.net_mode == "wifi") {
    client = tcpNonSslClient;
  } else {
    debugPrint("Ensuring mux 2 is closed before FOTA download\n");
    GSMFotaClient.stop();
    vTaskDelay(pdMS_TO_TICKS(3000));
    client = &GSMFotaClient;
  }

  client->setTimeout(60000);

  debugPrint("Connecting to %s:80\n", server.c_str());
  if (!client->connect(server.c_str(), 80)) {
    debugPrint("Connection failed\n");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(4000));

  // Build request
  String request = "GET " + path + " HTTP/1.1\r\n";
  request += "Host: " + server + "\r\n";
  request += "User-Agent: ESP32\r\n";
  request += "Connection: close\r\n";
  request += "Cache-Control: no-cache\r\n\r\n";

  client->print(request);
  client->flush();

  // Parse headers
  String headers = "";
  bool headersComplete = false;
  unsigned long startTime = millis();
  while (client->available() == 0) {
    vTaskDelay(pdMS_TO_TICKS(10));
    if (millis() - startTime > 10000) {
      debugPrint("Timeout waiting for response\n");
      client->stop();
      return false;
    }
  }
  while (!headersComplete) {
    String line = client->readStringUntil('\n');
    if (line.length() == 0) {
      debugPrint("Timeout waiting for headers\n");
      client->stop();
      return false;
    }
    if (line.endsWith("\r"))
      line.remove(line.length() - 1);
    if (line.length() == 0) {
      headersComplete = true;
      break;
    }
    headers += line + "\r\n";
  }

  // Parse status + length
  int firstNewline = headers.indexOf('\n');
  String statusLine = headers.substring(0, firstNewline);
  statusLine.trim();
  debugPrint("Status: %s\n", statusLine.c_str());
  if (statusLine.indexOf("200 OK") == -1) {
    debugPrint("HTTP error in status line\n");
    client->stop();
    return false;
  }

  int contentLength = 0;
  int searchStart = firstNewline + 1;
  while (searchStart < headers.length()) {
    int lineEnd = headers.indexOf('\n', searchStart);
    if (lineEnd == -1)
      lineEnd = headers.length();
    String line = headers.substring(searchStart, lineEnd);
    line.trim();
    if (line.startsWith("Content-Length:")) {
      String lengthStr = line.substring(15);
      lengthStr.trim();
      contentLength = lengthStr.toInt();
    }
    searchStart = lineEnd + 1;
  }
  if (contentLength <= 0) {
    debugPrint("Invalid content length: %d\n", contentLength);
    client->stop();
    return false;
  }

  if (!Update.begin(contentLength)) {
    debugPrint("Cannot start OTA update: %s\n", Update.errorString());
    client->stop();
    return false;
  }

  // --- NEW: Try mounting SD, but don't fail if missing ---
  // File out;
  // bool useSD = false;
  // String finalPath, partPath;
  // if (SD.begin()) {
  //   useSD = true;
  //   SD.mkdir("/fota");
  //   String lastSlash = path.substring(path.lastIndexOf('/') + 1);
  //   String baseName = lastSlash.length() ? lastSlash :
  //   String("firmware.bin"); finalPath = String("/fota/") + baseName; partPath
  //   = finalPath + ".part"; if (SD.exists(partPath)) SD.remove(partPath); out
  //   = SD.open(partPath, FILE_WRITE); if (!out) {
  //     debugPrint("Failed to open %s for write, disabling SD save\n",
  //     partPath.c_str()); useSD = false;
  //   }
  // } else {
  //   debugPrint("SD not mounted, continuing OTA without SD backup\n");
  // }
  // -------------------------------------------------------

  debugPrint("Begin OTA. This may take 2 - 5 mins...\n");
  rgbLed.setBrightness(rgbBrightness);
  rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 255));
  rgbLed.show();

  uint8_t buf[4096];
  size_t totalRead = 0, totalWritten = 0, lastLogMark = 0;
  unsigned long lastData = millis();
  const unsigned long stallMs = 30000;
  size_t yieldBudget = 0;

  while (totalRead < (size_t)contentLength) {
    size_t bytesLeft = contentLength - totalRead;
    size_t toRead = bytesLeft < sizeof(buf) ? bytesLeft : sizeof(buf);
    int n = client->readBytes(buf, toRead);
    if (n > 0) {
      lastData = millis();
      totalRead += (size_t)n;

      // If SD available, write also to file
      // if (useSD) {
      //   size_t wsd = out.write(buf, (size_t)n);
      //   if (wsd != (size_t)n) {
      //     debugPrint("SD write mismatch\n");
      //     out.flush(); out.close();
      //     SD.remove(partPath);
      //     Update.abort();
      //     client->stop();
      //     return false;
      //   }
      // }

      // Always write to OTA
      size_t wupd = Update.write(buf, (size_t)n);
      if (wupd != (size_t)n) {
        debugPrint("OTA write mismatch: %s\n", Update.errorString());
        // if (useSD) { out.flush(); out.close(); }
        Update.abort();
        client->stop();
        return false;
      }

      totalWritten += wupd;
      yieldBudget += wupd;

      esp_task_wdt_reset();
      if (yieldBudget >= 64 * 1024) {
        vTaskDelay(pdMS_TO_TICKS(1));
        yieldBudget = 0;
      }
      if (totalWritten - lastLogMark >= 65536) {
        debugPrint("[FOTA] %d / %d (%.1f%%)\n", (int)totalWritten,
                   contentLength, (100.0 * totalWritten) / contentLength);
        lastLogMark = totalWritten;
      }
    } else if (n == 0) {
      if (millis() - lastData > stallMs) {
        debugPrint("Stalled download\n");
        // if (useSD) { out.flush(); out.close(); }
        Update.abort();
        client->stop();
        return false;
      }
      vTaskDelay(pdMS_TO_TICKS(5));
    } else {
      debugPrint("Socket read error\n");
      // if (useSD) { out.flush(); out.close(); }
      Update.abort();
      client->stop();
      return false;
    }
  }

  // if (useSD) {
  //   out.flush(); out.close();
  //   if (SD.exists(finalPath)) SD.remove(finalPath);
  //   SD.rename(partPath, finalPath);
  //   debugPrint("[FOTA->SD] Saved to %s\n", finalPath.c_str());
  // }

  bool ok = false;
  if (!Update.end(true)) {
    debugPrint("Update.end failed: %s (err=%d)\n", Update.errorString(),
               Update.getError());
  } else if (!Update.isFinished()) {
    debugPrint("Update not finished\n");
  } else {
    debugPrint(
      "OTA done! Update successfully completed. Will apply after reboot.\n");
    ok = true;
  }

  client->stop();
  return ok;
}

/**
 * Checks the version of the firmware and performs an update if necessary.
 *
 * @throws ErrorType if there is an error connecting to the network or updating
 * the firmware.
 */
void checkFirmware(void *pvParameters) {
  void *pvParameter = pvParameters;
  if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {

    deviceSettings config;

    // CRITICAL: Reset fota_force immediately at start to prevent infinite loops
    // if FOTA fails
    debugPrint("Resetting fota_force to false at start of FOTA check\n");
    config.setDeviceSetting("fota_force", "false");

    // Save settings to SPIFFS immediately to persist the change
    if (!UnifiedSettingsStorage::saveAllSettings()) {
      debugPrint("ERROR: Failed to save fota_force reset to SPIFFS\n");
    } else {
      debugPrint("fota_force reset saved to SPIFFS successfully\n");
    }

    if (isFWChecked) {
      xSemaphoreGive(xSemaphore);
      vTaskDelete(NULL);
      return;
    }

    if (hubSettings.net_mode == "wifi") {
      if (WiFi.status() != WL_CONNECTED) {
        debugPrint("Internet WiFi Network not connected\n");
        isFWChecked = false;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        xSemaphoreGive(xSemaphore);
        isFWChecked = true;
        vTaskDelete(NULL);
        return;
      }
      WifiClient_SSL.setInsecure();
    } else {
      if (!modemAsync.isGprsConnected()) {
        debugPrint("Internet GPRS Network not connected\n");
        isFWChecked = false;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        xSemaphoreGive(xSemaphore);
        isFWChecked = true;
        vTaskDelete(NULL);
        return;
      }
      // Configure SSL with longer timeouts for firmware operations
      GSMClient_SSL.setInsecure();
      GSMClient_SSL.setTimeout(60000);          // 60 seconds timeout
      GSMClient_SSL.setHandshakeTimeout(30000); // 30 seconds for handshake
    }
    HttpClient *http = nullptr;

    if (hubSettings.net_mode == "wifi") {
      http = new HttpClient(WifiClient_SSL, LAYRZ_API_URL, HTTPS_PORT);
    } else {
      // Use single SSL client for API calls too
      http = new HttpClient(GSMClient_SSL, LAYRZ_API_URL, HTTPS_PORT);
    }

    std::string getPath =
      "/firmwares/" + std::string(hubSettings.fota_fw_id) +
      std::string(hubSettings.fota_fw_branch == 0 ? "/latest/stable"
                                                  : "/latest/development");
    debugPrint("Getting: %s\n", getPath.c_str());
    int err = http->get(getPath.c_str());
    if (err != 0) {
      debugPrint("fail to connect\n");
      delay(1000);
      http->stop();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      xSemaphoreGive(xSemaphore);
      isFWChecked = true;
      vTaskDelete(NULL);
      return;
    }
    int httpResponseCode = http->responseStatusCode();
    if (!httpResponseCode) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      http->stop();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      xSemaphoreGive(xSemaphore);
      isFWChecked = true;
      vTaskDelete(NULL);
      return;
    }
    debugPrint("Status code: %d\n", httpResponseCode);
    if (httpResponseCode > 0) {

      String payload = http->responseBody();
      payload.replace(" ", "");
      debugPrint("Payload: %s");
      if (Print *console = debugConsole())
        console->println(payload);
      StaticJsonDocument<500> doc;
      deserializeJson(doc, payload);
      // check if key build exist
      if (!doc.containsKey("build")) {
        debugPrint("No build key found\n");
        http->stop();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        xSemaphoreGive(xSemaphore);
        isFWChecked = true;
        vTaskDelete(NULL);
        return;
      }
      http->stop();
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      std::string lastFWBuild = doc["build"];
      std::string url = doc["url"];
      std::string fwTimeStamp = doc["created_at"];
      debugPrint("Firmware timestamp: %d\n", fwTimeStamp);
      debugPrint("Url: %s\n", url.c_str());
      debugPrint("Latest FW Build: %s\n", lastFWBuild.c_str());
      debugPrint("Current FW Build: %d\n", BUILD_NUMBER);
      debugPrint("Force Update: %s\n",
                 hubSettings.fota_force ? "true" : "false");
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      if (lastFWBuild != std::to_string(BUILD_NUMBER) ||
          hubSettings.fota_force) {
        debugPrint("New build available or FOTA forced\n");

        // Ensure all SSL connections are properly closed before FOTA
        if (hubSettings.net_mode != "wifi") {
          debugPrint("Closing API connection before FOTA\n");
          GSMClient_SSL.stop();            // Close API connection on mux 1
          vTaskDelay(pdMS_TO_TICKS(3000)); // Wait for proper closure
        }

        if (fotaUpgrade(url.c_str())) {
          debugPrint("firmware upgrade completed successfully\n");
          config.setDeviceSetting("fota_fw_ts",
                                  fwTimeStamp.c_str()); // Set timestamp
          rgbLed.setBrightness(rgbBrightness);
          rgbLed.setPixelColor(0, rgbLed.Color(0, 255, 255));
          rgbLed.show();
          debugPrint("Current stack size: %d\n",
                     uxTaskGetStackHighWaterMark(NULL));
          delay(3000);
          ESP.restart();
        } else {
          debugPrint("firmware upgrade failed\n");
          rgbLed.setBrightness(rgbBrightness);
          rgbLed.setPixelColor(0, rgbLed.Color(255, 0, 0)); // Red
          rgbLed.show();
        }
      } else {
        debugPrint("No new build available\n");
      }
    } else {
      debugPrint("Error code: %d\n", httpResponseCode);
      http->stop();
    }
    delete http;

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    xSemaphoreGive(xSemaphore);
    isFWChecked = true;
    vTaskDelete(NULL);
  }
}
