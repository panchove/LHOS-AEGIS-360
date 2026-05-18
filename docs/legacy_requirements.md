# Requisitos y especificaciones extraídas de legacy_reference

Objetivo: partir de las especificaciones y requisitos del código legacy sin heredar su código fuente; tomar ideas y traducirlas a una nueva implementación limpia, mantenible y conforme a las reglas del proyecto (ESP32-S3, FreeRTOS, heapless core, C principal).

Resumen ejecutivo
- El firmware legacy implementa un hub con múltiples subsistemas (BLE scanner + peripheral, modem cellular/A7670, WiFi, GNSS, Modbus/CAN/OBD2, Arducam, Zigbee, SD blackbox, serial/RS485). Nuestro objetivo es conservar requisitos funcionales y no funcionales, re-diseñar interfaces y re-implementar en código limpio.
- No se heredará código fuente; solo se reusarán protocolos, límites y decisiones de diseño extraídas del código legacy.

Principales hallazgos (funcionales)
- Mensajería centralizada: MessageBus que recibe BusMessage ({data,len,kind,persistIfFail,freeAfterSend}). Ownership de data pasa al bus en publish().
- Transport: modo raw TCP (socket persistente con protocolo de ACKs <Ao>/<Ar>/<Pc>) y modo HTTPS/HTTP POST para otros flujos (riTicket). Soporta reintento, persistencia en BlackBox si falla.
- BlackBox: append-only backlog en SD para persistir mensajes cuando la red no está disponible; permite replay y commitRead/commitWrite.
- Protocol framing: paquetes en XML-like tags: <Pd>, <Pc>, <Ps>, <Pi>, <Pb>, <Pm>, <Pa>, <Ao>, <Ar>, <As>, etc. Campo core incluye timestamp; CRC16 usado para validación en muchas partes.
- Comandos remotos: LayrzProtocol interpreta comandos (<Ac> / <Ao>/<Ar> handshake) y ejecuta handlers (reboot, get_config, set_config, fota, factory_reset, gpio, pwm, serial passthrough, io extender commands, etc.).
- Serial/RS485: SerialMonitor ofrece parsers y marshalling para múltiples dispositivos (HPC168, DN23E08), construcción de tramas, CRC16, y manejo de semáforos para acceso compartido al bus RS485.
- BLE: subsistema con scanner y peripheral (NUS) que publica paquetes decodificados; existe un conjunto de decoders por modelo (ELA_*, LYWSD03MMC, TELTONIKA_EYE, etc.).
- Settings: UnifiedSettingsStorage guarda JSON (SPIFFS) en PSRAM durante parseo; settings deserializados a hubSettings global leído por módulos.

Principales hallazgos (no funcionales y operativos)
- PSRAM (MALLOC_CAP_SPIRAM) se usa para buffers grandes y de larga vida (message payloads, media, settings JSON). Se hacen allocations con heap_caps_malloc y liberaciones explícitas.
- Internal heap (MALLOC_CAP_INTERNAL) reservado para stacks, FreeRTOS, y librerías (avoid fragmentation). Evitar Arduino String y operaciones que fragmenten.
- Concurrencia: FreeRTOS tasks con prioridades y tareas pinned a cores; watchdog (esp_task_wdt_reset) usado frecuentemente; semáforos (xSemaphore) para protección de recursos críticos.
- Memory circuit breaker y health monitor: políticas para detectar low-memory/fragmentation y reiniciar controladamente.
- Build variants: macros LAYRZ_HUB1_BUILD / HUB2 / HUB25 con gating de hardware; PlatformIO/Arduino sobre ESP-IDF (platformio.ini), C++17 flags presentes.

Protocolos y detalles técnicos relevantes (a reusar)
- Framing tags: <Pd> (sensor), <Pc> (command ack), <Ps> (settings), <Pi> (info), <Pb> (ble), <Pm> (media).
- CRC16: presente en múltiples tramas (serial extenders, DN23E08, HPC168, payload checks). Legacy tiene utilitarios para CRC (calculateCRC16), y scripts Python con ejemplos.
- Raw socket reliability: implementación con ACK/NAK (<Ao>/<Ar>) y temporizadores ACK_TIMEOUT_MS. Política de fairness entre mensajes realtime y replay desde backlog.
- Serial devices: patterns STX/ETX wrappers, ASCII-hex encodings, and checksums for device frames.
- Modbus maps: archivo modbusMaps.json contiene modelos de mapas de registros; útil como especificación de mapping para Modbus implementation.

Reglas y restricciones extraídas (de aplicación obligatoria)
- No malloc/new/delete en hot-paths; core runtime debe ser heapless. Pools estáticos (PSRAM-backed) para buffers grandes son permitidos bajo control.
- Ownership: ownership packet-centric: parser/producer cede la propiedad al MessageBus en publish(); MessageBus o BlackBox son responsables de free cuando corresponda.
- Tasks: prefer pinning a core y stacks estáticos (StaticTask_t) cuando corresponda; medir high-water marks.
- No eFuse / irreversible hardware changes: security model requiere recovery vía software.
- Instrumentación obligatoria: métricas de pool (alloc/release), parser timeouts, crc_failures, queue drops, blackbox enqueues/replays.

Riesgos técnicos observados en legacy (a corregir en la nueva implementación)
- Uso indiscriminado de std::string/Vector con placement-new en PSRAM: complejo y propenso a errores; preferir buffers C/structs o wrappers RAII controlados.
- Mezcla de políticas de memoria (PSRAM vs internal) sin layer de abstracción clara; crear un MemoryPolicy/Allocator layer explícito.
- Código monolítico con funciones muy largas y duplicadas (cmd handlers, serial parsing). Re-architect para componentes pequeños y testeables.
- Uso extensivo de strings y Arduino-specific apis; para ESP-IDF puro preferir APIs nativas o adaptadores claros.

Mapa de componentes propuesto (implementación limpia)
1. platform/startup: BootOrchestrator (boot paths, safe mode, health)
2. config/settings: Settings storage (NVS or SPIFFS) + JSON parse using controlled allocator
3. core/pool: heapless fixed-size packet pool (PSRAM-backed buffers) + diag counters
4. core/parser: packet-centric parser (FSM) + CRC16 verification + ownership handoff
5. core/message_bus: MessageBus abstraction with ownership contract + Blackbox adapter
6. transport/clients: raw-socket client (with ACK handling), http client for riTicket and HTTPS paths, cellular/wifi transport glue
7. drivers/serial: RS485/RS232 driver with framed I/O helpers and semaphore-protected access
8. services/layrz_protocol: command dispatcher (thin), command handlers modularized
9. devices/*: decoders/adapters (BLE decoders, DN23E08 parser, HPC168 parser) — pure parsing logic returning structured data
10. tools: contract_check (static checks for banned APIs), metrics exporter, test harnesses

Entregables inmediatos que propongo
1. docs/legacy_requirements.md (este archivo) — consolidado de requisitos (hecho).
2. docs/implementation_plan.md — especificación de interfaces públicas, límites y contrato de ownership para MessageBus/Pool/Parser.
3. Crear skeleton de componentes (headers en C) y tests unitarios (Unity) para pool+parser+CRC16.
4. Implementar CRC16 reference y pruebas con vectores conocidos (compatibilidad con legacy frames).
5. Instrumentación mínima: counters y health events (exponer via log o telemetry simple).

Preguntas rápidas (decisión requerida)
1. Lenguaje preferido para core: mantener C como principal con módulos C++ limitados (wrapper para tests/abstracciones) o permitir C++17 para el core (favor usar clases RAII y constexpr)? Recomendación: C para drivers y boot, C++17 (sin exceptions/RTTI) para core facilita abstracción segura. Confirma preferencia.

Próximo paso que puedo ejecutar ahora
- Generar docs/implementation_plan.md y el esqueleto inicial del pool (C API) y CRC16 con tests. ¿Procedo con eso?

Anexos (referencias rápidas dentro de legacy_reference)
- platformio.ini (build flags y variantes)
- src/main.cpp (startup sequence)
- src/modules/layrz_protocol/* (command handling)
- src/modules/messaging/MessageBusLayrzHub.* (message bus logic, backlog, senderTask, tcpSocketReception)
- src/modules/serial_comm/SerialLayrzHub.* (RS232/RS485 helpers, device parsers)
- src/modules/ble_devices/* (decoders)
- modbusMaps.json (modbus register mappings)

Si confirmas las pautas (especialmente la preferencia de lenguaje), empiezo a generar:
- docs/implementation_plan.md (interfases y contratos)
- esqueleto C limpio: components/pool, components/parser, components/message_bus (headers + tests)

Fin del resumen.
