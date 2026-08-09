/*
    Servidor Web de Configuración Modbus - Ejemplo Avanzado
    TAREA 3.4: SERVIDOR WEB DE CONFIGURACIÓN
    
    Este ejemplo muestra cómo implementar un servidor web completo para:
    - Monitorizar el estado del sistema en tiempo real
    - Ver y editar registros Modbus desde el navegador
    - Configurar parámetros de red y Modbus
    - Ver estadísticas de comunicación
    
    Hardware requerido:
    - ESP32 o ESP8266 con WiFi
    
    Conexiones:
    - RS485 MAX485:
      * DI -> GPIO17 (ESP32) / GPIO1 (ESP8266)
      * RO -> GPIO16 (ESP32) / GPIO3 (ESP8266)
      * DE+RE -> GPIO5 (ESP32) / GPIO2 (ESP8266)
    
    Instrucciones:
    1. Configurar SSID y contraseña WiFi
    2. Subir código al ESP
    3. Abrir Serial Monitor (115200 baud)
    4. Conectarse a la IP mostrada desde navegador
    
    Autor: Equipo Modbus
    Versión: 1.0.0
*/

#include <Modbus.h>
#include <ModbusRTU.h>
#include <ModbusWebConfig.h>

// ============================================================================
// CONFIGURACIÓN WIFI
// ============================================================================

const char* WIFI_SSID = "TuSSID";           // CAMBIAR por tu red WiFi
const char* WIFI_PASSWORD = "TuPassword";   // CAMBIAR por tu contraseña

// ============================================================================
// CONFIGURACIÓN MODBUS RTU
// ============================================================================

#define MODBUS_SERIAL Serial2               // UART para RS485
#define MODBUS_TX_ENABLE_PIN 5              // Pin DE/RE del MAX485
#define MODBUS_SLAVE_ID 1                   // ID de este dispositivo
#define MODBUS_BAUDRATE 9600                // Velocidad de comunicación

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

ModbusRTU mb;                               // Instancia Modbus RTU
ModbusWebServer webServer(&mb);             // Servidor web

// Registros Modbus de ejemplo
uint16_t holdingRegisters[100];             // Registros de salida (RW)
uint16_t inputRegisters[100];               // Registros de entrada (R)
bool coils[100];                            // Coils (RW boolean)
bool discreteInputs[100];                   // Entradas discretas (R)

// Contador para demostración
uint32_t uptimeCounter = 0;

// ============================================================================
// CALLBACKS MODBUS
// ============================================================================

/**
 * @brief Callback para lectura de holding registers
 */
int16_t onReadHoldingRegisters(uint16_t address, uint16_t value) {
    if (address < 100) {
        return holdingRegisters[address];
    }
    return 0;
}

/**
 * @brief Callback para escritura de holding registers
 */
bool onWriteHoldingRegisters(uint16_t address, uint16_t value) {
    if (address < 100) {
        holdingRegisters[address] = value;
        return true;
    }
    return false;
}

/**
 * @brief Callback para lectura de coils
 */
int16_t onReadCoils(uint16_t address) {
    if (address < 100) {
        return coils[address] ? 1 : 0;
    }
    return 0;
}

/**
 * @brief Callback para escritura de coils
 */
bool onWriteCoils(uint16_t address, bool value) {
    if (address < 100) {
        coils[address] = value;
        return true;
    }
    return false;
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    // Inicializar serial para debug
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    Serial.println("\n=== Servidor Web Modbus ===");
    
    // Inicializar registros con valores por defecto
    for (int i = 0; i < 100; i++) {
        holdingRegisters[i] = i * 10;       // Valores de ejemplo
        inputRegisters[i] = analogRead(i % 8); // Simular lecturas
        coils[i] = (i % 2 == 0);            // Alternar coils
        discreteInputs[i] = false;
    }
    
    // Configurar pin de enable para RS485
    pinMode(MODBUS_TX_ENABLE_PIN, OUTPUT);
    digitalWrite(MODBUS_TX_ENABLE_PIN, LOW);
    
    // Inicializar Modbus RTU
    MODBUS_SERIAL.begin(MODBUS_BAUDRATE, SERIAL_8N1);
    mb.begin(&MODBUS_SERIAL, MODBUS_TX_ENABLE_PIN);
    mb.setSlaveId(MODBUS_SLAVE_ID);
    
    // Configurar callbacks
    mb.onReadCoil(onReadCoils);
    mb.onWriteCoil(onWriteCoils);
    mb.onReadHoldingRegister(onReadHoldingRegisters);
    mb.onWriteHoldingRegister(onWriteHoldingRegisters);
    
    Serial.print("Iniciando WiFi... ");
    
    // Conectar a WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int timeout = 30;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(500);
        Serial.print(".");
        timeout--;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n¡Error! No se pudo conectar a WiFi");
        Serial.println("Reiniciando en 5 segundos...");
        delay(5000);
        ESP.restart();
        return;
    }
    
    Serial.println("\n✓ WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    // Iniciar servidor web
    Serial.print("Iniciando servidor web... ");
    if (webServer.begin()) {
        Serial.println("✓ Servidor web activo!");
        Serial.println("\n===========================================");
        Serial.println("Accede desde tu navegador a:");
        Serial.print("http://");
        Serial.println(WiFi.localIP());
        Serial.println("===========================================");
    } else {
        Serial.println("✗ Error al iniciar servidor web");
    }
    
    Serial.println("\nModbus RTU listo en puerto Serial2");
    Serial.print("ID esclavo: ");
    Serial.println(MODBUS_SLAVE_ID);
    Serial.print("Baudrate: ");
    Serial.println(MODBUS_BAUDRATE);
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
    // Procesar cliente web
    webServer.handleClient();
    
    // Procesar Modbus
    mb.task();
    
    // Actualizar datos de ejemplo cada segundo
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();
        uptimeCounter++;
        
        // Actualizar registro de uptime
        holdingRegisters[0] = uptimeCounter & 0xFFFF;
        holdingRegisters[1] = (uptimeCounter >> 16) & 0xFFFF;
        
        // Simular lectura de sensor en input register
        inputRegisters[0] = analogRead(0);
        
        // Mostrar estadísticas básicas
        if (uptimeCounter % 10 == 0) {
            Serial.print("[");
            Serial.print(uptimeCounter);
            Serial.print("s] Mensajes: ");
            Serial.print(mb.getTotalMessageCount());
            Serial.print(", Errores: ");
            Serial.println(mb.getTotalErrorCount());
        }
    }
    
    // Pequeño delay para estabilidad
    delay(10);
}
