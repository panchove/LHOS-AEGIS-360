// Decoder for Xiaomi LYWSD03MMC ATC custom firmware
#pragma once
#ifndef LYWSD03MMC_DEVICES_LAYRZHUB_H
#define LYWSD03MMC_DEVICES_LAYRZHUB_H

#include <modules/utilities/UtilitiesLayrzHub.h>

class LYWSD03MMC_decoder {
public:
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // LYWSD03MMC_DEVICES_LAYRZHUB_H
