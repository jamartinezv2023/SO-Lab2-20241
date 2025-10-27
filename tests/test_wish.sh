#!/bin/bash
# ============================================================
# 🧪 SCRIPT DE PRUEBAS AUTOMÁTICAS PARA WISH
# ============================================================

TEST_DIR="$(dirname "$0")"
BIN_DIR="$TEST_DIR/../bin"
INPUT_DIR="$TEST_DIR/input"
EXPECTED_DIR="$TEST_DIR/expected"
OUTPUT_DIR="$TEST_DIR/output"

mkdir -p "$OUTPUT_DIR"

TOTAL=0
PASSED=0

echo "==========================================================="
echo "🧪 INICIANDO PRUEBAS PARA WISH"
echo "==========================================================="

for test_input in "$INPUT_DIR"/*.in; do
  test_name=$(basename "$test_input" .in)
  expected_output="$EXPECTED_DIR/$test_name.out"
  actual_output="$OUTPUT_DIR/$test_name.actual"

  echo "-----------------------------------------------------------"
  echo "🔹 Ejecutando prueba: $test_name"
  echo "-----------------------------------------------------------"
  echo "📜 Comando de prueba:"
  cat "$test_input"
  echo "-----------------------------------------------------------"

  "$BIN_DIR/wish" < "$test_input" > "$actual_output" 2>&1

  echo "✅ Salida esperada:"
  cat "$expected_output"
  echo "-----------------------------------------------------------"
  echo "🧩 Salida obtenida:"
  cat "$actual_output"
  echo "-----------------------------------------------------------"

  if diff -q "$expected_output" "$actual_output" >/dev/null; then
    echo "✅ Resultado: PRUEBA SUPERADA"
    ((PASSED++))
  else
    echo "❌ Resultado: FALLÓ"
  fi
  ((TOTAL++))
done

echo "==========================================================="
echo "🏁 RESULTADO FINAL"
echo "==========================================================="
echo "✅ Pruebas superadas: $PASSED / $TOTAL"

if [ "$TOTAL" -gt 0 ]; then
  PERCENT=$((PASSED * 100 / TOTAL))
  echo "📊 Porcentaje de éxito: $PERCENT%"
else
  echo "⚠️ No se encontraron pruebas para ejecutar."
fi

if [ "$PASSED" -eq "$TOTAL" ]; then
  echo "🎉 TODAS LAS PRUEBAS HAN SIDO SUPERADAS CON ÉXITO."
else
  echo "🔍 Revisa las diferencias en la carpeta $OUTPUT_DIR"
fi
