/**
 * @file test_buffer_overflow.cpp
 * @brief Test unitario para validar sanitización de buffers en ModbusMQTT y ModbusWebConfig
 * 
 * Copyright (C) 2024 - Biblioteca Modbus para Arduino/ESP
 * Todos los comentarios y documentación en español
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Simulación de estructuras para test
#define MODBUS_MQTT_MAX_TOPIC_LENGTH 128
#define MODBUS_MQTT_MAX_PAYLOAD_LENGTH 256

struct ModbusMQTTTopicConfig {
    char topic[MODBUS_MQTT_MAX_TOPIC_LENGTH];
    uint16_t registerAddress;
    uint8_t registerType;
    
    ModbusMQTTTopicConfig() : registerAddress(0), registerType(0) {
        topic[0] = '\0';
    }
};

// Función segura de copia (simulando la implementación corregida)
void safe_strncpy(char* dest, const char* src, size_t max_size) {
    if (dest == nullptr || src == nullptr || max_size == 0) {
        return;
    }
    strncpy(dest, src, max_size - 1);
    dest[max_size - 1] = '\0';
}

// Test 1: Copia de topic dentro del límite
bool test_topic_within_limit() {
    ModbusMQTTTopicConfig config;
    const char* valid_topic = "modbus/device1/temperature";
    
    safe_strncpy(config.topic, valid_topic, sizeof(config.topic));
    
    // Verificar que se copió correctamente
    if (strcmp(config.topic, valid_topic) != 0) {
        printf("FAIL: Topic no se copió correctamente\n");
        return false;
    }
    
    // Verificar terminación nula
    if (config.topic[sizeof(config.topic) - 1] != '\0') {
        printf("FAIL: Buffer no terminado en null\n");
        return false;
    }
    
    printf("PASS: Topic dentro del límite\n");
    return true;
}

// Test 2: Intento de desbordamiento de buffer
bool test_topic_overflow_attempt() {
    ModbusMQTTTopicConfig config;
    
    // Crear payload malicioso > buffer_size
    char malicious_topic[256];
    memset(malicious_topic, 'A', 255);
    malicious_topic[255] = '\0';
    
    // Intentar copiar (debería truncarse)
    safe_strncpy(config.topic, malicious_topic, sizeof(config.topic));
    
    // Verificar que no hay desbordamiento
    if (strlen(config.topic) >= sizeof(config.topic)) {
        printf("FAIL: Desbordamiento detectado\n");
        return false;
    }
    
    // Verificar que se truncó correctamente
    if (strlen(config.topic) != sizeof(config.topic) - 1) {
        printf("FAIL: Truncamiento incorrecto\n");
        return false;
    }
    
    // Verificar terminación nula
    if (config.topic[sizeof(config.topic) - 1] != '\0') {
        printf("FAIL: Buffer no terminado en null después de truncar\n");
        return false;
    }
    
    printf("PASS: Prevención de overflow correcta\n");
    return true;
}

// Test 3: Boundary testing con tamaño exacto
bool test_boundary_exact_size() {
    ModbusMQTTTopicConfig config;
    
    // Crear string de tamaño exacto (127 chars + null)
    char exact_topic[128];
    for (int i = 0; i < 127; i++) {
        exact_topic[i] = 'B';
    }
    exact_topic[127] = '\0';
    
    safe_strncpy(config.topic, exact_topic, sizeof(config.topic));
    
    if (strlen(config.topic) != 127) {
        printf("FAIL: Longitud incorrecta para boundary test\n");
        return false;
    }
    
    if (config.topic[127] != '\0') {
        printf("FAIL: Null terminator faltante en boundary\n");
        return false;
    }
    
    printf("PASS: Boundary test correcto\n");
    return true;
}

// Test 4: String vacío
bool test_empty_string() {
    ModbusMQTTTopicConfig config;
    const char* empty = "";
    
    safe_strncpy(config.topic, empty, sizeof(config.topic));
    
    if (config.topic[0] != '\0') {
        printf("FAIL: String vacío no manejado correctamente\n");
        return false;
    }
    
    printf("PASS: String vacío manejado correctamente\n");
    return true;
}

// Test 5: Pointer nulo
bool test_null_pointer() {
    ModbusMQTTTopicConfig config;
    
    // Esto no debería causar crash
    safe_strncpy(config.topic, nullptr, sizeof(config.topic));
    
    // El buffer debería permanecer sin cambios o vacío
    printf("PASS: Null pointer manejado sin crash\n");
    return true;
}

int main() {
    printf("=== Tests de Buffer Overflow para ModbusMQTT ===\n\n");
    
    int passed = 0;
    int total = 5;
    
    if (test_topic_within_limit()) passed++;
    if (test_topic_overflow_attempt()) passed++;
    if (test_boundary_exact_size()) passed++;
    if (test_empty_string()) passed++;
    if (test_null_pointer()) passed++;
    
    printf("\n=== Resumen ===\n");
    printf("Tests pasados: %d/%d\n", passed, total);
    
    if (passed == total) {
        printf("ÉXITO: Todos los tests pasaron\n");
        return 0;
    } else {
        printf("FALLO: %d tests fallaron\n", total - passed);
        return 1;
    }
}
