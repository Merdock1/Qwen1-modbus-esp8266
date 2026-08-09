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

// Constantes de validación de frames
#define MODBUS_MIN_FRAME_LEN 3          // Frame válido mínimo: func + crc(2) or slaveId + func + crc(2)
#define MODBUS_MAX_PDU_LEN 253          // Tamaño máximo PDU según spec Modbus (256 - sobrecarga)
#define MODBUS_SAFE_MALLOC_SIZE 512     // Límite de seguridad para asignación dinámica to prevenir DoS
#define MODBUS_MAX_BUFFER_LEN 256       // Longitud máxima de buffer permitida for any Modbus frame

// Phase 3: Buffer Pool Configuration
#define MODBUS_BUFFER_POOL_SIZE 8       // Number of buffers in the pool
#define MODBUS_BUFFER_SIZE 256          // Size of each buffer in bytes

// Phase 3: CRC Optimization
#define CRC_USE_LOOKUP_TABLE 1          // Use lookup table for faster CRC calculation
#define CRC_DMA_SUPPORT 0               // Enable DMA support (platform dependent)

// Security validation macros
#define MODBUS_VALIDATE_FRAME_LEN(len) \
    (((len) >= MODBUS_MIN_FRAME_LEN) && ((len) <= MODBUS_MAX_BUFFER_LEN))

#define MODBUS_VALIDATE_MALLOC_SIZE(len) \
    (((len) > 0) && ((len) <= MODBUS_SAFE_MALLOC_SIZE))

#define MODBUS_VALIDATE_PDU_LEN(len) \
    (((len) > 0) && ((len) <= MODBUS_MAX_PDU_LEN))

// Security Event Types for Logging
typedef enum {
    SEC_EVENT_NONE = 0,
    SEC_EVENT_FRAME_TOO_SMALL,          // Frame below minimum length
    SEC_EVENT_FRAME_TOO_LARGE,          // Frame exceeds safe limits
    SEC_EVENT_PDU_LENGTH_VIOLATION,     // PDU length exceeds specification
    SEC_EVENT_MALLOC_FAILURE,           // Memory allocation failed
    SEC_EVENT_CRC_MISMATCH,             // CRC validation failed
    SEC_EVENT_SLAVE_ID_MISMATCH,        // Slave ID doesn't match expected
    SEC_EVENT_TIMEOUT,                  // Timeout de operación
    SEC_EVENT_INVALID_FUNCTION_CODE,    // Unsupported function code
    SEC_EVENT_REGISTER_OUT_OF_RANGE,    // Register address out of valid range
    SEC_EVENT_BUFFER_OVERFLOW_ATTEMPT,  // Attempt to write beyond buffer
    SEC_EVENT_DOSS_ATTEMPT,             // Potential DoS attack detected
    SEC_EVENT_BROADCAST_RECEIVED,       // Broadcast message received
    SEC_EVENT_SECURITY_CHECK_PASSED     // Security validation passed
} SecurityEventType_t;

// Security Event Severity Levels
typedef enum {
    SEC_SEVERITY_INFO = 0,              // Informational event
    SEC_SEVERITY_WARNING,               // Warning - unusual but not critical
    SEC_SEVERITY_ERROR,                 // Error - security violation
    SEC_SEVERITY_CRITICAL               // Critical - potential attack
} SecuritySeverity_t;

// Security Event Structure
typedef struct {
    SecurityEventType_t eventType;
    SecuritySeverity_t severity;
    uint32_t timestamp;
    uint8_t slaveId;
    uint8_t functionCode;
    uint16_t frameLength;
    const char* description;
} SecurityEvent_t;

// Security Logging Callback Type
#if defined(MODBUS_USE_STL)
#include <functional>
typedef std::function<void(const SecurityEvent_t*)> cbSecurityLog;
#else
typedef void (*cbSecurityLog)(const SecurityEvent_t*);
#endif

// Security Configuration Structure
typedef struct {
    bool enableLogging;                 // Enable/disable security logging
    bool enableStrictValidation;        // Enable strict Modbus compliance
    bool enableDoSProtection;           // Enable DoS attack protection
    bool enableRateLimiting;            // Enable message rate limiting
    uint32_t maxEventsPerSecond;        // Maximum events per second
    cbSecurityLog logCallback;          // Callback for security events
} SecurityConfig_t;

// Default security configuration
#define SECURITY_CONFIG_DEFAULT { \
    .enableLogging = true, \
    .enableStrictValidation = true, \
    .enableDoSProtection = true, \
    .enableRateLimiting = false, \
    .maxEventsPerSecond = 100, \
    .logCallback = nullptr \
}

// Helper macros for security logging
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

// Rate limiting structure
typedef struct {
    uint32_t lastResetTime;
    uint32_t eventCount;
    uint32_t droppedEvents;
} RateLimiter_t;

// Phase 3: Performance Statistics Structure
typedef struct {
    uint32_t totalFramesProcessed;
    uint32_t poolHits;                  // Number of times buffer pool was used
    uint32_t poolMisses;                // Number of times malloc was needed
    uint32_t crcCalculationTime;        // Total time spent in CRC calculation (microsegundos)
    uint32_t averageProcessingLatency;  // Average frame processing latency
    uint16_t bufferPoolUsage;           // Current buffer pool usage percentage
} PerformanceStats_t;

// Phase 3: Buffer Pool Configuration
typedef struct {
    bool enableBufferPool;              // Enable/disable buffer pooling
    uint8_t poolSize;                   // Número de buffers en el pool
    uint16_t bufferSize;                // Size of each buffer
} BufferPoolConfig_t;

// Default buffer pool configuration
#define BUFFER_POOL_CONFIG_DEFAULT { \
    .enableBufferPool = true, \
    .poolSize = MODBUS_BUFFER_POOL_SIZE, \
    .bufferSize = MODBUS_BUFFER_SIZE \
}

#endif