/*
 * Mejora #3: Simulador Integrado para Testing
 * Prioridad: Baja (Nice to Have)
 * 
 * Simulador de red Modbus para pruebas unitarias y de integración
 * sin necesidad de hardware físico.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Tipos de dispositivos simulados
typedef enum {
    SIM_DEVICE_MASTER = 0,    // Dispositivo Maestro
    SIM_DEVICE_SLAVE = 1,     // Dispositivo Esclavo
    SIM_DEVICE_BRIDGE = 2     // Dispositivo Puente
} SimDeviceType_t;

// Tipo de medio físico
typedef enum {
    SIM_MEDIUM_RTU = 0,       // RS-485 Serial
    SIM_MEDIUM_TCP = 1,       // Ethernet TCP/IP
    SIM_MEDIUM_WIFI = 2       // WiFi
} SimMediumType_t;

// Configuración de dispositivo simulado
typedef struct {
    uint8_t slaveId;                  // ID de esclavo (1-247)
    SimDeviceType_t deviceType;       // Tipo de dispositivo
    SimMediumType_t medium;           // Medio físico
    uint32_t baudrate;                // Velocidad en baudios (para RTU)
    char ipAddress[16];               // Dirección IP (para TCP)
    uint16_t port;                    // Puerto (para TCP)
    uint16_t numCoils;                // Número de bobinas
    uint16_t numInputs;               // Número de entradas discretas
    uint16_t numHoldingRegs;          // Número de registros de retención
    uint16_t numInputRegs;            // Número de registros de entrada
    bool (*coilCallback)(uint16_t addr, bool value);
    bool (*registerCallback)(uint16_t addr, uint16_t value);
} SimDeviceConfig_t;

// Estadísticas de simulación
typedef struct {
    uint32_t framesSent;              // Tramas enviadas
    uint32_t framesReceived;          // Tramas recibidas
    uint32_t framesDropped;           // Tramas perdidas
    uint32_t crcErrors;               // Errores CRC
    uint32_t timeoutErrors;           // Errores de timeout
    uint32_t validResponses;          // Respuestas válidas
    uint32_t exceptionResponses;      // Respuestas de excepción
    float averageLatency_ms;          // Latencia promedio en ms
} SimStats_t;

// Estructura de trama Modbus simulada
typedef struct {
    uint8_t data[256];                // Datos de la trama
    size_t length;                    // Longitud de la trama
    uint32_t timestamp;               // Timestamp de creación
    uint8_t sourceId;                 // ID de origen
    uint8_t destId;                   // ID de destino
    bool isValid;                     // Indica si la trama es válida
} SimFrame_t;

// Cola de tramas para simulación
typedef struct {
    SimFrame_t frames[32];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} SimFrameQueue_t;

// Contexto principal del simulador
typedef struct {
    SimDeviceConfig_t devices[16];    // Máximo 16 dispositivos
    uint8_t deviceCount;              // Número de dispositivos activos
    SimFrameQueue_t txQueue;          // Cola de transmisión
    SimFrameQueue_t rxQueue;          // Cola de recepción
    SimStats_t stats;                 // Estadísticas globales
    bool running;                     // Estado del simulador
    uint32_t simulationTime;          // Tiempo de simulación actual
    float packetLossRate;             // Tasa de pérdida de paquetes (0.0-1.0)
    float latencyMs;                  // Latencia artificial en ms
} ModbusSimulator_t;

/**
 * @brief Inicializa el simulador Modbus
 * 
 * @param sim Puntero a la estructura del simulador
 * @return true Si se inicializó correctamente
 * @return false Si falló la inicialización
 */
bool modbus_sim_init(ModbusSimulator_t* sim) {
    if (!sim) return false;
    
    memset(sim, 0, sizeof(ModbusSimulator_t));
    sim->running = false;
    sim->packetLossRate = 0.0f;
    sim->latencyMs = 0.0f;
    
    return true;
}

/**
 * @brief Agrega un dispositivo al simulador
 * 
 * @param sim Puntero a la estructura del simulador
 * @param config Configuración del dispositivo
 * @return int Índice del dispositivo agregado, -1 si falló
 */
int modbus_sim_add_device(ModbusSimulator_t* sim, const SimDeviceConfig_t* config) {
    if (!sim || !config) return -1;
    if (sim->deviceCount >= 16) return -1;
    
    memcpy(&sim->devices[sim->deviceCount], config, sizeof(SimDeviceConfig_t));
    return sim->deviceCount++;
}

/**
 * @brief Elimina un dispositivo del simulador
 * 
 * @param sim Puntero a la estructura del simulador
 * @param deviceIndex Índice del dispositivo a eliminar
 * @return true Si se eliminó correctamente
 * @return false Si el índice es inválido
 */
bool modbus_sim_remove_device(ModbusSimulator_t* sim, uint8_t deviceIndex) {
    if (!sim || deviceIndex >= sim->deviceCount) return false;
    
    // Mover dispositivos restantes
    for (uint8_t i = deviceIndex; i < sim->deviceCount - 1; i++) {
        memcpy(&sim->devices[i], &sim->devices[i + 1], sizeof(SimDeviceConfig_t));
    }
    
    sim->deviceCount--;
    return true;
}

/**
 * @brief Crea una trama Modbus RTU simulada
 * 
 * @param slaveId ID del esclavo
 * @param functionCode Código de función
 * @param data Datos de la trama
 * @param dataLength Longitud de los datos
 * @param[out] frame Estructura de trama resultante
 * @return true Si se creó correctamente
 * @return false Si falló la creación
 */
bool modbus_sim_create_frame(ModbusSimulator_t* sim, uint8_t slaveId, 
                              uint8_t functionCode, const uint8_t* data,
                              size_t dataLength, SimFrame_t* frame) {
    if (!sim || !frame || !data) return false;
    if (dataLength > 250) return false;  // Límite de seguridad
    
    frame->data[0] = slaveId;
    frame->data[1] = functionCode;
    
    if (dataLength > 0) {
        memcpy(&frame->data[2], data, dataLength);
    }
    
    frame->length = 2 + dataLength;
    
    // Calcular CRC (simplificado para simulación)
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < frame->length; i++) {
        crc ^= frame->data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    frame->data[frame->length++] = crc & 0xFF;
    frame->data[frame->length++] = (crc >> 8) & 0xFF;
    
    frame->timestamp = sim->simulationTime;
    frame->sourceId = 0x00;  // Maestro
    frame->destId = slaveId;
    frame->isValid = true;
    
    sim->stats.framesSent++;
    
    return true;
}

/**
 * @brief Procesa una trama entrante en el simulador
 * 
 * @param sim Puntero a la estructura del simulador
 * @param frame Trama entrante
 * @param[out] response Trama de respuesta
 * @return true Si se procesó correctamente
 * @return false Si hubo error
 */
bool modbus_sim_process_frame(ModbusSimulator_t* sim, const SimFrame_t* frame, 
                               SimFrame_t* response) {
    if (!sim || !frame || !response) return false;
    if (!frame->isValid) return false;
    
    sim->stats.framesReceived++;
    
    // Simular pérdida de paquete
    if (((float)rand() / RAND_MAX) < sim->packetLossRate) {
        sim->stats.framesDropped++;
        return false;
    }
    
    // Simular latencia
    if (sim->latencyMs > 0) {
        // En implementación real, usar delay o timer
        sim->simulationTime += (uint32_t)(sim->latencyMs * 1000);
    }
    
    // Validar CRC (simplificado)
    // ... código de validación CRC ...
    
    // Buscar dispositivo esclavo
    uint8_t slaveId = frame->data[0];
    uint8_t funcCode = frame->data[1];
    
    int deviceIndex = -1;
    for (uint8_t i = 0; i < sim->deviceCount; i++) {
        if (sim->devices[i].slaveId == slaveId) {
            deviceIndex = i;
            break;
        }
    }
    
    if (deviceIndex < 0) {
        // Esclavo no encontrado - generar excepción
        response->data[0] = slaveId;
        response->data[1] = funcCode | 0x80;  // Bit de excepción
        response->data[2] = 0x02;  // Código de excepción: Illegal Data Address
        response->length = 3;
        
        // Agregar CRC
        // ... cálculo CRC ...
        
        sim->stats.exceptionResponses++;
        return true;
    }
    
    // Procesar función Modbus (implementación simplificada)
    switch (funcCode) {
        case 0x03:  // Read Holding Registers
            // Implementar lógica de lectura
            break;
            
        case 0x06:  // Write Single Register
            // Implementar lógica de escritura
            break;
            
        default:
            // Función no soportada
            response->data[0] = slaveId;
            response->data[1] = funcCode | 0x80;
            response->data[2] = 0x01;  // Illegal Function
            response->length = 3;
            sim->stats.exceptionResponses++;
            break;
    }
    
    response->isValid = true;
    response->timestamp = sim->simulationTime;
    sim->stats.validResponses++;
    
    return true;
}

/**
 * @brief Inicia la simulación
 * 
 * @param sim Puntero a la estructura del simulador
 * @return true Si se inició correctamente
 * @return false Si ya estaba corriendo
 */
bool modbus_sim_start(ModbusSimulator_t* sim) {
    if (!sim || sim->running) return false;
    
    sim->running = true;
    sim->simulationTime = 0;
    memset(&sim->stats, 0, sizeof(SimStats_t));
    
    printf("[SIMULADOR] Iniciado con %d dispositivos\n", sim->deviceCount);
    
    return true;
}

/**
 * @brief Detiene la simulación
 * 
 * @param sim Puntero a la estructura del simulador
 * @return true Si se detuvo correctamente
 * @return false Si no estaba corriendo
 */
bool modbus_sim_stop(ModbusSimulator_t* sim) {
    if (!sim || !sim->running) return false;
    
    sim->running = false;
    printf("[SIMULADOR] Detenido. Tiempo total: %lu ms\n", 
           (unsigned long)sim->simulationTime);
    
    return true;
}

/**
 * @brief Configura parámetros de red para testing
 * 
 * @param sim Puntero a la estructura del simulador
 * @param packetLossRate Tasa de pérdida de paquetes (0.0-1.0)
 * @param latencyMs Latencia artificial en milisegundos
 */
void modbus_sim_configure_network(ModbusSimulator_t* sim, 
                                   float packetLossRate, 
                                   float latencyMs) {
    if (!sim) return;
    
    sim->packetLossRate = (packetLossRate < 0.0f) ? 0.0f : 
                          (packetLossRate > 1.0f) ? 1.0f : packetLossRate;
    sim->latencyMs = (latencyMs < 0.0f) ? 0.0f : latencyMs;
    
    printf("[SIMULADOR] Red configurada: Packet Loss=%.2f%%, Latencia=%.2fms\n",
           sim->packetLossRate * 100.0f, sim->latencyMs);
}

/**
 * @brief Imprime estadísticas de la simulación
 * 
 * @param sim Puntero a la estructura del simulador
 */
void modbus_sim_print_stats(ModbusSimulator_t* sim) {
    if (!sim) return;
    
    printf("\n=== Estadísticas del Simulador ===\n");
    printf("Dispositivos activos: %d\n", sim->deviceCount);
    printf("Tramas enviadas: %lu\n", (unsigned long)sim->stats.framesSent);
    printf("Tramas recibidas: %lu\n", (unsigned long)sim->stats.framesReceived);
    printf("Tramas perdidas: %lu\n", (unsigned long)sim->stats.framesDropped);
    printf("Respuestas válidas: %lu\n", (unsigned long)sim->stats.validResponses);
    printf("Respuestas de excepción: %lu\n", (unsigned long)sim->stats.exceptionResponses);
    printf("Tiempo de simulación: %lu ms\n", (unsigned long)sim->simulationTime);
    printf("===============================\n\n");
}

/**
 * @brief Ejecuta un ciclo de simulación
 * 
 * @param sim Puntero a la estructura del simulador
 * @param deltaMs Tiempo delta en milisegundos
 */
void modbus_sim_step(ModbusSimulator_t* sim, uint32_t deltaMs) {
    if (!sim || !sim->running) return;
    
    sim->simulationTime += deltaMs;
    
    // Procesar cola de transmisión
    // ... implementación ...
}

#ifdef __cplusplus
}
#endif

/*
 * Ejemplo de uso:
 * 
 * ModbusSimulator_t sim;
 * modbus_sim_init(&sim);
 * 
 * SimDeviceConfig_t slave1 = {
 *     .slaveId = 1,
 *     .deviceType = SIM_DEVICE_SLAVE,
 *     .medium = SIM_MEDIUM_RTU,
 *     .baudrate = 9600,
 *     .numHoldingRegs = 100
 * };
 * 
 * modbus_sim_add_device(&sim, &slave1);
 * modbus_sim_start(&sim);
 * 
 * // Configurar red con 5% de pérdida y 50ms de latencia
 * modbus_sim_configure_network(&sim, 0.05f, 50.0f);
 * 
 * // Ejecutar simulación
 * for (int i = 0; i < 1000; i++) {
 *     modbus_sim_step(&sim, 10);
 * }
 * 
 * modbus_sim_print_stats(&sim);
 * modbus_sim_stop(&sim);
 */
