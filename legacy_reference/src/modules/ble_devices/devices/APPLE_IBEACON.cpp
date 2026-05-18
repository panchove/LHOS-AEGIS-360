// Implementation for Apple iBeacon decoder (expects manufacturer entry 004c)
#include <modules/ble_devices/devices/APPLE_IBEACON.h>

static inline std::string lc_ib(std::string s) {
  for (auto &c : s)
    c = (char)std::tolower((unsigned char)c);
  return s;
}
static inline bleSensorData parseKV_ib(const std::string &e) {
  auto p = e.find(":");
  if (p == std::string::npos)
    return {"", ""};
  auto id = e.substr(0, p);
  auto v = e.substr(p + 1);
  if (id.empty())
    return {"", ""};
  return {lc_ib(id), v};
}
static std::string bytesHex_ib(const std::vector<uint8_t> &b) {
  std::ostringstream o;
  o << std::hex << std::setfill('0');
  for (auto v : b)
    o << std::setw(2) << (int)v;
  return o.str();
}

std::string APPLE_IBEACON_decoder::decode(const char *macAddress,
                                          const char *model, const char *rssi,
                                          const char *txPower,
                                          const char *manufData,
                                          const char *serviceData) {
  (void)serviceData;
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << (model ? model : "") << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << "," << "ble."
      << macAddress << ".txPower:" << (txPower ? txPower : "") << ",";
  // manufacturer data may be comma-separated id:hex entries
  std::vector<std::string> mEntries =
    splitString(manufData ? std::string(manufData) : std::string(), ",");
  for (const auto &m : mEntries) {
    if (m.empty())
      continue;
    auto kv = parseKV_ib(m);
    if (kv.id == "004c") {
      auto b = hexToBytes(kv.hexData.c_str());
      if (b.size() == 23) {
        uint8_t type = b[0];
        out << "ble." << macAddress << ".apple.ibeacon.type:" << (unsigned)type
            << ",";
        std::vector<uint8_t> uuid(b.begin() + 2, b.begin() + 18);
        std::string uuidHex = bytesHex_ib(uuid);
        std::transform(uuidHex.begin(), uuidHex.end(), uuidHex.begin(),
                       ::toupper); // format 8-4-4-4-12
        if (uuidHex.size() == 32) {
          out << "ble." << macAddress
              << ".apple.ibeacon.uuid:" << uuidHex.substr(0, 8) << "-"
              << uuidHex.substr(8, 4) << "-" << uuidHex.substr(12, 4) << "-"
              << uuidHex.substr(16, 4) << "-" << uuidHex.substr(20) << ",";
        }
        if (b.size() >= 22) {
          uint16_t major = (b[18] << 8) | b[19];
          uint16_t minor = (b[20] << 8) | b[21];
          out << "ble." << macAddress << ".apple.ibeacon.major:" << major
              << ",";
          out << "ble." << macAddress << ".apple.ibeacon.minor:" << minor
              << ",";
        }
        if (b.size() >= 23) {
          int8_t p = (int8_t)b[22];
          out << "ble." << macAddress << ".apple.ibeacon.tx.power:" << (int)p
              << ",";
        }
      }
    }
  }
  std::string res = out.str();
  if (!res.empty() && res.back() == ',')
    res.pop_back();
  return res;
}
