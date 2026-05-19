#if defined(ARDUINO_ARCH_ESP32)
#include "ESP32TWAI.h"
ESP32TWAIClass CAN;
#else
#include "MCP2515.h"
MCP2515 CAN;
#endif
