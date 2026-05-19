
#ifndef _CAN_H_
#define _CAN_H_

#include "CANController.h"

#if defined(ARDUINO_ARCH_ESP32)
#include "ESP32TWAI.h"
extern ESP32TWAIClass CAN;
#else
#include "MCP2515.h"
extern MCP2515 CAN;
#endif

#endif
