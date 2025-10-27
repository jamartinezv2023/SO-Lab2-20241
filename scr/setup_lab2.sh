#!/bin/bash
# ==========================================================
# setup_lab2.sh
# Autor: José Martínez V.
# Entorno: Ubuntu sobre WSL (Windows 10/11)
# Proyecto: Laboratorio 2 - Shell "wish" (Sistemas Operativos)
# Fecha: Octubre 2025
# ==========================================================
# Descripción:
# Este script automatiza la compilación, ejecución y validación
# del shell Unix "wish". Se diseñó con fines docentes y técnicos.
# Su ejecución crea logs detallados y resume resultados de pruebas.
# ==========================================================

PROJECT_DIR="/root/so_lab2"
SRC_DIR="$PROJECT_DIR/src"
TEST_DIR="$PROJECT_DIR/tests"
BIN_DIR="$PROJECT_DIR/bin"
LOG_FILE="$PROJECT_DIR/report_lab2.log"
EXECUTABLE="wish"

echo "==========================================================="
echo "🧪  LABORATORIO 2 - SISTEMAS OPERATIVOS (UdeA 2025-2)"
echo "🔧  Shell: WISH - Wisconsin Shell"
echo "==========================================================="

if ! command -v gcc &> /dev/null; then
  echo "❌ GCC no está instalado. Instálalo con: sudo apt install build-essential"
  exit 1
fi

if ! [ -d "$SRC_DIR" ]; then
  echo "❌ No se encuentra la carpeta src/. Verifica la estructura del proyecto."
  exit 1
fi

if ! [ -d "$TEST_DIR" ]; then
  echo "❌ No se encuentra la carpeta tests/. Asegúrate de haber descomprimido tester.zip"
  exit 1
fi

mkdir -p "$BIN_DIR"

echo "[1] 🔧 Compilando código fuente..."
cd "$SRC_DIR" || exit 1

if [ -f "$EXECUTABLE" ]; then
  rm -f "$EXECUTABLE"
fi

gcc -Wall -Wextra -g -o "$BIN_DIR/$EXECUTABLE" wish.c 2>&1 | tee "$LOG_FILE"

if [ $? -ne 0 ]; then
  echo "❌ Error de compilación. Revisa $LOG_FILE para más detalles."
  exit 1
else
  echo "✅ Compilación exitosa. Ejecutable generado en: $BIN_DIR/$EXECUTABLE"
fi

echo "[2] 🧩 Ejecutando pruebas..."
cd "$TEST_DIR" || exit 1

if [ -f "test_wish.sh" ]; then
  chmod +x test_wish.sh
  ./test_wish.sh > "$PROJECT_DIR/tests_output.log" 2>&1
  echo "✅ Pruebas ejecutadas correctamente."
else
  echo "❌ No se encontró test_wish.sh en $TEST_DIR"
  exit 1
fi

echo "[3] 📊 Analizando resultados..."
if grep -q "PASS" "$PROJECT_DIR/tests_output.log"; then
  PASSED=$(grep -c "PASS" "$PROJECT_DIR/tests_output.log")
  FAILED=$(grep -c "FAIL" "$PROJECT_DIR/tests_output.log")
  echo "✅ $PASSED pruebas superadas."
  echo "⚠️  $FAILED pruebas fallidas."
else
  echo "❌ No se encontraron resultados válidos en el log."
fi

echo "===========================================================" >> "$LOG_FILE"
echo "Fecha de ejecución: $(date)" >> "$LOG_FILE"
echo "Resumen de resultados:" >> "$LOG_FILE"
grep -E "PASS|FAIL" "$PROJECT_DIR/tests_output.log" >> "$LOG_FILE"
echo "===========================================================" >> "$LOG_FILE"
echo "🗂 Log completo en: $LOG_FILE"

echo "==========================================================="
echo "🎓 Script docente completado con éxito."
echo "Ahora puedes revisar docs/README_sustentacion.md"
echo "==========================================================="
