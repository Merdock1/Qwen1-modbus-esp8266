/*
    DataLogger con Timestamp - Ejemplo Avanzado
    TAREA 4.3: EJEMPLOS AVANZADOS
    
    Este ejemplo implementa un sistema de registro de datos (data logger) que:
    - Lee registros Modbus periódicamente
    - Añade timestamp RTC o NTP a cada lectura
    - Guarda datos en SD card o SPIFFS
    - Formato CSV compatible con Excel
    
    Hardware requerido:
    - ESP32 con RTC interno o módulo RTC DS3231
    - Lector SD card (opcional)
    - Dispositivo Modbus slave
    
    Autor: Equipo Modbus
    Versión: 1.0.0
*/

#include <Modbus.h>
#include <ModbusRTU.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

// ============================================================================
// CONFIGURACIÓN WIFI PARA NTP
// ============================================================================

const char* WIFI_SSID = "TuSSID";
const char* WIFI_PASSWORD = "TuPassword";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;     // UTC+1
const int   daylightOffset_sec = 3600; // DST

// ============================================================================
// CONFIGURACIÓN MODBUS
// ============================================================================

#define MODBUS_SERIAL Serial2
#define MODBUS_TX_ENABLE_PIN 5
#define MODBUS_SLAVE_ID 1
#define MODBUS_BAUDRATE 9600

// Dirección del slave a leer
#define SLAVE_ADDRESS 10
#define FIRST_REGISTER 0
#define REGISTER_COUNT 10

// ============================================================================
// CONFIGURACIÓN SD CARD
// ============================================================================

#define SD_CS_PIN 5
#define LOG_FILE "/modbus_log.csv"

// ============================================================================
// INTERVALO DE LOGGING (segundos)
// ============================================================================

#define LOG_INTERVAL 60  // Leer cada 60 segundos

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

ModbusRTU mb;
File logFile;
uint16_t registerValues[REGISTER_COUNT];
uint32_t lastLogTime = 0;
uint32_t logCount = 0;

// ============================================================================
// CONFIGURACIÓN DE HORA (NTP o RTC)
// ============================================================================

void configureNTP() {
    Serial.print("Configurando NTP... ");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5000)) {
        Serial.println("¡Error al obtener hora NTP!");
        return;
    }
    Serial.println("✓ Hora sincronizada");
}

String getTimestamp() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "ERROR";
    }
    
    char buffer[32];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
    return String(buffer);
}

// ============================================================================
// INICIALIZACIÓN SD CARD
// ============================================================================

bool initSDCard() {
    Serial.print("Inicializando SD card... ");
    
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("¡Error! SD card no detectada");
        return false;
    }
    
    Serial.println("✓ SD card lista");
    
    // Crear archivo si no existe y añadir cabecera CSV
    bool newFile = !SD.exists(LOG_FILE);
    logFile = SD.open(LOG_FILE, FILE_APPEND);
    
    if (!logFile) {
        Serial.println("¡Error al abrir archivo de log!");
        return false;
    }
    
    if (newFile) {
        // Escribir cabecera CSV
        logFile.println("Timestamp,Registro0,Registro1,Registro2,Registro3,Registro4,"
                       "Registro5,Registro6,Registro7,Registro8,Registro9");
        logFile.close();
    }
    
    return true;
}

// ============================================================================
// ESCRITURA DE DATOS EN LOG
// ============================================================================

void writeLogEntry() {
    logFile = SD.open(LOG_FILE, FILE_APPEND);
    if (!logFile) {
        Serial.println("¡Error al escribir en log!");
        return;
    }
    
    // Obtener timestamp
    String timestamp = getTimestamp();
    
    // Escribir línea CSV
    logFile.print(timestamp);
    for (int i = 0; i < REGISTER_COUNT; i++) {
        logFile.print(",");
        logFile.print(registerValues[i]);
    }
    logFile.println();
    logFile.close();
    
    logCount++;
    Serial.printf("[%s] Log #%d guardado\n", timestamp.c_str(), logCount);
}

// ============================================================================
// LECTURA MODBUS
// ============================================================================

bool readModbusRegisters() {
    Serial.print("Leyendo registros Modbus... ");
    
    // Leer holding registers del slave
    uint16_t result = mb.readHreg(SLAVE_ADDRESS, FIRST_REGISTER, 
                                   registerValues, REGISTER_COUNT);
    
    if (result == 0) {
        Serial.println("✓ Lectura exitosa");
        return true;
    } else {
        Serial.printf("✗ Error %d\n", result);
        return false;
    }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    
    Serial.println("\n=== DataLogger Modbus con Timestamp ===");
    
    // Conectar WiFi para NTP
    Serial.print("Conectando a WiFi... ");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int timeout = 30;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(500);
        Serial.print(".");
        timeout--;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi conectado");
        configureNTP();
    } else {
        Serial.println("\n✗ WiFi no disponible - usando reloj interno");
    }
    
    // Inicializar SD card
    if (!initSDCard()) {
        Serial.println("Continuando sin SD card (solo monitor serial)");
    }
    
    // Configurar Modbus
    MODBUS_SERIAL.begin(MODBUS_BAUDRATE, SERIAL_8N1);
    mb.begin(&MODBUS_SERIAL, MODBUS_TX_ENABLE_PIN);
    mb.setSlaveId(MODBUS_SLAVE_ID);
    
    Serial.println("\nDataLogger listo");
    Serial.printf("Intervalo: %d segundos\n", LOG_INTERVAL);
    Serial.printf("Registros: %d desde dirección %d\n", REGISTER_COUNT, FIRST_REGISTER);
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
    mb.task();
    
    uint32_t currentTime = millis();
    
    // Verificar si es hora de hacer log
    if (currentTime - lastLogTime >= (LOG_INTERVAL * 1000UL)) {
        lastLogTime = currentTime;
        
        // Leer registros Modbus
        if (readModbusRegisters()) {
            // Escribir en log si SD está disponible
            if (SD.begin(SD_CS_PIN)) {
                writeLogEntry();
            } else {
                // Mostrar en serial si no hay SD
                Serial.print("Datos: ");
                for (int i = 0; i < REGISTER_COUNT; i++) {
                    Serial.printf("%d ", registerValues[i]);
                }
                Serial.println();
            }
        }
    }
    
    delay(100);
}
