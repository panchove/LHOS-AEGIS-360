# ISO12207 Guidelines

## SDKCONFIG_ZOMBIE_PREVENTION (Regla vinculante)

Cada vez que se modifica `sdkconfig.defaults`, se debe ELIMINAR `sdkconfig` ANTES de reconstruir el proyecto. Esta regla evita el efecto "zombie" donde un `sdkconfig` antiguo anula los cambios en `sdkconfig.defaults`.

Procedimiento obligatorio:

```bash
# 1. Editar `sdkconfig.defaults` con el editor de preferencia
nano sdkconfig.defaults

# 2. Eliminar el sdkconfig zombie
rm -f sdkconfig

# 3. Reconstruir desde cero (regenera sdkconfig desde defaults)
idf.py set-target esp32s3
idf.py reconfigure
idf.py build
```

Script de verificación (colocar en la raíz del repo y dar permiso de ejecución):

```bash
#!/bin/bash
# verify_sdkconfig.sh - Ejecutar antes de cada build

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
```

Flujo de ejemplo (rápido):

```bash
cd /path/to/LHOS-AEGIS-360

# 1. Editar defaults
cat >> sdkconfig.defaults << 'EOF'
CONFIG_ESP32S3_SPIRAM_SUPPORT=y
CONFIG_SPIRAM_SIZE=8388608
EOF

# 2. Eliminar zombie
rm -f sdkconfig

# 3. Limpiar build (opcional pero recomendado)
idf.py fullclean

# 4. Regenerar y construir
idf.py set-target esp32s3
idf.py reconfigure
idf.py build

# 5. Flash y monitor
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

Comprobación rápida (post-build):

```bash
grep -E "CONFIG_ESP32S3_SPIRAM|CONFIG_ESPTOOLPY_FLASH" sdkconfig || true
ls -la sdkconfig.defaults sdkconfig
```

Razonamiento: eliminar `sdkconfig` obliga a que `idf.py set-target` y `idf.py reconfigure` regeneren la configuración desde `sdkconfig.defaults`, evitando que flags previos (zombies) persistan en el binario final.

---

Confirmación de THOR: aplicaré esta regla en cada cambio de `sdkconfig.defaults` y ejecutaré `verify_sdkconfig.sh` antes de builds importantes.
