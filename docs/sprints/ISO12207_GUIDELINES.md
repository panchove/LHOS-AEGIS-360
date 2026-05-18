# Guía de mapeo de documentos a ISO/IEC/IEEE 12207 (versión resumida)

Objetivo: esta guía explica cómo rellenar las plantillas de Feature e Issue para cumplir con los procesos clave de ISO/IEC/IEEE 12207.

Procesos principales y mapeo a secciones de la plantilla:
- Requirements (6.x en 12207): usar la sección 4 (Requisitos) en Feature; en Issue mapear a "Reproducción" y "Análisis".
- Architecture & Design: sección 5 de Feature.
- Implementation: sección 6 de Feature (artefactos y componentes a crear).
- Verification & Validation: sección 7 de Feature y sección 9 de Issue (criterios de verificación y cierre).
- Configuration Management: sección 8 de Feature (branch/PR/artefactos) y trazabilidad en Issue (sección 10).
- Operation & Maintenance: sección 9 de Feature y sección 11 de Issue (lecciones aprendidas).

Recomendaciones prácticas:
- Lenguaje: español técnico, frases claras y cuantificables. Evitar ambigüedades.
- Trazabilidad: siempre incluir IDs de requisito, nombre de PR/commit y ruta de tests automatizados.
- Evidencias: adjuntar logs y resultados de pruebas en docs/sprints/pilot_results o referenciar artifacts en CI.
- Revisión: cada documento debe recibir aprobación de Arquitecto (arquitectura), QA (tests) y Product Owner.

Control de calidad documental:
- Validar que cada feature tenga:
  - Requisitos trazables
  - Criterios de aceptación medibles
  - Plan de verificación
- Validar que cada issue tenga:
  - Pasos reproducibles
  - Evidencia adjunta
  - Plan de verificación y cierre

Automatización sugerida (opcional):
- Script CI que verifique plantillas completadas (campos obligatorios), p.ej. check-docs.sh que falle PR si falta trazabilidad mínima.

Fin de guía.

---

## Anexo: Habilitación/deshabilitación dinámica y persistente de servicios (por ConfIoT o Monitor)

### Requisitos contractuales
- Los servicios (tasks, handlers, APIs, drivers, etc.) deben poder activarse o desactivarse dinámicamente vía:
    - Interfaz Monitor (API HTTP/UI: switches por servicio, endpoint /api/v1/svc o similar).
    - Plataforma remota ConfIoT (push/sync del estado deseado).
- Algunos servicios son **indeactivables** por arquitectura ("core services"), nunca deben poder deshabilitarse ni manual, ni remota, ni por corrupción/config. Ejemplos: FreeRTOS tick/WDT, heap/system checker, recovery protector, API mínima, config starter/base, y compresión GZIP (GZIP es crítico, nunca puede desactivarse porque asegura la integridad y minimiza el uso de almacenamiento).
  - En backend, cualquier intento de alterar un core service es ignorado/logueado como warning y restaurado a enabled=1 automáticamente.
  - En UI/API/ConfIoT, estos servicios aparecen como "always-on" (toggle deshabilitado o no expuestos).
- La habilitación/deshabilitación del resto debe:
    - Persistir obligatoriamente en almacenamiento no volátil (NVS, archivo gzip, flash) y restaurarse tras cada reinicio.
    - Responder inmediatamente a los cambios (sin reboot salvo fuerza mayor).
    - Registrar cada cambio (fuente, timestamp) en logs gzip y eventos de monitor.
- El contrato de estado debe usar formato bitmask/mapa (keys cortas, ej: {"crc":1,"log":0,...}), donde core services no están expuestos o quedan readonly.
- Cualquier conflicto/confusión se resuelve por política:
    - ConfIoT tiene prioridad salvo override explícito desde Monitor.
    - Los cambios de estado deben ser auditable y reversibles vía logs.
- QA debe cubrir casos de persistencia, toggling concurrente, corrupción de NVS, intentos sobre core services, y correctos fallbacks seguros.

**Ejemplo de workflow:**
1. GET /api/v1/svc → retorna estado actual de los servicios habilitados/inhabilitados.
2. POST /api/v1/svc body: {"svc_logging":1} → habilita servicio, persiste, responde estado global actualizado.
3. Reboot/read arranca sólo los servicios activos.

Esto es de cumplimiento obligatorio a partir de Layrz v2 y recomendado como feature base desde Layrz v1.5.

---

## Anexo: Asignación de tareas a Core 0 y Core 1 (ESP32, ESP-IDF 5.5.3)

### Reglas de asignación contractuales

- **Core 0 (PRO CPU):**
    - Tareas críticas del sistema, FreeRTOS tick, WDT, WiFi/BLE stack, ISR principales, monitoreo, switching Layrz, drivers, NTP, GPIO/I2C/SPI.
    - Todo módulo cuyo fallo bloquea o corrompe el sistema debe fijarse a Core 0 usando xTaskCreatePinnedToCore(..., 0).
- **Core 1 (APP CPU):**
    - Lógica de usuario/Layrz, parsing, bus, handlers de aplicación, logging/compression (GZIP), tareas QA/background, dumps o persistencia masiva.
    - Tareas de procesamiento que no requieran latencia mínima, puedan ser preemptadas o retrasadas, o deban descargarse del core de sistema.
- **Política técnica:**
    - Se exige declarar explícitamente la asignación de cada task relevante en la documentación técnica y el encabezado del módulo.
    - Todo ISR o driver que afecte la estabilidad va a Core 0; ningún handler/API/log puede compartir core con sistema salvo excepción justificada/auditada.

**Ejemplo (C, ESP-IDF):**
```c
// Sistema crítico
xTaskCreatePinnedToCore(sys_monitor_task, "sys_mon", 4096, NULL, 5, NULL, 0);
// Lógica de usuario/log/QA
xTaskCreatePinnedToCore(gzip_logging_task, "gzlog", 8192, NULL, 3, NULL, 1);
```

Ver referencias oficiales en:
- https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/api-guides/freertos-smp.html
- https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/api-reference/system/freertos.html

---

## Anexo: Política de Uso de PSRAM vs SRAM (ESP32, ESP-IDF 5.5.3)

### Criterios y recomendaciones oficiales para uso de PSRAM y SRAM

- **SRAM (interna):** Siempre usar para funciones/críticos de tiempo real, parser, CRC16, control MessageBus, FSMs, ISR, variables globales y estructuras de control lógico-temporal donde la latencia y la determinismo son imprescindibles (heap_caps_malloc con MALLOC_CAP_INTERNAL/MALLOC_CAP_8BIT). Nunca usar PSRAM para ISR/alta prioridad.
- **PSRAM (externa):** Usar solo para:
    - Buffers grandes, logs, históricos QA, dumps API, compresión/decompresión GZIP, snapshots, data de uso no crítico en tiempo real.
    - Asignar memoria PSRAM sólo mediante heap_caps_malloc(MALLOC_CAP_SPIRAM) y documentar fallback seguro a SRAM donde sea posible.
    - Todo uso de PSRAM debe pasar pruebas QA extendidas, integridad del heap y benchmarks documentados de performance y corrupción. Si se detecta corrupción o lentitud inaceptable, fallback obligado a SRAM y registro de issue/bug.
- **Buenas prácticas:**
    - Prohibido usar PSRAM para rutinas/calculos críticos o variables accedidas en ISR/tasks sensibles.
    - Auditar y documentar cada transición relevante a PSRAM, asociando referencia contractual, motivo técnico y benchmark.
    - Integrar controles automáticos de heap_caps_check_integrity_all() tras operaciones clave con PSRAM.
    - Configurar, revisar y auditar los Kconfig/build flags ESP-IDF (CONFIG_SPIRAM_* y similares) en cada despliegue/handoff.
    - Mantener evidencia traceable de QA/fallback en logs y pilot_results.
- **Referencias oficiales:**
    - https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/api-reference/system/mem_alloc.html
    - https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/api-guides/external-ram.html
- **Resumen ejecutivo:** Usar siempre SRAM para misión crítica; PSRAM solo para almacenamiento temporal/masivo, con fallback y QA obligatorio.

---

## Anexo: Política de Persistencia, Filesystem y Almacenamiento
- Toda persistencia y almacenamiento no volátil debe realizarse ÚNICAMENTE sobre LittleFS.
- Queda prohibido el uso de SPIFFS en cualquier componente productivo, QA o ejemplo nuevo.
- Justificación: SPIFFS presenta riesgos altos de corrupción de datos; LittleFS es obligatorio para asegurar robustez, integridad, y compatibilidad futura con ESP-IDF y Layrz.
- Ejemplos y fragmentos legacy que mencionen SPIFFS son solo referencia histórica y deben migrarse o ignorarse en el diseño/código nuevos.
- QA: cualquier feature/configuración debe invalidarse si emplea o inicializa SPIFFS, aunque sea por error.

---

## Anexo: Política de Independencia del Legacy y Control de Contaminación Técnica (ISO/IEC 12207)

### Licencia y restricciones de reutilización heredadas
- El código originalmente alojado en `legacy_reference/` incluye componentes y librerías de terceros bajo diversas licencias open source: MIT, Apache 2.0, BSD-like, GPLv3, LGPL, entre otras (ver archivos LICENSE en cada subdirectorio de `legacy_reference/lib`).
- Para la nueva versión del core y capas superiores, queda absolutamente prohibido reutilizar, copiar, derivar o portar código fuente, fragmentos o implementaciones del legacy, sin importar la licencia original de cada parte.
- Únicamente los contratos funcionales, requerimientos y diagramas documentados pueden ser usados como base para la reimplementación; el código fuente, tests, helpers y utilidades previos NO se heredan.
- Esto garantiza que el producto resultante estará libre de “contaminación legal” y con una nueva licencia a determinar por la organización (y lo hará auditablemente conforme ISO/IEC 12207).
- Cuando en el futuro se integren APIs externas (geolocación, IP, NTP), se recomienda auditar y documentar la licencia/uso permitido de cada proveedor/servicio ANTES de liberar a producción.


### 1. Propósito
Establecer el lineamiento formal para que la reimplementación del CORE y las capas superiores del sistema LHOS-AEGIS-360 sea realizada completamente independiente del código legacy ("legacy_reference"), corrigiendo desviaciones y errores históricos, en conformidad con los principios arquitectónicos documentados en `/docs`.

### 2. Referencias Normativas
- ISO/IEC/IEEE 12207:2017 Ciclo de Vida de Software, procesos de Implementación y Verificación.
- docs/ARCHITECTURE.md, docs/implementation_plan.md, docs/BOOT_FLOW.md, y demás documentación contractual del sistema.

### 3. Declaración de Independencia Funcional
Toda nueva implementación del core y capas superiores debe:
- Evitar explícitamente la reutilización, portado, copia, adaptación o derivación directa de funciones, clases o fragmentos de código del código legacy (`legacy_reference/`), incluso en casos donde dicho legacy funcione.
- Basarse únicamente en el análisis de requisitos funcionales y de arquitectura extraídos de los documentos en `/docs`.

### 4. Metodología de diseño y construcción
- El diseño, implementación y pruebas deben partir de cero, siguiendo interfaces, contratos y restricciones indicados en la arquitectura y especificaciones actuales.
- Está prohibido replicar patrones de gestión de memoria, multitarea, sincronización o ownership que no estén validados y formalizados en la documentación contractual.
- Cualquier mejora, corrección o explicitación de contratos respecto al legacy debe documentarse como cambio deliberado, justificado y auditado.

#### 4.1 Restricción técnica de stack y librerías externas
- Queda prohibido el uso de cualquier librería Arduino y de terceros fuera de los componentes oficiales Espressif.
- El desarrollo y build debe realizarse exclusivamente sobre ESP-IDF v5.5.3, única versión autorizada para compilación y despliegue.
- El reloj del MCU debe sincronizarse obligatoriamente con hasta dos servidores NTP activos.
- PRIORIDAD: El uso de servidores NTP enviados por ConfIoT solo aplica desde el Sprint de integración ConfIoT. Hasta ese momento, SOLO se usan los internos predeterminados: 'pool.ntp.org', 'time.google.com'. La UI/log sólo muestra 'NTP/Default' en ese caso.
- El fallback entre NTP está requerido (si falla el primero, usar el segundo; si ambos fallan, dejar log crítico y mostrar alerta en UI/monitor).
- El formato de fecha/hora mostrado/log debe ser ISO8601 UTC con país ISO (ej: '2026-05-18 17:31 UTC (BR)'). Si no puede detectarse el país localmente, debe usarse un servicio público (myip.com o similar) para determinarlo.
- Toda excepción (dependencia externa, aunque sea open source/Espressif extra, etc.) debe justificarse por escrito y contar con aprobación explícita del equipo de Arquitectura previamente a su uso/documentación.

### 5. Criterios de calidad para evitar contaminación legacy
- No se permiten dependencias técnicas, llamadas, wrappers ni adaptadores que faciliten uso directo o indirecto del código legacy.
- Toda función/módulo crítico debe incluir comentario de encabezado indicando: `"Este código no está basado en legacy_reference; cumple los contratos de /docs/ARCHITECTURE.md"` (u otro doc relevante).
- Pruebas unitarias y de integración deben validar los contratos explicitados en docs, no la replicación de bugs/errores legacy.

### 6. Proceso de verificación (auditoría interna)
- Todo PR relevante será auditable mediante:
  - Comparación de historial de commits y detección automática de fragmentos copi-pegados o dependencias cruzadas.
  - Validación por parte del equipo de Arquitectura de que el diseño es nuevo, alineado al contrato actual y que los errores históricos han sido mitigados.
- Checklist de revisión:
  1. No hay coincidencias textuales significativas con legacy_reference.
  2. Todas las APIs cumplen lo definido en los documentos actuales.
  3. Las desviaciones están documentadas y justificadas.
  4. Evidencia de testing automatizado y verificación contractual.

### 7. Control documental y trazabilidad
- Todo artefacto relevante (features, issues, tests, decisiones técnicas) debe vincularse mediante IDs a requisitos y secciones específicas de docs, no de legacy_reference.
- Cambios a este anexo o a las políticas de independencia deben ser aprobados por Arquitectura y QA, y quedar reflejados en control de cambios formal.

---

**Esta política es de cumplimiento obligatorio para todo el equipo de desarrollo y será parte de las auditorías documentales regulares bajo ISO/IEC 12207.**

---
