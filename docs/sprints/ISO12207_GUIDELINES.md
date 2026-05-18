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
