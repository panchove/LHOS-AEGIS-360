// Implementation for ELA Innovation Blue PUCK ID decoder (supports minimal
// Eddystone / iBeacon fields)
#include <modules/ble_devices/BleSensorData.h>
#include <modules/ble_devices/devices/ELA_PUCK_ID.h>

static inline std::string toLowerId(std::string s) {
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
  return {toLowerId(id), v};
}
static std::string bytesToHex(const std::vector<uint8_t> &b) {
  std::ostringstream o;
  o << std::hex << std::setfill('0');
  for (auto v : b)
    o << std::setw(2) << (int)v;
  return o.str();
}

std::string ELA_PUCK_ID_decoder::decode(const char *macAddress,
                                        const char *model, const char *rssi,
                                        const char *txPower,
                                        const char *manufData,
                                        const char *serviceData) {
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << (model ? model : "") << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << ",";

  std::vector<std::string> services =
    splitString(serviceData ? std::string(serviceData) : std::string(), ",");
  std::vector<std::string> manufacturers =
    splitString(manufData ? std::string(manufData) : std::string(), ",");
  std::vector<uint8_t> ibeaconBytes;
  std::vector<uint8_t> eddystoneBytes;
  std::vector<uint8_t> batteryBytes;
  std::string eddystoneFrameType;

  for (const auto &s : services) {
    if (s.empty())
      continue;
    auto kv = parseKV(s);
    if (kv.id.empty())
      continue;
    auto b = hexToBytes(kv.hexData.c_str());
    if (b.empty())
      continue;
    if (kv.id == "feaa") {
      eddystoneBytes = b;
    }
  }

  for (const auto &s : manufacturers) {
    if (s.empty())
      continue;
    auto kv = parseKV(s);
    if (kv.id.empty())
      continue;
    auto b = hexToBytes(kv.hexData.c_str());
    if (b.empty())
      continue;
    if (kv.id == "004c") {
      ibeaconBytes = b;
    } else if (kv.id == "0757") {
      batteryBytes = b;
    }
  }

  // Prefer Eddystone if present
  if (!eddystoneBytes.empty()) {
    uint8_t frameType = eddystoneBytes[0];
    switch (frameType) {
    case 0x00: // UID
      eddystoneFrameType = "UID";
      if (eddystoneBytes.size() >= 18) {
        int8_t txp = (int8_t)eddystoneBytes[1];
        std::vector<uint8_t> ns(eddystoneBytes.begin() + 2,
                                eddystoneBytes.begin() + 12);
        std::vector<uint8_t> inst(eddystoneBytes.begin() + 12,
                                  eddystoneBytes.begin() + 18);
        out << "ble." << macAddress << ".eddystone.tx.power:" << (int)txp
            << ",";
        out << "ble." << macAddress << ".eddystone.namespace:" << bytesToHex(ns)
            << ",";
        out << "ble." << macAddress
            << ".eddystone.instance:" << bytesToHex(inst) << ",";
      }
      break;
    case 0x10:
      eddystoneFrameType = "URL";
      break;
    case 0x20:
      eddystoneFrameType = "TLM";
      break;
    case 0x30:
      eddystoneFrameType = "EID";
      break;
    default:
      eddystoneFrameType = "UNKNOWN";
      break;
    }
    out << "ble." << macAddress << ".ela.puck.id.mode:EDDYSTONE,";
    out << "ble." << macAddress
        << ".eddystone.frame.type:" << eddystoneFrameType << ",";
  }

  if (!ibeaconBytes.empty()) {
    // Expect: 0x02 0x15 then 16B UUID, 2B major, 2B minor, 1B power
    if (ibeaconBytes.size() >= 23 && ibeaconBytes[0] == 0x02 &&
        ibeaconBytes[1] == 0x15) {
      out << "ble." << macAddress << ".ela.puck.id.mode:IBEACON,";
      // Build UUID hex
      std::vector<uint8_t> uuidBytes(ibeaconBytes.begin() + 2,
                                     ibeaconBytes.begin() + 18);
      out << "ble." << macAddress << ".ibeacon.uuid:" << bytesToHex(uuidBytes)
          << ",";
      uint16_t major = (ibeaconBytes[18] << 8) | ibeaconBytes[19];
      uint16_t minor = (ibeaconBytes[20] << 8) | ibeaconBytes[21];
      int8_t rssi = (int8_t)ibeaconBytes[22];
      out << "ble." << macAddress << ".ibeacon.major:" << major << ",";
      out << "ble." << macAddress << ".ibeacon.minor:" << minor << ",";
      out << "ble." << macAddress << ".ibeacon.rssi.1meter:" << (int)rssi
          << ",";
    }
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
