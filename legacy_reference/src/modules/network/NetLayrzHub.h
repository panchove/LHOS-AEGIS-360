#pragma once

#ifndef __NETLAYRZHUB_H__
#define __NETLAYRZHUB_H__

#include <modules/fota/FotaLayrzHub.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class NetLayrzHub {
public:
  static bool checkSocketClient(void);
  static void connectWiFi(const char *ssid, const char *password,
                          const char *ntpserver1, const char *ntpserver2);
  static void connectGprs(const char *apn, const char *user, const char *pass,
                          const char *pin, const char *ntpserver1,
                          const char *ntpserver2);
  static void checkWifiNetwork(void *pvParameters);
  static void checkGSMNetwork(void *pvParameters);
  static void synchroNTP(void *pvParameters);
  static void socketKeepAlive(void *pvParameters);
  // DNS cache helpers
  static bool resolveServer(bool force = false);
  static IPAddress cachedServerIP();

private:
  static IPAddress s_cachedIP;
  static uint32_t s_lastResolveMs;
};

#endif