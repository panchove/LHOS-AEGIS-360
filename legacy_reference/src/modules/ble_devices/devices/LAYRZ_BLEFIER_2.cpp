#include <modules/ble_devices/devices/LAYRZ_BLEFIER_2.h>

static inline std::string toLower(std::string s) {
  for (auto &ch : s)
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return s;
}

static inline std::string boolToString(bool v) { return v ? "1" : "0"; }
static inline float roundTo(float v, int decimals) {
  float p = 1.0f;
  for (int i = 0; i < decimals; ++i)
    p *= 10.0f;
  return std::round(v * p) / p;
}

bleSensorData LAYRZ_BLEFIER_2_decoder::parseData(const std::string bleData) {
  size_t pos = bleData.find(":");
  if (pos == std::string::npos) {
    return {"", ""};
  }
  std::string id = bleData.substr(0, pos);
  std::string value = bleData.substr(pos + 1);

  if (id.length() == 0) {
    return {"", ""};
  }
  return {toLower(id), value};
}

std::string LAYRZ_BLEFIER_2_decoder::decode(const char *macAddress,
                                            const char *model, const char *rssi,
                                            const char *txPower,
                                            const char *manufData,
                                            const char *serviceData) {

  // debugPrint("[LAYRZ_BLEFIER_2] Decoding BLE data for MAC: %s\n",
  // macAddress);
  std::ostringstream oss;
  oss << "ble." << macAddress << ".model:" << model << ","
      << "ble." << macAddress << ".rssi:" << rssi << ","
      << "ble." << macAddress << ".txPower:" << txPower << ",";

  // --- Manufacturer data: battery voltage (mV -> V) ---
  {
    bleSensorData manuf = LAYRZ_BLEFIER_2_decoder::parseData(manufData);
    auto bytes = hexToBytes(manuf.hexData.c_str());
    if (bytes.size() >= 3) {
      uint16_t mv = (static_cast<uint16_t>(bytes[1]) << 8) | bytes[2];
      oss << "ble." << macAddress
          << ".battery.voltage.V:" << static_cast<float>(mv) / 1000.0f << ",";
    }
  }

  // --- Service data(s) ---
  std::vector<std::string> serviceArray = splitString(serviceData, ",");
  for (const auto &service : serviceArray) {
    if (service.empty())
      continue;
    bleSensorData svc = LAYRZ_BLEFIER_2_decoder::parseData(service);
    if (svc.id.empty())
      continue;
    auto b = hexToBytes(svc.hexData.c_str());
    if (b.empty())
      continue;

    // 0x1901 — Four Analog inputs A0..A3
    if (svc.id == "1901") {
      if (b.size() >= 9) {
        // mask bits: bit0->A0, bit1->A1, bit2->A2, bit3->A3 (0=voltage,
        // 1=temperature)
        std::array<uint8_t, 4> m{static_cast<uint8_t>(b[0] & 0x01),
                                 static_cast<uint8_t>((b[0] >> 1) & 0x01),
                                 static_cast<uint8_t>((b[0] >> 2) & 0x01),
                                 static_cast<uint8_t>((b[0] >> 3) & 0x01)};
        auto s16 = [&](size_t i) {
          return static_cast<int16_t>((b[i] << 8) | b[i + 1]);
        };
        int16_t a0 = s16(1), a1 = s16(3), a2 = s16(5), a3 = s16(7);
        // A0
        if (!m[0])
          oss << "ble." << macAddress
              << ".analog.input.0.V:" << static_cast<float>(a0) / 1000.0f
              << ",";
        else {
          float c = static_cast<float>(a0) / 100.0f;
          float f = c * 9.0f / 5.0f + 32.0f;
          oss << "ble." << macAddress << ".analog.input.0.temp.c:" << c << ",";
          oss << "ble." << macAddress << ".analog.input.0.temp.f:" << f << ",";
        }
        // A1
        if (!m[1])
          oss << "ble." << macAddress
              << ".analog.input.1.V:" << static_cast<float>(a1) / 1000.0f
              << ",";
        else {
          float c = static_cast<float>(a1) / 100.0f;
          float f = c * 9.0f / 5.0f + 32.0f;
          oss << "ble." << macAddress << ".analog.input.1.temp.c:" << c << ",";
          oss << "ble." << macAddress << ".analog.input.1.temp.f:" << f << ",";
        }
        // A2
        if (!m[2])
          oss << "ble." << macAddress
              << ".analog.input.2.V:" << static_cast<float>(a2) / 1000.0f
              << ",";
        else {
          float c = static_cast<float>(a2) / 100.0f;
          float f = c * 9.0f / 5.0f + 32.0f;
          oss << "ble." << macAddress << ".analog.input.2.temp.c:" << c << ",";
          oss << "ble." << macAddress << ".analog.input.2.temp.f:" << f << ",";
        }
        // A3
        if (!m[3])
          oss << "ble." << macAddress
              << ".analog.input.3.V:" << static_cast<float>(a3) / 1000.0f
              << ",";
        else {
          float c = static_cast<float>(a3) / 100.0f;
          float f = c * 9.0f / 5.0f + 32.0f;
          oss << "ble." << macAddress << ".analog.input.3.temp.c:" << c << ",";
          oss << "ble." << macAddress << ".analog.input.3.temp.f:" << f << ",";
        }
      }
    }
    // 0x1902 — Two Analog inputs A0..A1
    else if (svc.id == "1902") {
      if (b.size() >= 5) {
        // mask bits: bit0->A0, bit1->A1
        uint8_t m0 = (b[0] & 0x01), m1 = ((b[0] >> 1) & 0x01);
        auto s16 = [&](size_t i) {
          return static_cast<int16_t>((b[i] << 8) | b[i + 1]);
        };
        int16_t a0 = s16(1), a1 = s16(3);
        if (!m0)
          oss << "ble." << macAddress
              << ".analog.input.0.V:" << static_cast<float>(a0) / 1000.0f
              << ",";
        else {
          float c = static_cast<float>(a0) / 100.0f;
          float f = c * 9.0f / 5.0f + 32.0f;
          oss << "ble." << macAddress << ".analog.input.0.temp.c:" << c << ","
              << "ble." << macAddress << ".analog.input.0.temp.f:" << f << ",";
        }
        if (!m1)
          oss << "ble." << macAddress
              << ".analog.input.1.V:" << static_cast<float>(a1) / 1000.0f
              << ",";
        else {
          float c = static_cast<float>(a1) / 100.0f;
          float f = c * 9.0f / 5.0f + 32.0f;
          oss << "ble." << macAddress << ".analog.input.1.temp.c:" << c << ","
              << "ble." << macAddress << ".analog.input.1.temp.f:" << f << ",";
        }
      }
    }
    // 0x1903 — Two current inputs (4–20mA)
    else if (svc.id == "1903") {
      if (b.size() >= 4) {
        uint16_t i4 = (b[0] << 8) | b[1];
        uint16_t i5 = (b[2] << 8) | b[3];
        oss << "ble." << macAddress
            << ".analog.input.4.current.ma:" << static_cast<float>(i4) / 1000.0f
            << ",";
        oss << "ble." << macAddress
            << ".analog.input.5.current.ma:" << static_cast<float>(i5) / 1000.0f
            << ",";
      }
    }
    // 0x1904 — One current input (4–20mA) -> input 4
    else if (svc.id == "1904") {
      if (b.size() >= 2) {
        uint16_t i4 = (b[0] << 8) | b[1];
        oss << "ble." << macAddress
            << ".analog.input.4.current.ma:" << static_cast<float>(i4) / 1000.0f
            << ",";
      }
    }
    // 0x1905 — Four Digital inputs D2..D5 (mask bits 0..3)
    else if (svc.id == "1905") {
      if (b.size() >= 9) {
        bool d2s = (b[0] & 0x01) != 0;
        bool d3s = (b[0] & 0x02) != 0;
        bool d4s = (b[0] & 0x04) != 0;
        bool d5s = (b[0] & 0x08) != 0;
        uint16_t d2 = (b[1] << 8) | b[2];
        uint16_t d3 = (b[3] << 8) | b[4];
        uint16_t d4 = (b[5] << 8) | b[6];
        uint16_t d5 = (b[7] << 8) | b[8];
        oss << "ble." << macAddress
            << ".digital.input.2.status:" << boolToString(d2s) << ",";
        oss << "ble." << macAddress << ".digital.input.2.count:" << d2 << ",";
        oss << "ble." << macAddress
            << ".digital.input.3.status:" << boolToString(d3s) << ",";
        oss << "ble." << macAddress << ".digital.input.3.count:" << d3 << ",";
        oss << "ble." << macAddress
            << ".digital.input.4.status:" << boolToString(d4s) << ",";
        oss << "ble." << macAddress << ".digital.input.4.count:" << d4 << ",";
        oss << "ble." << macAddress
            << ".digital.input.5.status:" << boolToString(d5s) << ",";
        oss << "ble." << macAddress << ".digital.input.5.count:" << d5 << ",";
      }
    }
    // 0x1906 — Digital inputs D0..D1 (mask bits 0..1)
    else if (svc.id == "1906") {
      if (b.size() >= 5) {
        bool d0s = (b[0] & 0x02) != 0; // mask[1]
        bool d1s = (b[0] & 0x01) != 0; // mask[0]
        uint16_t d0 = (b[1] << 8) | b[2];
        uint16_t d1 = (b[3] << 8) | b[4];
        oss << "ble." << macAddress
            << ".digital.input.0.status:" << boolToString(d0s) << ",";
        oss << "ble." << macAddress << ".digital.input.0.count:" << d0 << ",";
        oss << "ble." << macAddress
            << ".digital.input.1.status:" << boolToString(d1s) << ",";
        oss << "ble." << macAddress << ".digital.input.1.count:" << d1 << ",";
      }
    }
    // 0x1907 — Digital inputs D2..D3 (use two LSBs)
    else if (svc.id == "1907") {
      if (b.size() >= 5) {
        bool d2s = (b[0] & 0x02) != 0; // mask[1]
        bool d3s = (b[0] & 0x01) != 0; // mask[0]
        uint16_t d2 = (b[1] << 8) | b[2];
        uint16_t d3 = (b[3] << 8) | b[4];
        oss << "ble." << macAddress
            << ".digital.input.2.status:" << boolToString(d2s) << ",";
        oss << "ble." << macAddress << ".digital.input.2.count:" << d2 << ",";
        oss << "ble." << macAddress
            << ".digital.input.3.status:" << boolToString(d3s) << ",";
        oss << "ble." << macAddress << ".digital.input.3.count:" << d3 << ",";
      }
    }
    // 0x1908 — Digital inputs D4..D5 (use two LSBs)
    else if (svc.id == "1908") {
      if (b.size() >= 5) {
        bool d4s = (b[0] & 0x02) != 0; // mask[1]
        bool d5s = (b[0] & 0x01) != 0; // mask[0]
        uint16_t d4 = (b[1] << 8) | b[2];
        uint16_t d5 = (b[3] << 8) | b[4];
        oss << "ble." << macAddress
            << ".digital.input.4.status:" << boolToString(d4s) << ",";
        oss << "ble." << macAddress << ".digital.input.4.count:" << d4 << ",";
        oss << "ble." << macAddress
            << ".digital.input.5.status:" << boolToString(d5s) << ",";
        oss << "ble." << macAddress << ".digital.input.5.count:" << d5 << ",";
      }
    }
    // 0x1909 — Accelerometer (mg) + event count
    else if (svc.id == "1909") {
      if (b.size() >= 8) {
        auto s16 = [&](size_t i) {
          return static_cast<int16_t>((b[i] << 8) | b[i + 1]);
        };
        int16_t x = s16(0), y = s16(2), z = s16(4);
        uint16_t events = (b[6] << 8) | b[7];
        oss << "ble." << macAddress << ".accelerometer.x.mg:" << x << ",";
        oss << "ble." << macAddress << ".accelerometer.y.mg:" << y << ",";
        oss << "ble." << macAddress << ".accelerometer.z.mg:" << z << ",";
        oss << "ble." << macAddress << ".accelerometer.event.count:" << events
            << ",";
      }
    }
    // 0x190A — Inclination angle (centi-degrees -> degrees)
    else if (svc.id == "190a") {
      if (b.size() >= 2) {
        int16_t raw = static_cast<int16_t>((b[0] << 8) | b[1]);
        float deg = static_cast<float>(raw) / 100.0f;
        oss << "ble." << macAddress << ".inclination.angle.degrees:" << deg
            << ",";
      }
    }
    // 0x190B — Illuminance (centilux -> lux)
    else if (svc.id == "190b") {
      if (b.size() >= 4) {
        uint32_t clx = (static_cast<uint32_t>(b[0]) << 24) |
                       (static_cast<uint32_t>(b[1]) << 16) |
                       (static_cast<uint32_t>(b[2]) << 8) |
                       static_cast<uint32_t>(b[3]);
        oss << "ble." << macAddress
            << ".illuminance.lux:" << static_cast<float>(clx) / 100.0f << ",";
      }
    }
    // 0x190C — 1-Wire temperature (centi-degC signed)
    else if (svc.id == "190c") {
      if (b.size() >= 2) {
        int16_t raw = static_cast<int16_t>((b[0] << 8) | b[1]);
        float c = static_cast<float>(raw) / 100.0f;
        float f = c * 9.0f / 5.0f + 32.0f;
        oss << "ble." << macAddress
            << ".1wire.temperature.celsius:" << roundTo(c, 2) << ",";
        oss << "ble." << macAddress
            << ".1wire.temperature.fahrenheit:" << roundTo(f, 2) << ",";
      }
    }
    // 0x190D — Ultrasonic distance (mm)
    else if (svc.id == "190d") {
      if (b.size() >= 2) {
        uint16_t mm = (b[0] << 8) | b[1];
        oss << "ble." << macAddress << ".ultrasonic.distance.milimeters:" << mm
            << ",";
      }
    }
    // 0x190E — Hourmeter (seconds + hours)
    else if (svc.id == "190e") {
      if (b.size() >= 4) {
        uint32_t secs = (static_cast<uint32_t>(b[0]) << 24) |
                        (static_cast<uint32_t>(b[1]) << 16) |
                        (static_cast<uint32_t>(b[2]) << 8) |
                        static_cast<uint32_t>(b[3]);
        float hours = static_cast<float>(secs) / 3600.0f;
        oss << "ble." << macAddress << ".hourmeter.seconds:" << secs << ",";
        oss << "ble." << macAddress << ".hourmeter.hours:" << roundTo(hours, 4)
            << ",";
      }
    }
    // 0x190F — Universal counter (mode-specific)
    else if (svc.id == "190f") {
      if (b.size() >= 7) {
        uint8_t mode = b[0];
        uint32_t value = (static_cast<uint32_t>(b[1]) << 24) |
                         (static_cast<uint32_t>(b[2]) << 16) |
                         (static_cast<uint32_t>(b[3]) << 8) |
                         static_cast<uint32_t>(b[4]);
        uint16_t rate = (b[5] << 8) | b[6];
        switch (mode) {
        case 0: { // Odometer
          float km = static_cast<float>(value) / 1000.0f;
          float kmh = static_cast<float>(rate) / 100.0f;
          oss << "ble." << macAddress << ".odometer.meters:" << value << ",";
          oss << "ble." << macAddress
              << ".odometer.kilometers:" << roundTo(km, 3) << ",";
          oss << "ble." << macAddress
              << ".speed.km.per.hour:" << roundTo(kmh, 2) << ",";
          break;
        }
        case 1: { // Volumetric counter
          double liters = static_cast<double>(value);
          double cubicMeters = static_cast<double>(value) / 1000.0; // L -> m^3
          double gallons = static_cast<double>(value) * 0.264172;   // L -> gal
          double lps = static_cast<double>(rate) / 10.0;            // L/s
          double m3ph = static_cast<double>(rate) * 0.36;           // m^3/h
          double gpm = static_cast<double>(rate) * 1.585;           // gal/min
          oss << "ble." << macAddress << ".volumetric.counter.liters:" << liters
              << ",";
          oss << "ble." << macAddress
              << ".volumetric.counter.cubic.meters:" << cubicMeters << ",";
          oss << "ble." << macAddress << ".volumetric.counter.gallons:"
              << roundTo(static_cast<float>(gallons), 2) << ",";
          oss << "ble." << macAddress << ".volumetric.flow.liters.per.second:"
              << roundTo(static_cast<float>(lps), 2) << ",";
          oss << "ble." << macAddress << ".volumetric.flow.m3.per.hour:"
              << roundTo(static_cast<float>(m3ph), 4) << ",";
          oss << "ble." << macAddress << ".volumetric.flow.gallons.per.minute:"
              << roundTo(static_cast<float>(gpm), 4) << ",";
          break;
        }
        case 2: { // Energy counter
          float kwh = static_cast<float>(value) / 1000.0f;
          float watts = static_cast<float>(rate);
          oss << "ble." << macAddress
              << ".energy.counter.kwh:" << roundTo(kwh, 2) << ",";
          oss << "ble." << macAddress
              << ".energy.rate.watts:" << roundTo(watts, 2) << ",";
          break;
        }
        case 3: { // Tachometer
          float rpm = static_cast<float>(rate);
          oss << "ble." << macAddress << ".tachometer.revolutions:" << value
              << ",";
          oss << "ble." << macAddress
              << ".tachometer.revolutions.per.minute:" << roundTo(rpm, 2)
              << ",";
          break;
        }
        case 4: { // Generic asset counter
          float cpm = static_cast<float>(rate);
          oss << "ble." << macAddress << ".asset.counter.count:" << value
              << ",";
          oss << "ble." << macAddress
              << ".asset.rate.count.per.minute:" << roundTo(cpm, 2) << ",";
          break;
        }
        default:
          // Unknown mode – emit raw values for debugging
          oss << "ble." << macAddress
              << ".universal.mode:" << static_cast<int>(mode) << ",";
          oss << "ble." << macAddress << ".universal.value:" << value << ",";
          oss << "ble." << macAddress << ".universal.rate:" << rate << ",";
          break;
        }
      }
    }
    // Add further service IDs here if the firmware exposes more.
  }
  // delete the last comma from oss
  std::string result = oss.str();
  if (!result.empty()) {
    result.erase(result.length() - 1); // Remove the last comma
  }

  return result;
}