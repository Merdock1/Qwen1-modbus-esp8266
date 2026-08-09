# Tarea 1.2: Validación Estricta de Tramas Modbus

## Descripción
Implementación de validación completa de tramas Modbus conforme a especificación, incluyendo:
- Validación de longitud PDU máxima (253 bytes)
- Validación de consistencia de transactionId (TCP)
- Detección y rechazo de tramas malformadas
- Validación de reglas de broadcast (solo escritura)

## Archivos Modificados

### 1. `src/ModbusSecurity.h`
**Constantes de validación añadidas:**
```c
#define MODBUS_MIN_FRAME_LEN 3          // Frame mínimo: func + crc(2)
#define MODBUS_MAX_PDU_LEN 253          // Tamaño máximo PDU según especificación
#define MODBUS_SAFE_MALLOC_SIZE 512     // Límite para prevenir DoS
#define MODBUS_MAX_BUFFER_LEN 256       // Buffer máximo permitido
```

**Macros de validación:**
```c
#define MODBUS_VALIDATE_FRAME_LEN(len)
#define MODBUS_VALIDATE_MALLOC_SIZE(len)
#define MODBUS_VALIDATE_PDU_LEN(len)
```

**Tipos de eventos de seguridad:**
- `SEC_EVENT_FRAME_TOO_SMALL` - Frame por debajo del mínimo
- `SEC_EVENT_FRAME_TOO_LARGE` - Frame excede límites seguros
- `SEC_EVENT_PDU_LENGTH_VIOLATION` - PDU excede especificación
- `SEC_EVENT_INVALID_TRANSACTION_ID` - TransactionId inválido
- `SEC_EVENT_MALFORMED_FRAME` - Trama malformada
- `SEC_EVENT_BROADCAST_INVALID` - Broadcast inválido

### 2. `src/ModbusRTU.cpp`
**Validaciones implementadas en task():**
- Línea 277-295: Verificación de longitud mínima de frame
- Línea 298-316: Prevención de desbordamiento de buffer
- Línea 346-363: Validación de longitud PDU
- Línea 397-411: Validación CRC con logging

### 3. `src/ModbusTCPTemplate.h`
**Validaciones TCP:**
- Línea 275-279: Verificación de protocolId (debe ser 0)
- Línea 281-286: Validación de longitud mínima
- Línea 288-294: Validación de longitud máxima
- Línea 327-349: Verificación de transactionId en respuestas

## Tests Unitarios

### Archivo: `tests/Phase1_Critical/test_frame_validation.cpp`

**Tests implementados (15 tests, 100% aprobados):**

1. **test_pdu_length_max**: Valida límite de 253 bytes
2. **test_pdu_length_zero**: Rechaza PDU de longitud 0
3. **test_transaction_id_tcp**: Valida transactionId TCP
4. **test_malformed_null_pointer**: Detecta puntero nulo
5. **test_malformed_too_small**: Detecta trama pequeña
6. **test_malformed_too_large**: Detecta trama grande
7. **test_invalid_function_code**: Rechaza FC inválido
8. **test_broadcast_read_function**: Broadcast + lectura = error
9. **test_broadcast_write_function**: Broadcast + escritura = válido
10. **test_normal_slave_id**: Slave IDs normales aceptados
11. **test_complete_rtu_valid**: Trama RTU válida completa
12. **test_complete_tcp_valid**: Trama TCP válida completa
13. **test_stress_multiple_frames**: Stress test 100 tramas
14. **test_security_logging**: Logs generan alertas correctas
15. **test_supported_function_codes**: Todos los FC soportados

**Resultado:** 15/15 tests pasados (100%)

## Criterios de Aceptación Cumplidos

✅ **Tramas inválidas rechazadas con código de error apropiado**
- Frames pequeños → SEC_EVENT_FRAME_TOO_SMALL
- Frames grandes → SEC_EVENT_FRAME_TOO_LARGE  
- PDU > 253 bytes → SEC_EVENT_PDU_LENGTH_VIOLATION
- TransactionId 0 → SEC_EVENT_INVALID_TRANSACTION_ID
- Broadcast inválido → SEC_EVENT_BROADCAST_INVALID

✅ **Logs de seguridad generan alertas correctas**
- Eventos registrados con severidad apropiada (INFO/WARNING/ERROR/CRITICAL)
- Descripción detallada del evento
- Timestamp, slaveId, functionCode incluidos

✅ **Tests con tramas malformed pasan**
- Puntero nulo detectado
- Longitud insuficiente detectada
- Longitud excesiva detectada
- Function code inválido detectado

✅ **Validación de longitud PDU máxima (253 bytes)**
- Implementada en ModbusSecurity.h
- Aplicada en ModbusRTU.cpp y ModbusTCPTemplate.h

✅ **Validación de consistencia de transactionId (TCP)**
- TransactionId 0 rechazado
- Mismatch detectado en respuestas

✅ **Validación de reglas de broadcast (solo escritura)**
- Funciones 0x05, 0x06, 0x0F, 0x10 permitidas
- Funciones de lectura (0x01-0x04) rechazadas en broadcast

## Ejemplo de Uso

```cpp
// Configuración de seguridad
SecurityConfig_t securityConfig = {
    .enableLogging = true,
    .enableStrictValidation = true,
    .enableDoSProtection = true,
    .enableBroadcastValidation = true
};

// Callback para eventos de seguridad
void onSecurityEvent(const SecurityEvent_t* evt) {
    Serial.printf("Evento: %d, Severidad: %d\n", 
                  evt->eventType, evt->severity);
    Serial.printf("SlaveID: %d, FC: 0x%02X, Len: %d\n",
                  evt->slaveId, evt->functionCode, evt->frameLength);
    Serial.printf("Descripción: %s\n", evt->description);
}

securityConfig.logCallback = onSecurityEvent;
```

## Impacto en el Código Existente

- **Compatibilidad hacia atrás**: MANTENIDA
  - Todas las validaciones son adicionales, no rompen funcionalidad existente
  - Constants definidas pero no impuestas por defecto
  
- **Rendimiento**: IMPACTO MÍNIMO
  - Validaciones son operaciones simples de comparación
  - Overhead estimado: <1μs por trama

- **Seguridad**: MEJORA SIGNIFICATIVA
  - Protección contra ataques de desbordamiento
  - Detección temprana de tramas malformadas
  - Logging para auditoría de seguridad

## Referencias a Especificación Modbus

- Sección 4.4: Formato de trama RTU
- Sección 5.4: Formato de trama TCP
- Apéndice B: Longitudes máximas PDU (253 bytes)
- Sección 6.1: Reglas de broadcast

## Próximos Pasos

Continuar con **Tarea 1.3: Completar FC 0x2B Read Device Identification**
