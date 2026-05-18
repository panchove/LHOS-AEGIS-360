#pragma once
#ifndef BLACKBOXLAYRZHUB_H
#define BLACKBOXLAYRZHUB_H
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <vector>

// Forward declaration for shared SPI bus mutex (defined in
// GlobalObjectsLayrzHub.cpp)
extern SemaphoreHandle_t spiBusMutex;

class BlackBoxLayrzHub {
public:
  struct Config {
    const char *baseDir = "/blackbox";
    const char *metaPath = "/blackbox/meta.txt";
    const char *historyPath = "/history.txt";
    size_t segmentMaxBytes = 512 * 1024; // rotate at 512 KB
    uint16_t syncEveryN = 1;             // flush every N appends/commits
    bool logOnEnqueue = true; // write enqueued messages to history.txt
    size_t historyMaxBytes =
      0; // 0 = unlimited; if >0 rotate history when size exceeded
  };

  explicit BlackBoxLayrzHub(fs::FS &fs);           // no default arg here
  BlackBoxLayrzHub(fs::FS &fs, const Config &cfg); // explicit config
  bool begin(); // mkdirs, discover/repair state
  bool enqueue(const char *msg,
               size_t len);       // append one line (adds '\n' if not present)
  bool hasData();                 // any unread bytes?
  bool readNext(String &outLine); // peek/read next line at read cursor
  bool
  commitRead(size_t bytesAdvanced); // advance read_off; delete segment if done
  bool readAndCommitNext(
    String &outLine); // convenience: read next line and commit automatically
  bool appendToHistory(const char *data, size_t len);
  void closeFiles();   // flush/close handles (optional)
  ~BlackBoxLayrzHub(); // destructor calls closeFiles
  // Debug / diagnostics
  size_t
  currentReadFileSize(); // size of currently open read segment (0 if none)

  // For telemetry
  uint32_t readSeg() const { return state.read_seg; }
  uint32_t writeSeg() const { return state.write_seg; }
  size_t readOff() const { return state.read_off; }
  size_t writeSz() const { return state.write_sz; }

  // Metrics
  size_t backlogBytes();      // total bytes across all backlog segment files
  size_t historyBytes();      // size of history file
  uint64_t mediaTotalBytes(); // total media capacity (best-effort)
  bool deleteBacklog();       // remove all segment files, reset state
  bool deleteHistory();       // remove history file
  bool formatMediaFAT32();    // attempt full media format (may be stub)
private:
  bool recursiveDelete(const char *path); // helper for pseudo-format

private:
  fs::FS &_fs;
  Config _cfg;

  struct State {
    uint32_t read_seg = 1;
    uint32_t write_seg = 1;
    uint32_t read_off = 0;
    uint32_t write_sz = 0;
  } state;

  File _wf;                    // current write segment handle
  File _rf;                    // current read segment handle
  File _hf;                    // history handle
  uint16_t _histSinceSync = 0; // cadence for history flushes
  bool appendToHistoryLocked(const char *data, size_t len); // no extra locking
  SemaphoreHandle_t _mtx = nullptr;
  uint16_t _appendsSinceSync = 0;
  uint16_t _commitsSinceSync = 0;

  // internals
  bool loadOrRebuildMeta();
  bool persistMeta();
  bool atomicWriteText(const char *path, const String &text);
  String segName(uint32_t seg);
  bool openWriteSegment(uint32_t seg, bool append);
  bool openReadSegment(uint32_t seg);
  bool rotateIfNeeded();
  bool trimLastPartialLine(File &f); // power-loss safety
  bool discoverSegments(uint32_t &minSeg, uint32_t &maxSeg);
  bool ensureDir(const char *dir);
  bool rotateHistoryIfNeeded();
};

#endif // BLACKBOXLAYRZHUB_H