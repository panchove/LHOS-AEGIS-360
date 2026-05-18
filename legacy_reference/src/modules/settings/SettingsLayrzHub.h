#pragma once

#ifndef __SETTINGSLAYRZHUB_H__
#define __SETTINGSLAYRZHUB_H_
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/settings/UnifiedSettingsStorage.h>
#include <modules/utilities/UtilitiesLayrzHub.h>

enum class ModbusDevice { none = 0, wsfg06 = 1, powerCommand2_3 = 2 };

class deviceSettings {
public:
  struct settingsKeyList {
    const char *key;
    const char *factoryValue;
    enum Type { INT, INT64, INT16, FLOAT, BOOL, STRING } type;
  };

  static void initDeviceSettings();
  static bool setDeviceSetting(const char *key, const char *value);
  static std::string getDeviceSettings();
  static std::string getDeviceShortSettings();
  static void setFactorySettings();
  static void initSettingsBuffer();
  static const settingsKeyList settingsKeyListData[];
  static const int settingsKeyListSize;
  static int getKeyIndex(const char *key);

private:
  // Private implementation details can go here
};

#endif // __SETTINGSLAYRZHUB_H__