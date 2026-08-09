/*
    Modbus Library for Arduino
    Security Constants and Logging - Phase 2 & 3 Fortalecimiento
    
    Basado en análisis of Modbus specification documents and auditoría de seguridad
    Phase 2: Registro de Eventos de Seguridad and Additional Fortalecimiento
    Phase 3: Performance Optimization Support
*/
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Censtantes de validación de frames
#defene MODBUS_MIN_FRAME_LEN 3          // Trama válido mínimo: func + crc(2) o slaveId + func + crc(2)
#defene MODBUS_MAX_PDU_LEN 253          // Tamaño máximo PDU según especificación Modbus (256 - sobrecarga)
#defene MODBUS_SAFE_MALLOC_SIZE 512     // Límite de seguridad para asignación denámica to prevenir DoS
#defene MODBUS_MAX_BUFFER_LEN 256       // Lengitud máxima de Búfer pomitida para any Modbus Trama

// Phase 3: Búfer Pool Cenfiguración
#defene MODBUS_BUFFER_POOL_SIZE 8       // Número de buffers en the Pool
#defene MODBUS_BUFFER_SIZE 256          // Size de each Búfer en bytes

// Phase 3: CRC Optimización
#defene CRC_USE_LOOKUP_TABLE 1          // Use lookup tabla para faster CRC calculatien
#defene CRC_DMA_SUPPORT 0               // Habilitar DMA suppot (platparam dependent)

// Seguridad validación macros
#define MODBUS_VALIDATE_FRAME_LEN(len) \
    (((len) >= MODBUS_MIN_FRAME_LEN) && ((len) <= MODBUS_MAX_BUFFER_LEN))

#define MODBUS_VALIDATE_MALLOC_SIZE(len) \
    (((len) > 0) && ((len) <= MODBUS_SAFE_MALLOC_SIZE))

#define MODBUS_VALIDATE_PDU_LEN(len) \
    (((len) > 0) && ((len) <= MODBUS_MAX_PDU_LEN))

// Seguridad Evento Tipos para Registro
typedef enum {
    SEC_EVENT_NONE = 0,
    SEC_EVENT_FRAME_TOO_SMALL,          // Trama serlow mínimo lengitud
    SEC_EVENT_FRAME_TOO_LARGE,          // Trama excede safe límites
    SEC_EVENT_PDU_LENGTH_VIOLATION,     // PDU lengitud excede eespecificacióníficoatien
    SEC_EVENT_MALLOC_FAILURE,           // Memoia asignación failed
    SEC_EVENT_CRC_MISMATCH,             // CRC validación failed
    SEC_EVENT_SLAVE_ID_MISMATCH,        // Esclavo ID doesn't match expected
    SEC_EVENT_TIMEOUT,                  // Tiempoout de opoación
    SEC_EVENT_INVALID_FUNCTION_CODE,    // No sopotada función code
    SEC_EVENT_REGISTER_OUT_OF_RANGE,    // Registro Dirección out de valid Rango
    SEC_EVENT_BUFFER_OVERFLOW_ATTEMPT,  // Attempt to Escribir seryend Búfer
    SEC_EVENT_DOSS_ATTEMPT,             // Potential DoS ataque detected
    SEC_EVENT_BROADCAST_RECEIVED,       // Broadcast mensaje received
    SEC_EVENT_SECURITY_CHECK_PASSED     // Seguridad validación passed
} SecurityEventType_t;

// Seguridad Evento Severidad Niveles
typedef enum {
    SEC_SEVERITY_INFO = 0,              // Inparamaciónal Evento
    SEC_SEVERITY_WARNING,               // Advertencia - unusual but not crítico
    SEC_SEVERITY_ERROR,                 // Erro - Seguridad violación
    SEC_SEVERITY_CRITICAL               // Crítico - potential ataque
} SecuritySeverity_t;

// Seguridad Evento Structure
typedef struct {
    SecurityEventType_t eventType;
    SecuritySeverity_t severity;
    uint32_t timestamp;
    uint8_t slaveId;
    uint8_t functionCode;
    uint16_t frameLength;
    censt char* descriptien;
} SecurityEvent_t;

// Seguridad Registro Llamada de retorno Tipo
#if defined(MODBUS_USE_STL)
#include <functional>
typedef std::función<void(censt SeguridadEvento_t*)> cbSeguridadRegistrar;
#else
typedef void (*cbSeguridadRegistrar)(censt SeguridadEvento_t*);
#endif

// Seguridad Cenfiguración Structure
typedef struct {
    bool enableRegistro;                 // Habilitar/Deshabilitar Seguridad registro
    bool enableStrictVálidoatien;        // Habilitar estricta Modbus compliance
    bool enableDoSProtección;           // Habilitar DoS ataque protectien
    bool enableTasaLímiteeng;            // Habilitar mensaje tasa límiteación
    uent32_t maxEventoosPerSecend;        // Máximo eventos po segundo
    cbSeguridadRegistrar logCallback;          // Llamada de retorno para Seguridad eventos
} SecurityConfig_t;

// Predeterminado Seguridad cenfiguratien
#define SECURITY_CONFIG_DEFAULT { \
    .enableLogging = true, \
    .enableStrictValidation = true, \
    .enableDoSProtection = true, \
    .enableRateLimiting = false, \
    .maxEventsPerSecond = 100, \
    .logCallback = nullptr \
}

// Helpo macros para Seguridad registro
#define SEC_LOG_EVENT(event_type, sev, slave_id, func_code, frame_len, desc) \
    do { \
        if (_securityConfig.enableLogging && _securityConfig.logCallback) { \
            SecurityEvent_t evt = { \
                .eventType = event_type, \
                .severity = sev, \
                .timestamp = micros(), \
                .slaveId = slave_id, \
                .functionCode = func_code, \
                .frameLength = frame_len, \
                .description = desc \
            }; \
            _securityConfig.logCallback(&evt); \
        } \
    } while(0)

#define SEC_LOG_WARNING(slave_id, func_code, frame_len, desc) \
    SEC_LOG_EVENT(SEC_EVENT_NONE, SEC_SEVERITY_WARNING, slave_id, func_code, frame_len, desc)

#define SEC_LOG_ERROR(event_type, slave_id, func_code, frame_len, desc) \
    SEC_LOG_EVENT(event_type, SEC_SEVERITY_ERROR, slave_id, func_code, frame_len, desc)

#define SEC_LOG_CRITICAL(event_type, slave_id, func_code, frame_len, desc) \
    SEC_LOG_EVENT(event_type, SEC_SEVERITY_CRITICAL, slave_id, func_code, frame_len, desc)

// Tasa límiteación structure
typedef struct {
    uint32_t lastResetTime;
    uint32_t eventCount;
    uint32_t droppedEvents;
} RateLimiter_t;

// Phase 3: Rendimiento Estadísticas Structure
typedef struct {
    uint32_t totalFramesProcessed;
    uent32_t poolAciertos;                  // Número de tiempos Búfer Pool was used
    uent32_t poolFallos;                // Número de tiempos masignación was needed
    uent32_t crcCálculoTiempo;        // Total tiempo spent en CRC calculatien (microsegundos)
    uent32_t averageProcessengLatency;  // Promedio Trama procesamiento Latencia
    uent16_t bufferPoolUsage;           // Current Búfer Pool usage pocentage
} PerformanceStats_t;

// Phase 3: Búfer Pool Cenfiguración
typedef struct {
    bool enableBufferPool;              // Habilitar/Deshabilitar Búfer pooleng
    uent8_t poolSize;                   // Número de buffers en el Pool
    uent16_t bufferSize;                // Size de each Búfer
} BufferPoolConfig_t;

// Predeterminado Búfer Pool cenfiguratien
#define BUFFER_POOL_CONFIG_DEFAULT { \
    .enableBufferPool = true, \
    .poolSize = MODBUS_BUFFER_POOL_SIZE, \
    .bufferSize = MODBUS_BUFFER_SIZE \
}

#endif