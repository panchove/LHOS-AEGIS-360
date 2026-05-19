#include "BleMemoryManager.h"

// Static member definitions
char *BleMemoryManager::largePacketBuffer = nullptr;
char *BleMemoryManager::mediumPacketBuffer = nullptr;
BlePacketEntry *BleMemoryManager::bleDevicesArray = nullptr;
bool BleMemoryManager::initialized = false;

bool BleMemoryManager::init() {
  if (initialized) {
    return true;
  }

  debugPrint(
    "[BleMemoryManager] Initializing SPIRAM buffers for BLE operations...\n");

  // Allocate buffers in SPIRAM
  largePacketBuffer =
    allocateSpiramBuffer(LARGE_PACKET_BUFFER_SIZE, "Large Packet Buffer");
  mediumPacketBuffer =
    allocateSpiramBuffer(MEDIUM_PACKET_BUFFER_SIZE, "Medium Packet Buffer");

  // Allocate BLE devices array in SPIRAM
  size_t arraySize = MAX_BLE_DEVICES * sizeof(BlePacketEntry);
  bleDevicesArray =
    (BlePacketEntry *)heap_caps_malloc(arraySize, MALLOC_CAP_SPIRAM);

  if (!largePacketBuffer || !mediumPacketBuffer || !bleDevicesArray) {
    debugPrint("[BleMemoryManager] ERROR: Failed to allocate SPIRAM buffers\n");
    cleanup();
    return false;
  }

  // Clear all buffers
  clearBuffer(largePacketBuffer, LARGE_PACKET_BUFFER_SIZE);
  clearBuffer(mediumPacketBuffer, MEDIUM_PACKET_BUFFER_SIZE);

  // Initialize BLE devices array
  clearBleDevicesArray();

  if (bleDevicesArray) {
    debugPrint(
      "[BleMemoryManager] Allocated BLE Devices Array: %zu bytes at %p\n",
      arraySize, bleDevicesArray);
  } else {
    debugPrint("[BleMemoryManager] ERROR: Failed to allocate BLE devices array "
               "(%zu bytes) in "
               "SPIRAM\n",
               arraySize);
  }

  initialized = true;
  printMemoryStatus();
  debugPrint("[BleMemoryManager] SPIRAM buffers initialized successfully\n");

  return true;
}

char *BleMemoryManager::getLargePacketBuffer() { return largePacketBuffer; }

char *BleMemoryManager::getMediumPacketBuffer() { return mediumPacketBuffer; }

BlePacketEntry *BleMemoryManager::getBleDevicesArray() {
  return bleDevicesArray;
}

bool BleMemoryManager::storeBlePacket(const std::string &macAddress,
                                      const std::string &packetData) {
  if (!bleDevicesArray || !initialized) {
    return false;
  }

  // First, check if MAC address already exists and update it
  for (size_t i = 0; i < MAX_BLE_DEVICES; i++) {
    if (bleDevicesArray[i].isValid &&
        strcmp(bleDevicesArray[i].macAddress, macAddress.c_str()) == 0) {
      // Update existing entry
      strncpy(bleDevicesArray[i].packetData, packetData.c_str(),
              sizeof(bleDevicesArray[i].packetData) - 1);
      bleDevicesArray[i].packetData[sizeof(bleDevicesArray[i].packetData) - 1] =
        '\0';
      bleDevicesArray[i].timestamp = millis();
      return true;
    }
  }

  // Find first available slot for new entry
  for (size_t i = 0; i < MAX_BLE_DEVICES; i++) {
    if (!bleDevicesArray[i].isValid) {
      strncpy(bleDevicesArray[i].macAddress, macAddress.c_str(),
              sizeof(bleDevicesArray[i].macAddress) - 1);
      bleDevicesArray[i].macAddress[sizeof(bleDevicesArray[i].macAddress) - 1] =
        '\0';

      strncpy(bleDevicesArray[i].packetData, packetData.c_str(),
              sizeof(bleDevicesArray[i].packetData) - 1);
      bleDevicesArray[i].packetData[sizeof(bleDevicesArray[i].packetData) - 1] =
        '\0';

      bleDevicesArray[i].isValid = true;
      bleDevicesArray[i].timestamp = millis();
      return true;
    }
  }

  // Array is full
  debugPrint("[BleMemoryManager] WARNING: BLE devices array is full, cannot "
             "store new packet\n");
  return false;
}

std::string BleMemoryManager::getBlePacket(const std::string &macAddress) {
  if (!bleDevicesArray || !initialized) {
    return "";
  }

  for (size_t i = 0; i < MAX_BLE_DEVICES; i++) {
    if (bleDevicesArray[i].isValid &&
        strcmp(bleDevicesArray[i].macAddress, macAddress.c_str()) == 0) {
      return std::string(bleDevicesArray[i].packetData);
    }
  }

  return ""; // Not found
}

void BleMemoryManager::clearBleDevicesArray() {
  if (!bleDevicesArray) {
    return;
  }

  for (size_t i = 0; i < MAX_BLE_DEVICES; i++) {
    memset(bleDevicesArray[i].macAddress, 0,
           sizeof(bleDevicesArray[i].macAddress));
    memset(bleDevicesArray[i].packetData, 0,
           sizeof(bleDevicesArray[i].packetData));
    bleDevicesArray[i].isValid = false;
    bleDevicesArray[i].timestamp = 0;
  }
}

size_t BleMemoryManager::getBleDeviceCount() {
  if (!bleDevicesArray || !initialized) {
    return 0;
  }

  size_t count = 0;
  for (size_t i = 0; i < MAX_BLE_DEVICES; i++) {
    if (bleDevicesArray[i].isValid) {
      count++;
    }
  }

  return count;
}

void BleMemoryManager::iterateBleDevices(
  void (*callback)(const char *macAddress, const char *packetData,
                   void *userData),
  void *userData) {
  if (!bleDevicesArray || !initialized || !callback) {
    return;
  }

  for (size_t i = 0; i < MAX_BLE_DEVICES; i++) {
    if (bleDevicesArray[i].isValid) {
      callback(bleDevicesArray[i].macAddress, bleDevicesArray[i].packetData,
               userData);
    }
  }
}

void BleMemoryManager::clearBuffer(char *buffer, size_t size) {
  if (buffer) {
    memset(buffer, 0, size);
  }
}

bool BleMemoryManager::copyToBuffer(char *buffer, size_t bufferSize,
                                    const std::string &source) {
  if (!buffer || source.length() >= bufferSize) {
    return false;
  }

  clearBuffer(buffer, bufferSize);
  strncpy(buffer, source.c_str(), bufferSize - 1);
  buffer[bufferSize - 1] = '\0';

  return true;
}

bool BleMemoryManager::appendToBuffer(char *buffer, size_t bufferSize,
                                      const std::string &source) {
  if (!buffer) {
    return false;
  }

  size_t currentLen = strlen(buffer);
  size_t sourceLen = source.length();

  if (currentLen + sourceLen >= bufferSize) {
    return false;
  }

  strncat(buffer, source.c_str(), bufferSize - currentLen - 1);

  return true;
}

std::string BleMemoryManager::bufferToString(const char *buffer) {
  if (!buffer) {
    return "";
  }
  return std::string(buffer);
}

void BleMemoryManager::printMemoryStatus() {
  debugPrint("[BleMemoryManager] Memory Status:\n");
  debugPrint("  Large Buffer: %p (size: %zu bytes)\n", largePacketBuffer,
             LARGE_PACKET_BUFFER_SIZE);
  debugPrint("  Medium Buffer: %p (size: %zu bytes)\n", mediumPacketBuffer,
             MEDIUM_PACKET_BUFFER_SIZE);
  debugPrint("  BLE Devices Array: %p (size: %zu entries, %zu bytes)\n",
             bleDevicesArray, MAX_BLE_DEVICES,
             MAX_BLE_DEVICES * sizeof(BlePacketEntry));
  debugPrint("  Active BLE Devices: %zu/%zu\n", getBleDeviceCount(),
             MAX_BLE_DEVICES);

  // Check SPIRAM status
  size_t spiramTotal = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
  size_t spiramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  size_t spiramUsed = spiramTotal - spiramFree;

  debugPrint("  SPIRAM Total: %zu bytes\n", spiramTotal);
  debugPrint("  SPIRAM Used: %zu bytes (%.1f%%)\n", spiramUsed,
             (float)spiramUsed / spiramTotal * 100.0f);
  debugPrint("  SPIRAM Free: %zu bytes\n", spiramFree);
}

bool BleMemoryManager::isInitialized() { return initialized; }

void BleMemoryManager::cleanup() {
  if (largePacketBuffer) {
    heap_caps_free(largePacketBuffer);
    largePacketBuffer = nullptr;
  }

  if (mediumPacketBuffer) {
    heap_caps_free(mediumPacketBuffer);
    mediumPacketBuffer = nullptr;
  }

  if (bleDevicesArray) {
    heap_caps_free(bleDevicesArray);
    bleDevicesArray = nullptr;
  }

  initialized = false;
  debugPrint("[BleMemoryManager] SPIRAM buffers cleaned up\n");
}

char *BleMemoryManager::allocateSpiramBuffer(size_t size,
                                             const char *description) {
  char *buffer = (char *)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);

  if (buffer) {
    debugPrint("[BleMemoryManager] Allocated %s: %zu bytes at %p\n",
               description, size, buffer);
  } else {
    debugPrint(
      "[BleMemoryManager] ERROR: Failed to allocate %s (%zu bytes) in SPIRAM\n",
      description, size);
  }

  return buffer;
}
