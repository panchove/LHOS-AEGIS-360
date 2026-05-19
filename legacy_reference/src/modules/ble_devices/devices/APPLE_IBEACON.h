// Decoder for Apple iBeacon minimal fields
#pragma once
#ifndef APPLE_IBEACON_DEVICES_LAYRZHUB_H
#define APPLE_IBEACON_DEVICES_LAYRZHUB_H

#include <modules/ble_devices/BleSensorData.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class APPLE_IBEACON_decoder {
public:
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // APPLE_IBEACON_DEVICES_LAYRZHUB_H
