# Plantilla de Issue / Incidente (ISO/IEC/IEEE 12207)

Propósito: uso obligatorio para registrar problemas técnicos, incidentes de integración, bugs críticos y desviaciones respecto a la definición contractual. Esta plantilla garantiza trazabilidad y cumplimiento con ISO/IEC/IEEE 12207.

1. Identificación
- ID de issue: I-{SPRINT}-{NNN}
- Título corto: <Título descriptivo>
- Reportado por: <Nombre, correo>
- Fecha reporte: YYYY-MM-DD

2. Resumen ejecutivo
- Descripción breve del síntoma/impacto (máx. 3-4 líneas).

3. Severidad y Prioridad
- Severidad: Critical / High / Medium / Low
- Prioridad: P0 / P1 / P2 / P3

4. Contexto y Reproducción
- Entorno (board, firmware version, toolchain, perfil build).
- Pasos exactos para reproducir (comandos, scripts, datos de input).
- Logs/Stack traces/artefactos adjuntos (referencias a /docs/sprints/pilot_results o carpeta de evidencia).

5. Impacto
- Systems affected (modules/components).
- Degradación observada (throughput, latency, crashes, OOM, memory corruption).

6. Análisis inicial
- Hipótesis raíz (root-cause hypothesis).
- Evidencia recopilada (logs, dumps, counters).

7. Contención temporal (Workarounds)
- Medidas inmediatas para mitigar impacto en campo o CI (p.ej. revert cambio, configuración temporal).

8. Acciones correctivas propuestas
- Plan de reparación: pasos, owner, estimación de esfuerzo, pruebas de verificación.

9. Verificación y Criterios de Cierre
- Qué pruebas deben pasar para cerrar el issue (unit, integration, soak, regression).
- Métricas objetivo (sin leaks por 4h, CPU < X%, p95 latency < Y ms).

10. Trazabilidad a Artefactos
- Referencias a commits/PRs, tests, documentación modificada (IDs o URLs).

11. Lecciones Aprendidas / Acciones Preventivas
- Cambios en procesos / CI / pruebas requeridos para evitar recurrencia.

12. Aprobaciones y Cierre
- QA verification: nombre/fecha
- Product owner: nombre/fecha
- Ingeniero responsable del fix: nombre/PR/commit

13. Historial
- Fecha — Autor — Acción/Comentario

Notas de uso:
- Cada issue debe mapearse con pruebas y artefactos. No cerrar issue sin evidencia reproducible y pruebas automatizadas que cubran el caso.
