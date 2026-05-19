// Implementation for ELA Innovation Blue PUCK Magnet
#include <modules/ble_devices/BleSensorData.h>
#include <modules/ble_devices/devices/ELA_PUCK_MAG.h>

static inline std::string toLowerMag(std::string s) {
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
  return {toLowerMag(id), v};
}

std::string ELA_PUCK_MAG_decoder::decode(const char *macAddress,
                                         const char *model, const char *rssi,
                                         const char *txPower,
                                         const char *manufData,
                                         const char *serviceData) {
  (void)manufData;
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << (model ? model : "") << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << ","
      << "ble." << macAddress << ".txPower:" << (txPower ? txPower : "") << ",";
  std::vector<std::string> services =
    splitString(serviceData ? std::string(serviceData) : std::string(), ",");
  std::vector<uint8_t> magBytes;
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
    if (kv.id == "2a06")
      magBytes = b;
  }
  if (magBytes.size() == 2) {
    uint16_t raw = (magBytes[1] << 8) | magBytes[0];
    bool status = (raw & 0x0001) == 0x0001;
    uint16_t count = (raw >> 1) & 0x7FFF;
    out << "ble." << macAddress << ".magnet.status:" << (status ? 1 : 0) << ",";
    out << "ble." << macAddress << ".magnet.count:" << count << ",";
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
