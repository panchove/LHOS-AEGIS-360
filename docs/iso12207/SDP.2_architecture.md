---
doc_id: SDP.2.ARCH.001
doc_type: ARCHITECTURE
version: 0.1.0
status: DRAFT
owner: ODIN
last_reviewed: 2026-05-19
---

# System Architecture

## Layers
1. HAL (UART, SPI, I2C, GPIO)
2. Core (Pool, CRC16, Parser, MessageBus)
3. Services (LTE, BLE, GNSS, Modbus)
4. Application (Main, Commands)

## Components
- `components/pool/` - PSRAM buffer pool
- `components/crc16/` - CRC calculation
- `components/parser/` - FSM packet parser
- `components/message_bus/` - RT/RI queues
- `components/blackbox/` - LittleFS persistence

## Memory Model
- PSRAM: buffers (16x512)
- DRAM: stacks, control structures
