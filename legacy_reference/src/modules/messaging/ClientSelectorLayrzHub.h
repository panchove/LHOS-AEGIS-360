#pragma once
#ifndef __CLIENTSELECTORLAYRZHUB_H__
#define __CLIENTSELECTORLAYRZHUB_H__

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/rgbled/RgbLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>

struct serverResponse {
  int responseCode;
  std::unique_ptr<std::string> responseStr;
  serverResponse() : responseCode(-1), responseStr(nullptr) {
    allocatePSRAMString("");
  }
  bool setResponseStr(const std::string &s) { return allocatePSRAMString(s); }

private:
  bool allocatePSRAMString(const std::string &s) {
    responseStr = std::make_unique<std::string>(s);
    return true;
  }
};

void blinkRGBLed(int responseCode);

class BaseClientLayrzHub {
public:
  virtual ~BaseClientLayrzHub() = default;
  virtual serverResponse sendDataToServer(const char *data) = 0;
  virtual serverResponse receiveDataFromServer() = 0;
};

class HttpsClientLayrzHub : public BaseClientLayrzHub {
private:
  const char *server;
  const char *path;

public:
  HttpsClientLayrzHub(const char *server, const char *path);
  serverResponse sendDataToServer(const char *data) override;
  serverResponse receiveDataFromServer() override;
};

class TcpSocketClientLayrzHub : public BaseClientLayrzHub {
public:
  serverResponse sendDataToServer(const char *data) override;
  serverResponse receiveDataFromServer() override;
};

class ClientFactoryLayrzHub {
public:
  static BaseClientLayrzHub *createClient();
};

#endif
