#!/bin/bash
# verify_sdkconfig.sh - Ejecutar antes de cada build

set -euo pipefail

if [ -f sdkconfig ] && [ sdkconfig.defaults -nt sdkconfig ]; then
    echo "⚠️  sdkconfig.defaults es más reciente que sdkconfig"
    echo "   Eliminando sdkconfig zombie..."
    rm -f sdkconfig
fi

if [ ! -f sdkconfig ]; then
    echo "✅ Regenerando sdkconfig desde defaults"
    cp sdkconfig.defaults sdkconfig
    idf.py set-target esp32s3
fi

echo "verify_sdkconfig: OK"
