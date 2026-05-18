#include <modules/utilities/UtilitiesLayrzHub.h>

namespace {
volatile uint32_t s_lastPdHeartbeatMs = 0;
volatile uint32_t s_lastBleSupervisorHeartbeatMs = 0;

static void logHealthAction(const char *reason, const char *action,
                            size_t freeInt, size_t maxInt, size_t minInt,
                            size_t freePs, size_t maxPs, size_t minPs,
                            uint32_t lastPdAgeMs, uint32_t lastBleAgeMs) {
  debugPrint("[SYS-HEALTH] %s reason=%s heapInt=%u maxInt=%u minInt=%u "
             "fragInt=%u%% heapPs=%u "
             "maxPs=%u minPs=%u fragPs=%u%% pdAge=%ums bleAge=%ums\n",
             action, reason, (unsigned)freeInt, (unsigned)maxInt,
             (unsigned)minInt,
             (unsigned)memoryFragmentationPercent(freeInt, maxInt),
             (unsigned)freePs, (unsigned)maxPs, (unsigned)minPs,
             (unsigned)memoryFragmentationPercent(freePs, maxPs),
             (unsigned)lastPdAgeMs, (unsigned)lastBleAgeMs);
}

static void handleHealthAction(const char *reason, size_t freeInt,
                               size_t maxInt, size_t minInt, size_t freePs,
                               size_t maxPs, size_t minPs, uint32_t lastPdAgeMs,
                               uint32_t lastBleAgeMs) {
#if SYS_HEALTH_AUTO_RESTART
  logHealthAction(reason, "Restart", freeInt, maxInt, minInt, freePs, maxPs,
                  minPs, lastPdAgeMs, lastBleAgeMs);
  vTaskDelay(pdMS_TO_TICKS(100));
  ESP.restart();
#else
  logHealthAction(reason, "Alert", freeInt, maxInt, minInt, freePs, maxPs,
                  minPs, lastPdAgeMs, lastBleAgeMs);
#endif
}
} // namespace

// Table generated for CRC-16/X.25 (poly=0x1021, refin=true, refout=true,
// init=0xFFFF, xorout=0xFFFF)
static const uint16_t crc16_x25_table[256] = {
  0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF, 0x8C48,
  0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7, 0x1081, 0x0108,
  0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E, 0x9CC9, 0x8D40, 0xBFDB,
  0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876, 0x2102, 0x308B, 0x0210, 0x1399,
  0x6726, 0x76AF, 0x4434, 0x55BD, 0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E,
  0xFAE7, 0xC87C, 0xD9F5, 0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E,
  0x54B5, 0x453C, 0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD,
  0xC974, 0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
  0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3, 0x5285,
  0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A, 0xDECD, 0xCF44,
  0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72, 0x6306, 0x728F, 0x4014,
  0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9, 0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5,
  0xA96A, 0xB8E3, 0x8A78, 0x9BF1, 0x7387, 0x620E, 0x5095, 0x411C, 0x35A3,
  0x242A, 0x16B1, 0x0738, 0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862,
  0x9AF9, 0x8B70, 0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E,
  0xF0B7, 0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
  0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036, 0x18C1,
  0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E, 0xA50A, 0xB483,
  0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5, 0x2942, 0x38CB, 0x0A50,
  0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD, 0xB58B, 0xA402, 0x9699, 0x8710,
  0xF3AF, 0xE226, 0xD0BD, 0xC134, 0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7,
  0x6E6E, 0x5CF5, 0x4D7C, 0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1,
  0xA33A, 0xB2B3, 0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72,
  0x3EFB, 0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
  0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A, 0xE70E,
  0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1, 0x6B46, 0x7ACF,
  0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9, 0xF78F, 0xE606, 0xD49D,
  0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330, 0x7BC7, 0x6A4E, 0x58D5, 0x495C,
  0x3DE3, 0x2C6A, 0x1EF1, 0x0F78};

uint16_t crc16_x25(const uint8_t *data, int len) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < len; i++) {
    uint8_t tbl_idx = (crc ^ data[i]) & 0xFF;
    crc = (crc >> 8) ^ crc16_x25_table[tbl_idx];
  }
  return ~crc;
}

std::string calculateCRC16(const char *data, int len) {
  if (len == 0) {
    len = strlen(data);
  }

  const uint8_t *p = reinterpret_cast<const uint8_t *>(data);

  uint16_t crc = crc16_x25(p, len);

  char hex[5];
  snprintf(hex, sizeof(hex), "%04X", crc);
  return std::string(hex);
}

rgbColor hexToRGB(const String &hexColor) {
  rgbColor color;

  // Check if the hex color starts with '#' and remove it
  String hex = hexColor;
  if (hex.startsWith("#")) {
    hex = hex.substring(1);
  }

  // Parse the hex string into the red, green, and blue components
  color.red = strtol(hex.substring(0, 2).c_str(), NULL, 16);
  color.green = strtol(hex.substring(2, 4).c_str(), NULL, 16);
  color.blue = strtol(hex.substring(4, 6).c_str(), NULL, 16);

  return color;
}

/**
 * Prints a debug message to the serial port.
 *
 * @param format The format of the message to be printed.
 * @param ...    The arguments to be printed.
 *
 * @return void
 *
 * @throws None
 */
bool debugConsoleEnabled() {
  return hubSettings.sys_debug_en && !hubSettings.zigbee_local_en;
}

Print *debugConsole() {
  return debugConsoleEnabled() ? static_cast<Print *>(&UARTSerial) : nullptr;
}

void debugPrint(const char *format, ...) {
  Print *console = debugConsole();
  if (!console)
    return;

  va_list args;
  va_start(args, format);
  char buffer[256];
  char timeBuffer[32];
  time_t timer = time(nullptr);
  struct tm *timeinfo = localtime(&timer);
  strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  console->printf("%s - %s", timeBuffer, buffer);
}

/**
 * Synchronizes the time with an NTP server.
 *
 * @return void
 *
 * @throws None
 */
void sincroTime(const char *netMode, const char *ntpserver1,
                const char *ntpserver2) {
  const char *ntpserver3 = "time.google.com";
  const time_t preSyncTime = time(nullptr);
  int sec = 0;
  float timezone = 0;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  tmElements_t tm;
  if (hubSettings.net_mode == "wifi") {
    configTime(0, 0, ntpserver1, ntpserver2, ntpserver3);
    long initTime = millis();
    while (time(nullptr) < SECS_YR_2000) { // wait until time is retrieved
      vTaskDelay(100 / portTICK_PERIOD_MS);
      if (millis() - initTime > 5000) {
        debugPrint("Time setup Time out\n");
        break;
      }
    }
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      debugPrint("Failed to obtain time\n");
      isNtpSynced = false;
      isValidTime = (time(nullptr) >= SECS_YR_2000);
      debugPrint("NTP sync result: fail (time_valid=%s)\n",
                 isValidTime ? "true" : "false");
      return;
    }
    year = timeinfo.tm_year + 1900;
    month = timeinfo.tm_mon + 1;
    day = timeinfo.tm_mday;
    hour = timeinfo.tm_hour;
    minute = timeinfo.tm_min;
    sec = timeinfo.tm_sec;
    debugPrint("WiFi Date_time: %d-%d-%d %d:%d:%d\n", year, month, day, hour,
               minute, sec);
    debugPrint("WiFi timestamp: %d\n", time(nullptr));
    isNtpSynced = true;
    isValidTime = (time(nullptr) >= SECS_YR_2000);
    debugPrint("NTP sync result: ok (time_valid=%s, delta=%ld)\n",
               isValidTime ? "true" : "false",
               (long)(time(nullptr) - preSyncTime));
    return;
  } else {
#ifdef USE_ASYNC_A7670
    // Try NTP sync with fallback servers
    if (!modemAsync.ntpServerSync(ntpserver1, 0)) {
      debugPrint("Failed to sync to NTP server %s, trying fallback servers\n",
                 ntpserver1);
      if (!modemAsync.ntpServerSync(ntpserver2, 0)) {
        debugPrint("Failed to sync to NTP server %s, trying fallback server\n",
                   ntpserver2);
        if (!modemAsync.ntpServerSync(ntpserver3, 0)) {
          debugPrint("Failed to sync to NTP servers\n");
          isNtpSynced = false;
          isValidTime = (time(nullptr) >= SECS_YR_2000);
          debugPrint("NTP sync result: fail (time_valid=%s)\n",
                     isValidTime ? "true" : "false");
          return;
        }
      }
    }
    debugPrint("NTP sync successful\n");

    int trial = 0;
    while (trial < 3) {
      if (modemAsync.getNetworkTime(&year, &month, &day, &hour, &minute, &sec,
                                    &timezone)) {
        tm.Year = year - 1970;
        tm.Month = month;
        tm.Day = day;
        tm.Hour = hour;
        tm.Minute = minute;
        tm.Second = sec;
        timestamp = makeTime(tm);
        struct timeval tv;
        tv.tv_sec = timestamp;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);
        long timeInitTime = millis();
        while (time(nullptr) < SECS_YR_2000) { // wait until time is retrieved
          vTaskDelay(100 / portTICK_PERIOD_MS);
          if (millis() - timeInitTime > 5000) {
            debugPrint("Time setup Time out\n");
            break;
          }
        }
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
          debugPrint("Failed to obtain time\n");
          isNtpSynced = false;
          isValidTime = (time(nullptr) >= SECS_YR_2000);
          debugPrint("NTP sync result: fail (time_valid=%s)\n",
                     isValidTime ? "true" : "false");
          return;
        }
        debugPrint("GSM Date_time: %d-%d-%d %d:%d:%d\n", year, month, day, hour,
                   minute, sec);
        debugPrint("GSM timestamp: %d\n", time(nullptr));
        isNtpSynced = true;
        isValidTime = (time(nullptr) >= SECS_YR_2000);
        debugPrint("NTP sync result: ok (time_valid=%s, delta=%ld)\n",
                   isValidTime ? "true" : "false",
                   (long)(time(nullptr) - preSyncTime));
        break;
      } else {
        debugPrint("Failed to get GSM date and time. Trial %d\n", trial + 1);
        isNtpSynced = false;
      }
      delay(1000);
      trial++;
    }
#else
    if (modem.NTPServerSync(String(ntpserver1), 0) != 0) {
      if (modem.NTPServerSync(String(ntpserver2), 0) != 0) {
        if (modem.NTPServerSync(String(ntpserver3), 0) != 0) {
          debugPrint("Failed to get time from NTP server\n");
        }
      }
    }
    int trial = 0;
    while (trial < 3) {
      if (modem.getNetworkTime(&year, &month, &day, &hour, &minute, &sec,
                               &timezone)) {
        tm.Year = year - 1970;
        tm.Month = month;
        tm.Day = day;
        tm.Hour = hour;
        tm.Minute = minute;
        tm.Second = sec;
        timestamp = makeTime(tm);
        struct timeval tv;
        tv.tv_sec = timestamp;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);
        long timeInitTime = millis();
        while (time(nullptr) < SECS_YR_2000) { // wait until time is retrieved
          vTaskDelay(100 / portTICK_PERIOD_MS);
          if (millis() - timeInitTime > 5000) {
            debugPrint("Time setup Time out\n");
            break;
          }
        }
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
          debugPrint("Failed to obtain time\n");
          isNtpSynced = false;
          isValidTime = (time(nullptr) >= SECS_YR_2000);
          debugPrint("NTP sync result: fail (time_valid=%s)\n",
                     isValidTime ? "true" : "false");
          return;
        }
        debugPrint("GSM Date_time: %d-%d-%d %d:%d:%d\n", year, month, day, hour,
                   minute, sec);
        debugPrint("GSM timestamp: %d\n", time(nullptr));
        isNtpSynced = true;
        isValidTime = (time(nullptr) >= SECS_YR_2000);
        debugPrint("NTP sync result: ok (time_valid=%s, delta=%ld)\n",
                   isValidTime ? "true" : "false",
                   (long)(time(nullptr) - preSyncTime));
        break;
      } else {
        debugPrint("Failed to get GSM date and time\n");
        isNtpSynced = false;
      }
      delay(1000);
      trial++;
    }
#endif
  }
  if (!isNtpSynced) {
    isValidTime = (time(nullptr) >= SECS_YR_2000);
    debugPrint("NTP sync result: fail (time_valid=%s)\n",
               isValidTime ? "true" : "false");
  }
  return;
}

/**
 * Converts a little-endian byte array to a decimal value.
 *
 * @param array Pointer to the byte array to be converted.
 * @param size  The size of the byte array.
 *
 * @return The decimal value converted from the byte array.
 *
 * @throws None.
 */
uint32_t littleEndianToDecimal(uint8_t *array, size_t size) {
  uint32_t result = 0;

  for (size_t i = 0; i < size; i++)
    result |= (uint32_t)array[i] << (8 * i);

  return result;
}
/// @brief Creates a unique name for the device.
/// @param void
/// @return void

std::string getDeviceName() {
  uint8_t baseMac[6];
  char deviceName[32]; // Allocate a buffer large enough for the device name

  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_BT);

// Write unique name into deviceName
#if defined(LAYRZ_HUB2_BUILD)
  sprintf(deviceName, "Layrz.HUB2.%02X%02X", baseMac[4], baseMac[5]);
#elif defined(LAYRZ_HUB1_BUILD)
  sprintf(deviceName, "Layrz.HUB.%02X%02X", baseMac[4], baseMac[5]);
#elif defined(LAYRZ_HUB25_BUILD)
  sprintf(deviceName, "Layrz.HUB25.%02X%02X", baseMac[4], baseMac[5]);
#endif
  return std::string(deviceName);
}

std::string getESP32SerialNumber() {
  uint64_t chipid =
    ESP.getEfuseMac(); // The chip ID is essentially the MAC address
  uint32_t high = (chipid >> 32) & 0xFFFFFFFF;
  uint32_t low = chipid & 0xFFFFFFFF;

  char serialNumber[17];
  sprintf(serialNumber, "%08X%08X", high, low);

  return std::string(serialNumber);
}

std::string getBluetoothMACAddress() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT); // Read the Bluetooth MAC address

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);

  return std::string(macStr);
}

std::string floatToString(float value, int precision) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.*f", precision, value);

  return std::string(buffer);
}

/// @brief Helper function to split a string into a vector of strings.
/// @param str String to be split.
/// @param delimiter Delimiter to split the string by.
/// @return a vector of strings.
std::vector<std::string> splitString(const std::string &str,
                                     const std::string &delimiter) {
  std::vector<std::string> tokens;
  size_t start = 0, end = 0;

  while ((end = str.find(delimiter, start)) != std::string::npos) {
    tokens.push_back(str.substr(start, end - start));
    start = end + delimiter.length();
  }

  tokens.push_back(str.substr(start));
  return tokens;
}

// ✅ Function to allocate std::vector<std::string> in PSRAM
std::unique_ptr<std::vector<std::string>> allocateVectorPSRAM() {
  // Keep ownership standard so destruction always releases memory correctly.
  return std::make_unique<std::vector<std::string>>();
}

// ✅ Updated splitStringRegex() that stores results in PSRAM
std::unique_ptr<std::vector<std::string>>
splitStringRegex(const std::unique_ptr<std::string> &strPtr) {
  if (!strPtr || strPtr->empty()) {
    debugPrint("⚠️ Empty or null input string\n");
    return nullptr;
  }

  std::unique_ptr<std::vector<std::string>> tokens = allocateVectorPSRAM();
  if (!tokens)
    return nullptr; // Return if PSRAM allocation fails

  std::regex re(R"(<[A-Z][a-z]>.*?</[A-Z][a-z]>)"); // ✅ Matches `<Xy>...</Xy>`
  std::sregex_iterator it(strPtr->begin(), strPtr->end(), re);
  std::sregex_iterator end;

  while (it != end) {
    tokens->push_back(it->str()); // ✅ Store result in PSRAM vector
    ++it;
  }

  return tokens;
}

std::unique_ptr<std::vector<std::string>>
splitStringManual(const std::unique_ptr<std::string> &strPtr) {
  if (!strPtr || strPtr->empty()) {
    debugPrint("⚠️ Empty or null input string\n");
    return nullptr;
  }

  // No need for cleanup since leading newline is now removed at TCP reception
  // level
  const std::string &cleanedString = *strPtr;

  std::unique_ptr<std::vector<std::string>> tokens =
    std::make_unique<std::vector<std::string>>();
  tokens->reserve(8);

  // ✅ Manual parsing without recursion
  size_t start = 0;
  int elementCount = 0;
  while ((start = cleanedString.find('<', start)) != std::string::npos) {
    size_t end = cleanedString.find('>', start);
    if (end == std::string::npos) {
      break; // Malformed tag
    }

    size_t closeTag = cleanedString.find("</", end);
    if (closeTag == std::string::npos) {
      break; // No closing tag
    }

    size_t closeEnd = cleanedString.find('>', closeTag);
    if (closeEnd == std::string::npos) {
      break;
    }

    // Extract the complete element
    std::string element = cleanedString.substr(start, closeEnd - start + 1);

    tokens->push_back(element);
    start = closeEnd + 1;
  }

  return tokens;
}

void checkMemory() {
  // Memory monitoring disabled for production
  // debugPrint("🔹 Total Free Heap: %d bytes\n", esp_get_free_heap_size());
  // debugPrint("🔹 Internal Free Heap: %d bytes\n",
  // heap_caps_get_free_size(MALLOC_CAP_INTERNAL)); debugPrint("🔹 PSRAM Free
  // Heap: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void notePdHeartbeat() { s_lastPdHeartbeatMs = millis(); }

void noteBleSupervisorHeartbeat(bool healthy) {
  if (healthy) {
    s_lastBleSupervisorHeartbeatMs = millis();
  }
}

uint8_t memoryFragmentationPercent(size_t freeBytes, size_t largestBlockBytes) {
  if (freeBytes == 0 || largestBlockBytes >= freeBytes) {
    return 0;
  }
  const size_t fragmentedBytes = freeBytes - largestBlockBytes;
  return (uint8_t)((fragmentedBytes * 100U) / freeBytes);
}

void systemHealthMonitorTask(void *pvParameters) {
  (void)pvParameters;
  esp_task_wdt_add(NULL);

  const uint32_t bootMs = millis();
  if (s_lastPdHeartbeatMs == 0)
    s_lastPdHeartbeatMs = bootMs;
  if (s_lastBleSupervisorHeartbeatMs == 0)
    s_lastBleSupervisorHeartbeatMs = bootMs;

  uint32_t lowMemSamples = 0;
  uint32_t firstLowMemMs = 0;
  uint32_t pdStallSamples = 0;
  uint32_t bleStallSamples = 0;
  uint32_t lastDiagMs = 0;

  for (;;) {
    const uint32_t now = millis();
    const size_t freeInt = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const size_t maxInt = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    const size_t minInt = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    const size_t freePs = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const size_t maxPs = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    const size_t minPs = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
    const uint8_t fragInt = memoryFragmentationPercent(freeInt, maxInt);
    const uint32_t lastPdAgeMs = now - s_lastPdHeartbeatMs;
    const uint32_t lastBleAgeMs = now - s_lastBleSupervisorHeartbeatMs;

    if ((now - lastDiagMs) >= SYS_HEALTH_MONITOR_PERIOD_MS &&
        hubSettings.sys_debug_en) {
      lastDiagMs = now;
      debugPrint("[SYS-HEALTH] heapInt=%u maxInt=%u minInt=%u fragInt=%u%% "
                 "heapPs=%u maxPs=%u "
                 "minPs=%u fragPs=%u%% pdAge=%ums bleAge=%ums socketAuth=%d "
                 "wifi=%d bleReady=%d\n",
                 (unsigned)freeInt, (unsigned)maxInt, (unsigned)minInt,
                 (unsigned)fragInt, (unsigned)freePs, (unsigned)maxPs,
                 (unsigned)minPs,
                 (unsigned)memoryFragmentationPercent(freePs, maxPs),
                 (unsigned)lastPdAgeMs, (unsigned)lastBleAgeMs,
                 (int)isSocketAuth, (int)WiFi.status(), (int)gBleStackReady);
    }

    if (now >= MEM_CB_BOOT_GRACE_MS) {
      const bool lowInt = freeInt <= MEM_CB_HEAP_INT_LOW_BYTES;
      const bool lowMaxInt = maxInt <= MEM_CB_MAX_INT_LOW_BYTES;
      const bool critInt = freeInt <= MEM_CB_HEAP_INT_CRIT_BYTES;
      const bool critMaxInt = maxInt <= MEM_CB_MAX_INT_CRIT_BYTES;
      const bool highFragInt = fragInt >= MEM_CB_FRAG_INT_LOW_PCT;
      const bool lowPs = freePs <= MEM_CB_HEAP_PS_LOW_BYTES;
      const bool criticalMem = critInt || critMaxInt ||
                               (lowInt && (lowMaxInt || highFragInt)) || lowPs;

      if (!criticalMem) {
        lowMemSamples = 0;
        firstLowMemMs = 0;
      } else {
        if (lowMemSamples == 0) {
          firstLowMemMs = now;
        }
        lowMemSamples++;
        if (lowMemSamples >= MEM_CB_CONSECUTIVE_SAMPLES &&
            (now - firstLowMemMs) >= MEM_CB_MIN_HOLD_MS) {
          handleHealthAction("low-memory", freeInt, maxInt, minInt, freePs,
                             maxPs, minPs, lastPdAgeMs, lastBleAgeMs);
          lowMemSamples = 0;
          firstLowMemMs = 0;
        }
      }

      if (hubSettings.net_en && hubSettings.data_pub_per > 0 && isValidTime) {
        uint32_t pdDeadlineMs = (uint32_t)hubSettings.data_pub_per * 1000UL *
                                SYS_HEALTH_PD_STALL_FACTOR;
        if (pdDeadlineMs < SYS_HEALTH_PD_STALL_MIN_MS) {
          pdDeadlineMs = SYS_HEALTH_PD_STALL_MIN_MS;
        }

        if (lastPdAgeMs > pdDeadlineMs) {
          pdStallSamples++;
          if (pdStallSamples >= SYS_HEALTH_STALL_SAMPLES) {
            handleHealthAction("pd-stall", freeInt, maxInt, minInt, freePs,
                               maxPs, minPs, lastPdAgeMs, lastBleAgeMs);
            pdStallSamples = 0;
          }
        } else {
          pdStallSamples = 0;
        }
      } else {
        pdStallSamples = 0;
      }

      if (!bleConfigConnected && gBleStackReady) {
        if (lastBleAgeMs > SYS_HEALTH_BLE_STALL_MS) {
          bleStallSamples++;
          if (bleStallSamples >= SYS_HEALTH_STALL_SAMPLES) {
            handleHealthAction("ble-supervisor-stall", freeInt, maxInt, minInt,
                               freePs, maxPs, minPs, lastPdAgeMs, lastBleAgeMs);
            bleStallSamples = 0;
          }
        } else {
          bleStallSamples = 0;
        }
      } else {
        bleStallSamples = 0;
        if (bleConfigConnected) {
          s_lastBleSupervisorHeartbeatMs = now;
        }
      }
    }

    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(SYS_HEALTH_MONITOR_PERIOD_MS));
  }
}

void displayTaskStackUsage() {
  // Task stack monitoring disabled for production
  /*
  if (updateSensorsHandle)
  debugPrint("Task: updateSensors | Free Stack: %d words\n",
  uxTaskGetStackHighWaterMark(updateSensorsHandle)); if (sendSensorDataHandle)
  debugPrint("Task: sendSensorData | Free Stack: %d words\n",
  uxTaskGetStackHighWaterMark(sendSensorDataHandle)); if
  (checkWifiNetworkHandle) debugPrint("Task: checkWifiNetwork | Free Stack: %d
  words\n", uxTaskGetStackHighWaterMark(checkWifiNetworkHandle)); if
  (checkGSMNetworkHandle) debugPrint("Task: checkGSMNetwork | Free Stack: %d
  words\n", uxTaskGetStackHighWaterMark(checkGSMNetworkHandle)); if
  (checkFirmwareHandle) debugPrint("Task: checkFirmware | Free Stack: %d
  words\n", uxTaskGetStackHighWaterMark(checkFirmwareHandle)); if
  (getCommandsHandle) debugPrint("Task: getCommands | Free Stack: %d words\n",
  uxTaskGetStackHighWaterMark(getCommandsHandle)); if (bleScanHandle)
  debugPrint("Task: bleScan | Free Stack: %d words\n",
  uxTaskGetStackHighWaterMark(bleScanHandle)); if (synchroNTPHandle)
  debugPrint("Task: synchroNTP | Free Stack: %d words\n",
  uxTaskGetStackHighWaterMark(synchroNTPHandle)); if (serialMonitorHandle)
  debugPrint("Task: serialMonitor | Free Stack: %d words\n",
  uxTaskGetStackHighWaterMark(serialMonitorHandle)); if
  (tcpSocketReceptionHandle) debugPrint("Task: tcpSocketReception | Free Stack:
  %d words\n", uxTaskGetStackHighWaterMark(tcpSocketReceptionHandle)); if
  (confiotOverBleHandle) debugPrint("Task: confiotOverBle | Free Stack: %d
  words\n", uxTaskGetStackHighWaterMark(confiotOverBleHandle));
  */
}

std::string Utils::GetDeviceName() {
  std::string deviceName = hubSettings.sys_dev_name;
  if (deviceName.length() > 20) {
    deviceName = deviceName.substr(0, 20);
  }

  return deviceName;
}

std::string Utils::BleGetDeviceName() {
  // Cast name using this spec:
  // If LAYRZ_HUB2_BUILD is defined, the device name will be "Layrz.HUB2.XXYY"
  // If LAYRZ_HUB25_BUILD is defined, the device name will be "Layrz.HUB25.XXYY"
  // If LAYRZ_HUB1_BUILD is not defined, the device name will be
  // "Layrz.HUB.XXYY"

  char deviceName[32]; // Allocate a buffer large enough for the device name
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_BT); // Read the Bluetooth MAC address
#if defined(LAYRZ_HUB2_BUILD)
  sprintf(deviceName, "Layrz.HUB2.%02X%02X", baseMac[4], baseMac[5]);
#elif defined(LAYRZ_HUB25_BUILD)
  sprintf(deviceName, "Layrz.HUB25.%02X%02X", baseMac[4], baseMac[5]);
#elif defined(LAYRZ_HUB1_BUILD)
  sprintf(deviceName, "Layrz.HUB.%02X%02X", baseMac[4], baseMac[5]);
#endif

  return std::string(deviceName);
}

static inline int hexNibble(unsigned char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return -1;
}

// Returns {} on invalid (non-hex, non-whitespace) characters.
// Odd number of hex digits is allowed: leading nibble becomes 0x0n.
std::vector<uint8_t> hexToBytes(const char *hex) {
  std::vector<uint8_t> out;
  if (!hex)
    return out;

  // First pass: count valid hex nibbles and validate characters
  size_t nibbleCount = 0;
  for (const unsigned char *p = reinterpret_cast<const unsigned char *>(hex);
       *p; ++p) {
    if (std::isspace(*p))
      continue;
    int v = hexNibble(*p);
    if (v < 0) { // invalid non-hex, non-space
      out.clear();
      return out;
    }
    ++nibbleCount;
  }
  if (nibbleCount == 0)
    return out;
  out.reserve((nibbleCount + 1) / 2);

  // Second pass: build bytes
  int carry = -1; // stores the high nibble until we get a pair
  for (const unsigned char *p = reinterpret_cast<const unsigned char *>(hex);
       *p; ++p) {
    if (std::isspace(*p))
      continue;
    int v = hexNibble(*p); // guaranteed >= 0 from first pass
    if (carry < 0) {
      carry = v; // hold as high nibble for now
    } else {
      out.push_back(static_cast<uint8_t>((carry << 4) | v));
      carry = -1;
    }
  }
  if (carry >= 0) {
    // Odd count: treat as 0x0n (carry is the lone nibble)
    out.push_back(static_cast<uint8_t>(carry));
  }
  return out;
}

/**
 * Non-blocking wait for async status change - generic template for single
 * status
 */
template <typename StatusType>
bool waitForAsyncStatus(StatusType &status, StatusType targetStatus,
                        StatusType errorStatus, uint32_t timeoutMs) {
  long initTime = millis();
  while (status != targetStatus && (millis() - initTime) < timeoutMs) {
    if (status == errorStatus) {
      return false; // Error occurred
    }
    esp_task_wdt_reset();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  return (status == targetStatus);
}

/**
 * Non-blocking wait for async status change - template for status arrays (like
 * TCP status)
 */
template <typename StatusType>
bool waitForAsyncStatusArray(StatusType *statusArray, int index,
                             StatusType targetStatus, StatusType errorStatus,
                             uint32_t timeoutMs) {
  long initTime = millis();
  while (statusArray[index] != targetStatus &&
         (millis() - initTime) < timeoutMs) {
    if (statusArray[index] == errorStatus) {
      return false; // Error occurred
    }
    esp_task_wdt_reset();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  return (statusArray[index] == targetStatus);
}

// Explicit template instantiations for the status types we use
template bool waitForAsyncStatus<GPRSStatus>(GPRSStatus &status,
                                             GPRSStatus targetStatus,
                                             GPRSStatus errorStatus,
                                             uint32_t timeoutMs);
template bool waitForAsyncStatus<NTPStatus>(NTPStatus &status,
                                            NTPStatus targetStatus,
                                            NTPStatus errorStatus,
                                            uint32_t timeoutMs);
template bool waitForAsyncStatusArray<TCPStatus>(TCPStatus *statusArray,
                                                 int index,
                                                 TCPStatus targetStatus,
                                                 TCPStatus errorStatus,
                                                 uint32_t timeoutMs);

/**
 * Non-blocking wait for boolean variable to reach target value
 */
bool waitForAsyncBool(bool &boolVar, bool targetValue, uint32_t timeoutMs) {
  long initTime = millis();
  while (boolVar != targetValue && (millis() - initTime) < timeoutMs) {
    esp_task_wdt_reset();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  return (boolVar == targetValue);
}
