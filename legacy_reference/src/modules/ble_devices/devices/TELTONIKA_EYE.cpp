// Implementation for Teltonika EYE Sensor decoder
#include <modules/ble_devices/devices/TELTONIKA_EYE.h>

static inline std::string lc_te(std::string s) {
  for (auto &c : s)
    c = (char)std::tolower((unsigned char)c);
  return s;
}
static inline bleSensorData parseKV_te(const std::string &e) {
  auto p = e.find(":");
  if (p == std::string::npos)
    return {"", ""};
  auto id = e.substr(0, p);
  auto v = e.substr(p + 1);
  if (id.empty())
    return {"", ""};
  return {lc_te(id), v};
}

std::string TELTONIKA_EYE_decoder::decode(const char *macAddress,
                                          const char *model, const char *rssi,
                                          const char *txPower,
                                          const char *manufData,
                                          const char *serviceData) {
  (void)serviceData;
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << (model ? model : "") << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << "," << "ble."
      << macAddress << ".txPower:" << (txPower ? txPower : "") << ",";
  std::vector<std::string> mEntries =
    splitString(manufData ? std::string(manufData) : std::string(), ",");
  for (const auto &m : mEntries) {
    if (m.empty())
      continue;
    auto kv = parseKV_te(m);
    if (kv.id == "089a") {
      auto b = hexToBytes(kv.hexData.c_str());
      if (b.size() < 2)
        continue;
      out << "ble." << macAddress
          << ".teltonika.protocol.version:" << (unsigned)b[0] << ",";
      uint8_t flagByte = b[1];
      std::bitset<8> flags(flagByte);
      size_t offset = 2; // flags[7] MSB ... flags[0] LSB; adjust per dart code
                         // using bits 7..0
      if (flags.test(7) && b.size() >= offset + 2) {
        int16_t t = (b[offset] << 8) | b[offset + 1];
        float c = t / 100.0f;
        float f = c * 9.0f / 5.0f + 32.0f;
        out << "ble." << macAddress << ".temperature.celsius:" << c << ",";
        out << "ble." << macAddress << ".temperature.fahrenheit:" << f << ",";
        offset += 2;
      }
      if (flags.test(6) && b.size() > offset) {
        out << "ble." << macAddress
            << ".humidity.percent:" << (unsigned)b[offset] << ",";
        offset += 1;
      }
      out << "ble." << macAddress
          << ".magnet.detection.enabled:" << (flags.test(5) ? 1 : 0) << ",";
      out << "ble." << macAddress << ".magnet.state:" << (flags.test(4) ? 1 : 0)
          << ",";
      if (flags.test(3) && b.size() >= offset + 2) {
        uint16_t mv = (b[offset] << 8) | b[offset + 1];
        std::bitset<16> cntBits(mv);
        bool movementStatus = cntBits.test(15);
        uint16_t movementCount = mv & 0x7FFF;
        out << "ble." << macAddress
            << ".movement.status:" << (movementStatus ? 1 : 0) << ",";
        out << "ble." << macAddress << ".movement.count:" << movementCount
            << ",";
        offset += 2;
      }
      if (flags.test(2) && b.size() >= offset + 3) {
        uint8_t pitchRaw = b[offset];
        int pitchSign = (pitchRaw & 0x80) ? -1 : 1;
        int pitchVal = pitchRaw & 0x7F;
        pitchVal = (pitchSign < 0) ? pitchVal - (0x7F / 2) : pitchVal;
        out << "ble." << macAddress << ".movement.angle.pitch:" << pitchVal
            << ",";
        int16_t rollRaw =
          (b[offset + 1] << 8) | b[offset + 2]; // treat as signed
        if (rollRaw & 0x8000)
          rollRaw -= 0x10000;
        out << "ble." << macAddress << ".movement.angle.roll:" << rollRaw
            << ",";
        offset += 3;
      }
      if (flags.test(1))
        out << "ble." << macAddress << ".battery.level.low:1,";
      if (flags.test(0) && b.size() > offset) {
        int mV = 2000 + (b[offset] * 10);
        out << "ble." << macAddress << ".battery.voltage:" << (mV / 1000.0f)
            << ",";
      }
    }
  }
  std::string res = out.str();
  if (!res.empty() && res.back() == ',')
    res.pop_back();
  return res;
}
