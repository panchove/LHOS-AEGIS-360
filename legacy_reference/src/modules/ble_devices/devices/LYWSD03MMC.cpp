// Implementation for Xiaomi LYWSD03MMC ATC custom firmware decoder
#include <modules/ble_devices/BleSensorData.h>
#include <modules/ble_devices/devices/LYWSD03MMC.h>

static inline std::string lc_x(std::string s) {
  for (auto &c : s)
    c = (char)std::tolower((unsigned char)c);
  return s;
}
static inline bleSensorData parseKV_x(const std::string &e) {
  auto p = e.find(":");
  if (p == std::string::npos)
    return {"", ""};
  auto id = e.substr(0, p);
  auto v = e.substr(p + 1);
  if (id.empty())
    return {"", ""};
  return {lc_x(id), v};
}

std::string LYWSD03MMC_decoder::decode(const char *macAddress,
                                       const char *model, const char *rssi,
                                       const char *txPower,
                                       const char *manufData,
                                       const char *serviceData) {
  (void)manufData;
  // debugPrint("[LYWSD03MMC] Decoding BLE data for %s\n", macAddress);
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << (model ? model : "") << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << "," << "ble."
      << macAddress << ".txPower:" << (txPower ? txPower : "") << ",";
  // Find service 0x181a
  std::vector<std::string> services =
    splitString(serviceData ? std::string(serviceData) : std::string(), ",");
  std::vector<uint8_t> atcBytes;
  for (const auto &s : services) {
    if (s.empty())
      continue;
    auto kv = parseKV_x(s);
    if (kv.id != "181a")
      continue;
    auto b = hexToBytes(kv.hexData.c_str());
    if (b.size() >= 12) {
      atcBytes = b;
      break;
    }
  }
  if (atcBytes.size() >= 12) { // Data layout per dart code
    // bytes 0..5 = MAC reversed? We pass original mac separately, but also
    // output parsed
    std::ostringstream macParsed;
    macParsed << std::uppercase << std::hex << std::setfill('0');
    for (size_t i = 0; i < 6; i++) {
      if (i)
        macParsed << ":";
      macParsed << std::setw(2) << (int)atcBytes[i];
    }
    int16_t raw = (int16_t)((uint16_t)(atcBytes[6]) << 8) |
                  atcBytes[7]; // Big endian signed 16-bit integer
    float c = raw / 10.0f;
    int humidity = atcBytes[8];
    int batteryLevel = atcBytes[9];
    int batteryVoltage = atcBytes[11] | (atcBytes[10] << 8);

    float f = c * 9.0f / 5.0f + 32.0f;
    f = std::round(f * 100.0f) / 100.0f;
    out << "ble." << macAddress << ".mac.address:" << macParsed.str() << ",";
    out << "ble." << macAddress << ".temperature.celsius:" << c << ",";
    out << "ble." << macAddress << ".temperature.fahrenheit:" << f << ",";
    out << "ble." << macAddress << ".humidity.percent:" << humidity << ",";
    out << "ble." << macAddress << ".battery.level:" << batteryLevel << ",";
    out << "ble." << macAddress
        << ".battery.voltage:" << batteryVoltage / 1000.0f << ",";
  }
  std::string res = out.str();
  if (!res.empty() && res.back() == ',')
    res.pop_back();
  return res;
}
