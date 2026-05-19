// Generic decoder for any BLE device (lists raw service & manufacturer data)
#pragma once
#ifndef GENERIC_BLE_DEVICE_DECODER_LAYRZHUB_H
#define GENERIC_BLE_DEVICE_DECODER_LAYRZHUB_H

#include <modules/ble_devices/BleSensorData.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class GENERIC_decoder {
public:
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // GENERIC_BLE_DEVICE_DECODER_LAYRZHUB_H
