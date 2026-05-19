#ifndef _CAN_CONTROLLER_H_
#define _CAN_CONTROLLER_H_

#include <Arduino.h>
#include <stdint.h>

#define CAN_MAX_DLEN 8

struct CANMessage {
  uint32_t id;
  uint8_t length;
  uint8_t data[CAN_MAX_DLEN];
};

class CANControllerClass {
public:
  CANControllerClass() = default;
  virtual ~CANControllerClass() = default;

  virtual bool begin(long baudrate) = 0;
  virtual void end() = 0;
  virtual int available() = 0;
  virtual CANMessage receive() = 0;
  virtual bool send(const CANMessage &message) = 0;

  // Add missing virtual methods:
  virtual int beginPacket(int id, int dlc, bool rtr) = 0;
  virtual int beginExtendedPacket(long id, int dlc, bool rtr) = 0;
  virtual int endPacket() = 0;
  virtual int parsePacket() = 0;
  virtual long packetId() = 0;
  virtual bool packetExtended() = 0;
  virtual bool packetRtr() = 0;
  virtual int packetDlc() = 0;
  virtual size_t write(uint8_t byte) = 0;
  virtual size_t write(const uint8_t *buffer, size_t size) = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  virtual void flush() = 0;
  virtual void onReceive(void (*callback)(int)) = 0;
  virtual int filter(int id, int mask) = 0;
  virtual int filterExtended(long id, long mask) = 0;
  virtual int observe() = 0;
  virtual int loopback() = 0;
  virtual int sleep() = 0;
  virtual int wakeup() = 0;

protected:
  // Transmit-related variables
  bool _packetBegun = false;
  long _txId = -1;
  bool _txExtended = false;
  bool _txRtr = false;
  int _txDlc = 0;
  int _txLength = 0;
  uint8_t _txData[CAN_MAX_DLEN] = {0};

  // Receive-related variables
  long _rxId = -1;
  bool _rxExtended = false;
  bool _rxRtr = false;
  int _rxDlc = 0;
  int _rxLength = 0;
  int _rxIndex = 0;
  uint8_t _rxData[CAN_MAX_DLEN] = {0};

  // Callback function
  void (*_onReceive)(int) = nullptr;
};

#endif // _CAN_CONTROLLER_H_
