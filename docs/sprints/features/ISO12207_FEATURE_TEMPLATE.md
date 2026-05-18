# Plantilla de Feature (ISO/IEC/IEEE 12207)

Propósito: plantilla obligatoria para documentar Features bajo el ciclo de vida y procesos definidos por ISO/IEC/IEEE 12207. Usar esta plantilla para todas las entradas en docs/sprints/features.

1. Identificación
- ID de feature: F-{SPRINT}-{NNN}
- Título corto: <Título descriptivo>
- Autor / Propietario: <Nombre, correo>
- Fecha creación: YYYY-MM-DD

2. Alcance y Objetivo (Scope)
- Resumen ejecutivo (2-3 oraciones).
- Limitaciones y exclusiones (qué no cubre esta feature).

3. Referencias Normativas y de Proyecto
- ISO/IEC/IEEE 12207:2017 (citar cláusulas relevantes)
- Documentos de arquitectura: docs/ARCHITECTURE.md
- Requisitos relacionados: ruta a especificación(s)

4. Requisitos (Process: Requirements)
- Requisito funcional 1: descripción, prioridad (Alta/Media/Baja), criterio de aceptación.
- Requisito no funcional 1: latencia, memoria, consumo energético, determinismo, etc.
- Trazabilidad: IDs de requisitos origen (REQ-...)

5. Diseño de Alto Nivel (Process: Architecture & Design)
- Diagrama/flujo de datos (breve): componentes implicados, interfaces, ownership de memoria.
- Afín a: tareas RTOS, afinidad de núcleos, uso DMA, restricciones de ISR.

6. Implementación propuesta (Process: Implementation)
- Archivos/Componentes a crear o modificar (ruta en repo).
- Lenguaje / estándares (C obligatorio, C++ restringido).
- Consideraciones de memoria (pool/heap/PSRAM) y seguridad.

7. Verificación & Validación (Process: Verification & Validation)
- Estrategia de pruebas unitarias (component tests) y de integración.
- Criterios de aceptación medibles (p.ej. throughput, latency p95, no leaks tras N horas).
- Casos de prueba mínimos y Soak test (referencia a tests/ o procedures).

8. Gestión de Configuración y Entregables (Process: Configuration Management)
- Branch/PR policy: PR review, gating rules (docs/handoff/CI_GATING_RULES.md).
- Artefactos esperados: código, pruebas, scripts de medición, changelog.

9. Seguridad y Estabilidad (Process: Operation & Maintenance)
- Impacto en seguridad (si aplica). Políticas de recuperación, watchdogs, brownout.

10. Riesgos y Mitigaciones (Process: Risk Management)
- Riesgo: descripción, probabilidad, severidad, mitigación propuesta.

11. Plan de Trabajo y Cronograma (Process: Project Management)
- Tareas principales, estimación (días/hombre), responsable.

12. Aprobaciones
- Firma/aceptación por: Arquitecto, QA Lead, Product Owner (nombres/fechas)

13. Historial de Cambios
- Fecha — Autor — Cambio realizado — Versión

Notas de uso:
- Cada sección debe mapearse a procesos ISO/IEC/IEEE 12207: Requirements, Architecture/Design, Implementation, Verification, Release, Operation, Maintenance.
- Mantener lenguaje claro, trazable y cuantificable (no ambigüedades en criterios de aceptación).
