/*
    Modbus Library for Arduino
    Security Constants and Logging - Phase 2 Hardening
    
    Based on analysis of Modbus specification documents and security audit
    Phase 2: Security Event Logging and Additional Hardening
*/
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Frame validation constants
#define MODBUS_MIN_FRAME_LEN 3          // Minimum valid frame: func + crc(2) or slaveId + func + crc(2)
#define MODBUS_MAX_PDU_LEN 253          // Max PDU size per Modbus spec (256 - overhead)
#define MODBUS_SAFE_MALLOC_SIZE 512     // Safety limit for dynamic allocation to prevent DoS
#define MODBUS_MAX_BUFFER_LEN 256       // Maximum allowed buffer length for any Modbus frame

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
    SEC_EVENT_TIMEOUT,                  // Operation timeout
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
