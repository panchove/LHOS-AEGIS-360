# Feature: UI/UX HTTP Monitor Minimalista – LHOS-AEGIS-360

(...) (párrafos previos iguales)

## Sincronización NTP y País

- Antes de la integración con ConfIoT, el Monitor solo utiliza y muestra los NTP predeterminados internos: 'pool.ntp.org' y 'time.google.com'. El campo de fuente dinámica y edición sólo se habilita en Sprint ConfIoT.
- El monitor siempre muestra la hora/fuente UTC obtenida de NTP y el país ISO real.
- Si ambos NTP fallan, alerta crítica y la UI debe mostrar "Sincronización fallida".
- El formato horario en UI siempre será 'AAAA-MM-DD HH:MM UTC (PAIS-ISO)'.
- Si el país no puede determinarse localmente, debe consultarse a una API HTTP pública (myip.com/ipapi).
- La fuente y servidores NTP activos deben ser siempre visibles y auditables por QA.

## Servicio de Watchdog Timer (WDT) y monitoreo de Task Registry

- El sistema embebido y el monitor deben implementar un servicio WDT para todas las tareas críticas.
- El WDT vigila que cada tarea "pinguee" periódicamente, y genera alerta cuando una tarea no responde a tiempo:
    - Se debe mostrar en la UI el estado global del WDT: verde (ok), ámbar (alerta reciente), rojo (timeout).
    - Log de última violación y alertas recientes del WDT deben verse en la OS/System Card.
- El Monitor debe incluir en las cards/principales/minicards una tabla de tareas:
    - Nombre de la tarea
    - Estado: "Ejecutando", "Bloqueada", "Timeout/Dead", "Desconocida"
    - Último heartbeat (UTC ISO), delta (s)
    - Estado WDT (OK/ALERTA)
    - Si hay alerta, resaltar en rojo/ámbar con ícono y tooltip.
- Datos expuestos por la API `/status`/`tasks`.

(...) (el resto de los requerimientos y anexo Layrz no se alteran)
