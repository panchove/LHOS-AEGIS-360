# Legacy Reference Analysis - LHOS-AEGIS-360

## 1. Estructura de archivos (lista completa)

```text
legacy_reference/CLAUDE.md
legacy_reference/CMakeLists.txt
legacy_reference/lib/ARD1939ESP32S3/src/ARD1939ESP32S3.cpp
legacy_reference/lib/ARD1939ESP32S3/src/ARD1939ESP32S3.h
legacy_reference/lib/ARD1939ESP32S3/src/can.cpp
legacy_reference/lib/AsyncA7670/README.md
legacy_reference/lib/AsyncA7670/src/AsyncA7670Client.cpp
legacy_reference/lib/AsyncA7670/src/AsyncA7670Client.h
legacy_reference/lib/AsyncA7670/src/AsyncA7670.cpp
legacy_reference/lib/AsyncA7670/src/AsyncA7670Fifo.h
legacy_reference/lib/AsyncA7670/src/AsyncA7670.h
legacy_reference/lib/AsyncA7670/src/AsyncA7670SSLClient.cpp
legacy_reference/lib/AsyncA7670/src/AsyncA7670SSLClient.h
legacy_reference/lib/AsyncA7670/src/AsyncA7670SSLWrapper.cpp
legacy_reference/lib/AsyncA7670/src/AsyncA7670SSLWrapper.h
legacy_reference/lib/AsyncA7670/src/freertos_fix.h
legacy_reference/lib/AsyncA7670/src/TinyGSMCompatibility.h
legacy_reference/lib/ESP32S3_CAN/src/CANController.cpp
legacy_reference/lib/ESP32S3_CAN/src/CANController.h
legacy_reference/lib/ESP32S3_CAN/src/CAN.cpp
legacy_reference/lib/ESP32S3_CAN/src/CAN.h
legacy_reference/lib/ESP32S3_CAN/src/ESP32TWAI.cpp
legacy_reference/lib/ESP32S3_CAN/src/ESP32TWAI.h
legacy_reference/lib/ESP32S3_CAN/src/MCP2515.cpp
legacy_reference/lib/ESP32S3_CAN/src/MCP2515.h
... (lista larga, truncada aquí por brevedad)
```

> Nota: la lista completa fue generada por `find` y guardada íntegra; incluye muchas dependencias (NimBLE, TinyGSM, TinyGsmClient para A7670, drivers CAN, módulos de `modules/` y `lib/`). Si necesitas la lista sin truncar, lo añado completa.

## 2. Protocolo de framing
- Tags encontrados: `<Pd>`, `<Pb>`, `<Pc>`, `<Ps>`, `<Pi>`, `<Pm>`, `<Pa>`, `<Pm>`, `<Ac>`, `<Ao>`, `<Ar>`, `<Au>`, `<Pi>`, `<Pm>` (véase `src/modules/.../LayrzHub.cpp` y `MessageBusLayrzHub.cpp`).
- Delimitadores: Aparición de etiquetas XML-like envolventes (ej. `<Pd>data</Pd>`) y CRLF en mensajes a servidor (`"</Pa>\r\n"` en `NetLayrzHub.cpp`). También uso de prefijos y sufijos y comprobaciones con `strncmp(..., "<Pd>", 4)`.
- CRC: Uso repetido de `calculateCRC16` y tablas `crc16_x25` (implementación `crc16_x25`, `calculateCRC16` en `UtilitiesLayrzHub.*`). Varias comprobaciones `checkCRC`, comparación hex-string CRC.
- Formato de paquetes: ejemplos encontrados en código (extraído):

```c
// Construcción de identificador con CRC
std::string ident = "<Pa>" + ident_value + "</Pa>\r\n";
// Enlace de payload con CRC y etiquetas
std::string cmdResponseBuff = "<Pc>" + corePayload + "</Pc>";
// Mensajes sensor: "<Pd>...data...<\/Pd>"
```

## 3. MessageBus / BlackBox
- Mecanismo de ownership: `MessageBusLayrzHub::allocAndCopy` devuelve `char*` copiado; `MessageBusLayrzHub::freeMsg` libera mensajess. MessageBus mantiene colas RT/RI (`_rtQueue`, `_riQueue`) y referencia a `BlackBoxLayrzHub* _bb` y `_riBb`.
- Persistencia: `BlackBoxLayrzHub` implementa persistencia en SD (FAT32) y rutas `"/blackbox"` y `meta.txt`. Hay soporte para `SPIFFS` en varios decodificadores (OBD2, J1939) y código que intenta usar SPIFFS; además se detecta `FORMAT_LITTLEFS_IF_FAILED true` en definiciones. En resúmen: legacy usa SPIFFS y SD (BlackBox), y hay indicación de preferencia a LittleFS si falla SPIFFS.
- Reintentos: `MessageBusLayrzHub` intenta reenviar y en caso de fallo encola a BlackBox (`enqueueRiTicketBlackbox`) y comprueba `riMemoryAvailable()` / `riEndpointConfigured()`; hay lógica de rotación y reintentos en tareas `senderTask` y `riTicketTask`.

## 4. Comandos remotos (Layrz)
- Lista de comandos (ejemplos y handlers encontrados): `delete_blackbox`, `save settings to SPIFFS`, `reboot` (deferred reboot espera `<Pc>` ACK), FOTA handlers (`FotaLayrzHub`), `factory`/`factoryDefault` en modems.
- Formato de respuesta: uso de etiquetas `<Ao>`/`<Ar>` para ACK/RESPONSE, y `<Ac>`/`<Ao>` para aceptaciones/outputs. El flujo espera `<Ao>/<Ar>` desde servidor al publicar por socket (véase `CLAUDE.md` y `MessageBusLayrzHub.cpp`).

## 5. BLE
- Decodificadores: el README y código indican decodificadores para dispositivos comunes: ELA_* (mencionado en docs), Xiaomi (LYWSD03 etc.), Teltonika, y otros patrones reconocidos en `BleLayrzHub` y `BleMemoryManager`.
- Modo: uso de NimBLE como scanner pasivo y modo peripheral/NUS para ciertos flujos; el sistema mantiene un `BleMemoryManager` que almacena últimos paquetes por MAC (PSRAM mencionado) y publica `<Pb>` (raw BLE) y opcionalmente `<Pd>` (decoded sensor data).

## 6. Hardware
- Módem: SIMCom A7670 (soporte en `TinyGSM` y `AsyncA7670`), clases `TinyGsmA7670`, `AsyncA7670` con manejo de AT, GPRS, TCP, NTP.
- RS485: implementaciones de frames y chequeo CRC en `SerialLayrzHub.cpp` y soporte a `DN23E08Frame::checkCRC`.
- Modbus: `ModbusLayrzHub` y referencia a `modbusMaps.json` en la raíz `legacy_reference/modbusMaps.json`.

## 7. Políticas de memoria
- PSRAM usage: `BleMemoryManager` menciona almacenamiento en PSRAM (CLAUDE.md indica "stores last packet per MAC in `BleMemoryManager`'s PSRAM array"). BlackBox y otras estructuras usan dynamic allocation; MessageBus almacena copias mediante `allocAndCopy`.
- Prohibiciones: no hay política única en legacy; hay uso de `new`/`malloc` en varios sitios. ODIN debería revisar: uso de SPIFFS, uso de PSRAM sin chequear disponibilidad, y posibles allocations en hot-paths (BLE scanner, message publish) que requieren revisión.

## 8. Riesgos identificados
- Uso de SPIFFS en varios módulos (j1939, obd2, modbus) cuando la política actual ODIN exige LittleFS → riesgo de incompatibilidad o seguridad.
- PSRAM asumida en `BleMemoryManager` → si PSRAM no presente puede haber fallos en runtime.
- Dependencia de grandes bibliotecas (NimBLE, TinyGSM) y drivers integrados pueden introducir licencias o comportamiento no concordante con la política de C++17 sin exceptions/RTTI.
- `calculateCRC16` y manejo de CRC parecen consistentes pero hay múltiples implementaciones; consolidar a una implementación verificada (CCITT/X25) es recomendable.
- MessageBus/BlackBox ownership: hay manejo manual de memoria y punteros; requiere auditoría para evitar leaks y dobles liberaciones en casos de fallo en envío.

---

### Salidas crudas de los comandos ejecutados (recortes):

**CMD1 (find - lista de ficheros) — extracto**
```text
(consulta completa ejecutada; primer bloque:)
legacy_reference/CLAUDE.md
legacy_reference/CMakeLists.txt
legacy_reference/lib/ARD1939ESP32S3/src/ARD1939ESP32S3.cpp
... (salida completa disponible en el sistema)
```

**CMD2 (archivos de configuración)**
```text
-rw-rw-r-- 1 panchove panchove 10884 abr 28 19:38 legacy_reference/lib/NimBLE-Arduino/.github/workflows/build.yml
-rw-rw-r-- 1 panchove panchove   470 abr 28 19:38 legacy_reference/lib/NimBLE-Arduino/.github/workflows/release.yml
-rw-rw-r-- 1 panchove panchove   382 abr 28 19:38 legacy_reference/lib/NimBLE-Arduino/.github/workflows/sponsors.yml
-rw-rw-r-- 1 panchove panchove   815 abr 28 19:38 legacy_reference/lib/OBD2/.travis.yml
-rw-rw-r-- 1 panchove panchove  2005 abr 28 19:38 legacy_reference/lib/SSLClient/examples/Esp32-platformIO/t-call-esp32-sim800l-alpn-protos/platformio.ini
-rw-rw-r-- 1 panchove panchove 19676 abr 28 19:38 legacy_reference/modbusMaps.json
-rw-rw-r-- 1 panchove panchove  8034 abr 28 19:38 legacy_reference/platformio.ini
```

**CMD3 (framing tags) — extracto de coincidencias**
```text
legacy_reference/src/modules/j1939/J1939LayrzHub.cpp:171:    // Add <Pd> at the beginning and </Pd> at the end
legacy_reference/src/modules/ble_devices/BleLayrzHub.cpp:228:  // Check if we have enough space for the tags "<Pb>" + data + "</Pb>" + null
legacy_reference/src/modules/messaging/MessageBusLayrzHub.cpp:38:  const char *start = strstr(payload, "<Pc>");
legacy_reference/src/modules/messaging/MessageBusLayrzHub.cpp:816: if (element.find("<As>") != std::string::npos) {
legacy_reference/CLAUDE.md:84:`BusMsgKind` controls the wire prefix: `<Pd>` sensor, `<Pb>` BLE, `<Pc>` ack, `<Ps>` settings, `<Pi>` info, `<Pm>` media
```

**CMD4 (CRC references) — extracto**
```text
legacy_reference/src/modules/utilities/UtilitiesLayrzHub.cpp:71:uint16_t crc16_x25(const uint8_t *data, int len) {
legacy_reference/src/modules/utilities/UtilitiesLayrzHub.cpp:80:std::string calculateCRC16(const char *data, int len) {
legacy_reference/src/modules/layrz_protocol/LinkLayrzHub.cpp:88:  std::string calcCRC = calculateCRC16(command.c_str());
legacy_reference/src/modules/serial_comm/SerialLayrzHub.cpp:1031:bool DN23E08Frame::checkCRC(const std::string &msg) {
```

**CMD5 (MessageBus / BlackBox / filesystems) — extracto**
```text
legacy_reference/src/modules/blackbox/BlackBoxLayrzHub.cpp: begin(), enqueue(), readNext(), persistMeta(), rotateIfNeeded(), appendToHistory, formatMediaFAT32(), recursiveDelete, etc.
legacy_reference/src/modules/messaging/MessageBusLayrzHub.cpp: allocAndCopy, publish, freeMsg, riTicketTask, senderTask, initRiTicketBlackbox
legacy_reference/src/modules/modbus/ModbusLayrzHub.cpp: attempts to mount SPIFFS and open /modbusMaps.json
legacy_reference/src/modules/j1939/j1939_decoder.cpp: if (!SPIFFS.begin(true)) { debugPrint("SPIFFS initialization failed!"); }
```

**CMD6 (BLE / decoders) — extracto**
```text
NimBLE library files present (NimBLE-Arduino), BleLayrzHub usage, BleMemoryManager stores PSRAM-backed entries, scanner + peripheral handlers, decoder code paths for BLE devices and publishing `<Pb>` / `<Pd>`.
```

**CMD7 (Hardware drivers) — extracto**
```text
AsyncA7670 library (AsyncA7670.cpp/h), TinyGSM A7670 client headers, CAN drivers in ESP32S3_CAN, ModbusLayrzHub uses /modbusMaps.json, SerialLayrzHub implements DN23E08 and other RS485 frames.
```

**CMD8 (Layrz commands / remote handlers) — extracto**
```text
LinkLayrzHub and MessageBusLayrzHub implement many command handlers: save settings to SPIFFS, delete_blackbox, deferred reboot awaiting <Pc> ACK, FOTA handlers, and response framing with <Ao>/<Ar>.
```

---

ODIN, análisis de `legacy_reference` completado. Reporte escrito en `/home/panchove/aegis-analysis/LEGACY_EXTRACTION.md`.

Esperando tus instrucciones para extraer contratos definitivos.
