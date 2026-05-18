# LHOS-AEGIS-360 — SAFE MODE

## Descripción
Safe mode fuerza operación mínima y máxima recuperabilidad ante fallos sistémicos, bucles de boot, corrupción o presión extrema.

## Policy
- BLE minimal (scanner + pairing core)
- WiFi optional, nunca heavy scan
- No OTA auto-start
- Solo minimal/core servicios (Monitor retro, Recovery API, failsafe serial)

## Triggers automáticos (agregado [CORREGIDO])
Safe Mode se activará automáticamente cuando ocurra cualquiera de los siguientes eventos:

1. Watchdog expirado 3 veces en un intervalo de 60 segundos.
2. Boot loop detectado: más de 3 reinicios en menos de 30 segundos.
3. Fallo de integridad de heap crítico: `esp_heap_caps_check_integrity` indica corrupción en `MALLOC_CAP_INTERNAL`.
4. Pool de paquetes agotado de forma sostenida (>5 segundos sin liberación suficiente).
5. Llamada explícita a la API: `system_enter_safe_mode()` (requiere autenticación/operador).

En todos los casos, Safe Mode deberá generar un evento auditado y persistir la razón en la blackbox antes de degradar servicios.

## Activos
- Recovery API disponible via monitor/serial
- Diagnostic events en EventBus

## Inactivos/Restringidos
- Sin servicio alto/carga
- Sin OTA auto ni scan agresivo
