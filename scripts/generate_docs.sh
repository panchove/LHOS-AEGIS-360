#!/bin/bash
# generate_docs.sh - Genera documentación Doxygen

cd "$(git rev-parse --show-toplevel)" || exit 1

echo "=== Generando documentación Doxygen ==="

# Limpiar documentación anterior
rm -rf docs/doxygen

# Ejecutar Doxygen
if ! command -v doxygen >/dev/null 2>&1; then
  echo "❌ doxygen no está instalado. Instálalo con: sudo apt install -y doxygen graphviz"
  exit 2
fi

mkdir -p docs/doxygen

doxygen Doxyfile 2>&1 | tee docs/doxygen/doxygen.log

if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "✅ Documentación generada en docs/doxygen/html/"
    echo "   Abrir con: firefox docs/doxygen/html/index.html"
else
    echo "❌ Error generando documentación. Ver: docs/doxygen/doxygen.log"
    exit 1
fi
