// Implementation for 3scort ESCORT DU decoder
#include <cstdlib>
#include <modules/ble_devices/BleSensorData.h>
#include <modules/ble_devices/devices/ESCORT_DU.h>
#include <sstream>
#include <vector>

static inline std::string lc_du(std::string s) {
  for (auto &c : s)
    c = (char)std::tolower((unsigned char)c);
  return s;
}
static inline bleSensorData parseKV_du(const std::string &e) {
  auto p = e.find(":");
  if (p == std::string::npos)
    return {"", ""};
  auto id = e.substr(0, p);
  auto v = e.substr(p + 1);
  if (id.empty())
    return {"", ""};
  return {lc_du(id), v};
}

std::string ESCORT_DU_decoder::decode(const char *macAddress, const char *model,
                                      const char *rssi, const char *txPower,
                                      const char *manufData,
                                      const char *serviceData) {
  (void)serviceData;
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << (model ? model : "") << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << ","
      << "ble." << macAddress << ".txPower:" << (txPower ? txPower : "") << ",";

  const std::vector<std::string> mEntries =
    splitString(manufData ? std::string(manufData) : std::string(), ",");
  for (const auto &m : mEntries) {
    if (m.empty())
      continue;
    auto kv = parseKV_du(m);
    if (kv.id != "0f16")
      continue;
    auto b = hexToBytes(kv.hexData.c_str());
    if (b.size() < 2)
      continue;
    // Mode dependent parsing needs at least 10 bytes in original logic
    if (b.size() >= 10) {
      uint16_t mode = (uint16_t(b[0]) << 8) | b[1];
      if (mode == 0x0406) { // ANGLE
        out << "ble." << macAddress << ".escort.du.mode:ANGLE,";
        if (b.size() > 2 && b[2] == 0x01)
          out << "ble." << macAddress << ".angle.event:1,";
        if (b.size() > 4) {
          int angle = (b[3] | b[4]);
          out << "ble." << macAddress << ".angle.degrees:" << angle << ",";
        }
        if (b.size() > 7) {
          int upper = (b[6] | b[7]);
          out << "ble." << macAddress
              << ".angle.upper.condition.degrees:" << upper << ",";
        }
        if (b.size() > 9) {
          int lower = (b[8] | b[9]);
          out << "ble." << macAddress
              << ".angle.lower.condition.degrees:" << lower << ",";
        }
      } else if (mode == 0x0409) { // BUCKET
        out << "ble." << macAddress << ".escort.du.mode:BUCKET,";
        if (b.size() > 4) {
          int angle = ((b[4] << 8) | b[3]);
          out << "ble." << macAddress << ".angle.degrees:" << angle << ",";
        }
        if (b.size() > 9) {
          int delta = (b[8] | b[9]);
          out << "ble." << macAddress << ".angle.delta.degrees:" << delta
              << ",";
        }
      } else if (mode == 0x0405 || mode == 0x0404) { // ROTATION
        bool vertical = (mode == 0x0404);
        out << "ble." << macAddress << ".escort.du.mode:ROTATION,";
        out << "ble." << macAddress
            << ".rotation.axis:" << (vertical ? "VERTICAL" : "HORIZONTAL")
            << ",";
        if (b.size() > 2) {
          int8_t signedRpm = static_cast<int8_t>(b[2]);
          if (signedRpm == 0)
            out << "ble." << macAddress << ".rotation.direction:NEUTRAL,";
          else if (signedRpm > 0)
            out << "ble." << macAddress << ".rotation.direction:RIGHT,";
          else
            out << "ble." << macAddress << ".rotation.direction:LEFT,";
          out << "ble." << macAddress
              << ".rotations.per.minute:" << abs((int)signedRpm) << ",";
        }
        if (b.size() > 4) {
          int rotCount = ((b[4] << 8) | b[3]);
          out << "ble." << macAddress << ".rotation.count:" << rotCount << ",";
        }
      } else if (mode == 0x040a) { // PLOW
        out << "ble." << macAddress << ".escort.du.mode:PLOW,";
        if (b.size() > 2 && b[2] == 0x01)
          out << "ble." << macAddress << ".angle.event:1,";
        if (b.size() > 4) {
          int angle = (b[3] | b[4]);
          out << "ble." << macAddress << ".angle.degrees:" << angle << ",";
        }
        if (b.size() > 7) {
          int upper = (b[6] | b[7]);
          out << "ble." << macAddress
              << ".angle.upper.condition.degrees:" << upper << ",";
        }
        if (b.size() > 9) {
          int lower = (b[8] | b[9]);
          out << "ble." << macAddress
              << ".angle.lower.condition.degrees:" << lower << ",";
        }
      } else { // SLEEPING / unknown
        out << "ble." << macAddress << ".escort.du.mode:SLEEPING,";
      }
    }
    // Battery / firmware if present (outside of mode size gate, but need index
    // guards)
    if (b.size() >= 11) {
      out << "ble." << macAddress << ".battery.voltage:" << b[10] / 10.0f
          << ",";
    }
    if (b.size() >= 12) {
      out << "ble." << macAddress << ".firmware.version:" << (unsigned)b[11]
          << ",";
    }
  }

  std::string res = out.str();
  if (!res.empty() && res.back() == ',')
    res.pop_back();
  return res;
}
