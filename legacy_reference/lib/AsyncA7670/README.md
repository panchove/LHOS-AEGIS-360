# AsyncA7670 Library

FreeRTOS-based asynchronous library for SIMCom A7670 GSM/LTE module.

## Features

- Non-blocking GPRS connection management
- Asynchronous TCP socket operations with chunking support (max 1500 bytes)
- NTP time synchronization
- FreeRTOS task-based architecture
- Compatible with TinyGSM API naming

## Usage

```cpp
#include "AsyncA7670.h"

HardwareSerial gsmSerial(2);
AsyncA7670 modemAsync(gsmSerial);

void setup() {
    gsmSerial.begin(115200);
    modemAsync.begin();
    modemAsync.gprsConnect("your-apn");
}
```
