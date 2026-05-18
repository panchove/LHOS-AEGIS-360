#include "BlackBoxLayrzHub.h"

#include <algorithm>
#include <cstdio>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>

BlackBoxLayrzHub::BlackBoxLayrzHub(fs::FS &fs)
    : BlackBoxLayrzHub(fs, Config{}) {} // delegate to the real ctor

BlackBoxLayrzHub::BlackBoxLayrzHub(fs::FS &fs, const Config &cfg)
    : _fs(fs), _cfg(cfg) {
  _mtx = xSemaphoreCreateMutex();
  if (_cfg.syncEveryN == 0)
    _cfg.syncEveryN = 1; // guard against misconfig
}

String BlackBoxLayrzHub::segName(uint32_t seg) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%s/%06u.log", _cfg.baseDir, seg);
  return String(buf);
}

bool BlackBoxLayrzHub::ensureDir(const char *dir) {
  if (_fs.exists(dir))
    return true;
  return _fs.mkdir(dir);
}

bool BlackBoxLayrzHub::atomicWriteText(const char *path, const String &text) {
  String tmp = String(path) + ".tmp";
  File f = _fs.open(tmp, FILE_WRITE);
  if (!f)
    return false;
  size_t w = f.print(text);
  f.flush(); // Arduino’s flush commits to FatFS/VFS
  f.close();
  if (w != text.length()) {
    _fs.remove(tmp);
    return false;
  }
  if (_fs.exists(path))
    _fs.remove(path);
  if (!_fs.rename(tmp, path)) {
    _fs.remove(tmp);
    return false;
  }
  return true;
}

bool BlackBoxLayrzHub::persistMeta() {
  // CSV: read_seg,read_off,write_seg,write_sz
  String s = String(state.read_seg) + "," + String(state.read_off) + "," +
             String(state.write_seg) + "," + String(state.write_sz) + "\n";
  return atomicWriteText(_cfg.metaPath, s);
}

bool BlackBoxLayrzHub::discoverSegments(uint32_t &minSeg, uint32_t &maxSeg) {
  minSeg = UINT32_MAX;
  maxSeg = 0;
  File dir = _fs.open(_cfg.baseDir);
  if (!dir)
    return false;
  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      String name = f.name();
      int slash = name.lastIndexOf('/');
      if (slash >= 0)
        name.remove(0, slash + 1);
      if (name.length() == 10 && name.endsWith(".log")) {
        String num = name.substring(0, 6);
        bool allDigits = true;
        for (int i = 0; i < (int)num.length(); ++i) {
          if (!isDigit(num[i])) {
            allDigits = false;
            break;
          }
        }
        if (allDigits) {
          uint32_t seg = num.toInt();
          if (seg > 0) {
            minSeg = std::min(minSeg, seg);
            maxSeg = std::max(maxSeg, seg);
          }
        }
      }
    }
    f.close();
    f = dir.openNextFile();
  }
  dir.close();
  if (maxSeg == 0) {
    minSeg = 1;
    maxSeg = 1;
  }
  return true;
}

bool BlackBoxLayrzHub::trimLastPartialLine(File &f) {
  if (!f)
    return false;
  String path = String(f.name()); // capture path before close
  size_t sz = f.size();
  if (sz == 0)
    return true;
  const size_t window = sz > 4096 ? 4096 : sz;
  size_t start = sz - window;
  f.seek(start);
  String chunk = f.readString();
  int lastNL = chunk.lastIndexOf('\n');
  if (lastNL == -1)
    return true; // no newline in window; keep as-is
  size_t lastGood = start + lastNL + 1;
  if (lastGood == sz)
    return true; // already ended with newline
  // Truncate to lastGood
  f.close();
  // Re-open for write to truncate (copy & replace approach)
  File src = _fs.open(path, FILE_READ);
  if (!src)
    return false;
  String tmp = path + ".trim";
  _fs.remove(tmp);
  File dst = _fs.open(tmp, FILE_WRITE);
  if (!dst) {
    src.close();
    return false;
  }
  const size_t BUFSZ = 1024;
  uint8_t buf[BUFSZ];
  size_t remaining = lastGood;
  bool copyOk = true;
  while (remaining) {
    size_t n = remaining > BUFSZ ? BUFSZ : remaining;
    size_t r = src.read(buf, n);
    if (r == 0) {
      copyOk = false;
      break;
    }
    dst.write(buf, r);
    remaining -= r;
  }
  dst.flush();
  dst.close();
  src.close();
  if (!copyOk || remaining != 0) {
    _fs.remove(tmp);
    return false;
  }
  if (_fs.exists(path))
    _fs.remove(path);
  if (!_fs.rename(tmp, path)) {
    _fs.remove(tmp);
    return false;
  }
  return true;
}

bool BlackBoxLayrzHub::openWriteSegment(uint32_t seg, bool append) {
  if (_wf)
    _wf.close();
  String path = segName(seg);
  _wf = _fs.open(path, append ? FILE_APPEND : FILE_WRITE);
  if (!_wf)
    return false;
  if (append) {
    state.write_sz = _wf.size();
  } else {
    state.write_sz = 0;
  }
  return true;
}

bool BlackBoxLayrzHub::openReadSegment(uint32_t seg) {
  if (_rf)
    _rf.close();
  String path = segName(seg);
  _rf = _fs.open(path, FILE_READ);
  if (!_rf) {
    // If missing, create an empty file then reopen for read to avoid VFS error
    // spam
    File create = _fs.open(path, FILE_WRITE);
    if (create) {
      create.flush();
      create.close();
    }
    _rf = _fs.open(path, FILE_READ);
    if (!_rf)
      return false;
  }
  // If needed, trim partial tail for safety
  trimLastPartialLine(_rf); // trim function internally uses open/close again
  _rf = _fs.open(path, FILE_READ);
  return _rf;
}

bool BlackBoxLayrzHub::loadOrRebuildMeta() {
  if (_fs.exists(_cfg.metaPath)) {
    File f = _fs.open(_cfg.metaPath, FILE_READ);
    String s = f.readString();
    f.close();
    int c1 = s.indexOf(',');
    int c2 = s.indexOf(',', c1 + 1);
    int c3 = s.indexOf(',', c2 + 1);
    if (c1 > 0 && c2 > c1 && c3 > c2) {
      state.read_seg = s.substring(0, c1).toInt();
      state.read_off = s.substring(c1 + 1, c2).toInt();
      state.write_seg = s.substring(c2 + 1, c3).toInt();
      state.write_sz = s.substring(c3 + 1).toInt();
    }
  }

  uint32_t minSeg, maxSeg;
  if (!discoverSegments(minSeg, maxSeg))
    return false;

  // If meta was missing/corrupt, rebuild conservative state
  if (state.write_seg < minSeg || state.write_seg > maxSeg)
    state.write_seg = maxSeg;
  if (state.read_seg < minSeg || state.read_seg > maxSeg)
    state.read_seg = minSeg;

  // Ensure open handles and sizes
  if (!_fs.exists(segName(state.write_seg))) {
    // create first segment
    openWriteSegment(state.write_seg, false);
  } else {
    openWriteSegment(state.write_seg, true);
  }

  if (_fs.exists(segName(state.read_seg))) {
    if (!openReadSegment(state.read_seg)) {
      // Fallback: create a fresh segment then reopen
      openWriteSegment(state.read_seg, false);
      openReadSegment(state.read_seg);
    }
    if (_rf && state.read_off > _rf.size())
      state.read_off = 0; // clamp
  } else {
    // create the read seg if nothing exists yet
    // ensure a write file exists first
    if (!_fs.exists(segName(state.write_seg))) {
      openWriteSegment(state.write_seg, false);
    }
    openReadSegment(state.write_seg);
    state.read_seg = state.write_seg;
    state.read_off = 0;
  }
  persistMeta();
  return true;
}

bool BlackBoxLayrzHub::begin() {
  if (!ensureDir(_cfg.baseDir))
    return false;
  return loadOrRebuildMeta();
}

bool BlackBoxLayrzHub::rotateIfNeeded() {
  if (state.write_sz < _cfg.segmentMaxBytes)
    return true;
  _wf.flush();
  _wf.close();
  state.write_seg++;
  state.write_sz = 0;
  bool ok = openWriteSegment(state.write_seg, false) && persistMeta();
  return ok;
}

bool BlackBoxLayrzHub::enqueue(const char *msg, size_t len) {
  if (!msg || len == 0)
    return false;

  // Take shared SPI bus first (to avoid inversion with other users like
  // camera), then internal mutex.
  if (spiBusMutex)
    xSemaphoreTake(spiBusMutex, portMAX_DELAY);
  xSemaphoreTake(_mtx, portMAX_DELAY);

  bool ok = true;

  // Pre-rotation: enforce strict bound before write so segment never exceeds
  // limit
  if (_wf && (state.write_sz + len + 1) >= _cfg.segmentMaxBytes) {
    _wf.flush();
    _wf.close();
    state.write_seg++;
    state.write_sz = 0;
    if (!openWriteSegment(state.write_seg, false)) {
      ok = false;
    } else {
      persistMeta();
    }
  }

  if (ok && !_wf) {
    if (!openWriteSegment(state.write_seg, true))
      ok = false;
  }

  size_t w1 = 0;
  bool needsNL = false;
  if (ok && _wf) {
    w1 = _wf.write((const uint8_t *)msg, len);
    needsNL = (len == 0 || msg[len - 1] != '\n');
    if (needsNL)
      _wf.write((uint8_t *)"\n", 1);
    state.write_sz += w1 + (needsNL ? 1 : 0);
  } else {
    ok = false;
  }

  // Sync & meta persistence cadence
  if (ok) {
    _appendsSinceSync++;
    if (_appendsSinceSync >= _cfg.syncEveryN) {
      if (_wf)
        _wf.flush();
      persistMeta();
      _appendsSinceSync = 0;
    }
  }

  // Determine final success (full write + rotation logic)
  if (ok) {
    if (w1 != len)
      ok = false; // partial write
    if (ok)
      ok = rotateIfNeeded();
  }

  // Debug instrumentation (guarded by sys_debug_en to reduce noise)
  if (hubSettings.sys_debug_en) {
    debugPrint("BB enqueue len=%u wrote=%u seg=%u sz=%u nl=%d ok=%d\n",
               (unsigned)len, (unsigned)w1, (unsigned)state.write_seg,
               (unsigned)state.write_sz, (int)needsNL, (int)ok);
  }

  xSemaphoreGive(_mtx);
  if (spiBusMutex)
    xSemaphoreGive(spiBusMutex);
  return ok;
}

bool BlackBoxLayrzHub::hasData() {
  xSemaphoreTake(_mtx, portMAX_DELAY);
  bool any = false;
  if (!_rf)
    openReadSegment(state.read_seg);
  if (_rf) {
    if (state.read_seg == state.write_seg && _wf) {
      _wf.flush();
    }
    size_t sz = _rf.size();
    // If writer advanced but read handle size stale, trust write_sz
    if (state.read_seg == state.write_seg && state.write_sz > sz)
      sz = state.write_sz;
    if (state.read_off > sz) {
      // Clamp impossible offset (power loss or corruption)
      state.read_off = 0;
      persistMeta();
    }
    any = (state.read_seg < state.write_seg) || (state.read_off < sz);
  }
  xSemaphoreGive(_mtx);
  return any;
}

bool BlackBoxLayrzHub::readNext(String &outLine) {
  outLine = "";
  xSemaphoreTake(_mtx, portMAX_DELAY);
  int safety = 0;
  for (;;) {
    if (!_rf && !openReadSegment(state.read_seg)) {
      xSemaphoreGive(_mtx);
      return false;
    }
    // Corruption salvage: if at start and file contains no newline at all,
    // append one so readStringUntil returns full content
    if (state.read_off == 0 && _rf) {
      size_t szScan = _rf.size();
      if (szScan > 0) {
        const size_t scanWindow =
          szScan < 4096
            ? szScan
            : 4096; // scan first 4KB for newline (enough for detection)
        bool foundNL = false;
        size_t savedPos = _rf.position();
        _rf.seek(0);
        for (size_t i = 0; i < scanWindow; ++i) {
          int c = _rf.read();
          if (c < 0)
            break;
          if (c == '\n') {
            foundNL = true;
            break;
          }
        }
        _rf.seek(savedPos);
        if (!foundNL) {
          String path = segName(state.read_seg);
          // Append a newline to convert entire segment into one logical line
          File fix = _fs.open(path, FILE_APPEND);
          if (fix) {
            fix.write((uint8_t *)"\n", 1);
            fix.flush();
            fix.close();
          }
          // Reopen read handle to refresh size
          _rf.close();
          openReadSegment(state.read_seg);
          // Adjust write size if this is also the active write segment so size
          // checks stay consistent
          if (state.read_seg == state.write_seg) {
            if (_rf)
              state.write_sz = _rf.size();
            persistMeta();
          }
        }
      }
    }
    // Refresh read handle if writer grew beyond visible size (common on some SD
    // libs)
    if (state.read_seg == state.write_seg && _rf.size() < state.write_sz) {
      _rf.close();
      openReadSegment(state.read_seg);
    }
    // Guard: if seek fails (rare), reset offset to 0 once
    if (!_rf.seek(state.read_off)) {
      if (state.read_off != 0) {
        state.read_off = 0;
        persistMeta();
        if (!_rf.seek(0)) {
          xSemaphoreGive(_mtx);
          return false;
        }
      } else {
        xSemaphoreGive(_mtx);
        return false;
      }
    }

    size_t visibleSize = _rf.size();
    if (state.read_seg == state.write_seg && state.write_sz > visibleSize)
      visibleSize = state.write_sz;

    // If at or beyond end of this segment and there are newer segments, advance
    // automatically
    if (state.read_off >= visibleSize) {
      if (state.read_seg < state.write_seg) {
        String path = segName(state.read_seg);
        _rf.close();
        _fs.remove(path); // remove fully consumed/empty segment
        state.read_seg++;
        state.read_off = 0;
        persistMeta();
        if (++safety > 64) {
          xSemaphoreGive(_mtx);
          return false;
        } // safety guard
        continue; // reattempt on next segment
      } else {
        xSemaphoreGive(_mtx);
        return false; // nothing to read
      }
    }

    size_t beforePos = _rf.position();
    outLine = _rf.readStringUntil('\n');
    size_t afterPos = _rf.position();
    if (outLine.length() == 0) {
      bool atEof = (afterPos >= visibleSize);
      if (!atEof && afterPos == beforePos) {
        // No progress: fallback to manual character read (robust against Stream
        // timeout issues)
        String manual;
        int chars = 0;
        const int MAX_MANUAL = 2048;
        int c;
        while (chars < MAX_MANUAL && (c = _rf.read()) >= 0) {
          afterPos = _rf.position();
          if (c == '\n')
            break;
          manual += (char)c;
          chars++;
        }
        if (manual.length() > 0 || (_rf.position() > beforePos)) {
          outLine = manual; // success via manual path (may include CR if CRLF)
          xSemaphoreGive(_mtx);
          return true;
        }
      }
      if (atEof) {
        // EOF with empty line: advance if more segments else return false
        if (state.read_seg < state.write_seg) {
          continue; // loop will advance
        }
        xSemaphoreGive(_mtx);
        return false;
      }
      // Empty line (just a newline). Treat as valid blank line so caller can
      // commit 1 byte.
      if (afterPos > beforePos) {
        xSemaphoreGive(_mtx);
        return true;
      }
      // Still no progress: attempt meta rebuild once
      uint32_t oldSeg = state.read_seg;
      size_t oldOff = state.read_off;
      size_t sz = _rf.size();
      xSemaphoreGive(_mtx); // release while rebuilding
      loadOrRebuildMeta();
      xSemaphoreTake(_mtx, portMAX_DELAY);
      if (state.read_seg == oldSeg && state.read_off == oldOff && sz > 0) {
        // Could not make progress; give up this cycle
        xSemaphoreGive(_mtx);
        return false;
      }
      continue; // retry with rebuilt state
    }
    xSemaphoreGive(_mtx);
    return true;
  }
}

size_t BlackBoxLayrzHub::currentReadFileSize() {
  xSemaphoreTake(_mtx, portMAX_DELAY);
  size_t sz = 0;
  if (_rf)
    sz = _rf.size();
  xSemaphoreGive(_mtx);
  return sz;
}

size_t BlackBoxLayrzHub::backlogBytes() {
  xSemaphoreTake(_mtx, portMAX_DELAY);
  uint32_t minSeg, maxSeg;
  discoverSegments(minSeg, maxSeg); // fast scan
  size_t total = 0;
  for (uint32_t s = minSeg; s <= maxSeg; ++s) {
    String path = segName(s);
    if (_fs.exists(path)) {
      File f = _fs.open(path, FILE_READ);
      if (f) {
        total += f.size();
        f.close();
      }
    }
  }
  xSemaphoreGive(_mtx);
  return total;
}

size_t BlackBoxLayrzHub::historyBytes() {
  xSemaphoreTake(_mtx, portMAX_DELAY);
  // Ensure pending buffered history data is flushed so size reflects recent
  // appends.
  if (_hf && _histSinceSync > 0) {
    _hf.flush();
    _histSinceSync = 0; // treat as synced
  }
  size_t sz = 0;
  if (_fs.exists(_cfg.historyPath)) {
    File f = _fs.open(_cfg.historyPath, FILE_READ);
    if (f) {
      sz = f.size();
      f.close();
    }
  }
  xSemaphoreGive(_mtx);
  return sz;
}

uint64_t BlackBoxLayrzHub::mediaTotalBytes() {
  // Use ESP32 SD cardSize() (bytes). Returns 0 if unavailable.
  uint64_t sz = 0;
#ifdef ARDUINO_ARCH_ESP32
  sz = SD.cardSize();
#endif
  return sz;
}

bool BlackBoxLayrzHub::deleteBacklog() {
  xSemaphoreTake(_mtx, portMAX_DELAY);
  uint32_t minSeg, maxSeg;
  discoverSegments(minSeg, maxSeg);
  for (uint32_t s = minSeg; s <= maxSeg; ++s) {
    String path = segName(s);
    _fs.remove(path);
  }
  // reset state
  state.read_seg = state.write_seg = 1;
  state.read_off = state.write_sz = 0;
  if (_rf) {
    _rf.close();
  }
  if (_wf) {
    _wf.close();
  }
  openWriteSegment(1, false);
  openReadSegment(1);
  persistMeta();
  xSemaphoreGive(_mtx);
  return true;
}

bool BlackBoxLayrzHub::deleteHistory() {
  xSemaphoreTake(_mtx, portMAX_DELAY);
  if (_hf) {
    _hf.flush();
    _hf.close();
  }
  bool ok = !_cfg.historyPath || !_fs.exists(_cfg.historyPath) ||
            _fs.remove(_cfg.historyPath);
  _hf = _fs.open(_cfg.historyPath, FILE_APPEND); // recreate empty file silently
  xSemaphoreGive(_mtx);
  return ok;
}

bool BlackBoxLayrzHub::formatMediaFAT32() {
  // Logical wipe: close all open files, delete managed directories/files,
  // recreate pristine structure. Do everything under mutex to avoid other tasks
  // touching handles mid-format (e.g., sensor publisher, protocol senders).
  xSemaphoreTake(_mtx, portMAX_DELAY);
  // Close any open files first so FS operations won't act on live handles.
  if (_wf) {
    _wf.flush();
    _wf.close();
  }
  if (_rf) {
    _rf.close();
  }
  if (_hf) {
    _hf.flush();
    _hf.close();
  }

  // Remove backlog directory tree (segments + meta) safely.
  recursiveDelete(_cfg.baseDir);
  // Remove history file explicitly (may live outside baseDir).
  if (_cfg.historyPath && _fs.exists(_cfg.historyPath)) {
    _fs.remove(_cfg.historyPath);
  }

  // Recreate directory structure.
  bool dirOk = ensureDir(_cfg.baseDir);

  // Reset internal state.
  state.read_seg = state.write_seg = 1;
  state.read_off = state.write_sz = 0;
  _appendsSinceSync = 0;
  _commitsSinceSync = 0;
  _histSinceSync = 0;

  // Reopen fresh segment & history file (don't append old data).
  bool segOk = openWriteSegment(1, false) && openReadSegment(1);
  _hf = _fs.open(
    _cfg.historyPath,
    FILE_APPEND);    // create empty history (append mode for future writes)
  bool histOk = _hf; // File evaluates to bool (open success)

  bool metaOk = persistMeta();

  bool ok = dirOk && segOk && histOk && metaOk;
  xSemaphoreGive(_mtx);
  return ok;
}

bool BlackBoxLayrzHub::recursiveDelete(const char *path) {
  if (!_fs.exists(path))
    return true;
  File dir = _fs.open(path);
  if (!dir)
    return false;
  if (!dir.isDirectory()) {
    dir.close();
    return _fs.remove(path);
  }
  File f = dir.openNextFile();
  while (f) {
    String name = f.name();
    bool isDir = f.isDirectory();
    // Capture before close
    String fullPath = name;
    // Some cores return relative names; normalize if needed
    if (!fullPath.startsWith("/")) {
      if (String(path).endsWith("/"))
        fullPath = String(path) + fullPath;
      else
        fullPath = String(path) + "/" + fullPath;
    }
    f.close();
    if (isDir) {
      recursiveDelete(fullPath.c_str());
    } else {
      _fs.remove(fullPath);
    }
    f = dir.openNextFile();
  }
  dir.close();
  // Finally remove the now-empty directory (ignore failure for root-like paths)
  if (strcmp(path, "/") != 0) {
    _fs.rmdir(path);
  }
  return true;
}

bool BlackBoxLayrzHub::commitRead(size_t bytesAdvanced) {
  xSemaphoreTake(_mtx, portMAX_DELAY);
  if (!_rf) {
    xSemaphoreGive(_mtx);
    return false;
  }
  state.read_off += bytesAdvanced;
  _commitsSinceSync++;
  // If reached end, delete segment and move to next
  if (state.read_off >= _rf.size()) {
    String path = segName(state.read_seg);
    _rf.close();
    _fs.remove(path);
    if (state.read_seg < state.write_seg) {
      state.read_seg++;
      state.read_off = 0;
      openReadSegment(state.read_seg); // ok if absent
    } else {
      // stay at current index; writer not advanced; nothing left
      state.read_off = 0;
    }
  }
  if (_commitsSinceSync >= _cfg.syncEveryN) {
    persistMeta();
    _commitsSinceSync = 0;
  }
  xSemaphoreGive(_mtx);
  return true;
}

bool BlackBoxLayrzHub::appendToHistoryLocked(const char *data, size_t len) {
  if (!_hf)
    _hf = _fs.open(_cfg.historyPath, FILE_APPEND);
  if (!_hf)
    return false;
  _hf.write((const uint8_t *)data, len);
  if (len == 0 || data[len - 1] != '\n')
    _hf.write((uint8_t *)"\n", 1);

  if (++_histSinceSync >= _cfg.syncEveryN) {
    _hf.flush();
    _histSinceSync = 0;
  }
  // History size management
  if (_cfg.historyMaxBytes > 0) {
    size_t sz = _hf.size();
    if (sz > _cfg.historyMaxBytes) {
      _hf.flush();
      _hf.close();
      String oldPath = String(_cfg.historyPath) + ".old";
      _fs.remove(oldPath);
      if (_fs.exists(_cfg.historyPath))
        _fs.rename(_cfg.historyPath, oldPath.c_str());
      _hf = _fs.open(_cfg.historyPath, FILE_WRITE); // new empty
      _histSinceSync = 0;
    }
  }
  return true;
}

// bool BlackBoxLayrzHub::appendToHistory(const char* data, size_t len) {
bool BlackBoxLayrzHub::appendToHistory(const char *data, size_t len) {
  if (!data)
    return false;
  xSemaphoreTake(_mtx, portMAX_DELAY);
  bool ok = appendToHistoryLocked(data, len);
  xSemaphoreGive(_mtx);
  return ok;
}

void BlackBoxLayrzHub::closeFiles() {
  xSemaphoreTake(_mtx, portMAX_DELAY);
  if (_wf) {
    _wf.flush();
    _wf.close();
  }
  if (_rf) {
    _rf.close();
  }
  if (_hf) {
    _hf.flush();
    _hf.close();
  }
  persistMeta();
  xSemaphoreGive(_mtx);
}

BlackBoxLayrzHub::~BlackBoxLayrzHub() {
  closeFiles();
  if (_mtx) {
    vSemaphoreDelete(_mtx);
    _mtx = nullptr;
  }
}

bool BlackBoxLayrzHub::readAndCommitNext(String &outLine) {
  if (!readNext(outLine))
    return false;
  size_t bytes = outLine.length();
  // Each stored line ends with a newline enforced at enqueue
  bytes += 1; // account for '\n'
  return commitRead(bytes);
}
