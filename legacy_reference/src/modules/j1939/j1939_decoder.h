#ifndef J1939_DECODER_H
#define J1939_DECODER_H

#include "ARD1939ESP32S3.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h> // Global objects

using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;

class J1939Decoder {
public:
  J1939Decoder();

  bool begin();        // Initialize SPIFFS and load JSON database
  bool loadDatabase(); // Load JSON data from SPIFFS into memory
  bool isDatabaseLoaded() const;

  std::string decode(uint32_t pgn, const uint8_t *data, size_t dataLength);
  std::string decodeSignals(uint32_t pgn, const uint8_t *data,
                            size_t dataLength);
  std::string decodeDTCs(uint32_t pgn, const uint8_t *data, size_t dataLength);

private:
  SpiRamJsonDocument *jsonPgnDoc = nullptr;
  SpiRamJsonDocument *jsonSpnDoc = nullptr;
  SpiRamJsonDocument *jsonFmiDoc = nullptr;
  bool loadJsonFile(const char *path, SpiRamJsonDocument *&doc);
  bool filesLoaded; // Flag indicating if JSON files are successfully loaded
  int64_t extractBitField(const uint8_t *data, int startBit, int bitLength,
                          bool isLittleEndian, bool isSigned);
};

#endif // J1939_DECODER_H
