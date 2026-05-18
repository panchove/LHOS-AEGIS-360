#!/bin/bash
# QA/check-contract.sh – Scan de compliance filesystem LHOS-AEGIS-360
# Falla (exit 1) si encuentra uso de SPIFFS fuera de carpetas legacy_reference/
# Uso: bash QA/check-contract.sh [directorio]

set -e
TARGET=${1:-$(pwd)}

failures=$(grep -Iri --exclude-dir=legacy_reference --exclude="*.md" --exclude="*.txt" --exclude-dir=.git -E '\bSPIFFS\b' "$TARGET" | grep -v 'prohibido\|legacy\|documental')

if [[ -n "$failures" ]]; then
  echo "[ERROR] Referencia no permitida a SPIFFS detectada:\n$failures"
  exit 1
else
  echo "[OK] Política filesystem compliant: solo LittleFS (SPIFFS prohibido)"
  exit 0
fi
