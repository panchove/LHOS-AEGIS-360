#ifndef BLE_MEMORY_MANAGER_H
#define BLE_MEMORY_MANAGER_H

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <modules/utilities/UtilitiesLayrzHub.h>
#include <string>

// Structure for BLE device packet storage in SPIRAM
struct BlePacketEntry {
  char macAddress[16]; // MAC address without colons (12 chars + null terminator
                       // + padding)
  char packetData[256]; // Complete BLE packet data
  bool isValid;         // Flag to indicate if entry contains valid data
  uint32_t timestamp;   // Timestamp when packet was stored
};

class BleMemoryManager {
public:
  // SPIRAM buffer sizes optimized for BLE operations
  static constexpr size_t LARGE_PACKET_BUFFER_SIZE =
    8192; // For packet concatenation (50 devices * ~150 bytes each)
  static constexpr size_t MEDIUM_PACKET_BUFFER_SIZE =
    512; // For individual BLE packets
  static constexpr size_t MAX_BLE_DEVICES =
    50; // Maximum number of BLE devices to track

  // Initialize SPIRAM buffers for BLE operations
  static bool init();

  // Get pre-allocated SPIRAM buffers
  static char *getLargePacketBuffer();  // For main packet concatenation
  static char *getMediumPacketBuffer(); // For individual BLE packets

  // BLE devices array management (replaces bleDevicesMap)
  static BlePacketEntry *getBleDevicesArray();
  static bool storeBlePacket(const std::string &macAddress,
                             const std::string &packetData);
  static std::string getBlePacket(const std::string &macAddress);
  static void clearBleDevicesArray();
  static size_t getBleDeviceCount();
  static void iterateBleDevices(void (*callback)(const char *macAddress,
                                                 const char *packetData,
                                                 void *userData),
                                void *userData);

  // Helper functions for safe string operations with SPIRAM buffers
  static void clearBuffer(char *buffer, size_t size);
  static bool copyToBuffer(char *buffer, size_t bufferSize,
                           const std::string &source);
  static bool appendToBuffer(char *buffer, size_t bufferSize,
                             const std::string &source);
  static std::string bufferToString(const char *buffer);

  // Memory status
  static void printMemoryStatus();
  static bool isInitialized();

  // Cleanup
  static void cleanup();

private:
  static char *largePacketBuffer;
  static char *mediumPacketBuffer;
  static BlePacketEntry *bleDevicesArray;
  static bool initialized;

  static char *allocateSpiramBuffer(size_t size, const char *description);
};

#endif // BLE_MEMORY_MANAGER_H
