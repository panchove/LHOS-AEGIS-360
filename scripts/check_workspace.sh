#!/bin/bash
# Verificar que NO se está usando workspace externo

REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)

if [ "$PWD" != "$REPO_ROOT" ]; then
    echo "❌ ERROR: Estás en $PWD"
    echo "   Todos los comandos deben ejecutarse desde la raíz del repositorio:"
    echo "   cd $REPO_ROOT"
    exit 1
fi

if [[ "$PWD" == *"aegis-workspace"* ]]; then
    echo "❌ ERROR: Prohibido usar ~/aegis-workspace/"
    echo "   Todo el desarrollo debe estar dentro de LHOS-AEGIS-360/"
    exit 1
fi

echo "✅ Workspace correcto: $PWD"
