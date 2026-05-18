#include <modules/ble_devices/devices/ELA_PUCK_RHT.h>

static inline float roundTo(float v, int decimals) {
  float p = 1.0f;
  for (int i = 0; i < decimals; ++i)
    p *= 10.0f;
  return std::round(v * p) / p;
}

static inline std::string toLower(std::string s) {
  for (auto &ch : s)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return s;
}

// Parse "<id>:<hex>" and normalize id to lowercase so comparisons are
// case-insensitive
static inline bleSensorData parseDataKV(const std::string &entry) {
  size_t pos = entry.find(":");
  if (pos == std::string::npos)
    return {"", ""};
  std::string id = entry.substr(0, pos);
  std::string value = entry.substr(pos + 1);
  if (id.empty())
    return {"", ""};
  return {toLower(id), value};
}

std::string ELA_PUCK_RHT_decoder::decode(const char *macAddress,
                                         const char *model, const char *rssi,
                                         const char *txPower,
                                         const char *manufData,
                                         const char *serviceData) {
  (void)manufData; // Battery handled from standard service if present; specific
                   // ELA MFG format handled elsewhere

  // debugPrint("[ELA_PUCK_RHT] Decoding BLE data for %s\n", macAddress);
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << model << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << ","
      << "ble." << macAddress << ".txPower:" << (txPower ? txPower : "") << ",";

  // Iterate service data entries: "<id>:<hex>,<id>:<hex>,..."
  std::vector<std::string> services =
    splitString(serviceData ? std::string(serviceData) : std::string(), ",");
  for (const auto &svcStr : services) {
    if (svcStr.empty())
      continue;
    bleSensorData svc = parseDataKV(svcStr);
    if (svc.id.empty())
      continue;
    std::vector<uint8_t> b = hexToBytes(svc.hexData.c_str());
    if (b.empty())
      continue; // invalid/empty hex payload

    // 0x2A6E (Temperature) — little-endian, hundredths of °C
    if (svc.id == "2a6e") {
      if (b.size() >= 2) {
        // little-endian: low, high
        int16_t raw =
          static_cast<int16_t>((static_cast<uint16_t>(b[1]) << 8) | b[0]);
        float c = static_cast<float>(raw) / 100.0f;
        float f = c * 9.0f / 5.0f + 32.0f;
        out << "ble." << macAddress << ".temperature.celsius:" << c << ",";
        out << "ble." << macAddress
            << ".temperature.fahrenheit:" << roundTo(f, 2) << ",";
      }
    }
    // 0x2A6F (Humidity) — first byte: percent RH
    else if (svc.id == "2a6f") {
      // Some devices might send 1B or 2B. Dart only uses the first byte.
      uint8_t rh = b[0];
      out << "ble." << macAddress
          << ".humidity.percent:" << static_cast<unsigned int>(rh) << ",";
    }
  }

  // Parse manufacturer data to get battery information
  std::vector<uint8_t> batteryBytes;
  if (manufData && *manufData) {
    auto kv = parseDataKV(std::string(manufData));
    if (!kv.id.empty())
      batteryBytes = hexToBytes(kv.hexData.c_str());
  }
  if (!batteryBytes.empty() && batteryBytes.size() >= 2) {
    uint16_t raw = (batteryBytes[2] << 8) | batteryBytes[1];
    out << "ble." << macAddress << ".battery.volts:" << (float)raw / 1000.0f
        << ",";
  }

  // delete the last comma from out
  std::string result = out.str();
  if (!result.empty()) {
    result.erase(result.length() - 1); // Remove the last comma
  }
  return result;
}
