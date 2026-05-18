#!/usr/bin/env bash
set -euo pipefail
# QA/check-contract.sh - Verifica reglas contractuales mínimas
# - No referencias a SPIFFS fuera de legacy_reference/
# - No uso de malloc/new/delete en core paths (except pool.cpp -> heap_caps_*)
# - Detecta hints de PSRAM/8MB en sdkconfig files

TARGET=${1:-$(pwd)}
EXIT_CODE=0

echo "Running QA contract checks in $TARGET"

echo "1) Checking for SPIFFS references outside legacy_reference (excluding docs/ and sprints)..."
if grep -R --line-number --exclude-dir=legacy_reference --exclude-dir=docs --exclude-dir=sprints -n "SPIFFS" "$TARGET" >/dev/null 2>&1; then
  echo "[ERROR] Found SPIFFS references in code outside legacy_reference and docs:" >&2
  grep -R --line-number --exclude-dir=legacy_reference --exclude-dir=docs --exclude-dir=sprints -n "SPIFFS" "$TARGET" || true
  EXIT_CODE=2
else
  echo "[OK] No SPIFFS references outside legacy_reference"
fi

echo "2) Checking for malloc/new/delete usage in core paths (pool.cpp is exception)..."
FAIL=0
while IFS= read -r file; do
    if [[ "$file" == *"components/pool/pool.cpp" ]]; then
        # pool.cpp: only heap_caps_malloc/heap_caps_free allowed; disallow plain malloc/free/new/delete
        if grep -n -E "\<new\>|\<delete\>|\<malloc\>|\<free\>" "$file" | grep -v -E "heap_caps_malloc|heap_caps_free|heap_caps_[a-z_]+" >/dev/null 2>&1; then
            echo "❌ $file contiene new/delete/malloc/free sin heap_caps_" >&2
            grep -n -E "\<new\>|\<delete\>|\<malloc\>|\<free\>" "$file" | grep -v -E "heap_caps_malloc|heap_caps_free|heap_caps_[a-z_]+" || true
            FAIL=1
        fi
        continue
    fi

    if grep -n -E "\<new\>|\<delete\>|\<malloc\>|\<free\>" "$file" >/dev/null 2>&1; then
        echo "❌ $file contiene new/delete/malloc/free (no permitido en core)" >&2
        grep -n -E "\<new\>|\<delete\>|\<malloc\>|\<free\>" "$file" || true
        FAIL=1
    fi
done < <(find components/ src/ core/ main/ -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null || true)

if [ "$FAIL" -eq 1 ]; then
    exit 1
fi
echo "[OK] No disallowed malloc/new/delete found in checked core paths"

echo "3) Checking sdkconfig files for PSRAM/8MB hints (N8R8)..."
PSRAM_OK=0
for f in sdkconfig sdkconfig.defaults; do
  if [ -f "$f" ]; then
    if grep -E "PSRAM|SPIRAM|8M|8MB|CONFIG_SPIRAM|CONFIG_ESP32_SPIRAM|CONFIG_ESP32S3_SPIRAM_SUPPORT" "$f" >/dev/null 2>&1; then
      echo "[OK] Found PSRAM/8MB hints in $f"
      PSRAM_OK=1
    fi
  fi
done
if [ $PSRAM_OK -eq 0 ]; then
  echo "[WARN] No explicit PSRAM/8MB hint found in sdkconfig files. Ensure target hardware N8R8 is documented." >&2
fi

echo "QA contract checks passed"
exit 0
