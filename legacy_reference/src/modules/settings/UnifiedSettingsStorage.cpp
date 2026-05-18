#include "UnifiedSettingsStorage.h"

#include <Preferences.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>

const char *UnifiedSettingsStorage::SETTINGS_FILE = "/settings.json";
DynamicJsonDocument *UnifiedSettingsStorage::settingsJson = nullptr;
bool UnifiedSettingsStorage::spiffsInitialized = false;
bool UnifiedSettingsStorage::settingsLoaded = false;

bool UnifiedSettingsStorage::init() {
  if (!spiffsInitialized) {
    if (!SPIFFS.begin(true)) {
      debugPrint("Failed to mount SPIFFS\n");
      return false;
    }
    spiffsInitialized = true;
    debugPrint("SPIFFS mounted successfully\n");
  }

  if (!allocateJsonInSpiram()) {
    debugPrint("Failed to allocate JSON document in SPIRAM\n");
    return false;
  }

  return loadAllSettings();
}

bool UnifiedSettingsStorage::allocateJsonInSpiram() {
  if (settingsJson != nullptr) {
    return true; // Already allocated
  }

  // Allocate in SPIRAM (external PSRAM) - 64KB for all settings
  void *spiramPtr =
    heap_caps_malloc(sizeof(DynamicJsonDocument), MALLOC_CAP_SPIRAM);
  if (spiramPtr == nullptr) {
    debugPrint("Failed to allocate SPIRAM for settings JSON\n");
    // Fallback to internal RAM
    settingsJson = new DynamicJsonDocument(65536); // 64KB
  } else {
    settingsJson = new (spiramPtr) DynamicJsonDocument(65536); // 64KB in SPIRAM
    debugPrint("Settings JSON allocated in SPIRAM successfully\n");
  }

  return settingsJson != nullptr;
}

bool UnifiedSettingsStorage::loadAllSettings() {
  if (!spiffsInitialized || settingsJson == nullptr) {
    return false;
  }

  if (!loadFromSPIFFS()) {
    debugPrint("Failed to load settings from SPIFFS, initializing defaults\n");
    initializeDefaults();
    saveToSPIFFS(); // Save defaults
  }

  settingsLoaded = true;
  return true;
}

bool UnifiedSettingsStorage::saveAllSettings() {
  if (!settingsLoaded || settingsJson == nullptr) {
    return false;
  }
  return saveToSPIFFS();
}

String UnifiedSettingsStorage::getString(const String &key,
                                         const String &defaultValue) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return defaultValue;
  }

  if (settingsJson->containsKey(key)) {
    return (*settingsJson)[key].as<String>();
  }
  return defaultValue;
}

int UnifiedSettingsStorage::getInt(const String &key, int defaultValue) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return defaultValue;
  }

  if (settingsJson->containsKey(key)) {
    return (*settingsJson)[key].as<int>();
  }
  return defaultValue;
}

int64_t UnifiedSettingsStorage::getInt64(const String &key,
                                         int64_t defaultValue) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return defaultValue;
  }

  if (settingsJson->containsKey(key)) {
    return (*settingsJson)[key].as<int64_t>();
  }
  return defaultValue;
}

float UnifiedSettingsStorage::getFloat(const String &key, float defaultValue) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return defaultValue;
  }

  if (settingsJson->containsKey(key)) {
    return (*settingsJson)[key].as<float>();
  }
  return defaultValue;
}

bool UnifiedSettingsStorage::getBool(const String &key, bool defaultValue) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return defaultValue;
  }

  if (settingsJson->containsKey(key)) {
    return (*settingsJson)[key].as<bool>();
  }
  return defaultValue;
}

bool UnifiedSettingsStorage::setString(const String &key, const String &value) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return false;
  }

  (*settingsJson)[key] = value;
  return true; // We'll save in batch later
}

bool UnifiedSettingsStorage::setInt(const String &key, int value) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return false;
  }

  (*settingsJson)[key] = value;
  return true;
}

bool UnifiedSettingsStorage::setInt64(const String &key, int64_t value) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return false;
  }

  (*settingsJson)[key] = value;
  return true;
}

bool UnifiedSettingsStorage::setFloat(const String &key, float value) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return false;
  }

  (*settingsJson)[key] = value;
  return true;
}

bool UnifiedSettingsStorage::setBool(const String &key, bool value) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return false;
  }

  (*settingsJson)[key] = value;
  return true;
}

bool UnifiedSettingsStorage::exists(const String &key) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return false;
  }
  return settingsJson->containsKey(key);
}

bool UnifiedSettingsStorage::remove(const String &key) {
  if (!settingsLoaded || settingsJson == nullptr) {
    return false;
  }

  settingsJson->remove(key);
  return true;
}

bool UnifiedSettingsStorage::clear() {
  if (settingsJson != nullptr) {
    settingsJson->clear();
    return true;
  } else {
    return false;
  }
}

bool UnifiedSettingsStorage::loadFromSPIFFS() {
  if (!spiffsInitialized || settingsJson == nullptr) {
    return false;
  }

  if (!SPIFFS.exists(SETTINGS_FILE)) {
    debugPrint("Settings file not found, will create new one\n");
    return false;
  }

  File file = SPIFFS.open(SETTINGS_FILE, "r");
  if (!file) {
    debugPrint("Failed to open settings file for reading\n");
    return false;
  }

  DeserializationError error = deserializeJson(*settingsJson, file);
  file.close();

  if (error) {
    debugPrint("Failed to parse settings JSON: %s\n", error.c_str());
    return false;
  }

  debugPrint("Settings loaded from SPIFFS successfully\n");
  return true;
}

bool UnifiedSettingsStorage::saveToSPIFFS() {
  if (!spiffsInitialized || settingsJson == nullptr) {
    return false;
  }

  File file = SPIFFS.open(SETTINGS_FILE, "w");
  if (!file) {
    debugPrint("Failed to open settings file for writing\n");
    return false;
  }

  if (serializeJson(*settingsJson, file) == 0) {
    debugPrint("Failed to write settings JSON\n");
    file.close();
    return false;
  }

  file.close();
  debugPrint("Settings saved to SPIFFS successfully\n");
  return true;
}

void UnifiedSettingsStorage::initializeDefaults() {
  if (settingsJson == nullptr) {
    return;
  }

  // Clear existing settings
  settingsJson->clear();

  // Initialize all default values from settingsKeyListData
  for (int i = 0; i < deviceSettings::settingsKeyListSize; i++) {
    String key = String(deviceSettings::settingsKeyListData[i].key);
    String factoryValue =
      String(deviceSettings::settingsKeyListData[i].factoryValue);

    switch (deviceSettings::settingsKeyListData[i].type) {
    case deviceSettings::settingsKeyList::STRING:
      (*settingsJson)[key] = factoryValue;
      break;
    case deviceSettings::settingsKeyList::BOOL:
      (*settingsJson)[key] = (factoryValue == "true" || factoryValue == "1");
      break;
    case deviceSettings::settingsKeyList::INT:
      (*settingsJson)[key] = factoryValue.toInt();
      break;
    case deviceSettings::settingsKeyList::INT64:
      (*settingsJson)[key] = (int64_t)factoryValue.toDouble();
      break;
    case deviceSettings::settingsKeyList::FLOAT:
      (*settingsJson)[key] = factoryValue.toFloat();
      break;
    }
  }

  debugPrint("Default settings initialized\n");
}

size_t UnifiedSettingsStorage::getUsedSpace() {
  if (!spiffsInitialized) {
    return 0;
  }
  return SPIFFS.usedBytes();
}

size_t UnifiedSettingsStorage::getFreeSpace() {
  if (!spiffsInitialized) {
    return 0;
  }
  return SPIFFS.totalBytes() - SPIFFS.usedBytes();
}

void UnifiedSettingsStorage::printAllSettings() {
  if (!settingsLoaded || settingsJson == nullptr) {
    debugPrint("Settings not loaded\n");
    return;
  }

  debugPrint("=== ALL SETTINGS ===\n");
  for (JsonPair kv : settingsJson->as<JsonObject>()) {
    debugPrint("%s: %s\n", kv.key().c_str(), kv.value().as<String>().c_str());
  }
  debugPrint("====================\n");
}

bool UnifiedSettingsStorage::migrateNetworkSettingsFromNVS() {
  if (!settingsLoaded || settingsJson == nullptr) {
    debugPrint("Cannot migrate: unified storage not initialized\n");
    return false;
  }

  // Initialize legacy Preferences temporarily for migration
  Preferences legacySettings;
  if (!legacySettings.begin("settings", true)) { // read-only mode
    debugPrint("Cannot open legacy NVS settings for migration\n");
    return false;
  }

  bool migrationNeeded = false;
  bool migrationSuccess = true;

  // Critical network parameters that need migration
  const char *networkKeys[] = {"net_en",        "net_mode",      "net_server",
                               "wifi_ssid",     "wifi_pass",     "gprs_apn",
                               "gprs_apn_user", "gprs_apn_pass", "gprs_pin"};

  debugPrint("Starting network settings migration from NVS to SPIFFS...\n");

  for (int i = 0; i < 9; i++) {
    String key = String(networkKeys[i]);

    // Check if this key exists in legacy NVS and not in SPIFFS (or has default
    // value)
    if (legacySettings.isKey(key.c_str())) {
      String nvsValue;
      String currentSpiffsValue = getString(key, "");

      // Get the factory default for this key
      String factoryDefault = "";
      for (int j = 0; j < deviceSettings::settingsKeyListSize; j++) {
        if (String(deviceSettings::settingsKeyListData[j].key) == key) {
          factoryDefault =
            String(deviceSettings::settingsKeyListData[j].factoryValue);
          break;
        }
      }

      // Only migrate if SPIFFS has default value or key doesn't exist
      if (currentSpiffsValue == "" || currentSpiffsValue == factoryDefault) {
        // Get value from NVS based on expected type
        if (key == "net_en") {
          bool nvsVal = legacySettings.getBool(key.c_str(), false);
          if (setBool(key, nvsVal)) {
            migrationNeeded = true;
            debugPrint("Migrated %s: %s\n", key.c_str(),
                       nvsVal ? "true" : "false");
          } else {
            migrationSuccess = false;
          }
        } else {
          // All other network keys are strings
          nvsValue = legacySettings.getString(key.c_str(), "");
          if (nvsValue != "" && nvsValue != factoryDefault) {
            if (setString(key, nvsValue)) {
              migrationNeeded = true;
              debugPrint("Migrated %s: %s\n", key.c_str(), nvsValue.c_str());
            } else {
              migrationSuccess = false;
            }
          }
        }
      } else {
        debugPrint("Skipping %s - already customized in SPIFFS\n", key.c_str());
      }
    }
  }

  legacySettings.end();

  if (migrationNeeded && migrationSuccess) {
    if (saveAllSettings()) {
      debugPrint("Network settings migration completed successfully\n");
      return true;
    } else {
      debugPrint("Failed to save migrated settings to SPIFFS\n");
      return false;
    }
  } else if (!migrationNeeded) {
    debugPrint("No network settings migration needed\n");
    return true;
  } else {
    debugPrint("Network settings migration failed\n");
    return false;
  }
}
