# GitHub Copilot Instructions - LHOS-AEGIS-360

## Project Context

This is a firmware project for ESP32-S3 using ESP-IDF v5.5.3. The system is named LHOS-AEGIS-360, a telemetry and control system for industrial IoT.

## Technology Stack

- **Language**: C++17 (primary), C11 (only for HAL/ISR)
- **Framework**: ESP-IDF v5.5.3 (official Espressif components only)
- **RTOS**: FreeRTOS (provided by ESP-IDF)
- **Build System**: CMake + idf.py
- **Testing**: Unity framework (ESP-IDF integrated)

## Coding Standards

### C++17 Rules

- **Allowed**: RAII, constexpr, std::optional, std::string_view, std::array, enum class, move semantics, auto
- **Forbidden**: std::string, std::vector, std::map, std::set, std::function, exceptions, RTTI, new/delete manual
- **Heap policy**: Heapless core. Only PacketPool may use heap_caps_malloc with MALLOC_CAP_SPIRAM

### Formatting

- Indentation: 2 spaces (no tabs)
- Line width: 100 characters
- Braces: Allman style (braces on new line)
- Pointer alignment: Left (Type* ptr)

### Naming Conventions

- **Classes**: PascalCase (e.g., `PacketPool`, `MessageBus`)
- **Functions**: snake_case (e.g., `ccitt_false`, `to_hex`)
- **Variables**: snake_case (e.g., `num_buffers`, `buffer_size`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_BUFFERS`)
- **Namespaces**: snake_case (e.g., `crc16`)
- **Files**: snake_case (e.g., `pool.hpp`, `message_bus.cpp`)

### Comments (Doxygen)

All public interfaces must have Doxygen comments:

```cpp
/**
 * @brief Calculate CRC-16/CCITT-FALSE for given data
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return uint16_t Calculated CRC value
 */
uint16_t ccitt_false(const uint8_t* data, size_t len);
```

## Architecture

### Layers

1. **HAL**: UART, SPI, I2C, GPIO (C only)
2. **Core**: Pool, CRC16, Parser, MessageBus (C++17 heapless)
3. **Services**: LTE, BLE, GNSS, Modbus (C++17)
4. **Application**: Main, Commands (C++17)

### Key Components

- **PacketPool**: Fixed-size buffer pool (16 x 512 bytes) in PSRAM/DRAM
- **CRC16**: ROM wrapper for esp_rom_crc16_be()
- **PacketParser**: FSM for `<Px>data</Px>CRC` packets
- **MessageBus**: RT/RI queues with ownership transfer
- **BlackBox**: LittleFS persistence for message replay

## Prohibited Features (Never Generate)

- ❌ Arduino APIs (String, SPIFFS, Serial)
- ❌ External libraries not in ESP-IDF (TinyGSM, AsyncA7670)
- ❌ eFuse functions (esp_efuse_*)
- ❌ Secure Boot, Flash Encryption
- ❌ PRs, CI/CD workflows
- ❌ std::string, std::vector, std::map
- ❌ Exceptions, RTTI
- ❌ Manual new/delete

## Hardware Target

- **Board**: ESP32-S3-WROOM-1-N8R8
- **Flash**: 8MB
- **PSRAM**: 8MB (optional, detect at runtime)
- **LTE Modem**: SIMCom A7670 (UART AT commands)
- **Serial Port**: /dev/ttyUSB0
- **Target**: esp32s3

## Build Commands

```bash
# Clean build
idf.py fullclean && rm -rf build/ sdkconfig

# Configure
cp sdkconfig.defaults sdkconfig
idf.py set-target esp32s3

# Build
idf.py build

# Flash and monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## Project Directives

1. **No external workspaces**: All code must be in this repository
2. **No CI/CD**: Development only, no GitHub Actions
3. **No PRs**: Direct commits to main only
4. **RAII mandatory**: All resources must use RAII wrappers
5. **Documentation first**: Update docs before implementing changes
6. **Hardware validation**: Test on real ESP32-S3 before commit

## Code Generation Priority

When generating code, follow this priority:

1. **Correctness**: Must compile and run on ESP32-S3
2. **Heapless**: No dynamic allocation in hot paths
3. **RAII**: Resources must self-manage their lifecycle
4. **Documentation**: Doxygen comments for all public APIs
5. **Performance**: Efficient within constraints (512 byte packets)

## Example: Correct Code Generation

```cpp
// ✅ CORRECT: RAII, heapless, Doxygen
/**
 * @brief Allocate buffer from pool
 * @return std::optional<pool_buf_t> Buffer if available
 */
std::optional<pool_buf_t> PacketPool::alloc() {
    for (size_t i = 0; i < num_buffers_; ++i) {
        bool expected = false;
        if (slots_[i].used.compare_exchange_strong(expected, true)) {
            alloc_count_.fetch_add(1);
            free_count_.fetch_sub(1);
            return pool_buf_t{slots_[i].buffer, buffer_size_, MAGIC};
        }
    }
    return std::nullopt;
}
```

## Example: Incorrect Code (DO NOT GENERATE)

```cpp
// ❌ INCORRECT: Manual new/delete
char* buf = new char[512];
delete[] buf;

// ❌ INCORRECT: std::string
std::string data = "<Pd>test</Pd>";

// ❌ INCORRECT: Exceptions
throw std::runtime_error("error");

// ❌ INCORRECT: Arduino
String message = "<Pd>test</Pd>";
SPIFFS.begin();
```

## Response Style

- Be concise and direct
- Generate complete, compilable code
- Prefer constexpr over runtime calculation
- Use std::optional instead of error codes
- Include Doxygen comments for all public APIs
- Follow the 2-space indentation strictly
