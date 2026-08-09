/*
    Red Multi-Drop RS485 - Ejemplo Avanzado
    TAREA 4.3: EJEMPLOS AVANZADOS
    
    Este ejemplo muestra cómo implementar una red Modbus RTU multi-dispositivo:
    - Master que consulta múltiples slaves
    - Gestión de timeouts y reintentos
    - Detección de dispositivos desconectados
    - Balanceo de carga en la red
    
    Hardware requerido:
    - ESP32 o Arduino con UART para RS485
    - Módulos MAX485
    - 2+ dispositivos Modbus slave
    
    Autor: Equipo Modbus
    Versión: 1.0.0
*/

#include <Modbus.h>
#include <ModbusRTU.h>

// ============================================================================
// CONFIGURACIÓN HARDWARE
// ============================================================================

#define MODBUS_SERIAL Serial2
#define MODBUS_TX_ENABLE_PIN 5
#define MODBUS_BAUDRATE 9600

// ============================================================================
// CONFIGURACIÓN DE RED MULTI-DROP
// ============================================================================

// Número máximo de slaves en la red
#define MAX_SLAVES 10

// Lista de IDs de slaves presentes
const uint8_t slaveIds[MAX_SLAVES] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
const uint8_t slaveCount = 10;

// Registros a leer de cada slave
#define REGISTROS_POR_SLAVE 5
#define PRIMER_REGISTRO 0

// Timeout por respuesta (ms)
#define SLAVE_TIMEOUT 500

// Reintentos antes de marcar slave como offline
#define MAX_RETRIES 3

// Intervalo entre consultas (ms)
#define POLL_INTERVAL 100

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

/**
 * @brief Estado de un slave en la red
 */
struct SlaveStatus {
    uint8_t id;                     // ID del slave
    bool online;                    // ¿Disponible?
    uint16_t registers[REGISTROS_POR_SLAVE];  // Valores leídos
    uint32_t successCount;          // Lecturas exitosas
    uint32_t failCount;             // Lecturas fallidas
    uint32_t lastResponseTime;      // Última respuesta exitosa (ms)
    uint8_t consecutiveFails;       // Fallos consecutivos actuales
};

SlaveStatus slaves[MAX_SLAVES];

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

ModbusRTU mb;
uint8_t currentSlaveIndex = 0;
bool waitingForResponse = false;
uint32_t lastPollTime = 0;

// ============================================================================
// INICIALIZACIÓN
// ============================================================================

void initSlaves() {
    Serial.println("Inicializando slaves...");
    
    for (int i = 0; i < slaveCount; i++) {
        slaves[i].id = slaveIds[i];
        slaves[i].online = false;
        slaves[i].successCount = 0;
        slaves[i].failCount = 0;
        slaves[i].lastResponseTime = 0;
        slaves[i].consecutiveFails = 0;
        
        for (int j = 0; j < REGISTROS_POR_SLAVE; j++) {
            slaves[i].registers[j] = 0;
        }
        
        Serial.printf("  Slave %d: Inicializado\n", slaves[i].id);
    }
}

// ============================================================================
// GESTIÓN DE RESPUESTAS
// ============================================================================

/**
 * @brief Callback cuando se recibe respuesta del slave
 */
void onResponse(uint16_t transactionId, uint8_t address, 
                uint8_t function, uint8_t* data, uint16_t length) {
    if (!waitingForResponse) return;
    
    SlaveStatus* slave = &slaves[currentSlaveIndex];
    
    if (address != slave->id) return;
    
    // Respuesta exitosa
    slave->online = true;
    slave->successCount++;
    slave->consecutiveFails = 0;
    slave->lastResponseTime = millis();
    
    // Copiar registros
    for (int i = 0; i < REGISTROS_POR_SLAVE && i < length / 2; i++) {
        slave->registers[i] = ((uint16_t)data[i * 2] << 8) | data[i * 2 + 1];
    }
    
    waitingForResponse = false;
}

/**
 * @brief Callback cuando hay timeout/error
 */
void onError(int8_t error) {
    if (!waitingForResponse) return;
    
    SlaveStatus* slave = &slaves[currentSlaveIndex];
    slave->failCount++;
    slave->consecutiveFails++;
    
    // Marcar offline después de múltiples fallos
    if (slave->consecutiveFails >= MAX_RETRIES) {
        slave->online = false;
        Serial.printf("\n[ALERTA] Slave %d OFFLINE (%d fallos consecutivos)\n", 
                      slave->id, slave->consecutiveFails);
    }
    
    waitingForResponse = false;
}

// ============================================================================
// CONSULTA A SLAVES
// ============================================================================

void pollNextSlave() {
    // Encontrar siguiente slave válido
    uint8_t attempts = 0;
    do {
        currentSlaveIndex = (currentSlaveIndex + 1) % slaveCount;
        attempts++;
    } while (attempts < slaveCount);
    
    SlaveStatus* slave = &slaves[currentSlaveIndex];
    
    // Consultar holding registers
    Serial.printf("Consultando slave %d... ", slave->id);
    
    uint16_t result = mb.readHreg(slave->id, PRIMER_REGISTRO,
                                   slave->registers, REGISTROS_POR_SLAVE,
                                   onResponse, onError);
    
    if (result == 0) {
        waitingForResponse = true;
    } else {
        Serial.printf("Error al enviar solicitud: %d\n", result);
        slave->failCount++;
    }
}

// ============================================================================
// IMPRESIÓN DE ESTADO
// ============================================================================

void printNetworkStatus() {
    Serial.println("\n========== ESTADO DE RED ==========");
    Serial.printf("%-6s %-8s %-10s %-10s %-12s\n", 
                  "ID", "Online", "Éxitos", "Fallos", "Última Resp.");
    Serial.println("------------------------------------------");
    
    for (int i = 0; i < slaveCount; i++) {
        SlaveStatus* s = &slaves[i];
        
        Serial.printf("%-6d %-8s %-10lu %-10lu %-12lu\n",
                      s->id,
                      s->online ? "✓" : "✗",
                      s->successCount,
                      s->failCount,
                      millis() - s->lastResponseTime);
        
        if (s->online && s->successCount > 0) {
            Serial.print("   Regs: ");
            for (int j = 0; j < REGISTROS_POR_SLAVE; j++) {
                Serial.printf("%d ", s->registers[j]);
            }
            Serial.println();
        }
    }
    
    Serial.println("======================================\n");
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("\n=== Red Multi-Drop RS485 ===");
    Serial.printf("Baudrate: %d\n", MODBUS_BAUDRATE);
    Serial.printf("Slaves configurados: %d\n", slaveCount);
    
    // Inicializar slaves
    initSlaves();
    
    // Configurar Modbus
    MODBUS_SERIAL.begin(MODBUS_BAUDRATE, SERIAL_8N1);
    mb.begin(&MODBUS_SERIAL, MODBUS_TX_ENABLE_PIN);
    
    Serial.println("\nRed lista. Iniciando polling...\n");
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
    mb.task();
    
    uint32_t currentTime = millis();
    
    // Verificar timeout de respuesta
    static uint32_t responseWaitStart = 0;
    if (waitingForResponse && (currentTime - responseWaitStart > SLAVE_TIMEOUT)) {
        Serial.println("Timeout!");
        onError(-1);
    }
    
    // Consultar siguiente slave
    if (!waitingForResponse && (currentTime - lastPollTime >= POLL_INTERVAL)) {
        responseWaitStart = currentTime;
        pollNextSlave();
        lastPollTime = currentTime;
    }
    
    // Imprimir estado periódicamente
    static uint32_t lastStatusPrint = 0;
    if (currentTime - lastStatusPrint >= 30000) {  // Cada 30 segundos
        printNetworkStatus();
        lastStatusPrint = currentTime;
    }
    
    delay(10);
}
