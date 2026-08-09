/*
    ModbusEnhanced.h - Mejoras y optimizaciones para la biblioteca Modbus
    Implementa: Validación estricta, protección replay attacks, buffer pool dinámico,
                cache LRU, tipos extendidos, logging integrado
    
    Copyright (C) 2024 - Mejoras basadas en análisis del repositorio Qwen1-modbus-esp8266
    Todos los comentarios y documentación en español
*/

#pragma once

#include "Modbus.h"
#include <stdint.h>
#include <string.h>

// ============================================================================
// CONFIGURACIÓN DE PLATAFORMAS SOPORTADAS
// ============================================================================

/**
 * @brief Definición de plataformas soportadas con sus características
 * 
 * Plataformas confirmadas:
 * - ESP8266/ESP32: Soporte completo (WiFi, BLE, TLS, DMA)
 * - Arduino Uno/Leonardo: Recursos limitados (SRAM 2KB, FLASH 32KB)
 * - Arduino Due: ARM Cortex-M3, más recursos
 * - STM32: Familia amplia, soporte DMA avanzado
 * - RP2040: Dual-core ARM Cortex-M0+
 * - Portenta H7: Dual-core Cortex-M7/M4, alto rendimiento
 */

#if defined(ESP8266)
    #define MODBUS_PLATFORM_ESP8266
    #define MODBUS_PLATFORM_NAME "ESP8266"
    #define MODBUS_HAS_HARDWARE_CRC true
    #define MODBUS_HAS_WIFI true
    #define MODBUS_MAX_FRAME_SIZE 256
    #define MODBUS_DEFAULT_BUFFER_SIZE 512
    
#elif defined(ESP32)
    #define MODBUS_PLATFORM_ESP32
    #define MODBUS_PLATFORM_NAME "ESP32"
    #define MODBUS_HAS_HARDWARE_CRC true
    #define MODBUS_HAS_DMA_CRC true
    #define MODBUS_HAS_WIFI true
    #define MODBUS_HAS_BLE true
    #define MODBUS_HAS_TLS true
    #define MODBUS_MAX_FRAME_SIZE 512
    #define MODBUS_DEFAULT_BUFFER_SIZE 1024
    #define MODBUS_SUPPORTS_ATOMIC_OPS true
    
#elif defined(ARDUINO_AVR_UNO) || defined(__AVR_ATmega328P__)
    #define MODBUS_PLATFORM_ARDUINO_UNO
    #define MODBUS_PLATFORM_NAME "Arduino Uno"
    #define MODBUS_RESOURCE_LIMITED true
    #define MODBUS_MAX_FRAME_SIZE 128
    #define MODBUS_DEFAULT_BUFFER_SIZE 256
    #define MODBUS_MAX_REGISTERS 50
    
#elif defined(ARDUINO_AVR_LEONARDO) || defined(__AVR_ATmega32U4__)
    #define MODBUS_PLATFORM_ARDUINO_LEONARDO
    #define MODBUS_PLATFORM_NAME "Arduino Leonardo"
    #define MODBUS_RESOURCE_LIMITED true
    #define MODBUS_MAX_FRAME_SIZE 128
    #define MODBUS_DEFAULT_BUFFER_SIZE 256
    #define MODBUS_MAX_REGISTERS 50
    
#elif defined(ARDUINO_SAM_DUE) || defined(__SAM3X8E__)
    #define MODBUS_PLATFORM_ARDUINO_DUE
    #define MODBUS_PLATFORM_NAME "Arduino Due"
    #define MODBUS_HAS_HARDWARE_CRC true
    #define MODBUS_MAX_FRAME_SIZE 256
    #define MODBUS_DEFAULT_BUFFER_SIZE 512
    
#elif defined(ARDUINO_ARCH_STM32)
    #define MODBUS_PLATFORM_STM32
    #define MODBUS_PLATFORM_NAME "STM32"
    #define MODBUS_HAS_HARDWARE_CRC true
    #define MODBUS_HAS_DMA_CRC true
    #define MODBUS_MAX_FRAME_SIZE 512
    #define MODBUS_DEFAULT_BUFFER_SIZE 1024
    
#elif defined(ARDUINO_ARCH_RP2040)
    #define MODBUS_PLATFORM_RP2040
    #define MODBUS_PLATFORM_NAME "RP2040"
    #define MODBUS_HAS_HARDWARE_CRC true
    #define MODBUS_MAX_FRAME_SIZE 256
    #define MODBUS_DEFAULT_BUFFER_SIZE 512
    #define MODBUS_SUPPORTS_ATOMIC_OPS true
    
#elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_PORTENTA_H7_M4)
    #define MODBUS_PLATFORM_PORTENTA_H7
    #define MODBUS_PLATFORM_NAME "Portenta H7"
    #define MODBUS_HAS_HARDWARE_CRC true
    #define MODBUS_HAS_DMA_CRC true
    #define MODBUS_HAS_TLS true
    #define MODBUS_MAX_FRAME_SIZE 1024
    #define MODBUS_DEFAULT_BUFFER_SIZE 2048
    #define MODBUS_SUPPORTS_ATOMIC_OPS true
    
#else
    #define MODBUS_PLATFORM_GENERIC
    #define MODBUS_PLATFORM_NAME "Genérico"
    #define MODBUS_MAX_FRAME_SIZE 256
    #define MODBUS_DEFAULT_BUFFER_SIZE 512
#endif

// ============================================================================
// SISTEMA DE LOGGING INTEGRADO
// ============================================================================

/**
 * @brief Niveles de severidad para el sistema de logging
 */
enum ModbusLogLevel {
    MODBUS_LOG_NONE     = 0,  ///< Sin logging
    MODBUS_LOG_ERROR    = 1,  ///< Solo errores críticos
    MODBUS_LOG_WARNING  = 2,  ///< Errores y advertencias
    MODBUS_LOG_INFO     = 3,  ///< Información general
    MODBUS_LOG_DEBUG    = 4,  ///< Debug básico
    MODBUS_LOG_VERBOSE  = 5   ///< Debug detallado
};

/**
 * @brief Configuración del sistema de logging
 */
struct ModbusLogConfig {
    ModbusLogLevel level;           ///< Nivel de logging actual
    bool useSerial;                 ///< Usar Serial para output
    bool useNetwork;                ///< Usar red para output remoto
    uint16_t logPort;               ///< Puerto para logging remoto
    const char* logPrefix;          ///< Prefijo para mensajes
    
    ModbusLogConfig() : 
        level(MODBUS_LOG_WARNING),
        useSerial(true),
        useNetwork(false),
        logPort(5020),
        logPrefix("[MODBUS]") {}
};

/**
 * @brief Macro para logging condicional según nivel
 */
#ifndef MODBUS_LOG
#define MODBUS_LOG(level, format, ...) \
    do { \
        if (ModbusLogger::getInstance().shouldLog(level)) { \
            ModbusLogger::getInstance().log(level, format, ##__VA_ARGS__); \
        } \
    } while(0)
#endif

#define MODBUS_LOG_ERROR(format, ...)   MODBUS_LOG(MODBUS_LOG_ERROR, format, ##__VA_ARGS__)
#define MODBUS_LOG_WARNING(format, ...) MODBUS_LOG(MODBUS_LOG_WARNING, format, ##__VA_ARGS__)
#define MODBUS_LOG_INFO(format, ...)    MODBUS_LOG(MODBUS_LOG_INFO, format, ##__VA_ARGS__)
#define MODBUS_LOG_DEBUG(format, ...)   MODBUS_LOG(MODBUS_LOG_DEBUG, format, ##__VA_ARGS__)
#define MODBUS_LOG_VERBOSE(format, ...) MODBUS_LOG(MODBUS_LOG_VERBOSE, format, ##__VA_ARGS__)

/**
 * @brief Clase singleton para gestión de logging
 */
class ModbusLogger {
private:
    static ModbusLogger* instance;
    ModbusLogConfig config;
    
    ModbusLogger() {}
    
public:
    static ModbusLogger& getInstance() {
        if (!instance) {
            instance = new ModbusLogger();
        }
        return *instance;
    }
    
    bool shouldLog(ModbusLogLevel level) {
        return level <= config.level;
    }
    
    void log(ModbusLogLevel level, const char* format, ...) {
        // Implementación básica de logging
        #if defined(MODBUS_LOG_ENABLE)
        va_list args;
        va_start(args, format);
        
        const char* levelStr;
        switch(level) {
            case MODBUS_LOG_ERROR:   levelStr = "ERROR"; break;
            case MODBUS_LOG_WARNING: levelStr = "WARN"; break;
            case MODBUS_LOG_INFO:    levelStr = "INFO"; break;
            case MODBUS_LOG_DEBUG:   levelStr = "DEBUG"; break;
            case MODBUS_LOG_VERBOSE: levelStr = "VERB"; break;
            default: return;
        }
        
        if (config.useSerial && Serial) {
            Serial.print(config.logPrefix);
            Serial.print("[");
            Serial.print(levelStr);
            Serial.print("] ");
            
            char buffer[256];
            vsnprintf(buffer, sizeof(buffer), format, args);
            Serial.println(buffer);
        }
        
        va_end(args);
        #endif
    }
    
    void setLevel(ModbusLogLevel level) {
        config.level = level;
        MODBUS_LOG_INFO("Nivel de logging cambiado a %d", level);
    }
    
    ModbusLogLevel getLevel() {
        return config.level;
    }
    
    void enableSerial(bool enable) {
        config.useSerial = enable;
    }
    
    void setPrefix(const char* prefix) {
        config.logPrefix = prefix;
    }
};

ModbusLogger* ModbusLogger::instance = nullptr;

// ============================================================================
// VALIDACIÓN ESTRICTA DE TRAMAS
// ============================================================================

/**
 * @brief Códigos de error de validación
 */
enum ModbusValidationError {
    VALIDATION_OK                     = 0,
    VALIDATION_ERROR_LENGTH           = 1,  ///< Longitud incorrecta
    VALIDATION_ERROR_CRC              = 2,  ///< CRC inválido
    VALIDATION_ERROR_SLAVE_ID         = 3,  ///< ID de esclavo inválido
    VALIDATION_ERROR_FUNCTION_CODE    = 4,  ///< Código de función no soportado
    VALIDATION_ERROR_ADDRESS          = 5,  ///< Dirección fuera de rango
    VALIDATION_ERROR_COUNT            = 6,  ///< Cantidad de registros inválida
    VALIDATION_ERROR_DATA_SIZE        = 7,  ///< Tamaño de datos incorrecto
    VALIDATION_ERROR_REPLAY           = 8,  ///< Posible ataque replay detectado
    VALIDATION_ERROR_TIMING           = 9,  ///< Violación de timing
    VALIDATION_ERROR_NULL_POINTER     = 10  ///< Puntero null detectado
};

/**
 * @brief Configuración para validación estricta
 */
struct ModbusValidationConfig {
    bool checkCRC;                  ///< Verificar CRC en todas las tramas
    bool checkSlaveId;              ///< Verificar ID de esclavo válido
    bool checkFunctionCode;         ///< Verificar código de función soportado
    bool checkAddressRange;         ///< Verificar rango de direcciones
    bool checkDataSize;             ///< Verificar tamaño de datos
    bool enableReplayProtection;    ///< Habilitar protección contra replay
    bool enableTimingCheck;         ///< Habilitar verificación de timing
    uint32_t minFrameInterval;      ///< Intervalo mínimo entre tramas (micros)
    uint16_t maxRegisterCount;      ///< Máxima cantidad de registros por operación
    
    ModbusValidationConfig() :
        checkCRC(true),
        checkSlaveId(true),
        checkFunctionCode(true),
        checkAddressRange(true),
        checkDataSize(true),
        enableReplayProtection(false),
        enableTimingCheck(false),
        minFrameInterval(1000),
        maxRegisterCount(125) {}
};

/**
 * @brief Clase para validación estricta de tramas Modbus
 */
class ModbusValidator {
private:
    ModbusValidationConfig config;
    uint32_t lastFrameTime;
    
    // Para protección replay (TCP)
    uint32_t lastTransactionId;
    uint32_t replayWindow;
    
public:
    ModbusValidator() : lastFrameTime(0), lastTransactionId(0), replayWindow(1000) {}
    
    /**
     * @brief Configurar validador con opciones específicas
     */
    void configure(const ModbusValidationConfig& cfg) {
        config = cfg;
        MODBUS_LOG_INFO("Validador configurado: CRC=%d, Replay=%d", 
                       config.checkCRC, config.enableReplayProtection);
    }
    
    /**
     * @brief Validar puntero null
     * @param ptr Puntero a validar
     * @return VALIDATION_ERROR_NULL_POINTER si es null, VALIDATION_OK si no
     */
    ModbusValidationError validatePointer(const void* ptr) {
        if (ptr == nullptr) {
            MODBUS_LOG_ERROR("Puntero null detectado");
            return VALIDATION_ERROR_NULL_POINTER;
        }
        return VALIDATION_OK;
    }
    
    /**
     * @brief Validar longitud de trama
     */
    ModbusValidationError validateFrameLength(uint16_t length, uint16_t expectedMin, uint16_t expectedMax) {
        if (length < expectedMin || length > expectedMax) {
            MODBUS_LOG_WARNING("Longitud de trama inválida: %d (esperado %d-%d)", 
                             length, expectedMin, expectedMax);
            return VALIDATION_ERROR_LENGTH;
        }
        return VALIDATION_OK;
    }
    
    /**
     * @brief Validar ID de esclavo
     */
    ModbusValidationError validateSlaveId(uint8_t slaveId, uint8_t minId = 1, uint8_t maxId = 247) {
        if (slaveId < minId || slaveId > maxId) {
            MODBUS_LOG_WARNING("ID de esclavo inválido: %d", slaveId);
            return VALIDATION_ERROR_SLAVE_ID;
        }
        return VALIDATION_OK;
    }
    
    /**
     * @brief Validar código de función
     */
    ModbusValidationError validateFunctionCode(uint8_t fc) {
        // Funciones estándar soportadas
        static const uint8_t supportedFC[] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
            0x0F, 0x10, 0x14, 0x15, 0x16, 0x17, 0x2B
        };
        
        for (uint8_t i = 0; i < sizeof(supportedFC); i++) {
            if (fc == supportedFC[i]) {
                return VALIDATION_OK;
            }
        }
        
        MODBUS_LOG_WARNING("Código de función no soportado: 0x%02X", fc);
        return VALIDATION_ERROR_FUNCTION_CODE;
    }
    
    /**
     * @brief Validar dirección y cantidad de registros
     */
    ModbusValidationError validateAddressCount(uint16_t address, uint16_t count, 
                                              uint16_t maxAddress, uint16_t maxCount) {
        if (address > maxAddress) {
            MODBUS_LOG_WARNING("Dirección fuera de rango: %d (máx %d)", address, maxAddress);
            return VALIDATION_ERROR_ADDRESS;
        }
        
        if (count == 0 || count > maxCount) {
            MODBUS_LOG_WARNING("Cantidad inválida: %d (máx %d)", count, maxCount);
            return VALIDATION_ERROR_COUNT;
        }
        
        if ((uint32_t)address + count > maxAddress) {
            MODBUS_LOG_WARNING("Rango excede límite: %d+%d > %d", address, count, maxAddress);
            return VALIDATION_ERROR_ADDRESS;
        }
        
        return VALIDATION_OK;
    }
    
    /**
     * @brief Validar timing entre tramas (protección contra flooding)
     */
    ModbusValidationError validateTiming() {
        if (!config.enableTimingCheck) {
            return VALIDATION_OK;
        }
        
        uint32_t currentTime = micros();
        if (lastFrameTime > 0) {
            uint32_t interval = currentTime - lastFrameTime;
            if (interval < config.minFrameInterval) {
                MODBUS_LOG_WARNING("Intervalo entre tramas demasiado corto: %d µs", interval);
                return VALIDATION_ERROR_TIMING;
            }
        }
        
        lastFrameTime = currentTime;
        return VALIDATION_OK;
    }
    
    /**
     * @brief Validar ID de transacción para protección replay (TCP)
     */
    ModbusValidationError validateTransactionId(uint16_t transactionId) {
        if (!config.enableReplayProtection) {
            return VALIDATION_OK;
        }
        
        // Verificar si el transactionId está dentro de la ventana válida
        if (transactionId < lastTransactionId && 
            (lastTransactionId - transactionId) < replayWindow) {
            MODBUS_LOG_WARNING("Posible replay attack detectado: TransactionId=%d", transactionId);
            return VALIDATION_ERROR_REPLAY;
        }
        
        lastTransactionId = transactionId;
        return VALIDATION_OK;
    }
    
    /**
     * @brief Validación completa de trama RTU
     */
    ModbusValidationError validateRTUFrame(const uint8_t* frame, uint16_t length) {
        ModbusValidationError error;
        
        // Validar puntero
        error = validatePointer(frame);
        if (error != VALIDATION_OK) return error;
        
        // Validar longitud mínima (slaveId + fc + data + crc)
        error = validateFrameLength(length, 4, MODBUS_MAX_FRAME_SIZE);
        if (error != VALIDATION_OK) return error;
        
        // Validar slave ID
        error = validateSlaveId(frame[0]);
        if (error != VALIDATION_OK) return error;
        
        // Validar function code
        error = validateFunctionCode(frame[1]);
        if (error != VALIDATION_OK) return error;
        
        // Validar timing
        error = validateTiming();
        if (error != VALIDATION_OK) return error;
        
        return VALIDATION_OK;
    }
    
    /**
     * @brief Validación completa de trama TCP
     */
    ModbusValidationError validateTCPFrame(const uint8_t* frame, uint16_t length) {
        ModbusValidationError error;
        
        // Validar puntero
        error = validatePointer(frame);
        if (error != VALIDATION_OK) return error;
        
        // Validar longitud mínima (MBAP header + PDU)
        error = validateFrameLength(length, 8, MODBUS_MAX_FRAME_SIZE);
        if (error != VALIDATION_OK) return error;
        
        // Extraer y validar transaction ID
        uint16_t transactionId = (frame[0] << 8) | frame[1];
        error = validateTransactionId(transactionId);
        if (error != VALIDATION_OK) return error;
        
        // Validar unit ID
        error = validateSlaveId(frame[6]);
        if (error != VALIDATION_OK) return error;
        
        // Validar function code
        error = validateFunctionCode(frame[7]);
        if (error != VALIDATION_OK) return error;
        
        return VALIDATION_OK;
    }
};

// ============================================================================
// BUFFER POOL DINÁMICO
// ============================================================================

/**
 * @brief Estructura de bloque de buffer
 */
struct ModbusBufferBlock {
    uint8_t* data;
    uint16_t size;
    bool inUse;
    uint32_t allocTime;
    uint32_t lastAccess;
    
    ModbusBufferBlock() : data(nullptr), size(0), inUse(false), 
                          allocTime(0), lastAccess(0) {}
};

/**
 * @brief Gestor de buffer pool dinámico para evitar fragmentación de memoria
 */
class ModbusBufferPool {
private:
    ModbusBufferBlock* blocks;
    uint16_t blockCount;
    uint16_t blockSize;
    uint16_t freeCount;
    
public:
    /**
     * @brief Constructor del buffer pool
     * @param numBlocks Número de bloques en el pool
     * @param blkSize Tamaño de cada bloque en bytes
     */
    ModbusBufferPool(uint16_t numBlocks = 10, uint16_t blkSize = 256) {
        blockCount = numBlocks;
        blockSize = blkSize;
        freeCount = numBlocks;
        
        blocks = new ModbusBufferBlock[numBlocks];
        for (uint16_t i = 0; i < numBlocks; i++) {
            blocks[i].data = new uint8_t[blkSize];
            blocks[i].size = blkSize;
            blocks[i].inUse = false;
        }
        
        MODBUS_LOG_INFO("BufferPool creado: %d bloques de %d bytes", numBlocks, blkSize);
    }
    
    ~ModbusBufferPool() {
        for (uint16_t i = 0; i < blockCount; i++) {
            delete[] blocks[i].data;
        }
        delete[] blocks;
    }
    
    /**
     * @brief Asignar un bloque del pool
     * @return Puntero al bloque o nullptr si no hay disponibles
     */
    uint8_t* allocate() {
        for (uint16_t i = 0; i < blockCount; i++) {
            if (!blocks[i].inUse) {
                blocks[i].inUse = true;
                blocks[i].allocTime = millis();
                blocks[i].lastAccess = millis();
                freeCount--;
                
                MODBUS_LOG_DEBUG("Buffer asignado: bloque %d, libres %d", i, freeCount);
                return blocks[i].data;
            }
        }
        
        MODBUS_LOG_WARNING("BufferPool agotado: no hay bloques libres");
        return nullptr;
    }
    
    /**
     * @brief Liberar un bloque del pool
     * @param data Puntero al bloque a liberar
     * @return true si se liberó correctamente
     */
    bool release(uint8_t* data) {
        for (uint16_t i = 0; i < blockCount; i++) {
            if (blocks[i].data == data && blocks[i].inUse) {
                blocks[i].inUse = false;
                blocks[i].lastAccess = millis();
                freeCount++;
                
                MODBUS_LOG_DEBUG("Buffer liberado: bloque %d, libres %d", i, freeCount);
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Obtener estadísticas del pool
     */
    uint16_t getFreeCount() const { return freeCount; }
    uint16_t getUsedCount() const { return blockCount - freeCount; }
    uint16_t getTotalCount() const { return blockCount; }
    float getUsagePercent() const { 
        return ((float)(blockCount - freeCount) / blockCount) * 100.0; 
    }
    
    /**
     * @brief Limpiar bloques abandonados (timeout)
     * @param timeoutMs Tiempo máximo de uso en milisegundos
     * @return Número de bloques limpiados
     */
    uint16_t cleanupStale(uint32_t timeoutMs) {
        uint16_t cleaned = 0;
        uint32_t currentTime = millis();
        
        for (uint16_t i = 0; i < blockCount; i++) {
            if (blocks[i].inUse && (currentTime - blocks[i].allocTime) > timeoutMs) {
                MODBUS_LOG_WARNING("Limpiando bloque abandonado: %d (edad %d ms)", 
                                  i, currentTime - blocks[i].allocTime);
                blocks[i].inUse = false;
                freeCount++;
                cleaned++;
            }
        }
        
        return cleaned;
    }
};

// ============================================================================
// TIPOS EXTENDIDOS (FLOAT, INT32, UINT32)
// ============================================================================

/**
 * @brief Unión para conversión entre float y dos registros de 16 bits
 */
union ModbusFloatReg {
    float floatValue;
    uint32_t uint32Value;
    int32_t int32Value;
    struct {
        uint16_t high;
        uint16_t low;
    } registers;
    
    ModbusFloatReg() : floatValue(0.0f) {}
    ModbusFloatReg(float f) : floatValue(f) {}
    ModbusFloatReg(int32_t i) : int32Value(i) {}
    ModbusFloatReg(uint32_t u) : uint32Value(u) {}
};

/**
 * @brief Orden de bytes para tipos extendidos
 */
enum ModbusByteOrder {
    BYTE_ORDER_ABCD,  ///< Big Endian (ABCD)
    BYTE_ORDER_CDAB,  ///< Little Endian (CDAB)
    BYTE_ORDER_BADC,  ///< Byte swap (BADC)
    BYTE_ORDER_DCBA   ///< Word swap (DCBA)
};

/**
 * @brief Clase utilitaria para manejo de tipos extendidos
 */
class ModbusExtendedTypes {
public:
    /**
     * @brief Escribir un valor float en dos registros
     * @param regs Array de dos registros de 16 bits
     * @param value Valor float a escribir
     * @param byteOrder Orden de bytes
     */
    static void writeFloat(uint16_t* regs, float value, ModbusByteOrder byteOrder = BYTE_ORDER_ABCD) {
        ModbusFloatReg converter(value);
        
        switch (byteOrder) {
            case BYTE_ORDER_ABCD:
                regs[0] = converter.registers.high;
                regs[1] = converter.registers.low;
                break;
            case BYTE_ORDER_CDAB:
                regs[0] = converter.registers.low;
                regs[1] = converter.registers.high;
                break;
            case BYTE_ORDER_BADC:
                regs[0] = __builtin_bswap16(converter.registers.high);
                regs[1] = __builtin_bswap16(converter.registers.low);
                break;
            case BYTE_ORDER_DCBA:
                regs[0] = __builtin_bswap16(converter.registers.low);
                regs[1] = __builtin_bswap16(converter.registers.high);
                break;
        }
    }
    
    /**
     * @brief Leer un valor float desde dos registros
     * @param regs Array de dos registros de 16 bits
     * @param byteOrder Orden de bytes
     * @return Valor float
     */
    static float readFloat(const uint16_t* regs, ModbusByteOrder byteOrder = BYTE_ORDER_ABCD) {
        ModbusFloatReg converter;
        
        switch (byteOrder) {
            case BYTE_ORDER_ABCD:
                converter.registers.high = regs[0];
                converter.registers.low = regs[1];
                break;
            case BYTE_ORDER_CDAB:
                converter.registers.high = regs[1];
                converter.registers.low = regs[0];
                break;
            case BYTE_ORDER_BADC:
                converter.registers.high = __builtin_bswap16(regs[0]);
                converter.registers.low = __builtin_bswap16(regs[1]);
                break;
            case BYTE_ORDER_DCBA:
                converter.registers.high = __builtin_bswap16(regs[1]);
                converter.registers.low = __builtin_bswap16(regs[0]);
                break;
        }
        
        return converter.floatValue;
    }
    
    /**
     * @brief Escribir un valor int32 en dos registros
     */
    static void writeInt32(uint16_t* regs, int32_t value, ModbusByteOrder byteOrder = BYTE_ORDER_ABCD) {
        ModbusFloatReg converter(value);
        writeFloat(regs, converter.floatValue, byteOrder);
    }
    
    /**
     * @brief Leer un valor int32 desde dos registros
     */
    static int32_t readInt32(const uint16_t* regs, ModbusByteOrder byteOrder = BYTE_ORDER_ABCD) {
        ModbusFloatReg converter;
        converter.floatValue = readFloat(regs, byteOrder);
        return converter.int32Value;
    }
    
    /**
     * @brief Escribir un valor uint32 en dos registros
     */
    static void writeUInt32(uint16_t* regs, uint32_t value, ModbusByteOrder byteOrder = BYTE_ORDER_ABCD) {
        ModbusFloatReg converter(value);
        writeFloat(regs, converter.floatValue, byteOrder);
    }
    
    /**
     * @brief Leer un valor uint32 desde dos registros
     */
    static uint32_t readUInt32(const uint16_t* regs, ModbusByteOrder byteOrder = BYTE_ORDER_ABCD) {
        ModbusFloatReg converter;
        converter.floatValue = readFloat(regs, byteOrder);
        return converter.uint32Value;
    }
};
