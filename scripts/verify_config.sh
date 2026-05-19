#!/bin/bash
echo "=== Verificando configuración ==="

# Verificar sdkconfig existe
if [ ! -f sdkconfig ]; then
    echo "❌ sdkconfig no existe - regenerando..."
    cp sdkconfig.defaults sdkconfig
fi

# Verificar PSRAM deshabilitado
if grep -q "CONFIG_ESP32S3_SPIRAM_SUPPORT=y" sdkconfig; then
    echo "⚠️ PSRAM habilitado - hardware no lo soporta. Corrigiendo..."
    sed -i 's/CONFIG_ESP32S3_SPIRAM_SUPPORT=y/# CONFIG_ESP32S3_SPIRAM_SUPPORT is not set/g' sdkconfig
fi

# Verificar target correcto
if ! grep -q "CONFIG_IDF_TARGET=\"esp32s3\"" sdkconfig; then
    echo "❌ Target incorrecto - regenerando..."
    rm -f sdkconfig
    cp sdkconfig.defaults sdkconfig
    idf.py set-target esp32s3 || true
fi

# Verificar no eFuse en código
if grep -r "esp_efuse" --include="*.cpp" --include="*.h" --include="*.hpp" . 2>/dev/null; then
    echo "❌ ERROR: Código contiene esp_efuse"
    exit 1
fi

# Verificar sdkconfig secure boot deshabilitado
if grep -q "CONFIG_SECURE_BOOT_V2_ENABLED=y" sdkconfig; then
    echo "❌ ERROR: Secure Boot habilitado"
    exit 1
fi

echo "✅ Configuración válida"
