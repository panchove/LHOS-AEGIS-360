// Decoder for ELA Innovation Blue PUCK Digital Input
#pragma once
#ifndef ELA_PUCK_DI_DEVICES_LAYRZHUB_H
#define ELA_PUCK_DI_DEVICES_LAYRZHUB_H

#include <modules/utilities/UtilitiesLayrzHub.h>

class ELA_PUCK_DI_decoder {
public:
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // ELA_PUCK_DI_DEVICES_LAYRZHUB_H
