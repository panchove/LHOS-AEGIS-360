#!/usr/bin/env bash
set -euo pipefail
# QA/check-contract.sh - Verifica reglas contractuales mínimas
# - No referencias a SPIFFS fuera de legacy_reference/
# - No uso de malloc/new/delete en core paths
# - Detecta hints de PSRAM/8MB en sdkconfig files

TARGET=${1:-$(pwd)}
EXIT_CODE=0

echo "Running QA contract checks in $TARGET"

echo "1) Checking for SPIFFS references outside legacy_reference..."
if grep -R --line-number --exclude-dir=legacy_reference -n "SPIFFS" "$TARGET" >/dev/null 2>&1; then
  echo "[ERROR] Found SPIFFS references outside legacy_reference:" >&2
  grep -R --line-number --exclude-dir=legacy_reference -n "SPIFFS" "$TARGET" || true
  EXIT_CODE=2
else
  echo "[OK] No SPIFFS references outside legacy_reference"
fi

echo "2) Checking for malloc/new/delete usage in core paths..."
CORE_PATHS=(components src core main)
FOUND_CORE_ISSUES=0
for p in "${CORE_PATHS[@]}"; do
  if [ -d "$p" ]; then
    # Find files that contain any of the allocation keywords
    while IFS= read -r file; do
      if [[ "$file" == *"components/pool/pool.cpp" ]]; then
        # Pool.cpp may use heap_caps_malloc with MALLOC_CAP_SPIRAM only
        if grep -n "heap_caps_malloc" "$file" | grep -v "MALLOC_CAP_SPIRAM" > /dev/null; then
          echo "[ERROR] pool.cpp using heap_caps_malloc without MALLOC_CAP_SPIRAM:" >&2
          grep -n "heap_caps_malloc" "$file" | grep -v "MALLOC_CAP_SPIRAM" || true
          EXIT_CODE=3
        fi
        if grep -n -E "new|delete|malloc|free" "$file" | grep -v -E "heap_caps_malloc|heap_caps_free" > /dev/null; then
          echo "[ERROR] pool.cpp contains new/delete/malloc/free not allowed (only heap_caps_malloc/heap_caps_free permitted)" >&2
          grep -n -E "new|delete|malloc|free" "$file" | grep -v -E "heap_caps_malloc|heap_caps_free" || true
          EXIT_CODE=3
        fi
        # skip general checks for this file
        continue
      fi

      if grep -n -E "new|delete|malloc|free" "$file" > /dev/null; then
        echo "[WARN] Potential heap usage in $file:" >&2
        grep -n -E "new|delete|malloc|free" "$file" || true
        FOUND_CORE_ISSUES=1
      fi
    done < <(grep -R --line-number --files-with-matches -n -E "new|delete|malloc|free" "$p" || true)
  fi
done

if [ $EXIT_CODE -ne 0 ]; then
  echo "[ERROR] One or more files violated allocation policy (exit $EXIT_CODE)" >&2
  # keep EXIT_CODE as set
elif [ $FOUND_CORE_ISSUES -eq 1 ]; then
  echo "[ERROR] Found heap usage patterns in core paths. Allocations should be avoided in core hot-paths." >&2
  EXIT_CODE=3
else
  echo "[OK] No disallowed malloc/new/delete found in checked core paths"
fi

echo "3) Checking sdkconfig files for PSRAM/8MB hints (N8R8)..."
PSRAM_OK=0
for f in sdkconfig sdkconfig.defaults; do
  if [ -f "$f" ]; then
    if grep -E "PSRAM|SPIRAM|8M|8MB|CONFIG_SPIRAM|CONFIG_ESP32_SPIRAM" "$f" >/dev/null 2>&1; then
      echo "[OK] Found PSRAM/8MB hints in $f"
      PSRAM_OK=1
    fi
  fi
done
if [ $PSRAM_OK -eq 0 ]; then
  echo "[WARN] No explicit PSRAM/8MB hint found in sdkconfig files. Ensure target hardware N8R8 is documented." >&2
fi

if [ $EXIT_CODE -ne 0 ]; then
  echo "QA contract checks failed (exit $EXIT_CODE)" >&2
fi
exit $EXIT_CODE
