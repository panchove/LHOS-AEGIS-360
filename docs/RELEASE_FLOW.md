# LHOS-AEGIS-360 — Release Flow

## Naming Policy
- Todos artefactos firmes: lhos_aegis_360_v{VERSION}_b{BUILD}_{PROFILE}.bin
- Metadata embedded: VERSION, GIT_HASH, BUILD, PROFILE

## eFuse Policy
- PROHIBIDO usar secure boot, flash encryption, write-protect, key storage, anti-rollback ni configuración por eFuse
- Policy de autenticación, provisioning, bindout siempre via software, NVS cifrado lógico o partición especial recovery

## Recovery Model
- Todo release/provisioning debe ser 100% recoverable via flashing estándar, UART recovery, OTA o factory jumper

## Proceso
1. Pre-release check: make clean, check-format, check-contract, verify-release
2. Build (por perfil):
    - make build PROFILE=hub1
    - make build PROFILE=hub2
    - make build PROFILE=hub25
3. size para baseline flash/ram usage
4. ota-publish genera release/ota folder
5. Todos los artefactos acompañados de hash SHA256 y changelog VERSION
6. No pasos irreversibles, ningún eFuse step

## Provisioning
- Always rollback possible via flashing/OTA/UART
- Nunca lock hardware ni irreversible
