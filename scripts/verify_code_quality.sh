#!/bin/bash
# Script de Verificación de Calidad de Código Modbus
# Versión 1.0 - Seguridad y Optimización

echo "=========================================="
echo "  Script de Verificación de Calidad"
echo "  Modbus ESP8266/AVR Library"
echo "=========================================="
echo ""

REPORT_FILE="${1:-quality_report_$(date +%Y%m%d_%H%M%S).txt}"

echo "Generando reporte: $REPORT_FILE"
echo ""

# Inicializar reporte
{
    echo "=========================================="
    echo "  REPORTE DE CALIDAD DE CÓDIGO MODBUS"
    echo "  Fecha: $(date)"
    echo "=========================================="
    echo ""
} > "$REPORT_FILE"

# Contador de errores
ERRORS=0
WARNINGS=0

# Función para registrar error
log_error() {
    echo "[ERROR] $1" | tee -a "$REPORT_FILE"
    ((ERRORS++))
}

# Función para registrar warning
log_warning() {
    echo "[WARNING] $1" | tee -a "$REPORT_FILE"
    ((WARNINGS++))
}

# Función para registrar info
log_info() {
    echo "[INFO] $1" | tee -a "$REPORT_FILE"
}

echo "=== FASE 1: Análisis de Estructura ===" | tee -a "$REPORT_FILE"

# Verificar existencia de archivos críticos
CRITICAL_FILES=(
    "src/Modbus.h"
    "src/ModbusRTU.h"
    "src/ModbusAPI.h"
)

for file in "${CRITICAL_FILES[@]}"; do
    if [ -f "$file" ]; then
        log_info "Archivo crítico encontrado: $file"
    else
        log_error "Archivo crítico NO encontrado: $file"
    fi
done

echo "" | tee -a "$REPORT_FILE"
echo "=== FASE 2: Búsqueda de Vulnerabilidades Críticas ===" | tee -a "$REPORT_FILE"

# Buscar funciones inseguras
log_info "Buscando uso de funciones inseguras (strcpy, strcat, sprintf sin límites)..."

UNSAFE_CALLS=$(grep -rn "strcpy\|strcat\|sprintf" src/*.h src/*.cpp 2>/dev/null | grep -v "strncpy\|strncat\|snprintf" || true)
if [ -n "$UNSAFE_CALLS" ]; then
    log_warning "Funciones inseguras detectadas:"
    echo "$UNSAFE_CALLS" | tee -a "$REPORT_FILE"
else
    log_info "No se detectaron funciones inseguras obvias"
fi

echo "" | tee -a "$REPORT_FILE"
echo "=== FASE 3: Balance de Memoria ===" | tee -a "$REPORT_FILE"

# Contar malloc/free
MALLOC_COUNT=$(grep -rn "malloc" src/*.h src/*.cpp 2>/dev/null | wc -l)
FREE_COUNT=$(grep -rn "free(" src/*.h src/*.cpp 2>/dev/null | wc -l)
NEW_COUNT=$(grep -rn "\\bnew\\b" src/*.h src/*.cpp 2>/dev/null | wc -l)
DELETE_COUNT=$(grep -rn "\\bdelete\\b" src/*.h src/*.cpp 2>/dev/null | wc -l)

log_info "Conteo de gestión de memoria:"
log_info "  malloc: $MALLOC_COUNT"
log_info "  free: $FREE_COUNT"
log_info "  new: $NEW_COUNT"
log_info "  delete: $DELETE_COUNT"

if [ "$MALLOC_COUNT" -ne "$FREE_COUNT" ]; then
    log_warning "Desbalance malloc/free: $((MALLOC_COUNT - FREE_COUNT)) asignaciones sin liberar"
fi

if [ "$NEW_COUNT" -ne "$DELETE_COUNT" ]; then
    log_warning "Desbalance new/delete: $((NEW_COUNT - DELETE_COUNT)) asignaciones sin liberar"
fi

echo "" | tee -a "$REPORT_FILE"
echo "=== FASE 4: Validación de Límites memcpy ===" | tee -a "$REPORT_FILE"

MEMCPY_CALLS=$(grep -rn "memcpy\|memmove" src/*.h src/*.cpp 2>/dev/null || true)
if [ -n "$MEMCPY_CALLS" ]; then
    log_info "Llamadas memcpy/memmove encontradas: $(echo "$MEMCPY_CALLS" | wc -l)"
    # Verificar si hay validación de límites
    SAFE_MEMCPY=$(grep -rn "memcpy\|memmove" src/*.h src/*.cpp 2>/dev/null | grep -i "min\|max\|limit\|size" | wc -l)
    log_info "Llamadas con validación aparente de límites: $SAFE_MEMCPY"
fi

echo "" | tee -a "$REPORT_FILE"
echo "=== FASE 5: Consistencia de Comentarios ===" | tee -a "$REPORT_FILE"

# Contar comentarios en español vs inglés
COMMENTS_ES=$(grep -rn "//.*[áéíóúñÁÉÍÓÚÑ]" src/*.h src/*.cpp 2>/dev/null | wc -l)
COMMENTS_EN=$(grep -rn "//.*\(the\|and\|or\|but\|if\|then\|else\)" src/*.h src/*.cpp 2>/dev/null | wc -l)

log_info "Comentarios en español (aproximado): $COMMENTS_ES"
log_info "Comentarios en inglés (aproximado): $COMMENTS_EN"

echo "" | tee -a "$REPORT_FILE"
echo "=== FASE 6: Documentación Doxygen ===" | tee -a "$REPORT_FILE"

DOXYGEN_BLOCKS=$(grep -rn "/\*\*" src/*.h 2>/dev/null | wc -l)
log_info "Bloques de documentación Doxygen encontrados: $DOXYGEN_BLOCKS"

echo "" | tee -a "$REPORT_FILE"
echo "=== FASE 7: Uso de PROGMEM ===" | tee -a "$REPORT_FILE"

PROGMEM_USAGE=$(grep -rn "PROGMEM" src/*.h src/*.cpp 2>/dev/null | wc -l)
log_info "Usos de PROGMEM encontrados: $PROGMEM_USAGE"

echo "" | tee -a "$REPORT_FILE"
echo "==========================================" | tee -a "$REPORT_FILE"
echo "  RESUMEN FINAL" | tee -a "$REPORT_FILE"
echo "==========================================" | tee -a "$REPORT_FILE"
log_info "Errores críticos: $ERRORS"
log_info "Warnings: $WARNINGS"

if [ "$ERRORS" -eq 0 ] && [ "$WARNINGS" -eq 0 ]; then
    echo "✅ PASÓ: Sin errores ni warnings críticos" | tee -a "$REPORT_FILE"
    exit 0
elif [ "$ERRORS" -eq 0 ]; then
    echo "⚠️  PASÓ CON WARNINGS: $WARNINGS warnings encontrados" | tee -a "$REPORT_FILE"
    exit 0
else
    echo "❌ FALLÓ: $ERRORS errores críticos encontrados" | tee -a "$REPORT_FILE"
    exit 1
fi
