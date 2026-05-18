#include <modules/obd2/obd2_decoder.h> // J1939 decoder class

Obd2Decoder::Obd2Decoder() : filesLoaded(false) {}
Obd2Decoder obd2Decoder;
SpiRamJsonDocument *jsonPidDoc = nullptr;
SpiRamJsonDocument *jsonDtcDoc = nullptr;

bool Obd2Decoder::begin() {
  if (!SPIFFS.begin(true)) {
    debugPrint("SPIFFS initialization failed!\n");
    return false;
  }

  if (!psramFound()) {
    debugPrint("PSRAM not available, cannot allocate JSON documents.\n");
    return false;
  }

  return loadDatabase();
}

bool Obd2Decoder::loadDatabase() {
  if (!psramFound()) {
    debugPrint("PSRAM not available!\n");
    return false;
  }

  // Load PID database
  if (!loadJsonFile("/obd2PID.json", jsonPidDoc))
    return false;
  // Load DTC database
  if (!loadJsonFile("/obd2DTC.json", jsonDtcDoc))
    return false;

  return true;
}

bool Obd2Decoder::loadJsonFile(const char *path, SpiRamJsonDocument *&doc) {
  File file = SPIFFS.open(path, "r");
  if (!file) {
    debugPrint("Failed to open file: %s\n", path);
    return false;
  }

  size_t fileSize = file.size();
  size_t jsonSize =
    2.1 * fileSize; // Adjust multiplier based on your JSON structure

  if (doc) {
    delete doc;
    doc = nullptr;
  }
  doc = new SpiRamJsonDocument(jsonSize);

  DeserializationError error = deserializeJson(*doc, file);
  file.close();

  if (error) {
    debugPrint("Failed to deserialize JSON file: %s\n", path);
    delete doc;
    doc = nullptr;
    return false;
  }

  return true;
}

// // Function to extract a bit field from a byte array
// uint32_t Obd2Decoder::extractBitField(const uint8_t *data, int startBit, int
// bitLength, bool isLittleEndian, bool isSigned) {
//     uint32_t result = 0;           // The final extracted value
//     int byteOffset = startBit / 8;  // Starting byte index
//     int bitOffset = startBit % 8;   // Bit offset within the starting byte
//     int bitsRemaining = bitLength;  // How many bits we still need to extract
//     int shiftAmount = 0;            // How much to shift each new chunk into
//     place

//     while (bitsRemaining > 0) {
//         // Extract bits from the current byte
//         uint8_t currentByte = data[byteOffset];

//         // How many bits can we take from this byte?
//         int bitsInThisByte = min(8 - bitOffset, bitsRemaining);

//         // Mask out the bits we want (shift to align with LSB if needed)
//         uint8_t bitMask = (1 << bitsInThisByte) - 1;
//         uint8_t extractedBits = (currentByte >> bitOffset) & bitMask;

//         // Insert into result
//         if (isLittleEndian) {
//             // Little-endian: lower bits come from earlier bytes
//             result |= (uint32_t)extractedBits << shiftAmount;
//         } else {
//             // Big-endian: higher bits come from earlier bytes
//             result = (result << bitsInThisByte) | extractedBits;
//         }

//         // Move to the next byte and update counters
//         bitsRemaining -= bitsInThisByte;
//         if (isLittleEndian) {
//             shiftAmount += bitsInThisByte;
//         }
//         bitOffset = 0;  // After the first byte, we always start at bit 0
//         byteOffset++;
//     }

//     return result;
// }

int64_t Obd2Decoder::extractBitField(const uint8_t *data, int startBit,
                                     int bitLength, bool isLittleEndian,
                                     bool isSigned) {
  int64_t result = 0; // Use 64-bit signed type for sign extension
  int byteOffset = startBit / 8;
  int bitOffset = startBit % 8;
  int bitsRemaining = bitLength;
  int shiftAmount = 0;

  while (bitsRemaining > 0) {
    uint8_t currentByte = data[byteOffset];
    int bitsInThisByte = std::min(8 - bitOffset, bitsRemaining);
    uint8_t bitMask = (1 << bitsInThisByte) - 1;
    uint8_t extractedBits = (currentByte >> bitOffset) & bitMask;

    if (isLittleEndian) {
      result |= static_cast<int64_t>(extractedBits) << shiftAmount;
      shiftAmount += bitsInThisByte;
    } else {
      result = (result << bitsInThisByte) | extractedBits;
    }

    bitsRemaining -= bitsInThisByte;
    bitOffset = 0;
    byteOffset++;
  }

  // Sign extension for 64-bit (two's complement)
  if (isSigned && bitLength > 0) {
    int64_t signBitMask = 1LL << (bitLength - 1);
    if (result & signBitMask) { // Check if sign bit is set
      int64_t signExtension = ~((1LL << bitLength) - 1);
      result |= signExtension;
    }
  }

  return result;
}

std::string Obd2Decoder::decodePIDs(uint32_t pid, const uint8_t *data,
                                    size_t dataLength) {
  // print the raw data
  //  debugPrint("PID 0x");
  //  if (pid < 0x10) UARTSerial.print("0");  // Leading zero for single hex
  //  digit UARTSerial.print(pid, HEX); UARTSerial.print(" Data: "); for (int i
  //  = 0; i < dataLength; i++) {
  //      UARTSerial.printf("%02X ", data[i]);
  //  }
  //  UARTSerial.println();
  //  debugPrint("Is Json PID Doc loaded: %d\n", jsonPidDoc != nullptr);
  if (pid == 0x01) {
    return decodePID01(data);
  }
  if (jsonPidDoc) {
    // serializeJson(*jsonPidDoc, UARTSerial);
    JsonArray pidArray = (*jsonPidDoc)["pids"].as<JsonArray>();
    for (JsonObject pidObj : pidArray) {
      if ((uint32_t)pidObj["p"] == pid) {
        JsonArray signals = pidObj["s"].as<JsonArray>();
        std::string result = "";

        for (JsonObject signal : signals) {
          const char *label = signal["l"];
          int startBit = signal["sb"];
          int bitLength = signal["bl"];
          bool isLittleEndian = signal["il"];
          bool isSigned = signal["is"];
          float factor = signal["f"];
          float offset = signal["o"];

          // debugPrint("Start Bit: %d, Bit Length: %d, endianess: %d, Factor:
          // %f, Offset: %f\n", startBit, bitLength, isLittleEndian, factor,
          // offset);
          int64_t rawValue = extractBitField(data, startBit, bitLength,
                                             isLittleEndian, isSigned);
          // debugPrint("Raw Value: %d\n", rawValue);
          float value = (rawValue * factor) + offset;
          // debugPrint("Value: %f\n", value);

          // Use stringstream for formatting
          std::ostringstream oss;

          if (!signal.containsKey("st")) {
            if (std::fmod(value, 1.0) ==
                0.0) { // Check if the decimal part is 0
              oss << std::fixed << std::setprecision(0)
                  << value; // Format as integer
            } else {
              oss << std::fixed << std::setprecision(2)
                  << value; // Round to two decimal places
            }
          } else {
            JsonArray states = signal["st"].as<JsonArray>();
            for (JsonObject state : states) {
              if (state["v"] == value) {
                oss << state["st"];
                break;
              }
            }
            if (oss.str().empty()) {
              oss << value;
            }
          }

          // Build the result string
          result += std::string(label) + ":" + oss.str() + ",";
        }

        // Trim the trailing comma
        if (result.length() > 1)
          result.erase(result.length() - 1);

        return result;
      }
    }
  }

  // If PGN not found or database not loaded return raw j1939 data in hex
  std::ostringstream oss;

  // Format PGN
  oss << "obd2.raw.data." << pid << ":";

  // Convert data to hex and format properly
  for (size_t i = 0; i < dataLength; i++) {
    oss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
        << (int)data[i];
  }

  return oss.str();
}

std::string Obd2Decoder::decodeDTC(const char *dtc) {
  std::stringstream ss;

  // Lookup DTC label
  std::string dtcLabel = "unknown";
  if (jsonDtcDoc) {
    for (JsonObject entry : jsonDtcDoc->as<JsonArray>()) {
      if (strcmp(entry["d"], dtc) == 0) {
        dtcLabel = entry["l"].as<const char *>();
        break;
      }
    }
  }

  // Append DTC information
  ss << "obd2.dtc." << dtc << ":" << dtcLabel << ",";
  return ss.str();
}

bool Obd2Decoder::isDatabaseLoaded() const { return filesLoaded; }

std::string Obd2Decoder::decodePID01(const uint8_t *data) {
  if (!data)
    return "obd2.s01.pid.1.pid.1:no valid data"; // Ensure data is valid

  // Extract bytes A, B, C, and D
  uint8_t A = data[0];
  uint8_t B = data[1];
  uint8_t C = data[2];
  uint8_t D = data[3];

  // Decode MIL (Malfunction Indicator Lamp) status
  bool mil = (A & 0x80) != 0; // A7 bit

  // Decode number of DTCs (Diagnostic Trouble Codes)
  uint8_t dtc_count = A & 0x7F; // A6-A0 bits

  // Decode common test availability and completeness
  bool components_test_availability = (B & 0x04) != 0; // B2 bit
  bool components_test_completeness = (B & 0x40) != 0; // B6 bit

  bool fuel_system_test_availability = (B & 0x02) != 0; // B1 bit
  bool fuel_system_test_completeness = (B & 0x20) != 0; // B5 bit

  bool misfire_test_availability = (B & 0x01) != 0; // B0 bit
  bool misfire_test_completeness = (B & 0x10) != 0; // B4 bit

  // Determine engine type (0 = Spark ignition, 1 = Compression ignition)
  bool engine_type = (B & 0x08) != 0; // B3 bit

  // Decode engine-type specific tests availability and completeness
  std::bitset<8> availability_bits(C);
  std::bitset<8> completeness_bits(D);

  // Create result string
  std::ostringstream result;
  int dtcCount = static_cast<int>(dtc_count);
  requestObd2Dtcs =
    dtcCount > 0 || mil; // Request DTCs if MIL is on or DTCs are present

  result << "obd2.s01.pid.1.mil:" << (mil ? "true" : "false") << ", ";
  result << "obd2.s01.pid.1.dtc.count:" << dtcCount << ", ";

  // Common test availability & completeness
  result << "obd2.s01.pid.1.components.ta:"
         << (components_test_availability ? "true" : "false") << ", ";
  result << "obd2.s01.pid.1.components.tc:"
         << (components_test_completeness ? "true" : "false") << ", ";

  result << "obd2.s01.pid.1.fuel.system.ta:"
         << (fuel_system_test_availability ? "true" : "false") << ", ";
  result << "obd2.s01.pid.1.fuel.system.tc:"
         << (fuel_system_test_completeness ? "true" : "false") << ", ";

  result << "obd2.s01.pid.1.misfire.ta:"
         << (misfire_test_availability ? "true" : "false") << ", ";
  result << "obd2.s01.pid.1.misfire.tc:"
         << (misfire_test_completeness ? "true" : "false") << ", ";

  // Engine type specific tests
  result << "obd2.s01.pid.1.engine.type:" << (engine_type ? "diesel" : "spark")
         << ", ";

  // Append engine-type specific test results
  const char *spark_tests[] = {"catalyst",
                               "heated.catalyst",
                               "evap.system",
                               "secondary.air.system",
                               "gasoline.particulate.filter",
                               "oxygen.sensor",
                               "oxygen.sensor.heater",
                               "egr.vvt"};

  const char *diesel_tests[] = {"nmhc.catalyst",     "nox.scr.monitor",
                                "reserved",          "boost.pressure",
                                "reserved",          "exhaust.gas.sensor",
                                "pm.filter.monitor", "egr.vvt"};

  const char **test_names = engine_type ? diesel_tests : spark_tests;

  for (int i = 0; i < 8; i++) {
    if (test_names[i][0] != '-') { // Skip reserved bits
      result << "obd2.s01.pid.1." << test_names[i]
             << ".ta:" << (availability_bits[i] ? "true" : "false") << ", ";
      result << "obd2.s01.pid.1." << test_names[i]
             << ".tc:" << (completeness_bits[i] ? "false" : "true") << ", ";
    }
  }

  std::string output = result.str();
  output.pop_back(); // Remove last comma
  output.pop_back(); // Remove last space
  return output;
}
