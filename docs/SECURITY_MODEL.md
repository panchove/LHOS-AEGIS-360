# LHOS-AEGIS-360 — Security Model (Sprint 01, eFuse-Free)

## Fundamentales
- PROHIBIDO uso de eFuse para cualquier función de seguridad, identidad, bind ni rollback.
- No secure boot hardware, encrypted flash hardware, write-protect hardware, MAC binding por eFuse ni anti-rollback fuse.
- Todo mecanismo recuperable y auditable sólo por software.

## Alternativas permitidas
- Pin activación: vía storage/config software
- Secure Boot: Metadata firmada SW, verificación init
- Flash Encryption: Cifrado lógico NVS/partition, software
- Key Storage: Rotación SW, celda en NVS

## Recovery Paths
- Todo device, cualquier estado → 100% recoverable vía flashing, UART, factory jumper, o OTA
