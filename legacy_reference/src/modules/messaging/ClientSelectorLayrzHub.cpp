#include "ClientSelectorLayrzHub.h"

// Factory Method
BaseClientLayrzHub *ClientFactoryLayrzHub::createClient() {
  if (hubSettings.net_protocol == 1) {
    return new HttpsClientLayrzHub(hubSettings.net_server.c_str(), DATA_PATH);
  } else if (hubSettings.net_protocol == 0) {
    return new TcpSocketClientLayrzHub();
  }
  debugPrint(
    "WARNING: Invalid network transmission protocol! Returning nullptr.\n");
  return nullptr;
}

HttpsClientLayrzHub::HttpsClientLayrzHub(const char *server, const char *path)
    : server(server), path(path) {}

serverResponse HttpsClientLayrzHub::sendDataToServer(const char *data) {
  long startTime = millis();
  serverResponse response;
  response.responseCode = -1;
  if (!tcpSslClient) {
    debugPrint("Https client not initialized\n");
    return response;
  }
  tcpSslClient->setInsecure();
  HttpClient *http = static_cast<HttpClient *>(
    heap_caps_malloc(sizeof(HttpClient), MALLOC_CAP_SPIRAM));
  if (!http) {
    debugPrint("Failed to allocate HttpClient in PSRAM\n");
    return response;
  }
  http = new (http) HttpClient(*tcpSslClient, server, HTTPS_PORT);
  const std::string httpAuth = "LayrzAuth " + getBluetoothMACAddress() + ";";
  int trialCount = 0;
  while (trialCount < NET_TRIALS && response.responseCode != 200) {
    http->beginRequest();
    http->post(path);
    http->sendHeader("Content-Type", "text/plain");
    http->sendHeader("Content-Length", strlen(data));
    http->sendHeader("Authorization", httpAuth.c_str());
    http->beginBody();
    http->print(data);
    http->endRequest();
    response.responseCode = http->responseStatusCode();
    response.setResponseStr(http->responseBody().c_str());
    if (response.responseCode != 200) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    trialCount++;
  }
  http->stop();
  http->~HttpClient();
  heap_caps_free(http);
  blinkRGBLed(response.responseCode);
  debugPrint("Time to post http data: %d\n", millis() - startTime);
  return response;
}

serverResponse HttpsClientLayrzHub::receiveDataFromServer() {
  serverResponse response;
  response.responseCode = -1;
  if (!tcpSslClient) {
    debugPrint("Https client not initialized\n");
    return response;
  }
  tcpSslClient->setInsecure();
  HttpClient *http = static_cast<HttpClient *>(
    heap_caps_malloc(sizeof(HttpClient), MALLOC_CAP_SPIRAM));
  if (!http) {
    debugPrint("Failed to allocate HttpClient in PSRAM\n");
    return response;
  }
  http = new (http) HttpClient(*tcpSslClient, server, HTTPS_PORT);
  const std::string httpAuth = "LayrzAuth " + getBluetoothMACAddress() + ";";
  int trialCount = 0;
  while (trialCount < NET_TRIALS && response.responseCode != 200) {
    http->beginRequest();
    http->get(COMMAND_PATH);
    http->sendHeader("Content-Type", "text/plain");
    http->sendHeader("Authorization", httpAuth.c_str());
    http->endRequest();
    response.responseCode = http->responseStatusCode();
    if (!response.setResponseStr(http->responseBody().c_str())) {
      response.responseCode = -2;
      break;
    }
    if (response.responseCode != 200) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    trialCount++;
  }
  http->stop();
  http->~HttpClient();
  heap_caps_free(http);
  return response;
}

serverResponse TcpSocketClientLayrzHub::sendDataToServer(const char *data) {
  serverResponse response;
  response.responseCode = -1;
  if (!tcpNonSslClient || !tcpNonSslClient->connected())
    return response;
  socketSendSuccess = false;
  time_t initialTime = millis();
  tcpNonSslClient->print(data);
  xSemaphoreGive(xSemaphore);
  while (!socketSendSuccess && (millis() - initialTime < 5000)) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  response.responseCode = socketSendSuccess ? 200 : -1;
  return response;
}

serverResponse TcpSocketClientLayrzHub::receiveDataFromServer() {
  serverResponse response;
  if (!tcpNonSslClient || !tcpNonSslClient->connected() ||
      !tcpNonSslClient->available())
    return response;
  size_t bufferSize = 4096;
  char *psramBuffer = (char *)heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM);
  if (!psramBuffer) {
    response.responseCode = -2;
    return response;
  }
  size_t index = 0;
  while (tcpNonSslClient->available() && index < bufferSize - 1) {
    psramBuffer[index++] = tcpNonSslClient->read();
  }
  psramBuffer[index] = '\0';
  if (index > 0) {
    if (!response.setResponseStr(std::string(psramBuffer))) {
      response.responseCode = -2;
      heap_caps_free(psramBuffer);
      return response;
    }
    response.responseCode = 200;
  }
  heap_caps_free(psramBuffer);
  return response;
}

void blinkRGBLed(int responseCode) {
  if (!hubSettings.rgb_en)
    return;
  MorseCode morse(LED, 1);
  morse.display(0, "H", responseCode == 200 ? 0x00FFFF : 0xFF0000);
  if (bleConfigConnected) {
    rgbLed.setBrightness(rgbBrightness);
    rgbLed.setPixelColor(0, rgbLed.Color(255, 255, 0));
    rgbLed.show();
    return;
  }
  uint32_t color = 0xFF0000; // default red
  if (hubSettings.net_mode == "wifi" && WiFi.status() == WL_CONNECTED)
    color = isSocketAuth ? 0x00FF00 : 0xFF0000;
#ifdef USE_ASYNC_A7670
  else if (hubSettings.net_mode == "cellular" && modemAsync.isGprsConnected())
    color = isSocketAuth ? 0x0000FF : 0xFF0000;
#else
  else if (hubSettings.net_mode == "cellular" && modem.isGprsConnected())
    color = isSocketAuth ? 0x0000FF : 0xFF0000;
#endif
  rgbLed.setBrightness(rgbBrightness);
  rgbLed.setPixelColor(0, ((color >> 16) & 0xFF), ((color >> 8) & 0xFF),
                       (color & 0xFF));
  rgbLed.show();
}
