#ifndef _ESP32_TWAI_H_
#define _ESP32_TWAI_H_

#include "CANController.h"

#include <driver/twai.h>

#define TWAI_TX_PIN GPIO_NUM_1
#define TWAI_RX_PIN GPIO_NUM_2

class ESP32TWAIClass : public CANControllerClass {
public:
  ESP32TWAIClass();

  // Core CAN functionality
  bool begin(long baudrate) override;
  void end() override;
  int available() override;
  CANMessage receive() override;
  bool send(const CANMessage &message) override;

  // Packet management
  int beginPacket(int id, int dlc, bool rtr) override;
  int beginPacket(int id, int dlc); // Overloaded for single parameter
  int beginExtendedPacket(long id, int dlc, bool rtr) override;
  int beginExtendedPacket(long id, int dlc); // Overloaded for single parameter
  int endPacket() override;
  int parsePacket() override;
  long packetId() override;
  bool packetExtended() override;
  bool packetRtr() override;
  int packetDlc() override;

  // Data handling
  size_t write(uint8_t byte) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  int read() override;
  size_t readBytes(uint8_t *buffer, size_t length);
  int peek() override;
  void flush() override;

  // Filtering
  // bool setFilter(uint32_t id, uint32_t mask);
  int filter(int id, int mask) override;
  int filter(int id); // Overloaded for single parameter
  int filterExtended(long id, long mask) override;
  int filterExtended(long id); // Overloaded for single parameter

  // Additional features
  int observe() override;
  int loopback() override;
  int sleep() override;
  int wakeup() override;

  // Receive callback
  void onReceive(void (*callback)(int)) override;
  void (*_onReceive)(int) = nullptr;

private:
  // Internal helper functions
  bool configureTWAI(long baudrate);
  void handleError();

  // TWAI configurations
  twai_general_config_t g_config;
  twai_timing_config_t t_config;
  twai_filter_config_t f_config;

  // TWAI transmission buffers
  twai_message_t _txMsg;
  uint8_t _txData[CAN_MAX_DLEN];
  int _txLength;

  // TWAI reception buffers
  twai_message_t _rxMsg;
  uint8_t _rxData[CAN_MAX_DLEN];
  int _rxLength;
  int _rxIndex;
};

extern ESP32TWAIClass CAN;

#endif // _ESP32_TWAI_H_
