// Implementation for ELA Innovation Blue COIN Temperature
#include <modules/ble_devices/devices/ELA_COIN_T.h>

static inline std::string lc_coinT(std::string s) {
  for (auto &c : s)
    c = (char)std::tolower((unsigned char)c);
  return s;
}
static inline bleSensorData parseKV(const std::string &e) {
  auto p = e.find(":");
  if (p == std::string::npos)
    return {"", ""};
  auto id = e.substr(0, p);
  auto v = e.substr(p + 1);
  if (id.empty())
    return {"", ""};
  return {lc_coinT(id), v};
}
static inline float round2_coinT(float v) {
  return std::round(v * 100.0f) / 100.0f;
}

std::string ELA_COIN_T_decoder::decode(const char *macAddress,
                                       const char *model, const char *rssi,
                                       const char *txPower,
                                       const char *manufData,
                                       const char *serviceData) {
  (void)manufData;
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << (model ? model : "") << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << "," << "ble."
      << macAddress << ".txPower:" << (txPower ? txPower : "") << ",";
  std::vector<std::string> services =
    splitString(serviceData ? std::string(serviceData) : std::string(), ",");
  std::vector<uint8_t> tempBytes;
  std::vector<uint8_t> batteryBytes;
  for (const auto &s : services) {
    if (s.empty())
      continue;
    auto kv = parseKV(s);
    if (kv.id.empty())
      continue;
    auto b = hexToBytes(kv.hexData.c_str());
    if (b.empty())
      continue;
    if (kv.id == "2a6e")
      tempBytes = b;
  }
  if (tempBytes.size() == 2) {
    int16_t raw = static_cast<int16_t>(
      (static_cast<uint16_t>(tempBytes[1]) << 8) | tempBytes[0]);
    float c = raw / 100.0f;
    float f = c * 9.0f / 5.0f + 32.0f;
    out << "ble." << macAddress << ".temperature.celsius:" << c << ",";
    out << "ble." << macAddress << ".temperature.fahrenheit:" << round2_coinT(f)
        << ",";
  }

  // Parse manufacturer data to get battery information
  if (manufData && *manufData) {
    auto kv = parseKV(std::string(manufData));
    if (!kv.id.empty())
      batteryBytes = hexToBytes(kv.hexData.c_str());
  }
  if (!batteryBytes.empty() && batteryBytes.size() >= 2) {
    uint16_t raw = (batteryBytes[2] << 8) | batteryBytes[1];
    out << "ble." << macAddress << ".battery.volts:" << (float)raw / 1000.0f
        << ",";
  }

  std::string res = out.str();
  if (!res.empty() && res.back() == ',')
    res.pop_back();
  return res;
}
