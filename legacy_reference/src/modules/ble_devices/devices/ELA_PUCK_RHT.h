#pragma once

#ifndef ELA_PUCK_RHT_DEVICES_LAYRZHUB_H
#define ELA_PUCK_RHT_DEVICES_LAYRZHUB_H

#include <modules/ble_devices/BleSensorData.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class ELA_PUCK_RHT_decoder {
public:
  // Decodes service/manufacturer data into "ble.<mac>.*" key:value CSV
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // ELA_PUCK_RHT_DEVICES_LAYRZHUB_H
