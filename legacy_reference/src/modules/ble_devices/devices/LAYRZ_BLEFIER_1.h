#pragma once

#ifndef LAYRZ_BLEFIER_1_DEVICES_LAYRZHUB_H
#define LAYRZ_BLEFIER_1_DEVICES_LAYRZHUB_H

#include <modules/ble_devices/BleSensorData.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class LAYRZ_BLEFIER_1_decoder {
public:
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);

private:
  static bleSensorData parseData(const std::string bleData);
};

#endif // LAYRZ_BLEFIER_1_DEVICES_LAYRZHUB_H