// Decoder for ELA Innovation Blue PUCK Movement
#pragma once
#ifndef ELA_PUCK_MOV_DEVICES_LAYRZHUB_H
#define ELA_PUCK_MOV_DEVICES_LAYRZHUB_H

#include <modules/ble_devices/BleSensorData.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

class ELA_PUCK_MOV_decoder {
public:
  // Decodes service/manufacturer data into CSV key:value pairs prefixed with
  // "ble.<mac>."
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // ELA_PUCK_MOV_DEVICES_LAYRZHUB_H
