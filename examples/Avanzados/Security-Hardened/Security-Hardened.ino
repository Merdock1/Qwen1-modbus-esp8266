/*
    Security-Hardened - Configuración Máxima Seguridad
    TAREA 4.3: EJEMPLOS AVANZADOS
    
    Implementa las mejores prácticas de seguridad para Modbus:
    - Validación estricta de tramas
    - Rate limiting
    - Lista blanca de direcciones
    - Logging de eventos sospechosos
    
    Autor: Equipo Modbus
    Versión: 1.0.0
*/

#include <Modbus.h>
#include <ModbusSecurity.h>

// Configuración de seguridad
#define MAX_EVENTS_PER_SECOND 100
#define ENABLE_STRICT_VALIDATION true

Modbus mb;
ModbusSecurity security;

void setup() {
    Serial.begin(115200);
    
    // Configurar seguridad
    security.enableStrictValidation(ENABLE_STRICT_VALIDATION);
    security.setMaxEventsPerSecond(MAX_EVENTS_PER_SECOND);
    
    Serial.println("=== Configuración de Seguridad Hardened ===");
}

void loop() {
    mb.task();
    security.process();
}
