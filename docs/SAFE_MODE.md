# LHOS-AEGIS-360 — SAFE MODE

## Descripción
Safe mode fuerza operación mínima y máxima recuperabilidad ante fallos sistémicos, bucles de boot, corrupción o presión extrema.

## Policy
- BLE minimal (scanner + pairing core)
- WiFi optional, nunca heavy scan
- No OTA auto-start
- Solo minimal/core servicios (Monitor retro, Recovery API, failsafe serial)

## Activos
- Recovery API disponible via monitor/serial
- Diagnostic events en EventBus

## Inactivos/Restringidos
- Sin servicio alto/carga
- Sin OTA auto ni scan agresivo
