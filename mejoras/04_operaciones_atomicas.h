/*
 * Mejora #4: Operaciones Atómicas Multi-Registro
 * Prioridad: Baja (Nice to Have)
 * 
 * Implementa operaciones atómicas para lectura/escritura de múltiples registros
 * que garantizan consistencia en entornos multi-hilo o con interrupciones.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tipos de operación atómica
typedef enum {
    ATOMIC_READ_HOLDING = 0,      // Lectura atómica de registros de retención
    ATOMIC_WRITE_HOLDING = 1,     // Escritura atómica de registros de retención
    ATOMIC_READ_INPUT = 2,        // Lectura atómica de registros de entrada
    ATOMIC_READ_COILS = 3,        // Lectura atómica de bobinas
    ATOMIC_WRITE_COILS = 4,       // Escritura atómica de bobinas
    ATOMIC_READ_WRITE = 5         // Lectura/Escritura atómica combinada
} AtomicOperationType_t;

// Resultado de operación atómica
typedef enum {
    ATOMIC_SUCCESS = 0,           // Operación completada exitosamente
    ATOMIC_ERROR_LOCK = 1,        // No se pudo adquirir el lock
    ATOMIC_ERROR_TIMEOUT = 2,     // Timeout de la operación
    ATOMIC_ERROR_RANGE = 3,       // Rango de registros inválido
    ATOMIC_ERROR_NULL_PTR = 4,    // Puntero null
    ATOMIC_ERROR_CONFLICT = 5     // Conflicto con otra operación
} AtomicResult_t;

// Configuración de operación atómica
typedef struct {
    AtomicOperationType_t type;   // Tipo de operación
    uint16_t startAddress;        // Dirección inicial
    uint16_t count;               // Número de registros
    uint16_t* values;             // Valores a leer/escribir
    uint32_t timeout_ms;          // Timeout en milisegundos
    bool useInterruptLock;        // Usar bloqueo por interrupción
} AtomicOperationConfig_t;

// Estadísticas de operaciones atómicas
typedef struct {
    uint32_t totalOperations;     // Total de operaciones
    uint32_t successfulOps;       // Operaciones exitosas
    uint32_t failedLocks;         // Fallos al adquirir lock
    uint32_t timeouts;            // Timeouts
    uint32_t conflicts;           // Conflictos detectados
    uint32_t averageLatency_us;   // Latencia promedio en microsegundos
} AtomicStats_t;

// Estructura de registro con soporte atómico
typedef struct {
    volatile uint16_t value;      // Valor del registro
    volatile bool locked;         // Estado de bloqueo
    volatile uint32_t lastAccess; // Timestamp del último acceso
    volatile uint8_t accessCount; // Contador de accesos concurrentes
} AtomicRegister_t;

// Gestor de registros atómicos
typedef struct {
    AtomicRegister_t* registers;  // Array de registros
    uint16_t registerCount;       // Número total de registros
    volatile bool globalLock;     // Lock global para operaciones críticas
    AtomicStats_t stats;          // Estadísticas
    bool initialized;             // Estado de inicialización
} AtomicRegisterManager_t;

/**
 * @brief Inicializa el gestor de registros atómicos
 * 
 * @param manager Puntero al gestor
 * @param registers Array de registros
 * @param count Número de registros
 * @return true Si se inicializó correctamente
 * @return false Si falló la inicialización
 */
bool atomic_manager_init(AtomicRegisterManager_t* manager,
                         AtomicRegister_t* registers,
                         uint16_t count) {
    if (!manager || !registers || count == 0) {
        return false;
    }
    
    manager->registers = registers;
    manager->registerCount = count;
    manager->globalLock = false;
    manager->initialized = true;
    
    // Inicializar todos los registros
    for (uint16_t i = 0; i < count; i++) {
        registers[i].value = 0;
        registers[i].locked = false;
        registers[i].lastAccess = 0;
        registers[i].accessCount = 0;
    }
    
    memset(&manager->stats, 0, sizeof(AtomicStats_t));
    
    return true;
}

/**
 * @brief Adquiere un lock atómico (implementación portable)
 * 
 * @param lock Puntero a variable de lock
 * @param timeout_ms Timeout en milisegundos
 * @return true Si se adquirió el lock
 * @return false Si hubo timeout
 */
static inline bool atomic_try_lock(volatile bool* lock, uint32_t timeout_ms) {
    uint32_t start = millis();
    
    while (true) {
        // Critical section - deshabilitar interrupciones temporalmente
        bool expected = false;
        
        // Simulación de compare-and-swap (CAS)
        // En hardware real usar instrucciones atómicas del procesador
        noInterrupts();
        if (*lock == false) {
            *lock = true;
            interrupts();
            return true;
        }
        interrupts();
        
        // Verificar timeout
        if (millis() - start > timeout_ms) {
            return false;
        }
        
        // Pequeña espera para evitar busy-waiting excesivo
        delayMicroseconds(10);
    }
}

/**
 * @brief Libera un lock atómico
 * 
 * @param lock Puntero a variable de lock
 */
static inline void atomic_unlock(volatile bool* lock) {
    noInterrupts();
    *lock = false;
    interrupts();
}

/**
 * @brief Lee múltiples registros atómicamente
 * 
 * @param manager Puntero al gestor
 * @param startAddress Dirección inicial
 * @param count Número de registros a leer
 * @param[out] values Buffer para almacenar valores
 * @param timeout_ms Timeout en milisegundos
 * @return AtomicResult_t Resultado de la operación
 */
AtomicResult_t atomic_read_registers(AtomicRegisterManager_t* manager,
                                      uint16_t startAddress,
                                      uint16_t count,
                                      uint16_t* values,
                                      uint32_t timeout_ms) {
    if (!manager || !values) {
        return ATOMIC_ERROR_NULL_PTR;
    }
    
    if (startAddress + count > manager->registerCount) {
        return ATOMIC_ERROR_RANGE;
    }
    
    manager->stats.totalOperations++;
    
    // Adquirir lock global
    if (!atomic_try_lock(&manager->globalLock, timeout_ms)) {
        manager->stats.failedLocks++;
        return ATOMIC_ERROR_LOCK;
    }
    
    uint32_t start_time = micros();
    
    // Leer todos los registros atómicamente
    for (uint16_t i = 0; i < count; i++) {
        AtomicRegister_t* reg = &manager->registers[startAddress + i];
        
        noInterrupts();
        values[i] = reg->value;
        reg->lastAccess = millis();
        reg->accessCount++;
        interrupts();
    }
    
    uint32_t elapsed = micros() - start_time;
    manager->stats.averageLatency_us = 
        (manager->stats.averageLatency_us + elapsed) / 2;
    
    // Liberar lock
    atomic_unlock(&manager->globalLock);
    
    manager->stats.successfulOps++;
    
    return ATOMIC_SUCCESS;
}

/**
 * @brief Escribe múltiples registros atómicamente
 * 
 * @param manager Puntero al gestor
 * @param startAddress Dirección inicial
 * @param count Número de registros a escribir
 * @param values Valores a escribir
 * @param timeout_ms Timeout en milisegundos
 * @return AtomicResult_t Resultado de la operación
 */
AtomicResult_t atomic_write_registers(AtomicRegisterManager_t* manager,
                                       uint16_t startAddress,
                                       uint16_t count,
                                       const uint16_t* values,
                                       uint32_t timeout_ms) {
    if (!manager || !values) {
        return ATOMIC_ERROR_NULL_PTR;
    }
    
    if (startAddress + count > manager->registerCount) {
        return ATOMIC_ERROR_RANGE;
    }
    
    manager->stats.totalOperations++;
    
    // Adquirir lock global
    if (!atomic_try_lock(&manager->globalLock, timeout_ms)) {
        manager->stats.failedLocks++;
        return ATOMIC_ERROR_LOCK;
    }
    
    uint32_t start_time = micros();
    
    // Escribir todos los registros atómicamente
    for (uint16_t i = 0; i < count; i++) {
        AtomicRegister_t* reg = &manager->registers[startAddress + i];
        
        noInterrupts();
        reg->value = values[i];
        reg->lastAccess = millis();
        reg->accessCount++;
        interrupts();
    }
    
    uint32_t elapsed = micros() - start_time;
    manager->stats.averageLatency_us = 
        (manager->stats.averageLatency_us + elapsed) / 2;
    
    // Liberar lock
    atomic_unlock(&manager->globalLock);
    
    manager->stats.successfulOps++;
    
    return ATOMIC_SUCCESS;
}

/**
 * @brief Operaición atómica de lectura y escritura combinada
 * 
 * Lee un conjunto de registros y escribe otro conjunto atómicamente.
 * Útil para implementar la función Modbus 0x17 (Read/Write Multiple Registers).
 * 
 * @param manager Puntero al gestor
 * @param readAddress Dirección de lectura
 * @param readCount Número de registros a leer
 * @param writeAddress Dirección de escritura
 * @param writeCount Número de registros a escribir
 * @param writeValues Valores a escribir
 * @param readValues Buffer para valores leídos
 * @param timeout_ms Timeout en milisegundos
 * @return AtomicResult_t Resultado de la operación
 */
AtomicResult_t atomic_read_write_registers(AtomicRegisterManager_t* manager,
                                            uint16_t readAddress,
                                            uint16_t readCount,
                                            uint16_t writeAddress,
                                            uint16_t writeCount,
                                            const uint16_t* writeValues,
                                            uint16_t* readValues,
                                            uint32_t timeout_ms) {
    if (!manager || !writeValues || !readValues) {
        return ATOMIC_ERROR_NULL_PTR;
    }
    
    if (readAddress + readCount > manager->registerCount ||
        writeAddress + writeCount > manager->registerCount) {
        return ATOMIC_ERROR_RANGE;
    }
    
    manager->stats.totalOperations++;
    
    // Adquirir lock global
    if (!atomic_try_lock(&manager->globalLock, timeout_ms)) {
        manager->stats.failedLocks++;
        return ATOMIC_ERROR_LOCK;
    }
    
    uint32_t start_time = micros();
    
    // Primero leer
    for (uint16_t i = 0; i < readCount; i++) {
        AtomicRegister_t* reg = &manager->registers[readAddress + i];
        
        noInterrupts();
        readValues[i] = reg->value;
        reg->lastAccess = millis();
        reg->accessCount++;
        interrupts();
    }
    
    // Luego escribir
    for (uint16_t i = 0; i < writeCount; i++) {
        AtomicRegister_t* reg = &manager->registers[writeAddress + i];
        
        noInterrupts();
        reg->value = writeValues[i];
        reg->lastAccess = millis();
        reg->accessCount++;
        interrupts();
    }
    
    uint32_t elapsed = micros() - start_time;
    manager->stats.averageLatency_us = 
        (manager->stats.averageLatency_us + elapsed) / 2;
    
    // Liberar lock
    atomic_unlock(&manager->globalLock);
    
    manager->stats.successfulOps++;
    
    return ATOMIC_SUCCESS;
}

/**
 * @brief Obtiene estadísticas del gestor atómico
 * 
 * @param manager Puntero al gestor
 * @param[out] stats Estructura para almacenar estadísticas
 * @return true Si se obtuvieron las estadísticas
 * @return false Si el gestor no está inicializado
 */
bool atomic_get_stats(AtomicRegisterManager_t* manager, AtomicStats_t* stats) {
    if (!manager || !stats || !manager->initialized) {
        return false;
    }
    
    noInterrupts();
    memcpy(stats, &manager->stats, sizeof(AtomicStats_t));
    interrupts();
    
    return true;
}

/**
 * @brief Imprime estadísticas atómicas en formato legible
 * 
 * @param manager Puntero al gestor
 */
void atomic_print_stats(AtomicRegisterManager_t* manager) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    printf("\n=== Estadísticas de Operaciones Atómicas ===\n");
    printf("Total operaciones: %lu\n", (unsigned long)manager->stats.totalOperations);
    printf("Exitosas: %lu\n", (unsigned long)manager->stats.successfulOps);
    printf("Fallos de lock: %lu\n", (unsigned long)manager->stats.failedLocks);
    printf("Timeouts: %lu\n", (unsigned long)manager->stats.timeouts);
    printf("Conflictos: %lu\n", (unsigned long)manager->stats.conflicts);
    printf("Latencia promedio: %lu µs\n", (unsigned long)manager->stats.averageLatency_us);
    
    if (manager->stats.totalOperations > 0) {
        float success_rate = ((float)manager->stats.successfulOps / 
                              (float)manager->stats.totalOperations) * 100.0f;
        printf("Tasa de éxito: %.2f%%\n", success_rate);
    }
    
    printf("============================================\n\n");
}

/**
 * @brief Reinicia las estadísticas del gestor
 * 
 * @param manager Puntero al gestor
 */
void atomic_reset_stats(AtomicRegisterManager_t* manager) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    noInterrupts();
    memset(&manager->stats, 0, sizeof(AtomicStats_t));
    interrupts();
}

/**
 * @brief Verifica si un rango de registros está disponible
 * 
 * @param manager Puntero al gestor
 * @param startAddress Dirección inicial
 * @param count Número de registros
 * @return true Si el rango está disponible
 * @return false Si algún registro está bloqueado
 */
bool atomic_is_range_available(AtomicRegisterManager_t* manager,
                                uint16_t startAddress,
                                uint16_t count) {
    if (!manager || !manager->initialized) {
        return false;
    }
    
    if (startAddress + count > manager->registerCount) {
        return false;
    }
    
    bool available = true;
    
    noInterrupts();
    for (uint16_t i = 0; i < count; i++) {
        if (manager->registers[startAddress + i].locked) {
            available = false;
            break;
        }
    }
    interrupts();
    
    return available;
}

#ifdef __cplusplus
}
#endif

/*
 * Ejemplo de uso:
 * 
 * // Definir array de registros
 * AtomicRegister_t myRegisters[100];
 * AtomicRegisterManager_t manager;
 * 
 * // Inicializar
 * atomic_manager_init(&manager, myRegisters, 100);
 * 
 * // Escritura atómica de 5 registros
 * uint16_t values[] = {100, 200, 300, 400, 500};
 * AtomicResult_t result = atomic_write_registers(&manager, 10, 5, values, 1000);
 * 
 * if (result == ATOMIC_SUCCESS) {
 *     printf("Escritura exitosa!\n");
 * }
 * 
 * // Lectura atómica de 5 registros
 * uint16_t readValues[5];
 * result = atomic_read_registers(&manager, 10, 5, readValues, 1000);
 * 
 * // Operación combinada read/write
 * uint16_t writeVals[] = {999};
 * uint16_t readVals[3];
 * result = atomic_read_write_registers(&manager, 
 *                                       10, 3,    // Leer 3 desde addr 10
 *                                       15, 1,    // Escribir 1 en addr 15
 *                                       writeVals, 
 *                                       readVals, 
 *                                       1000);
 * 
 * // Ver estadísticas
 * atomic_print_stats(&manager);
 */
