#pragma once

#ifndef __UTILITIESLAYRZHUB_H__
#define __UTILITIESLAYRZHUB_H__

#include <cstdint>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>
#include <regex>
#include <stdarg.h>
#include <vector>

struct rgbColor {
  int red;
  int green;
  int blue;
};
constexpr size_t maxKeySize = 64;
constexpr size_t maxLongKeySize = 8192;
rgbColor hexToRGB(const String &hexColor);
void sincroTime(const char *netMode, const char *ntpserver1,
                const char *ntpserver2);
uint16_t crc16_x25(const uint8_t *data, int len = 0);
std::string calculateCRC16(const char *data, int len = 0);
bool debugConsoleEnabled();
Print *debugConsole();
void debugPrint(const char *format, ...);
std::string getDeviceName();
std::string getESP32SerialNumber();
std::string getBluetoothMACAddress();
std::string floatToString(float value, int precision = 2);
std::vector<std::string> splitString(const std::string &str,
                                     const std::string &delimiter);
std::unique_ptr<std::vector<std::string>> allocateVectorPSRAM();
std::unique_ptr<std::vector<std::string>>
splitStringRegex(const std::unique_ptr<std::string> &strPtr);
// std::vector<std::string> splitStringRegex(const std::string& str);
std::unique_ptr<std::vector<std::string>>
splitStringManual(const std::unique_ptr<std::string> &strPtr);
void checkMemory();
void displayTaskStackUsage();
std::vector<uint8_t> hexToBytes(const char *hex);
void notePdHeartbeat();
void noteBleSupervisorHeartbeat(bool healthy = true);
void systemHealthMonitorTask(void *pvParameters);
uint8_t memoryFragmentationPercent(size_t freeBytes, size_t largestBlockBytes);

// Non-blocking async operation helpers
template <typename StatusType>
bool waitForAsyncStatus(StatusType &status, StatusType targetStatus,
                        StatusType errorStatus, uint32_t timeoutMs = 15000);

template <typename StatusType>
bool waitForAsyncStatusArray(StatusType *statusArray, int index,
                             StatusType targetStatus, StatusType errorStatus,
                             uint32_t timeoutMs = 15000);

// Helper for boolean async operations
bool waitForAsyncBool(bool &boolVar, bool targetValue,
                      uint32_t timeoutMs = 15000);

class Utils {
public:
  static std::string GetDeviceName();
  static std::string BleGetDeviceName();
};
#endif
