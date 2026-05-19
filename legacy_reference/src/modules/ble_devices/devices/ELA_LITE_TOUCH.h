// Decoder for ELA Innovation Blue LITE Touch
#pragma once
#ifndef ELA_LITE_TOUCH_DEVICES_LAYRZHUB_H
#define ELA_LITE_TOUCH_DEVICES_LAYRZHUB_H

#include <modules/ble_devices/BleSensorData.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class ELA_LITE_TOUCH_decoder {
public:
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // ELA_LITE_TOUCH_DEVICES_LAYRZHUB_H
