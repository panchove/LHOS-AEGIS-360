#pragma once

#ifndef OBD2_DECODER_H
#define OBD2_DECODER_H

#include "OBD2.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h> // Global objects

using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;

class Obd2Decoder {
public:
  Obd2Decoder();

  bool begin();        // Initialize SPIFFS and load JSON database
  bool loadDatabase(); // Load JSON data from SPIFFS into memory
  bool isDatabaseLoaded() const;

  std::string decodePIDs(uint32_t pid, const uint8_t *data, size_t dataLength);
  std::string decodeDTC(const char *dtc);

private:
  bool loadJsonFile(const char *path, SpiRamJsonDocument *&doc);
  // Function to extract a bit field from a byte array
  int64_t extractBitField(const uint8_t *data, int startBit, int bitLength,
                          bool isLittleEndian, bool isSigned);
  bool filesLoaded; // Flag indicating if JSON files are successfully loaded
  std::string decodePID01(const uint8_t *data);
};
extern Obd2Decoder obd2Decoder;
extern SpiRamJsonDocument *jsonPidDoc;
extern SpiRamJsonDocument *jsonDtcDoc;

#endif // OBD2_DECODER_H