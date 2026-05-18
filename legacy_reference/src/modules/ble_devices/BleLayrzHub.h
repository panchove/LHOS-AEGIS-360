#pragma once

#ifndef __BLELAYRZHUB_H__
#define __BLELAYRZHUB_H__

#include <modules/ble_devices/BleDecoder.h>
#include <modules/ble_devices/BleMemoryManager.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/gnss/GnssLayrzHub.h>
#include <modules/layrz_protocol/LinkLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>
#include <modules/settings/UnifiedSettingsStorage.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

enum class BleDevices : uint8_t {
  LAYRZ_BLEFIER_1,
  ELA_PUCK_ID,
  ELA_PUCK_MOV,
  ELA_PUCK_PIR,
  ELA_PUCK_RHT,
  ELA_COIN_MAG,
  ELA_PUCK_MAG,
  LYWSD03MMC_PVVX,
  ESCORT_DU
};

extern std::string lastPosition;

class BleDevLayrzCallBack : public NimBLEScanCallbacks {
public:
  void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override;
  void onScanEnd(const NimBLEScanResults &scanResults, int reason) override;
};

class BleDevicesLayrzHub {
public:
  static int
  init(); // Initialize BLE devices. Returns the number of active BLE devices.
  static void bleScanTask(void *pvParameters);
};

#endif