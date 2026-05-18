// Decoder for Teltonika EYE Sensor
#pragma once
#ifndef TELTONIKA_EYE_DEVICES_LAYRZHUB_H
#define TELTONIKA_EYE_DEVICES_LAYRZHUB_H

#include <modules/ble_devices/BleSensorData.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class TELTONIKA_EYE_decoder {
public:
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // TELTONIKA_EYE_DEVICES_LAYRZHUB_H
