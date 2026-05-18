#pragma once
// Unique include guard (previously collided with other headers using
// LAYRZ_MESSAGE_BUS_LAYRZHUB_H)
#ifndef BLE_DECODER_LAYRZHUB_H
#define BLE_DECODER_LAYRZHUB_H

#include <modules/ble_devices/BleSensorData.h>
#include <modules/ble_devices/devices/APPLE_IBEACON.h>
#include <modules/ble_devices/devices/ELA_COIN_MAG.h>
#include <modules/ble_devices/devices/ELA_COIN_T.h>
#include <modules/ble_devices/devices/ELA_LITE_TOUCH.h>
#include <modules/ble_devices/devices/ELA_PUCK_DI.h>
#include <modules/ble_devices/devices/ELA_PUCK_ID.h>
#include <modules/ble_devices/devices/ELA_PUCK_MAG.h>
#include <modules/ble_devices/devices/ELA_PUCK_MOV.h>
#include <modules/ble_devices/devices/ELA_PUCK_PIR.h>
#include <modules/ble_devices/devices/ELA_PUCK_RHT.h>
#include <modules/ble_devices/devices/ELA_PUCK_TPROBE.h>
#include <modules/ble_devices/devices/ESCORT_DU.h>
#include <modules/ble_devices/devices/GENERIC.h>
#include <modules/ble_devices/devices/LAYRZ_BLEFIER_1.h>
#include <modules/ble_devices/devices/LAYRZ_BLEFIER_2.h>
#include <modules/ble_devices/devices/LYWSD03MMC.h>
#include <modules/ble_devices/devices/TELTONIKA_EYE.h>
#include <modules/utilities/UtilitiesLayrzHub.h>
#include <string>

class BleDecoder {
public:
  // Decode a semicolon-separated BLE packet string into a structured JSON (TBD)
  // or log fields.
  static std::string decode(const char *blePacket);
};
#endif // BLE_DECODER_LAYRZHUB_H