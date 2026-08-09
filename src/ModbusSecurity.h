/*
    Modbus Library for Arduino
    Security Constants and Logging - Phase 2 & 3 Hardening
    
    Based in analysis of Modbus specification documents and audit of security
    Phase 2: Register of Events of Security and Additional Hardening
    Phase 3: Performance Optimization Support
*/
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Constants of validation of frames
#define MODBUS_MIN_FRAME_LEN 3          // Frame válido minimum: func + crc(2) or slaveId + func + crc(2)
#define MODBUS_MAX_PDU_LEN 253          // Tamaño máximo PDU según especificación Modbus (256 - sobrecarga)
#define MODBUS_SAFE_MALLOC_SIZE 512     // Límite of security for allocation denámica to prevenir DoS
#define MODBUS_MAX_BUFFER_LEN 256       // Length maximum of Buffer permitted for any Modbus Frame

// Phase 3: Buffer Pool Configuration
// Tarea 2.2: Soporte para asignación estática en dispositivos limitados (AVR)
#if defined(MODBUS_RESOURCE_LIMITED) || defined(__AVR__) || defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_LEONARDO)
    // En dispositivos con recursos limitados, usar buffers estáticos pre-asignados
    // Esto evita llamadas a malloc/free durante operación normal
    #define MODBUS_STATIC_BUFFER 1
    #define MODBUS_BUFFER_POOL_SIZE 4       // Reducido a 4 buffers para ahorrar RAM (4 x 128 = 512 bytes)
    #define MODBUS_BUFFER_SIZE 128          // Buffer más pequeño para ahorrar memoria
#else
    #define MODBUS_STATIC_BUFFER 0          // Usar asignación dinámica en plataformas con más recursos
    #define MODBUS_BUFFER_POOL_SIZE 8       // Number of buffers in the Pool
    #define MODBUS_BUFFER_SIZE 256          // Size of each Buffer in bytes
#endif

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

// Security Event Tipos for Register
typedef enum {
    SEC_EVENT_NONE = 0,
    SEC_EVENT_FRAME_TOO_SMALL,          // Frame below minimum length
    SEC_EVENT_FRAME_TOO_LARGE,          // Frame exceeds safe limits
    SEC_EVENT_PDU_LENGTH_VIOLATION,     // PDU length exceeds specification
    SEC_EVENT_MALLOC_FAILURE,           // Memory allocation failed
    SEC_EVENT_CRC_MISMATCH,             // CRC validation failed
    SEC_EVENT_SLAVE_ID_MISMATCH,        // Slave ID doesn't match expected
    SEC_EVENT_TIMEOUT,                  // Timeout of operation
    SEC_EVENT_INVALID_FUNCTION_CODE,    // No supported function code
    SEC_EVENT_REGISTER_OUT_OF_RANGE,    // Register Address out of valid Range
    SEC_EVENT_BUFFER_OVERFLOW_ATTEMPT,  // Attempt to Write beyond Buffer
    SEC_EVENT_DOSS_ATTEMPT,             // Potential DoS attack detected
    SEC_EVENT_BROADCAST_RECEIVED,       // Broadcast message received
    SEC_EVENT_SECURITY_CHECK_PASSED,    // Security validation passed
    SEC_EVENT_INVALID_CONFIG,           // Invalid configuration parameter (Tarea 2.3)
    SEC_EVENT_CONFIG_CHANGED            // Configuration changed (Tarea 2.3)
} SecurityEventType_t;

// Security Event Severity Levels
typedef enum {
    SEC_SEVERITY_INFO = 0,              // Informational Event
    SEC_SEVERITY_WARNING,               // Warning - unusual but not critical
    SEC_SEVERITY_ERROR,                 // Error - Security violation
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

// Security Register Callback Type
#if defined(MODBUS_USE_STL)
#include <functional>
typedef std::function<void(const SecurityEvent_t*)> cbSecurityRegister;
#else
typedef void (*cbSecurityRegister)(const SecurityEvent_t*);
#endif

// Security Configuration Structure
typedef struct {
    bool enableLogging;                 // Enable/Disable Security register
    bool enableStrictValidation;        // Enable strict Modbus compliance
    bool enableDoSProtection;           // Enable DoS attack protection
    bool enableRateLimiting;            // Enable message rate límiteación
    uint32_t maxEventsPerSecond;        // Maximum events per second
    cbSecurityRegister logCallback;          // Callback for Security events
} SecurityConfig_t;

// Default Security configuration
#define SECURITY_CONFIG_DEFAULT { \
    .enableLogging = true, \
    .enableStrictValidation = true, \
    .enableDoSProtection = true, \
    .enableRateLimiting = true, \
    .maxEventsPerSecond = 100, \
    .logCallback = nullptr \
}

// Helper macros for Security register
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
    uint32_t poolHits;                  // Number of times Buffer Pool was used
    uint32_t poolMisses;                // Number of times malloc was needed
    uint32_t crcCalcTime;        // Total time spent in CRC calculation (microseconds)
    uint32_t averageProcessengLatency;  // Promedio Frame processing Latencia
    uint16_t bufferPoolUsage;           // Current Buffer Pool usage percentage
} PerformanceStats_t;

// Phase 3: Buffer Pool Configuration
typedef struct {
    bool enableBufferPool;              // Enable/Disable Buffer pooling
    uint8_t poolSize;                   // Number of buffers in the Pool
    uint16_t bufferSize;                // Size of each Buffer
} BufferPoolConfig_t;

// Default Buffer Pool configuration
#define BUFFER_POOL_CONFIG_DEFAULT { \
    .enableBufferPool = true, \
    .poolSize = MODBUS_BUFFER_POOL_SIZE, \
    .bufferSize = MODBUS_BUFFER_SIZE \
}

#endif