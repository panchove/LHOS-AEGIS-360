# Sprint 01 – Estado

## Estado: IN-PROGRESS (BLOQUEADO por QA práctico)

### Cumplimientos:
- Todos los features están desarrollados y validados en entorno host (gcc/tests unitarios OK).
- Checklist contractual, ownership, heapless y modularidad completados.
- QA de integración funcional, diseño y pipeline PASARON.

### Bloqueo/crítico:
- Falta evidencia de ejecución práctica en hardware ESP32 o CI real:
  - `idf.py build`
  - `idf.py -p <puerto> flash`
  - `idf.py -p <puerto> monitor`
- Ninguna entrega puede pasar a estado DONE ni abrir Sprint 02 hasta que esto sea subsanado.

### Acción requerida
- Realizar pruebas reales en hardware o CI, capturar evidencia/logs y referenciarla aquí.
- Solo entonces mover a "DONE".

### Responsables/Checklist QA:
- QA o devs/designados con acceso a hardware deben ejecutar el ciclo completo y subir evidencia clara (print/screenshot/log monitor/CI approved).

---

Instrumentación, CI, detalles y logs deben quedar adjuntos para cierre contractual. No avanzar siguiente Sprint hasta desbloquear este paso.
