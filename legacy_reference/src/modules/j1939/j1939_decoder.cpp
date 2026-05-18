#include <modules/j1939/j1939_decoder.h> // J1939 decoder class

J1939Decoder::J1939Decoder() : filesLoaded(false) {}

bool J1939Decoder::begin() {
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

bool J1939Decoder::loadDatabase() {
  if (!psramFound()) {
    debugPrint("PSRAM not available!\n");
    return false;
  }

  // Load PGN database
  if (!loadJsonFile("/j1939PGN.json", jsonPgnDoc))
    return false;
  // Load SPN database
  if (!loadJsonFile("/j1939SPN.json", jsonSpnDoc))
    return false;
  // Load FMI database
  if (!loadJsonFile("/j1939FMI.json", jsonFmiDoc))
    return false;

  return true;
}

bool J1939Decoder::loadJsonFile(const char *path, SpiRamJsonDocument *&doc) {
  File file = SPIFFS.open(path, "r");
  if (!file) {
    debugPrint("Failed to open %s\n", path);
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
    debugPrint("Deserialization error in %s: %s\n", path, error.c_str());
    delete doc;
    doc = nullptr;
    return false;
  }

  return true;
}

int64_t J1939Decoder::extractBitField(const uint8_t *data, int startBit,
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

std::string J1939Decoder::decodeSignals(uint32_t pgn, const uint8_t *data,
                                        size_t dataLength) {
  if (jsonPgnDoc) {
    // serializeJson(*jsonPgnDoc, UARTSerial);
    JsonArray pgnArray = (*jsonPgnDoc)["p"].as<JsonArray>();
    for (JsonObject pgnObj : pgnArray) {
      if ((uint32_t)pgnObj["p"] == pgn) {
        std::string pgnName = pgnObj["n"];
        JsonArray signals = pgnObj["s"].as<JsonArray>();
        std::string result =
          "j1939." + pgnName + ".pgn:" + std::to_string(pgn) + ",";

        for (JsonObject signal : signals) {
          const char *label = signal["l"];
          int startBit = signal["sb"];
          int bitLength = signal["bl"];
          float factor = signal["f"];
          float offset = signal["o"];
          bool isSigned = signal["is"];
          bool isLittleEndian = signal["il"];
          float min = signal["mn"];
          float max = signal["mx"];

          // Extract and process the signal value
          int64_t rawValue = extractBitField(data, startBit, bitLength,
                                             isLittleEndian, isSigned);

          float value = (rawValue * factor) + offset;
          // check if the value is within the min and max Srange
          if (value < min || value > max) {
            debugPrint("Value for signal %s is out of range: %f\n", label,
                       value);
            value = NAN;
            if (hubSettings.can_nan_filter)
              continue;
          }

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
            // Lookup state label
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
  oss << "j1939.raw.data." << pgn << ":";

  // Convert data to hex and format properly
  for (size_t i = 0; i < dataLength; i++) {
    oss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
        << (int)data[i];
  }

  return oss.str();
}

#include <sstream>
#include <string>

std::string J1939Decoder::decodeDTCs(uint32_t pgn, const uint8_t *data,
                                     size_t dataLength) {
  std::stringstream ss;
  const std::string dmn = (pgn == 65226) ? "dm1" : "dm2";

  // Lambda helpers for status labels
  auto getStatusLabel = [](uint8_t val) -> const char * {
    switch (val) {
    case 0:
      return "off";
    case 1:
      return "on";
    case 2:
      return "reserved";
    case 3:
      return "na";
    default:
      return "invalid";
    }
  };

  auto getFlashLabel = [](uint8_t val) -> const char * {
    switch (val) {
    case 0:
      return ".slow.flash";
    case 1:
      return ".fast.flash";
    case 2:
      return ".reserved";
    case 3:
      return ".steady";
    default:
      return ".invalid";
    }
  };

  // Parse lamp statuses
  if (dataLength < 2)
    return "";
  const uint16_t lampBits = (data[0] << 8) | data[1];

  struct Lamp {
    uint8_t status;
    uint8_t flash;
  } lamps[4] = {
    // Protect Lamp (bits 15-14 status, 7-6 flash)
    {(uint8_t)((lampBits & 0xC000) >> 14), (uint8_t)((lampBits & 0x00C0) >> 6)},
    // Amber Warning (bits 13-12 status, 5-4 flash)
    {(uint8_t)((lampBits & 0x3000) >> 12), (uint8_t)((lampBits & 0x0030) >> 4)},
    // Red Stop (bits 11-10 status, 3-2 flash)
    {(uint8_t)((lampBits & 0x0C00) >> 10), (uint8_t)((lampBits & 0x000C) >> 2)},
    // MIL (bits 9-8 status, 1-0 flash)
    {(uint8_t)((lampBits & 0x0300) >> 8), (uint8_t)(lampBits & 0x0003)}};

  const char *plStatusLabel = getStatusLabel(lamps[0].status);
  const char *plFlashLabel =
    strcmp(plStatusLabel, "off") == 0 ? "" : getFlashLabel(lamps[0].flash);
  const char *awlStatusLabel = getStatusLabel(lamps[1].status);
  const char *awlFlashLabel =
    strcmp(awlStatusLabel, "off") == 0 ? "" : getFlashLabel(lamps[1].flash);
  const char *rslStatusLabel = getStatusLabel(lamps[2].status);
  const char *rslFlashLabel =
    strcmp(rslStatusLabel, "off") == 0 ? "" : getFlashLabel(lamps[2].flash);
  const char *milStatusLabel = getStatusLabel(lamps[3].status);
  const char *milFlashLabel =
    strcmp(milStatusLabel, "off") == 0 ? "" : getFlashLabel(lamps[3].flash);

  // Build lamp status string
  ss << "j1939." << dmn << ".pl:" << plStatusLabel << plFlashLabel << ", j1939."
     << dmn << ".awl:" << awlStatusLabel << awlFlashLabel << ", j1939." << dmn
     << ".rsl:" << rslStatusLabel << rslFlashLabel << ", j1939." << dmn
     << ".mil:" << milStatusLabel << milFlashLabel;

  // Process DTCs
  const size_t dtcCount = (dataLength - 2) / 4;
  if (dtcCount < 1)
    return ss.str();

  const uint8_t *dtcPtr = data + 2;
  for (size_t i = 0; i < dtcCount; ++i, dtcPtr += 4) {
    // skip the reserved bytes in the DTC
    if (dtcPtr[0] == 0xFF && dtcPtr[1] == 0xFF && dtcPtr[2] == 0xFF &&
        dtcPtr[3] == 0xFF) {
      continue;
    }

    // Extract SPN (19 bits)
    const uint32_t spn =
      dtcPtr[0] | (dtcPtr[1] << 8) | ((dtcPtr[2] >> 5) << 16);

    // Extract FMI (5 bits)
    const uint8_t fmi = dtcPtr[2] & 0x1F;

    // Extract OC (7 bits)
    const uint8_t oc = dtcPtr[3] & 0x7F;

    // Lookup SPN label
    std::string spnLabel = "unknown";
    if (jsonSpnDoc) {
      for (JsonObject entry : jsonSpnDoc->as<JsonArray>()) {
        if (entry["s"].as<uint32_t>() == spn) {
          spnLabel = entry["l"].as<const char *>();
          break;
        }
      }
    }

    // Lookup FMI label
    std::string fmiLabel = "unknown";
    if (jsonFmiDoc) {
      for (JsonObject entry : jsonFmiDoc->as<JsonArray>()) {
        if (entry["f"].as<uint8_t>() == fmi) {
          fmiLabel = entry["l"].as<const char *>();
          break;
        }
      }
    }

    // Append DTC information
    ss << ", j1939." << dmn << ".dtc." << spn << ":" << spnLabel << "_"
       << fmiLabel << "_";

    if (oc == 127) {
      ss << "na";
    } else {
      ss << static_cast<int>(oc);
    }
  }

  return ss.str();
}

std::string J1939Decoder::decode(uint32_t pgn, const uint8_t *data,
                                 size_t dataLength) {
#ifdef DECODE_J1939
  std::string dmPGNs = "65226,65227";
  if (dmPGNs.find(std::to_string(pgn)) != std::string::npos) {
    return decodeDTCs(pgn, data, dataLength);
  } else {
    return decodeSignals(pgn, data, dataLength);
  }
#endif
}

bool J1939Decoder::isDatabaseLoaded() const { return filesLoaded; }
