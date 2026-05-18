// MessageBusLayrzHub.h - Centralized message dispatch & backlog replay
#pragma once
#ifndef LAYRZ_MESSAGE_BUS_LAYRZHUB_H
#define LAYRZ_MESSAGE_BUS_LAYRZHUB_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <modules/blackbox/BlackBoxLayrzHub.h>
#include <modules/global_objects/GlobalObjectsLayrzHub.h>
#include <modules/layrz_protocol/LinkLayrzHub.h>
#include <modules/messaging/ClientSelectorLayrzHub.h>
#include <modules/settings/SettingsLayrzHub.h>

// Classification of messages (expand as needed)
enum class BusMsgKind : uint8_t {
  PdSensor,   // <Pd>
  PcAck,      // <Pc>
  PsSettings, // <Ps>
  PiInfo,     // <Pi>
  Media,      // <Pm> images/audio
  BleSensor,  // <Pb> BLE sensor data
  RiTicket,   // RI505 ticket JSON
  Custom
};

// Runtime message instance placed onto queue.
// Ownership of 'data' passes to the bus on successful publish.
struct BusMessage {
  char *data = nullptr; // Null-terminated payload
  size_t len = 0;       // Length excluding null terminator
  BusMsgKind kind = BusMsgKind::Custom;
  bool persistIfFail = true; // Persist to blackbox on send failure
  bool freeAfterSend = true; // Free data buffer after send/persist
  bool skipHistory = false;  // Skip history.txt even when sys_hist_en=true
};

class MessageBusLayrzHub {
public:
  struct Config {
    size_t realtimeQueueDepth; // Realtime queue depth
    size_t backlogBurstMax;    // Max backlog lines per cycle
    size_t realtimeBurstMax;   // Max realtime msgs per cycle before yielding
    size_t realtimeLowWater;   // Backlog replay allowed when queue <= this
    uint32_t idleDelayMs;      // Delay when nothing to process
  };

  // Initialize bus (call once after BlackBox ready). Returns false on
  // allocation failure.
  static bool begin(BlackBoxLayrzHub *blackbox, const Config &cfg);

  // Enqueue a message (takes ownership of 'data' on success). Falls back to
  // blackbox if queue full and persistIfFail.
  static bool publish(const BusMessage &msg);

  // Helper allocation in PSRAM (len written if outLen != nullptr). Returns null
  // on OOM.
  static char *allocAndCopy(const char *src, size_t *outLen = nullptr);

  // Access underlying blackbox (may be nullptr)
  static BlackBoxLayrzHub *blackbox() { return _bb; }

  // Obtain a default configuration.
  static Config defaultConfig();

  // Exposed TCP socket reception task (migrated from LayrzProtocol)
  static void tcpSocketReception(void *arg);

private:
  static void senderTask(void *arg);
  static void riTicketTask(void *arg);
  static bool sendOne(const BusMessage &m); // Attempt a network send
  static void freeMsg(const BusMessage &m);
  static bool networkReady();
  static bool sendRiTicketHttp(const char *payload, size_t len);
  static bool initRiTicketBlackbox();
  static bool enqueueRiTicketBlackbox(const char *payload, size_t len);
  static bool riNetworkAvailable();
  static bool riEndpointConfigured();
  static bool riMemoryAvailable();
  static void pauseSocketForHttp();
  static void resumeSocketAfterHttp();

  // State
  static QueueHandle_t _rtQueue;
  static QueueHandle_t _riQueue;
  static BlackBoxLayrzHub *_bb;
  static BlackBoxLayrzHub *_riBb;
  static Config _cfg;
  static TaskHandle_t _senderHandle;
  static TaskHandle_t _riHandle;
};

#endif // LAYRZ_MESSAGE_BUS_LAYRZHUB_H
