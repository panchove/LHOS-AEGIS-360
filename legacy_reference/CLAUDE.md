# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build system

This is a PlatformIO project targeting the ESP32-S3-WROOM-1 (N8R8 — 8 MB flash, 8 MB PSRAM). All builds use the Arduino framework on top of ESP-IDF via `espressif32@6.7.0`.

### Local development builds (flash to device)

```bash
# Default environment (development_hub2)
pio run

# Flash and open serial monitor
pio run --target upload --target monitor

# Select a specific environment
pio run -e development_hub1
pio run -e development_hub2
pio run -e development_hub25
```

`ESP32_COM_PORT` must be set in the environment (or a `.env` file at the project root) for Hub1 builds; Hub2/Hub25 use USB CDC so no port is needed locally.

### Production / CI builds

Production builds require `LAYRZ_BUILD_NUMBER` to be exported (or it will be fetched automatically from the Layrz API by `build_number.py`).

```bash
make prod_hub1    # production_hub1
make prod_hub2    # production_hub2
make prod_hub25   # production_hub25
```

CI runs `uv run layrz-app-build platformio platformio --ci --version platformio` on pushes to `main` (stable) and `next` (pre-release).

### Build variant flags

| Variant | Macro | Modem | GPIO set |
|---|---|---|---|
| Hub1 | `LAYRZ_HUB1_BUILD` | none | IO1-2, IO5-7, IO14, IO45-47 |
| Hub2 | `LAYRZ_HUB2_BUILD` | AsyncA7670 (LTE) | IO3-7, IO19-20, IO41-42 |
| Hub25 | `LAYRZ_HUB25_BUILD` | AsyncA7670 (LTE) | DI1-4, AI1/AI2, DO1-2 |

`LOCAL_COMPILATION` is defined only in `development_*` environments and enables debug helpers/serial logging.

## Architecture overview

### Startup flow

`setup()` in `main.cpp` calls a sequence of static methods on `StartupLayrzHub` (in order) and then returns. `loop()` only resets the watchdog. **All ongoing work runs in FreeRTOS tasks** spawned during startup.

Key startup phases:
1. USB/serial, settings, RGB LED, factory-reset check, SPI bus and SD card
2. NimBLE stack, FOTA pre-check (before any other network work)
3. A7670 modem or WiFi, GNSS, TCP socket listener, NTP sync
4. Peripheral tasks: CAN bus, Modbus, serial monitor, BLE scanner, Arducam, Zigbee
5. Sensor publisher and message bus sender tasks

### Message flow

All outbound data goes through `MessageBusLayrzHub` (singleton-style static class):

```
sensor / command handler
        │
        ▼
MessageBusLayrzHub::publish(BusMessage)
        │
   ┌────┴────────────────────────────┐
   │ _rtQueue (FreeRTOS queue)       │  full?
   └────────────────────────────────┘
        │ senderTask                  └──► BlackBoxLayrzHub (SD backlog)
        ▼
   TCP raw socket  ──► await <Ao>/<Ar> ACK from server
        │ timeout / NAK
        ▼
   BlackBoxLayrzHub  ◄── re-enqueued
```

`BusMessage.data` is a `heap_caps_malloc` PSRAM buffer. Ownership transfers to the bus on a successful `publish()`. **If publish is skipped (e.g., `!isValidTime`), the caller must free `bm.data` explicitly.**

`BusMsgKind` controls the wire prefix: `<Pd>` sensor, `<Pb>` BLE, `<Pc>` ack, `<Ps>` settings, `<Pi>` info, `<Pm>` media, RI505 ticket (separate HTTP queue).

### Settings

`UnifiedSettingsStorage` stores all configuration as JSON in SPIFFS, with the JSON document allocated in PSRAM via `SpiRamAllocator`. At runtime, settings are deserialized into the `settingsVars` struct (declared in `GlobalObjectsLayrzHub.h`) which every module reads directly via `hubSettings`.

### Memory strategy

- **PSRAM (`MALLOC_CAP_SPIRAM`)**: all large/long-lived buffers (message bus payloads, sensor/CAN/Modbus/UART buffers, BLE device array, media buffer, settings JSON). Use `heap_caps_malloc` / `heap_caps_free`.
- **Internal heap**: FreeRTOS internals, NimBLE/WiFi/SSL stacks, short-lived locals. Avoid Arduino `String` or `std::string` with `+=` in hot paths — they fragment the 320 KB internal heap.
- The `SpiRamAllocator` struct in `GlobalObjectsLayrzHub.h` is the custom allocator for `BasicJsonDocument` objects that must live in PSRAM.

### Command handling

`LinkLayrzHub::executeCmd()` dispatches inbound commands (from TCP socket, BLE UART service, or serial) to individual `*_Handler` static methods. Commands arrive as `<key>=<value>` strings. `cmdSource` enum (`BLE`, `NET`, `UART`) determines how `sendCommandAck` routes the response.

### Blackbox (offline backlog)

`BlackBoxLayrzHub` manages a segment-based append log on the SD card (default segment max 512 KB). The message bus calls `enqueue()` when the socket is down; `senderTask` replays via `readNext()` / `commitRead()`. A separate RI ticket backlog (`_riBb`) is managed by `riTicketTask` and drained over HTTPS independently of the main socket.

### BLE subsystem

Two independent BLE roles run simultaneously:
- **Scanner** (`BleLayrzHub`): passive NimBLE scan, decodes known device models (ELA, Xiaomi, Teltonika, …), stores last packet per MAC in `BleMemoryManager`'s PSRAM array, publishes `<Pb>` and optionally decoded `<Pd>` messages.
- **Peripheral** (`BleConfigLayrzHub`): Nordic UART Service (NUS) for device configuration via the Layrz Confiot mobile app.

### Hub-variant gating

Hardware-specific code is gated at compile time:
```cpp
#ifdef LAYRZ_HUB1_BUILD
  // Hub1-only code
#elif defined(LAYRZ_HUB2_BUILD)
  // Hub2-only code
#elif defined(LAYRZ_HUB25_BUILD)
  // Hub25-only code
#endif
```
`DefinitionsLayrzHub.h` maps all pin names to the correct GPIOs per variant. Check this file first when touching anything hardware-related.

## Commit conventions

Follow the Conventional Commits format (see `CONTRIBUTING`):

```
<type>: <description in imperative mood, no period>

[optional body]
[extra <type>: <description>]

[Co-authored-by: Name <email>]
```

Types: `feat`, `fix`, `perf`, `refactor`, `docs`, `test`, `style`, `chore`. Multiple types in one commit are allowed — list each on its own line separated by a blank line.
