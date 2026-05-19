// Implementation for ELA Innovation Blue PUCK Movement decoder
#include <modules/ble_devices/devices/ELA_PUCK_MOV.h>

// Helper to lowercase ID
static inline std::string toLower(std::string s) {
  for (auto &ch : s)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return s;
}

// Parse "<id>:<hex>" entry
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

std::string ELA_PUCK_MOV_decoder::decode(const char *macAddress,
                                         const char *model, const char *rssi,
                                         const char *txPower,
                                         const char *manufData,
                                         const char *serviceData) {
  (void)manufData; // Not used presently

  // debugPrint("[ELA_PUCK_MOV] Decoding BLE data for %s\n", macAddress);
  std::ostringstream out;
  out << "ble." << macAddress << ".model:" << (model ? model : "") << ","
      << "ble." << macAddress << ".rssi:" << (rssi ? rssi : "") << ","
      << "ble." << macAddress << ".txPower:" << (txPower ? txPower : "") << ",";

  // Collect service entries
  std::vector<std::string> services =
    splitString(serviceData ? std::string(serviceData) : std::string(), ",");
  std::vector<uint8_t> angleBytes;    // id 0x2AA1
  std::vector<uint8_t> movementBytes; // id 0x2A06
  std::vector<uint8_t> batteryBytes;

  for (const auto &svcStr : services) {
    if (svcStr.empty())
      continue;
    bleSensorData svc = parseDataKV(svcStr);
    if (svc.id.empty())
      continue;
    auto bytes = hexToBytes(svc.hexData.c_str());
    if (bytes.empty())
      continue;
    if (svc.id == "2aa1")
      angleBytes = std::move(bytes);
    else if (svc.id == "2a06")
      movementBytes = std::move(bytes);
  }

  if (!angleBytes.empty() && angleBytes.size() == 6) {
    // Three little-endian signed 16-bit acceleration axes
    auto s16 = [&](size_t i) {
      return static_cast<int16_t>((angleBytes[i + 1] << 8) | angleBytes[i]);
    };
    int16_t x = s16(0);
    int16_t y = s16(2);
    int16_t z = s16(4);
    out << "ble." << macAddress << ".ela.puck.mov.mode:ANGLE,";
    out << "ble." << macAddress << ".acceleration.x.axis:" << x << ",";
    out << "ble." << macAddress << ".acceleration.y.axis:" << y << ",";
    out << "ble." << macAddress << ".acceleration.z.axis:" << z << ",";
  } else if (!movementBytes.empty() && movementBytes.size() == 2) {
    // Two bytes: high bit = status, lower 15 bits = count. movementBytes
    // assumed little-endian per other devices.
    uint16_t value =
      (static_cast<uint16_t>(movementBytes[1]) << 8) | movementBytes[0];
    bool status = (value & 0x0001) == 0x0001;
    uint16_t count = (value >> 1) & 0x7FFF;
    out << "ble." << macAddress << ".ela.puck.mov.mode:MOVEMENT,";
    out << "ble." << macAddress << ".movement.status:" << (status ? 1 : 0)
        << ",";
    out << "ble." << macAddress << ".movement.count:" << count << ",";
  } else {
    out << "ble." << macAddress << ".ela.puck.mov.mode:UNKNOWN,";
  }

  // Parse manufacturer data to get battery information
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
  // Remove trailing comma if present
  std::string result = out.str();
  if (!result.empty() && result.back() == ',') {
    result.pop_back();
  }
  return result;
}
