// Decoder for 3scort ESCORT DU
#pragma once
#ifndef ESCORT_DU_DEVICES_LAYRZHUB_H
#define ESCORT_DU_DEVICES_LAYRZHUB_H

#include <modules/utilities/UtilitiesLayrzHub.h>

class ESCORT_DU_decoder {
public:
  static std::string decode(const char *macAddress, const char *model,
                            const char *rssi, const char *txPower,
                            const char *manufData, const char *serviceData);
};

#endif // ESCORT_DU_DEVICES_LAYRZHUB_H
