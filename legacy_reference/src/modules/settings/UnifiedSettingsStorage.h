#pragma once

#ifndef __UNIFIED_SETTINGS_STORAGE_H__
#define __UNIFIED_SETTINGS_STORAGE_H__

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <esp_heap_caps.h>

class UnifiedSettingsStorage {
public:
  static bool init();
  static bool loadAllSettings();
  static bool saveAllSettings();

  // Generic getters
  static String getString(const String &key, const String &defaultValue = "");
  static int getInt(const String &key, int defaultValue = 0);
  static int64_t getInt64(const String &key, int64_t defaultValue = 0);
  static float getFloat(const String &key, float defaultValue = 0.0f);
  static bool getBool(const String &key, bool defaultValue = false);

  // Generic setters
  static bool setString(const String &key, const String &value);
  static bool setInt(const String &key, int value);
  static bool setInt64(const String &key, int64_t value);
  static bool setFloat(const String &key, float value);
  static bool setBool(const String &key, bool value);

  // Batch operations
  static bool exists(const String &key);
  static bool remove(const String &key);
  static bool clear();

  // Debug and maintenance
  static size_t getUsedSpace();
  static size_t getFreeSpace();
  static void printAllSettings();

  // Migration from NVS/Preferences
  static bool migrateNetworkSettingsFromNVS();

private:
  static const char *SETTINGS_FILE;
  static DynamicJsonDocument *settingsJson; // Allocated in SPIRAM
  static bool spiffsInitialized;
  static bool settingsLoaded;

  static bool allocateJsonInSpiram();
  static bool loadFromSPIFFS();
  static bool saveToSPIFFS();
  static void initializeDefaults();
};

#endif
