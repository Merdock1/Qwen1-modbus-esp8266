/*
 * Mejora #2: Cache LRU para Registros Frecuentes
 * Prioridad: Baja (Nice to Have)
 * 
 * Implementa un sistema de caché LRU (Least Recently Used) para acelerar
 * el acceso a registros Modbus frecuentemente consultados.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Configuración del caché LRU
#ifndef MODBUS_CACHE_SIZE
#define MODBUS_CACHE_SIZE 16  // Número máximo de entradas en caché
#endif

#ifndef MODBUS_CACHE_HIT_THRESHOLD
#define MODBUS_CACHE_HIT_THRESHOLD 3  // Mínimo de accesos para considerar "frecuente"
#endif

// Tipo de registro Modbus
typedef enum {
    CACHE_COIL = 0,      // Bobinas
    CACHE_ISTS = 1,      // Entradas discretas
    CACHE_IREG = 2,      // Registros de entrada
    CACHE_HREG = 3       // Registros de retención
} CacheRegisterType_t;

// Entrada individual del caché
typedef struct {
    uint16_t address;           // Dirección del registro
    CacheRegisterType_t type;   // Tipo de registro
    uint16_t value;             // Valor almacenado en caché
    uint32_t lastAccess;        // Timestamp del último acceso
    uint32_t accessCount;       // Número de accesos totales
    bool valid;                 // Indica si la entrada es válida
    bool dirty;                 // Indica si el valor fue modificado (write-back pendiente)
} LRUCacheEntry_t;

// Estadísticas del caché
typedef struct {
    uint32_t totalAccesses;     // Total de accesos al caché
    uint32_t hits;              // Número de aciertos (hits)
    uint32_t misses;            // Número de fallos (misses)
    uint32_t evictions;         // Número de entradas eliminadas
    uint32_t writebacks;        // Número de escrituras pendientes
    float hitRate;              // Porcentaje de aciertos
} LRUCacheStats_t;

// Estructura principal del caché LRU
typedef struct {
    LRUCacheEntry_t entries[MODBUS_CACHE_SIZE];
    LRUCacheStats_t stats;
    bool initialized;
} LRUCache_t;

/**
 * @brief Inicializa el caché LRU
 * 
 * @param cache Puntero a la estructura del caché
 */
static inline void lru_cache_init(LRUCache_t* cache) {
    if (!cache) return;
    
    memset(cache, 0, sizeof(LRUCache_t));
    cache->initialized = true;
}

/**
 * @brief Busca una entrada en el caché
 * 
 * @param cache Puntero a la estructura del caché
 * @param address Dirección del registro a buscar
 * @param type Tipo de registro
 * @param[out] value Puntero donde almacenar el valor encontrado
 * @return true Si se encontró la entrada (hit)
 * @return false Si no se encontró la entrada (miss)
 */
static inline bool lru_cache_get(LRUCache_t* cache, uint16_t address, 
                                  CacheRegisterType_t type, uint16_t* value) {
    if (!cache || !cache->initialized || !value) {
        return false;
    }
    
    cache->stats.totalAccesses++;
    
    // Buscar entrada existente
    for (int i = 0; i < MODBUS_CACHE_SIZE; i++) {
        if (cache->entries[i].valid &&
            cache->entries[i].address == address &&
            cache->entries[i].type == type) {
            
            // Entrada encontrada - actualizar estadísticas
            cache->stats.hits++;
            cache->entries[i].lastAccess = millis();
            cache->entries[i].accessCount++;
            *value = cache->entries[i].value;
            
            return true;
        }
    }
    
    // Entrada no encontrada
    cache->stats.misses++;
    return false;
}

/**
 * @brief Encuentra la entrada menos recientemente usada para eviction
 * 
 * @param cache Puntero a la estructura del caché
 * @return int Índice de la entrada a eliminar, o -1 si hay espacio libre
 */
static int lru_find_victim(LRUCache_t* cache) {
    int victim = -1;
    uint32_t oldest_time = UINT32_MAX;
    
    for (int i = 0; i < MODBUS_CACHE_SIZE; i++) {
        if (!cache->entries[i].valid) {
            // Espacio libre disponible
            return i;
        }
        
        // Buscar entrada más antigua
        if (cache->entries[i].lastAccess < oldest_time) {
            oldest_time = cache->entries[i].lastAccess;
            victim = i;
        }
    }
    
    return victim;
}

/**
 * @brief Almacena un valor en el caché
 * 
 * @param cache Puntero a la estructura del caché
 * @param address Dirección del registro
 * @param type Tipo de registro
 * @param value Valor a almacenar
 * @param from_register true si el valor viene del registro real, false si es nuevo
 * @return true Si se almacenó correctamente
 * @return false Si falló el almacenamiento
 */
static inline bool lru_cache_put(LRUCache_t* cache, uint16_t address,
                                  CacheRegisterType_t type, uint16_t value,
                                  bool from_register) {
    if (!cache || !cache->initialized) {
        return false;
    }
    
    // Verificar si ya existe la entrada
    for (int i = 0; i < MODBUS_CACHE_SIZE; i++) {
        if (cache->entries[i].valid &&
            cache->entries[i].address == address &&
            cache->entries[i].type == type) {
            
            // Actualizar entrada existente
            cache->entries[i].value = value;
            cache->entries[i].lastAccess = millis();
            cache->entries[i].accessCount++;
            cache->entries[i].dirty = !from_register;
            
            return true;
        }
    }
    
    // Necesita nueva entrada - encontrar víctima
    int slot = lru_find_victim(cache);
    
    if (slot < 0) {
        return false;  // Caché lleno y no se pudo encontrar víctima
    }
    
    // Si la entrada estaba ocupada, contar como eviction
    if (cache->entries[slot].valid) {
        cache->stats.evictions++;
        
        // Write-back si está dirty
        if (cache->entries[slot].dirty) {
            cache->stats.writebacks++;
            // Aquí se debería llamar a la función de escritura real
        }
    }
    
    // Crear nueva entrada
    cache->entries[slot].address = address;
    cache->entries[slot].type = type;
    cache->entries[slot].value = value;
    cache->entries[slot].lastAccess = millis();
    cache->entries[slot].accessCount = 1;
    cache->entries[slot].valid = true;
    cache->entries[slot].dirty = !from_register;
    
    return true;
}

/**
 * @brief Invalida una entrada del caché
 * 
 * @param cache Puntero a la estructura del caché
 * @param address Dirección del registro
 * @param type Tipo de registro
 * @return true Si se invalidó correctamente
 * @return false Si no se encontró la entrada
 */
static inline bool lru_cache_invalidate(LRUCache_t* cache, uint16_t address,
                                         CacheRegisterType_t type) {
    if (!cache || !cache->initialized) {
        return false;
    }
    
    for (int i = 0; i < MODBUS_CACHE_SIZE; i++) {
        if (cache->entries[i].valid &&
            cache->entries[i].address == address &&
            cache->entries[i].type == type) {
            
            cache->entries[i].valid = false;
            cache->entries[i].dirty = false;
            
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Limpia todo el caché
 * 
 * @param cache Puntero a la estructura del caché
 */
static inline void lru_cache_flush(LRUCache_t* cache) {
    if (!cache || !cache->initialized) {
        return;
    }
    
    // Write-back de todas las entradas dirty
    for (int i = 0; i < MODBUS_CACHE_SIZE; i++) {
        if (cache->entries[i].valid && cache->entries[i].dirty) {
            cache->stats.writebacks++;
            // Aquí se debería llamar a la función de escritura real
        }
    }
    
    // Limpiar todas las entradas
    memset(cache->entries, 0, sizeof(cache->entries));
    cache->initialized = true;
}

/**
 * @brief Calcula y actualiza el hit rate del caché
 * 
 * @param cache Puntero a la estructura del caché
 * @return float Hit rate como porcentaje (0.0 - 100.0)
 */
static inline float lru_cache_update_stats(LRUCache_t* cache) {
    if (!cache || !cache->initialized) {
        return 0.0f;
    }
    
    if (cache->stats.totalAccesses > 0) {
        cache->stats.hitRate = ((float)cache->stats.hits / 
                                (float)cache->stats.totalAccesses) * 100.0f;
    } else {
        cache->stats.hitRate = 0.0f;
    }
    
    return cache->stats.hitRate;
}

/**
 * @brief Obtiene estadísticas del caché en formato legible
 * 
 * @param cache Puntero a la estructura del caché
 * @param buffer Buffer para almacenar el string de salida
 * @param buffer_size Tamaño del buffer
 * @return int Número de caracteres escritos
 */
int lru_cache_get_stats_string(LRUCache_t* cache, char* buffer, size_t buffer_size) {
    if (!cache || !buffer || buffer_size == 0) {
        return 0;
    }
    
    float hit_rate = lru_cache_update_stats(cache);
    
    int written = snprintf(buffer, buffer_size,
        "=== Estadísticas Caché LRU ===\n"
        "Accesos totales: %lu\n"
        "Hits: %lu\n"
        "Misses: %lu\n"
        "Evicciones: %lu\n"
        "Write-backs: %lu\n"
        "Hit Rate: %.2f%%\n"
        "Entradas válidas: %d/%d\n",
        (unsigned long)cache->stats.totalAccesses,
        (unsigned long)cache->stats.hits,
        (unsigned long)cache->stats.misses,
        (unsigned long)cache->stats.evictions,
        (unsigned long)cache->stats.writebacks,
        hit_rate,
        (int)lru_cache_count_valid(cache),
        MODBUS_CACHE_SIZE
    );
    
    return written;
}

/**
 * @brief Cuenta el número de entradas válidas en el caché
 * 
 * @param cache Puntero a la estructura del caché
 * @return int Número de entradas válidas
 */
static inline int lru_cache_count_valid(LRUCache_t* cache) {
    if (!cache || !cache->initialized) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < MODBUS_CACHE_SIZE; i++) {
        if (cache->entries[i].valid) {
            count++;
        }
    }
    
    return count;
}

/**
 * @brief Imprime contenido del caché para depuración
 * 
 * @param cache Puntero a la estructura del caché
 */
void lru_cache_dump(LRUCache_t* cache) {
    if (!cache || !cache->initialized) {
        return;
    }
    
    printf("\n=== Contenido del Caché LRU ===\n");
    printf("Idx | Addr | Type | Value  | LastAcc  | Count | Dirty\n");
    printf("----|------|------|--------|----------|-------|------\n");
    
    for (int i = 0; i < MODBUS_CACHE_SIZE; i++) {
        if (cache->entries[i].valid) {
            const char* type_str = "";
            switch (cache->entries[i].type) {
                case CACHE_COIL: type_str = "COIL"; break;
                case CACHE_ISTS: type_str = "ISTS"; break;
                case CACHE_IREG: type_str = "IREG"; break;
                case CACHE_HREG: type_str = "HREG"; break;
            }
            
            printf("%3d | %4d | %4s | 0x%04X | %8lu | %5lu | %s\n",
                   i,
                   cache->entries[i].address,
                   type_str,
                   cache->entries[i].value,
                   (unsigned long)cache->entries[i].lastAccess,
                   (unsigned long)cache->entries[i].accessCount,
                   cache->entries[i].dirty ? "Yes" : "No"
            );
        }
    }
    
    printf("===============================\n\n");
}
