/*
    Biblioteca Modbus para Arduino
    Constantes de Seguridad y Logging - Endurecimiento Fase 2 y 3
    
    Basado en análisis de documentos de especificación Modbus y auditoría de seguridad
    Fase 2: Registro de Eventos de Seguridad y Endurecimiento Adicional
    Fase 3: Soporte para Optimización de Rendimiento
*/
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Constantes de validación de tramas
#define MODBUS_MIN_FRAME_LEN 3          // Longitud mínima de trama válida: func + crc(2) o slaveId + func + crc(2)
#define MODBUS_MAX_PDU_LEN 253          // Tamaño máximo PDU según especificación Modbus (256 - sobrecarga)
#define MODBUS_SAFE_MALLOC_SIZE 512     // Límite de seguridad para asignación dinámica para prevenir DoS
#define MODBUS_MAX_BUFFER_LEN 256       // Longitud máxima de Buffer permitida para cualquier trama Modbus

// Fase 3: Configuración de Buffer Pool
// Tarea 2.2: Soporte para asignación estática en dispositivos limitados (AVR)
#if defined(MODBUS_RESOURCE_LIMITED) || defined(__AVR__) || defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_LEONARDO)
    // En dispositivos con recursos limitados, usar buffers estáticos pre-asignados
    // Esto evita llamadas a malloc/free durante operación normal
    #define MODBUS_STATIC_BUFFER 1
    #define MODBUS_BUFFER_POOL_SIZE 4       // Reducido a 4 buffers para ahorrar RAM (4 x 128 = 512 bytes)
    #define MODBUS_BUFFER_SIZE 128          // Buffer más pequeño para ahorrar memoria
#else
    #define MODBUS_STATIC_BUFFER 0          // Usar asignación dinámica en plataformas con más recursos
    #define MODBUS_BUFFER_POOL_SIZE 8       // Número de buffers en el Pool
    #define MODBUS_BUFFER_SIZE 256          // Tamaño de cada Buffer en bytes
#endif

// Fase 3: Optimización de CRC
#define CRC_USE_LOOKUP_TABLE 1          // Usar tabla de búsqueda para cálculo CRC más rápido
#define CRC_DMA_SUPPORT 0               // Habilitar soporte DMA (depende de la plataforma)

// Macros de validación de seguridad
#define MODBUS_VALIDATE_FRAME_LEN(len) \
    (((len) >= MODBUS_MIN_FRAME_LEN) && ((len) <= MODBUS_MAX_BUFFER_LEN))

#define MODBUS_VALIDATE_MALLOC_SIZE(len) \
    (((len) > 0) && ((len) <= MODBUS_SAFE_MALLOC_SIZE))

#define MODBUS_VALIDATE_PDU_LEN(len) \
    (((len) > 0) && ((len) <= MODBUS_MAX_PDU_LEN))

// Tipos de Eventos de Seguridad para Registro
typedef enum {
    SEC_EVENT_NONE = 0,
    SEC_EVENT_FRAME_TOO_SMALL,          // Trama por debajo de longitud mínima
    SEC_EVENT_FRAME_TOO_LARGE,          // Trama excede límites seguros
    SEC_EVENT_PDU_LENGTH_VIOLATION,     // Longitud PDU excede especificación
    SEC_EVENT_MALLOC_FAILURE,           // Falló asignación de memoria
    SEC_EVENT_CRC_MISMATCH,             // Validación CRC fallida
    SEC_EVENT_SLAVE_ID_MISMATCH,        // ID de esclavo no coincide con esperado
    SEC_EVENT_TIMEOUT,                  // Timeout de operación
    SEC_EVENT_INVALID_FUNCTION_CODE,    // Código de función no soportado
    SEC_EVENT_REGISTER_OUT_OF_RANGE,    // Dirección de registro fuera de rango válido
    SEC_EVENT_BUFFER_OVERFLOW_ATTEMPT,  // Intento de escritura más allá del Buffer
    SEC_EVENT_DOSS_ATTEMPT,             // Potencial ataque DoS detectado
    SEC_EVENT_BROADCAST_RECEIVED,       // Mensaje broadcast recibido
    SEC_EVENT_SECURITY_CHECK_PASSED,    // Validación de seguridad exitosa
    SEC_EVENT_INVALID_CONFIG,           // Parámetro de configuración inválido (Tarea 2.3)
    SEC_EVENT_CONFIG_CHANGED            // Configuración cambiada (Tarea 2.3)
} SecurityEventType_t;

// Niveles de Severidad de Eventos de Seguridad
typedef enum {
    SEC_SEVERITY_INFO = 0,              // Evento Informativo
    SEC_SEVERITY_WARNING,               // Advertencia - inusual pero no crítico
    SEC_SEVERITY_ERROR,                 // Error - violación de seguridad
    SEC_SEVERITY_CRITICAL               // Crítico - potencial ataque
} SecuritySeverity_t;

// Estructura de Evento de Seguridad
typedef struct {
    SecurityEventType_t eventType;
    SecuritySeverity_t severity;
    uint32_t timestamp;
    uint8_t slaveId;
    uint8_t functionCode;
    uint16_t frameLength;
    const char* description;
} SecurityEvent_t;

// Tipo de Callback para Registro de Seguridad
#if defined(MODBUS_USE_STL)
#include <functional>
typedef std::function<void(const SecurityEvent_t*)> cbSecurityRegister;
#else
typedef void (*cbSecurityRegister)(const SecurityEvent_t*);
#endif

// Estructura de Configuración de Seguridad
typedef struct {
    bool enableLogging;                 // Habilitar/Deshabilitar registro de Seguridad
    bool enableStrictValidation;        // Habilitar cumplimiento estricto Modbus
    bool enableDoSProtection;           // Habilitar protección contra ataques DoS
    bool enableRateLimiting;            // Habilitar límite de tasa de mensajes
    uint32_t maxEventsPerSecond;        // Máximo eventos por segundo
    cbSecurityRegister logCallback;          // Callback para eventos de Seguridad
} SecurityConfig_t;

// Configuración de Seguridad por defecto
#define SECURITY_CONFIG_DEFAULT { \
    .enableLogging = true, \
    .enableStrictValidation = true, \
    .enableDoSProtection = true, \
    .enableRateLimiting = true, \
    .maxEventsPerSecond = 100, \
    .logCallback = nullptr \
}

// Macros auxiliares para registro de Seguridad
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

// Estructura de Limitación de Tasa
typedef struct {
    uint32_t lastResetTime;
    uint32_t eventCount;
    uint32_t droppedEvents;
} RateLimiter_t;

// Fase 3: Estructura de Estadísticas de Rendimiento
typedef struct {
    uint32_t totalFramesProcessed;
    uint32_t poolHits;                  // Número de veces que se usó Buffer Pool
    uint32_t poolMisses;                // Número de veces que se necesitó malloc
    uint32_t crcCalcTime;        // Tiempo total en cálculo CRC (microsegundos)
    uint32_t averageProcessengLatency;  // Promedio de Latencia de procesamiento de tramas
    uint16_t bufferPoolUsage;           // Porcentaje de uso actual de Buffer Pool
} PerformanceStats_t;

// Fase 3: Configuración de Buffer Pool
typedef struct {
    bool enableBufferPool;              // Habilitar/Deshabilitar pooling de buffers
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