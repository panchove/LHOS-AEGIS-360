#include <cmath>
#include <modules/gnss/GnssLayrzHub.h>

namespace {
struct ParsedGnssPosition {
  int fixQuality = 0;
  float latitude = 0.0f;
  float longitude = 0.0f;
  float altitude = 0.0f;
  float speedKmh = 0.0f;
  int satellites = 0;
  float hdop = 0.0f;
};

static constexpr uint32_t kGnssAsyncWaitMs = 9000;
static constexpr uint32_t kGnssFixCacheTtlMs = 120000;
static std::string s_lastValidPosition = ";;;;;;;";
static uint32_t s_lastValidPositionMs = 0;

String normalizeGnssRaw(const String &rawInput) {
  String raw = rawInput;
  debugPrint("Raw CGNSSINFO: %s\n", raw.c_str());
  raw.trim();
  if (raw.startsWith("+CGNSSINFO:")) {
    int colonPos = raw.indexOf(':');
    if (colonPos >= 0) {
      raw = raw.substring(colonPos + 1);
      raw.trim();
    }
  }
  return raw;
}

bool isLatDir(const String &v) { return v == "N" || v == "S"; }

bool isLonDir(const String &v) { return v == "E" || v == "W"; }

float parseGnssCoord(const String &coordStr, const String &dir, bool isLat) {
  if (coordStr.length() == 0) {
    return 0.0f;
  }

  float raw = coordStr.toFloat();
  float value = raw;
  int dotPos = coordStr.indexOf('.');
  bool likelyDegMin = (dotPos > 3) || (fabsf(raw) > (isLat ? 90.0f : 180.0f));
  if (likelyDegMin) {
    float absRaw = fabsf(raw);
    float degrees = floorf(absRaw / 100.0f);
    float minutes = absRaw - (degrees * 100.0f);
    value = degrees + (minutes / 60.0f);
  }

  if (dir == "S" || dir == "W") {
    value = -fabsf(value);
  } else if (dir == "N" || dir == "E") {
    value = fabsf(value);
  }

  return value;
}

bool parseCgNssInfo(const String &rawInput, ParsedGnssPosition &out) {
  String raw = normalizeGnssRaw(rawInput);
  if (raw.length() == 0) {
    return false;
  }

  String fields[20];
  int fieldCount = 0;
  int startPos = 0;
  for (int i = 0; i <= raw.length() && fieldCount < 20; i++) {
    if (i == raw.length() || raw.charAt(i) == ',') {
      fields[fieldCount] = raw.substring(startPos, i);
      fields[fieldCount].trim();
      fieldCount++;
      startPos = i + 1;
    }
  }

  if (fieldCount == 0) {
    return false;
  }

  out.fixQuality = fields[0].toInt();
  if (out.fixQuality <= 0) {
    return false;
  }

  int latIndex = -1;
  for (int i = 5; (i + 1) < fieldCount; i++) {
    if (fields[i].length() > 0 && isLatDir(fields[i + 1])) {
      latIndex = i;
      break;
    }
  }
  if (latIndex < 0) {
    return false;
  }

  const int lonIndex = latIndex + 2;
  const int dateIndex = lonIndex + 2;
  const int altIndex = dateIndex + 2;
  const int speedIndex = dateIndex + 3;
  const int hdopIndex = dateIndex + 5;
  const int satUsedIndex = dateIndex + 8;

  if ((lonIndex + 1) >= fieldCount || !isLonDir(fields[lonIndex + 1])) {
    return false;
  }

  out.latitude = parseGnssCoord(fields[latIndex], fields[latIndex + 1], true);
  out.longitude = parseGnssCoord(fields[lonIndex], fields[lonIndex + 1], false);
  if (fabsf(out.latitude) > 90.0f || fabsf(out.longitude) > 180.0f) {
    return false;
  }

  out.altitude = (altIndex < fieldCount && fields[altIndex].length() > 0)
                   ? fields[altIndex].toFloat()
                   : 0.0f;
  out.speedKmh = (speedIndex < fieldCount && fields[speedIndex].length() > 0)
                   ? (fields[speedIndex].toFloat() * 1.852f)
                   : 0.0f;
  if (out.speedKmh < 4.0f) {
    out.speedKmh = 0.0f;
  }
  out.hdop = (hdopIndex < fieldCount && fields[hdopIndex].length() > 0)
               ? fields[hdopIndex].toFloat()
               : 0.0f;
  out.satellites =
    (satUsedIndex < fieldCount && fields[satUsedIndex].length() > 0)
      ? fields[satUsedIndex].toInt()
      : 0;
  if (out.satellites == 0 && fieldCount > 1 && fields[1].length() > 0) {
    out.satellites = fields[1].toInt();
  }

  return true;
}

std::string buildPositionString(const ParsedGnssPosition &parsed) {
  std::string position = floatToString(parsed.latitude, 7) + ';';
  position += floatToString(parsed.longitude, 7) + ';';
  position += floatToString(parsed.altitude, 1) + ';';
  position += floatToString(parsed.speedKmh, 1) + ';';
  position += ";";
  position += std::to_string(parsed.satellites) + ';';
  position += floatToString(parsed.hdop, 1) + ';';
  return position;
}
} // namespace

void GNSS::initGNSS(int mode) {
#ifdef USE_ASYNC_A7670
  modemAsync.gnssEnabled = false;
  modemAsync.enableGnss();
  waitForAsyncBool(modemAsync.gnssEnabled, true, 10000);
  modemAsync.setGnssMode(mode + 1);
  vTaskDelay(500 / portTICK_PERIOD_MS);
#else
  modem.enableGPS();
  delay(5000);
  modem.setGNSSMode(mode + 1);
  delay(3000);
#endif
  debugPrint("Setting GNSS mode to %d\n", mode + 1);
}

std::string GNSS::getPosition() {
  const std::string emptyPosition = ";;;;;;;";

  if (hubSettings.gnss_use_static) {
    char position[50];
    snprintf(position, sizeof(position), "%.7f;%.7f;%d;;;;;",
             hubSettings.gnss_static_lat, hubSettings.gnss_static_lon,
             hubSettings.gnss_static_alt);
    return std::string(position);
  }

  if (hubSettings.gnss_en) {
#ifdef USE_ASYNC_A7670
    if (!modemAsync.isGnssEnabled()) {
      debugPrint("GNSS not enabled\n");
      initGNSS(hubSettings.gnss_mode);
      if (!modemAsync.isGnssEnabled()) {
        debugPrint("Failed to enable GNSS\n");
        return emptyPosition;
      }
    }
    modemAsync.gnssDataReceived = false;
    bool reqAccepted = modemAsync.getGnssData();
    if (reqAccepted) {
      waitForAsyncBool(modemAsync.gnssDataReceived, true, kGnssAsyncWaitMs);
    }

    String rawSnapshot = modemAsync.gnssData.rawData;
    ParsedGnssPosition parsed;
    if (parseCgNssInfo(rawSnapshot, parsed)) {
      std::string position = buildPositionString(parsed);
      s_lastValidPosition = position;
      s_lastValidPositionMs = millis();
      return position;
    }

    // If GNSS is temporarily unavailable or modem is busy, reuse the last valid
    // fix briefly.
    if ((millis() - s_lastValidPositionMs) <= kGnssFixCacheTtlMs) {
      return s_lastValidPosition;
    }
    return emptyPosition;

#else
    String rawGnssData = modem.getGPSraw();
    std::string gnssData = std::string(rawGnssData.c_str());
    debugPrint("Raw GNSS Data: %s\n", gnssData.c_str());
    if (rawGnssData.length() == 0 || rawGnssData == ",,,,,,,,") {
      return emptyPosition;
    }

    ParsedGnssPosition parsed;
    std::string position = emptyPosition;
    if (parseCgNssInfo(rawGnssData, parsed)) {
      position = buildPositionString(parsed);
      s_lastValidPosition = position;
      s_lastValidPositionMs = millis();
    } else if ((millis() - s_lastValidPositionMs) <= kGnssFixCacheTtlMs) {
      position = s_lastValidPosition;
    }

    debugPrint("Position: %s\n", position.c_str());
    return position;
#endif
  } else {
    return emptyPosition;
  }
}

int GNSS::findCharIndex(const char *str, char ch) {
  const char *found = strchr(str, ch);
  if (found != NULL) {
    return found - str; // Calculate and return the index
  }
  return -1; // Return -1 if the character is not found
}
