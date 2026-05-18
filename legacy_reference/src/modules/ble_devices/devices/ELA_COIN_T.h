// Decoder for ELA Innovation Blue COIN Temperature
#pragma once
#ifndef ELA_COIN_T_DEVICES_LAYRZHUB_H
#define ELA_COIN_T_DEVICES_LAYRZHUB_H

#include <modules/ble_devices/BleSensorData.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class ELA_COIN_T_decoder {
public:
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // ELA_COIN_T_DEVICES_LAYRZHUB_H
