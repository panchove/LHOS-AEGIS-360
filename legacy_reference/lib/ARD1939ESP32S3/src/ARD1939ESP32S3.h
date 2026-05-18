#ifndef ARD1939ESP32S3_H
#define ARD1939ESP32S3_H

#include "CAN.h"

#include <Arduino.h> // Arduino framework

// System Configuration
#define SYSTEM_TIME 1 // Default system time in milliseconds

// Message Types
#define J1939_MSG_NONE 0
#define J1939_MSG_PROTOCOL 1
#define J1939_MSG_APP 2
#define J1939_MSG_NETWORKDATA 3

// Address Claim Status
#define ADDRESSCLAIM_INPROGRESS 0
#define ADDRESSCLAIM_FAILED 1
#define ADDRESSCLAIM_FINISHED 2
#define NORMALDATATRAFFIC 2

// Constants
#define SA_PREFERRED 0x40
#define ADDRESSRANGEBOTTOM 129
#define ADDRESSRANGETOP 247
#define GLOBALADDRESS 0xFF
#define NULLADDRESS 0xFE
#define MAX_MESSAGE_FILTERS 100
#define MAX_NAME_LENGTH 8
#define MAX_DATA_LENGTH 8
#define MAX_TRANSPORT_LENGTH 1785

// NAME Fields Default
#define NAME_IDENTITY_NUMBER 0xABCDE1  // Unique device identity
#define NAME_MANUFACTURER_CODE 0x123   // New manufacturer code (was 0xFFF)
#define NAME_FUNCTION_INSTANCE 1       // Changed to 1 (was 0)
#define NAME_ECU_INSTANCE 0x02         // Changed to 2 (was 1)
#define NAME_FUNCTION 0x01             // Changed to 1 (was 0xFF)
#define NAME_RESERVED 0                // Keep as is
#define NAME_VEHICLE_SYSTEM 0x20       // Different system (was 0x7F)
#define NAME_VEHICLE_SYSTEM_INSTANCE 0 // Keep as is
#define NAME_INDUSTRY_GROUP 0x01       // Changed to 1 (was 0)
#define NAME_ARBITRARY_ADDRESS_CAPABLE                                         \
  0x01 // Keep enabled for dynamic address claiming

// Compiler Settings
#define OK 0
#define ERR 1

// Debugger Settings
#define DEBUG 1

#if DEBUG == 1

#define DEBUG_INIT() char sDebug[128];
#define DEBUG_PRINTHEX(T, v)                                                   \
  Serial.print(T);                                                             \
  sprintf(sDebug, "%x\n\r", v);                                                \
  Serial.print(sDebug);
#define DEBUG_PRINTDEC(T, v)                                                   \
  Serial.print(T);                                                             \
  sprintf(sDebug, "%d\n\r", v);                                                \
  Serial.print(sDebug);
#define DEBUG_PRINTARRAYHEX(T, a, l)                                           \
  Serial.print(T);                                                             \
  if (l == 0)                                                                  \
    Serial.print("Empty.\n\r");                                                \
  else {                                                                       \
    for (int x = 0; x < l; x++) {                                              \
      sprintf(sDebug, "%x ", a[x]);                                            \
      Serial.print(sDebug);                                                    \
    }                                                                          \
    Serial.print("\n\r");                                                      \
  }
#define DEBUG_PRINTARRAYDEC(T, a, l)                                           \
  Serial.print(T);                                                             \
  if (l == 0)                                                                  \
    Serial.print("Empty.\n\r");                                                \
  else {                                                                       \
    for (int x = 0; x < l; x++) {                                              \
      sprintf(sDebug, "%d ", a[x]);                                            \
      Serial.print(sDebug);                                                    \
    }                                                                          \
    Serial.print("\n\r");                                                      \
  }
#define DEBUG_HALT()                                                           \
  while (Serial.available() == 0)                                              \
    ;                                                                          \
  Serial.setTimeout(1);                                                        \
  Serial.readBytes(sDebug, 1);

#endif

#define MAX_TRANSPORT_LENGTH 1785
#define MAX_TRANSPORT_PACKETS 255
#define T1_TIMEOUT_MS 200
#define T3_TIMEOUT_MS 125

// Struct for transport protocol message
struct TransportMessage {
  uint32_t pgn;
  byte srcAddr;
  uint16_t messageSize;
  uint8_t numPackets;
  uint8_t receivedPackets;
  byte dataBuffer[1785];    // Max transport length
  bool packetReceived[255]; // Max transport packets
};

struct TransportProtocolState {
  uint32_t pgn = 0;              // PGN of the message being assembled
  byte srcAddr = 0xFF;           // Source address
  uint16_t messageSize = 0;      // Total size of the message
  uint8_t numPackets = 0;        // Total number of packets
  uint8_t receivedPackets = 0;   // Count of received packets
  byte dataBuffer[1785] = {};    // Buffer to store the complete message
  bool packetReceived[255] = {}; // Flags to track received packets
  unsigned long timeout = 0;     // Timeout for the current process
  bool inProgress = false;       // Flag to indicate if a TP process is active
};

// Timer class for timeout management
class Timer {
public:
  void start(int timeoutMs) {
    expiryTime = millis() + timeoutMs;
    active = true;
  }

  bool hasExpired() { return active && (millis() > expiryTime); }

  void stop() { active = false; }

private:
  unsigned long expiryTime = 0;
  bool active = false;
};

// Declare the variables as extern
extern TransportMessage currentTPMessage;
extern TransportProtocolState tpState;
extern Timer transportTimeoutTimer;

struct TimerState {
  int countdown;
  bool isActive;
  bool hasExpired;
};

class ARD1939ESP32S3 {
public:
  // Initialization and Termination
  byte Init(int systemTimeMs, int baudRate);
  void Terminate();

  // Configuration
  void SetPreferredAddress(byte address);
  void SetAddressRange(byte start, byte end);
  void SetNAME(long identityNumber, int manufacturerCode, byte functionInstance,
               byte ecuInstance, byte function, byte vehicleSystem,
               byte vehicleSystemInstance, byte industryGroup,
               byte arbitraryAddressCapable);

  // Communication
  byte Operate(byte *messageType, uint32_t *pgn, byte *data, int *length,
               byte *destAddr, byte *srcAddr, byte *priority);
  byte Transmit(byte priority, uint32_t pgn, byte srcAddr, byte destAddr,
                byte *data, int length);

  // Message Filtering
  byte SetMessageFilter(uint32_t pgn);
  void DeleteMessageFilter(uint32_t pgn);

private:
  // Address Claim Management
  byte HandleAddressClaim(byte srcAddr, byte *incomingName);
  bool AllocateNextAddress();
  byte CompareNAMEs(byte *incomingName, byte *localName);

  // Message Processing
  byte ParseReceivedMessage(uint32_t *pgn, byte *data, int *length,
                            byte *destAddr, byte *srcAddr, byte *priority);

  byte HandleTransportProtocol(byte *data, uint32_t pgn, byte srcAddr);
  byte HandleBAM(byte *data, byte srcAddr);
  byte HandleDT(byte *data, byte srcAddr);
  void CheckTimeouts();
  void ProcessAssembledMessage(uint32_t pgn, byte *data, uint16_t length);

  // Transport Protocol BAM Handling
  byte HandleTransportProtocolBAM(byte srcAddr, const byte *data, int length);

  // Transport Protocol Data Transfer
  byte HandleTransportProtocolDT(byte srcAddr, byte *data, int *length,
                                 uint32_t *pgn);

  TimerState addressClaimTimer, retryTimer;
  // Utilities
  void ResetTimers();
  void InitializeTimer(TimerState *timer);
  bool IsProtocolMessage(uint32_t pgn, byte *data);

  // Internal State
  struct ProtocolState {
    bool inProgress;
    byte currentAddress;
    bool preferredAddressAvailable;
    byte preferredAddress;
    byte addressRangeStart;
    byte addressRangeEnd;
    byte claimedAddress;
    bool addressClaimSuccessful;
    bool addressClaimFailed;
  } protocolState;

  struct MessageFilter {
    bool active;
    long pgn;
  } messageFilters[MAX_MESSAGE_FILTERS];

  byte nodeName[MAX_NAME_LENGTH]; // J1939 NAME field
};

#endif
