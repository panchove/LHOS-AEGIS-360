// Implementation for generic BLE decoder
#include <modules/ble_devices/devices/GENERIC.h>

static inline std::string lc_gen(std::string s) {
  for (auto &c : s)
    c = (char)std::tolower((unsigned char)c);
  return s;
}
static inline bleSensorData parseKV_gen(const std::string &e) {
  auto p = e.find(":");
  if (p == std::string::npos)
    return {"", ""};
  auto id = e.substr(0, p);
  auto v = e.substr(p + 1);
  if (id.empty())
    return {"", ""};
  return {lc_gen(id), v};
}
static inline std::string up4(std::string id) {
  std::string h = id;
  std::transform(h.begin(), h.end(), h.begin(), ::toupper);
  if (h.size() < 4)
    h = std::string(4 - h.size(), '0') + h;
  return h;
}

std::string GENERIC_decoder::decode(const char *macAddress, const char *model,
                                    const char *rssi, const char *txPower,
                                    const char *manufData,
                                    const char *serviceData) {
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << (model ? model : "") << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << "," << "ble."
      << macAddress << ".txPower:" << (txPower ? txPower : "") << ",";
  // Services
  std::vector<std::string> services =
    splitString(serviceData ? std::string(serviceData) : std::string(), ",");
  for (const auto &s : services) {
    if (s.empty())
      continue;
    auto kv = parseKV_gen(s);
    if (kv.id.empty())
      continue;
    out << "ble." << macAddress << ".service." << up4(kv.id) << ":"
        << kv.hexData << ",";
  }
  // Manufacturer
  std::vector<std::string> mEntries =
    splitString(manufData ? std::string(manufData) : std::string(), ",");
  for (const auto &m : mEntries) {
    if (m.empty())
      continue;
    auto kv = parseKV_gen(m);
    if (kv.id.empty())
      continue;
    out << "ble." << macAddress << ".manufacturer.data." << up4(kv.id) << ":"
        << kv.hexData << ",";
  }
  std::string res = out.str();
  if (!res.empty() && res.back() == ',')
    res.pop_back();
  return res;
}
