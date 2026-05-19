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

## Diagrams

Architecture diagram (embedded) and saved Mermaid sources in `docs/diagrams/`.

### Architecture

![Architecture Diagram](../diagrams/architecture.mmd)

```mermaid
graph TB
	subgraph "Hardware"
		HW[ESP32-S3 + SIMCom A7670]
	end
    
	subgraph "HAL (C/ISR)"
		UART[UART Driver]
		SPI[SPI Driver]
		I2C[I2C Driver]
		GPIO[GPIO Driver]
	end
    
	subgraph "Core Services (C++17 Heapless)"
		POOL[PacketPool<br/>PSRAM Buffer Pool]
		CRC[CRC16<br/>ROM Wrapper]
		PARSER[PacketParser<br/>FSM]
		BUS[MessageBus<br/>RT/RI Queues]
		BB[BlackBox<br/>LittleFS]
	end
    
	subgraph "Domain Services"
		LTE[LTE Modem<br/>SIMCom A7670]
		BLE[BLE Scanner<br/>NimBLE Native]
		MODBUS[Modbus RTU<br/>RS485]
		GNSS[GNSS<br/>NMEA Parser]
	end
    
	subgraph "Application"
		CMD[Command Dispatcher<br/>Layrz Protocol]
		HEALTH[Health Monitor<br/>Watchdog]
		SAFE[Safe Mode<br/>Degraded Ops]
	end
    
	HW --> UART
	HW --> SPI
	HW --> I2C
	HW --> GPIO
    
	UART --> PARSER
	UART --> LTE
	UART --> GNSS
	SPI --> MODBUS
	I2C --> BLE
    
	PARSER --> POOL
	POOL --> BUS
	BUS --> BB
	BUS --> LTE
    
	LTE --> CMD
	BLE --> BUS
	MODBUS --> BUS
    
	CMD --> HEALTH
	HEALTH --> SAFE
	SAFE --> BUS
```

### Packet lifecycle

![Packet Lifecycle](../diagrams/packet_lifecycle.mmd)

```mermaid
sequenceDiagram
	participant UART as UART RX
	participant PARSER as PacketParser
	participant POOL as PacketPool
	participant BUS as MessageBus
	participant LTE as LTE Modem
	participant BB as BlackBox
    
	UART->>PARSER: feed_byte(bytes)
	PARSER->>POOL: alloc()
	POOL-->>PARSER: pool_buf_t*
    
	alt CRC Válido
		PARSER->>BUS: publish(buf, priority)
		BUS->>LTE: consume()
		LTE-->>BUS: ACK del servidor
		BUS->>POOL: release(buf)
	else CRC Inválido
		PARSER->>POOL: release(buf)
		PARSER->>PARSER: crc_failures++
	else Timeout
		PARSER->>POOL: release(buf)
		PARSER->>PARSER: timeouts++
	else Red desconectada (Reliable Priority)
		BUS->>BB: append(buf)
		BB-->>BUS: persistido
		Note over LTE: Cuando red disponible
		LTE->>BB: read_next()
		BB-->>LTE: pool_buf_t*
		LTE->>LTE: send()
		LTE->>BB: commit_current()
	end
```

### FreeRTOS Task Layout

![FreeRTOS Tasks](../diagrams/freertos_tasks.mmd)

```mermaid
graph LR
	subgraph "Core 1 (Critical)"
		PARSER[Parser Task<br/>Priority 5<br/>Stack 4KB]
		BUS[MessageBus Task<br/>Priority 5<br/>Stack 4KB]
		HEALTH[Health Task<br/>Priority 6<br/>Stack 2KB]
	end
    
	subgraph "Core 0 (Services)"
		LTE[LTE Task<br/>Priority 3<br/>Stack 8KB]
		BLE[BLE Task<br/>Priority 3<br/>Stack 8KB]
		MODBUS[Modbus Task<br/>Priority 2<br/>Stack 4KB]
		CMD[Command Task<br/>Priority 3<br/>Stack 4KB]
	end
    
	subgraph "Sincronización"
		RTQ[RT Queue<br/>Real-time]
		RIQ[RI Queue<br/>Retry/Blackbox]
		EG[Event Group<br/>System Events]
	end
    
	PARSER -->|publish| RTQ
	PARSER -->|publish| RIQ
	BUS -->|consume| RTQ
	BUS -->|consume| RIQ
	LTE -->|signal| EG
	BLE -->|signal| EG
	HEALTH -->|monitor| EG
```

### Parser FSM

![Parser FSM](../diagrams/parser_fsm.mmd)

```mermaid
stateDiagram-v2
	[*] --> IDLE: Inicio/Reset
    
	IDLE --> SYNC: Recibe '<'
	SYNC --> HEADER: Recibe 'P' o 'A'
	HEADER --> BODY: Recibe '>' (cierre de tag)
	BODY --> CRC: Recibe '<' (cierre de paquete)
	CRC --> DISPATCH: CRC válido
	CRC --> ERROR: CRC inválido
	DISPATCH --> IDLE: Callback ejecutado
    
	ERROR --> RECOVERY: Limpiar estado
	RECOVERY --> IDLE: Listo para próximo
    
	IDLE --> IDLE: timeout/reset
	SYNC --> IDLE: timeout
	HEADER --> IDLE: timeout
	BODY --> IDLE: timeout
	CRC --> IDLE: timeout
```

### Memory Map

![Memory Map](../diagrams/memory.mmd)

```mermaid
graph TB
	subgraph "PSRAM (Externa, 8MB opcional)"
		POOL_BUFFERS[Packet Pool Buffers<br/>16 x 512 bytes = 8KB]
		BLE_CACHE[BLE Device Cache<br/>MAC → Last Packet]
		CONFIG_JSON[Configuración JSON<br/>En runtime]
	end
    
	subgraph "DRAM (Interna, 512KB + 512KB)"
		STACKS[Task Stacks<br/>~30KB total]
		HEAP[ESP-IDF Heap<br/>~200KB]
		QUEUES[FreeRTOS Queues<br/>~4KB]
		CONTROL[Control Structures<br/>Pool Slots, Stats]
	end
    
	subgraph "IRAM (Cache, 64KB)"
		ISR[ISR Routines<br/>UART, GPIO]
		CRC_ROM[CRC16 ROM Function<br/>esp_rom_crc16_be]
	end
    
	subgraph "Flash (8MB)"
		CODE[Program Code<br/>C++17 Text]
		LITTLFS[LittleFS Partition<br/>BlackBox, Config]
	end
    
	POOL_BUFFERS -->|fallback if no PSRAM| HEAP
```

### Boot Sequence

![Boot Sequence](../diagrams/boot_sequence.mmd)

```mermaid
sequenceDiagram
	participant ROM as ROM Bootloader
	participant BOOT as Bootloader
	participant APP as app_main()
	participant POOL as PacketPool
	participant BUS as MessageBus
	participant BB as BlackBox
	participant HEALTH as Health Monitor
	participant TASKS as FreeRTOS Tasks
    
	ROM->>BOOT: Power On
	BOOT->>APP: jump to app_main()
    
	APP->>POOL: init()
	alt PSRAM disponible
		POOL-->>APP: buffers en PSRAM
	else PSRAM no disponible
		POOL-->>APP: fallback a DRAM
	end
    
	APP->>BB: init(LittleFS)
	BB-->>APP: mount /blackbox
    
	APP->>BUS: init()
	BUS-->>APP: RT/RI queues creadas
    
	APP->>HEALTH: init()
	HEALTH-->>APP: task registry lista
    
	APP->>TASKS: xTaskCreateStatic()
	TASKS-->>APP: Parser, LTE, BLE Tasks
    
	APP->>HEALTH: set_state(READY)
	HEALTH-->>APP: System Ready
    
	Note over APP,TASKS: Sistema operativo normal
```

### Class Diagram

![Class Diagram](../diagrams/classes.mmd)

```mermaid
classDiagram
	class PacketPool {
		+instance() PacketPool&
		+init() bool
		+alloc() optional~pool_buf_t~
		+release(pool_buf_t&&)
		+free_count() size_t
		-slots_ Slot*
		-num_buffers_ size_t
	}
    
	class PacketParser {
		+init(callback) bool
		+feed(bytes, len)
		+check_timeout()
		+reset()
		-state_ ParserState
		-current_buf_ pool_buf_t*
	}
    
	class MessageBus {
		+instance() MessageBus&
		+publish(buf, priority) bool
		+consume(priority) optional~pool_buf_t~
		+enqueue_to_blackbox(buf)
		-rt_queue_ QueueHandle_t
		-ri_queue_ QueueHandle_t
	}
    
	class BlackBox {
		+instance() BlackBox&
		+append(buf) bool
		+read_next() optional~pool_buf_t~
		+commit_current() bool
		-file_ FILE*
	}
    
	class LTEModem {
		+instance() LTEModem&
		+connect(host, port) bool
		+send(data, len) bool
		+at_command(cmd) string
		-uart_ uart_port_t
	}
    
	class CommandDispatcher {
		+instance() CommandDispatcher&
		+dispatch(command) string
		+register_handler(cmd, handler)
		-handlers_ map
	}
    
	PacketParser --> PacketPool : uses
	MessageBus --> PacketPool : uses
	MessageBus --> BlackBox : persists
	LTEModem --> MessageBus : consumes
	CommandDispatcher --> LTEModem : receives commands from
```
