/*
    Gateway-MQTT-Modbus - Puente bidireccional entre Modbus y MQTT
    Ejemplo avanzado para ESP8266/ESP32
    
    Descripción:
    Este ejemplo muestra cómo crear un gateway IoT que:
    1. Expone registros Modbus a través de MQTT
    2. Permite controlar registros Modbus desde MQTT
    3. Gestiona reconexión automática a ambos protocolos
    4. Publica estadísticas de funcionamiento
    
    Hardware requerido:
    - ESP8266 o ESP32
    - Módulo RS485 (MAX485) para comunicación Modbus RTU (opcional)
    - Conexión WiFi
    
    Conexiones RS485 (si se usa Modbus RTU):
    - MAX485 RO -> RX del ESP (GPIO 3 en Uno, GPIO 3/16 en ESP)
    - MAX485 DI -> TX del ESP (GPIO 1 en Uno, GPIO 1/17 en ESP)
    - MAX485 DE+RE -> GPIO para control (ej: GPIO 2)
    - MAX485 A/B -> Bus RS485
    
    Autor: Equipo de Desarrollo Modbus
    Fecha: 2024
    Licencia: LGPL-2.1
*/

#include <ModbusMQTT.h>
#include <ModbusRTU.h>  // O ModbusTCP según tu configuración

// ============================================================================
// CONFIGURACIÓN WIFI
// ============================================================================
#ifndef STASSID
#define STASSID "tu_red_wifi"
#endif
#ifndef STAPSK
#define STAPSK "tu_password_wifi"
#endif

// ============================================================================
// CONFIGURACIÓN MQTT
// ============================================================================
#define MQTT_BROKER "test.mosquitto.org"  // Broker público para testing
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "modbus_gateway_001"
#define MQTT_USER ""
#define MQTT_PASS ""

// Topics base
#define MQTT_BASE_TOPIC "modbus/gateway001"

// Topics específicos
#define MQTT_TOPIC_TEMPERATURE MQTT_BASE_TOPIC "/sensor/temperature"
#define MQTT_TOPIC_HUMIDITY    MQTT_BASE_TOPIC "/sensor/humidity"
#define MQTT_TOPIC_SETPOINT    MQTT_BASE_TOPIC "/control/setpoint"
#define MQTT_TOPIC_STATUS      MQTT_BASE_TOPIC "/status"
#define MQTT_TOPIC_COMMAND     MQTT_BASE_TOPIC "/command"

// ============================================================================
// CONFIGURACIÓN MODBUS
// ============================================================================
#define MODBUS_SERIAL Serial2  // UART2 para RS485 en ESP32
#define MODBUS_DE_PIN 2        // Pin para controlar DE/RE del MAX485
#define MODBUS_BAUDRATE 9600
#define MODBUS_SERVER_ID 1

// Direcciones de registros
#define REG_TEMPERATURE 0      // Registro 0: Temperatura (input)
#define REG_HUMIDITY    1      // Registro 1: Humedad (input)
#define REG_SETPOINT    10     // Registro 10: Setpoint (holding)
#define REG_STATUS      11     // Registro 11: Estado del sistema

// ============================================================================
// INSTANCIAS GLOBALES
// ============================================================================
ModbusRTU modbusServer;
ModbusMQTT mqttBridge;

// Variables de estado
uint32_t lastPublishTime = 0;
uint32_t lastSensorReadTime = 0;
int16_t lastTemperature = 0;
int16_t lastHumidity = 0;
bool systemReady = false;

// Contadores para estadísticas
uint32_t modbusRequestsCount = 0;
uint32_t mqttMessagesCount = 0;
uint32_t errorsCount = 0;

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

/**
 * @brief Simular lectura de sensores (reemplazar con lecturas reales)
 * 
 * En una aplicación real, aquí leerías sensores físicos:
 * - DHT22/DHT11 para temperatura/humedad
 * - DS18B20 para temperatura
 * - ADC para sensores analógicos
 */
void readSensors(int16_t* temperature, int16_t* humidity) {
    // SIMULACIÓN: Generar valores aleatorios para demo
    // Temperatura: 20.0°C a 30.0°C (escalado x10 = 200-300)
    *temperature = 200 + random(100);
    
    // Humedad: 40% a 60% (escalado x10 = 400-600)
    *humidity = 400 + random(200);
    
    // En producción, usar algo como:
    // float dhtTemp = dht.readTemperature();
    // *temperature = (int16_t)(dhtTemp * 10);
}

/**
 * @brief Formatear valor para publicación MQTT
 */
String formatValue(int16_t value, int decimalPlaces = 1) {
    String result;
    if (decimalPlaces > 0) {
        float floatValue = value / pow(10, decimalPlaces);
        result = String(floatValue, decimalPlaces);
    } else {
        result = String(value);
    }
    return result;
}

/**
 * @brief Publicar estado del gateway
 */
void publishStatus(const char* status) {
    char payload[128];
    snprintf(payload, sizeof(payload), 
             "{\"status\":\"%s\",\"uptime\":%lu,\"modbus_requests\":%lu,\"mqtt_messages\":%lu}",
             status,
             millis() / 1000,
             modbusRequestsCount,
             mqttMessagesCount);
    
    mqttBridge.publishMessage(MQTT_TOPIC_STATUS, (uint8_t*)payload, strlen(payload));
    mqttMessagesCount++;
}

/**
 * @brief Callback para mensajes MQTT entrantes
 */
void onMqttMessage(const char* topic, const uint8_t* payload, 
                   size_t length, void* userData) {
    Serial.print("→ MQTT [");
    Serial.print(topic);
    Serial.print("]: ");
    
    // Imprimir payload
    for (size_t i = 0; i < length && i < 50; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    
    // Procesar comando
    if (strcmp(topic, MQTT_TOPIC_SETPOINT) == 0) {
        // Parsear nuevo setpoint
        char buffer[50];
        strncpy(buffer, (char*)payload, min(length, sizeof(buffer)-1));
        buffer[min(length, sizeof(buffer)-1)] = '\0';
        
        int16_t newSetpoint = atoi(buffer);
        
        // Escribir en registro Modbus
        if (modbusServer.Hreg(REG_SETPOINT, newSetpoint)) {
            Serial.printf("✓ Setpoint actualizado: %d\n", newSetpoint);
            
            // Confirmar por MQTT
            char confirm[64];
            snprintf(confirm, sizeof(confirm), "{\"setpoint\":%d,\"ok\":true}", newSetpoint);
            mqttBridge.publishMessage(MQTT_TOPIC_SETPOINT, (uint8_t*)confirm, strlen(confirm));
            mqttMessagesCount++;
        } else {
            Serial.println("✗ Error al escribir setpoint");
            errorsCount++;
        }
    }
    else if (strcmp(topic, MQTT_TOPIC_COMMAND) == 0) {
        // Procesar comandos especiales
        if (strncmp((char*)payload, "RESET", 5) == 0) {
            Serial.println("Reiniciando gateway...");
            publishStatus("rebooting");
            delay(1000);
            ESP.restart();
        }
        else if (strncmp((char*)payload, "STATUS", 6) == 0) {
            publishStatus("online");
        }
    }
}

/**
 * @brief Callback para cambios de estado MQTT
 */
void onMqttStateChange(ModbusMQTTConnectionState state, void* userData) {
    switch(state) {
        case MQTT_CONNECTED:
            Serial.println("✓ Conectado al broker MQTT");
            systemReady = true;
            publishStatus("connected");
            break;
            
        case MQTT_DISCONNECTED:
            Serial.println("✗ Desconectado del broker MQTT");
            systemReady = false;
            break;
            
        case MQTT_RECONNECTING:
            Serial.println("⟳ Reconectando al broker MQTT...");
            break;
            
        case MQTT_ERROR:
            Serial.println("⚠ Error de conexión MQTT");
            errorsCount++;
            break;
    }
}

/**
 * @brief Callback para eventos Modbus (opcional)
 */
void onModbusRequest() {
    modbusRequestsCount++;
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // Inicializar serial para debug
    Serial.begin(115200);
    Serial.println("\n=== Gateway MQTT-Modbus ===");
    Serial.println("Iniciando sistema...");
    
    // Configurar pin DE para RS485
    pinMode(MODBUS_DE_PIN, OUTPUT);
    digitalWrite(MODBUS_DE_PIN, LOW);  // Modo recepción inicial
    
    // Inicializar WiFi
    Serial.print("Conectando a WiFi");
    WiFi.begin(STASSID, STAPSK);
    
    int wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
        delay(500);
        Serial.print(".");
        wifiAttempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ WiFi conectado");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n✗ Error al conectar WiFi");
        // Continuar sin WiFi (modo offline)
    }
    
    // Configurar servidor Modbus RTU
    MODBUS_SERIAL.begin(MODBUS_BAUDRATE, SERIAL_8N1);
    modbusServer.server(&MODBUS_SERIAL, MODBUS_DE_PIN);
    modbusServer.serverId(MODBUS_SERVER_ID);
    
    // Inicializar registros Modbus
    modbusServer.addHreg(REG_SETPOINT, 250, 1);   // Setpoint default: 25.0°C
    modbusServer.addHreg(REG_STATUS, 0, 1);       // Estado inicial: 0
    modbusServer.addIreg(REG_TEMPERATURE, 0, 1);  // Temperatura inicial: 0
    modbusServer.addIreg(REG_HUMIDITY, 0, 1);     // Humedad inicial: 0
    
    // Configurar callback Modbus
    modbusServer.onRequest(onModbusRequest);
    
    Serial.printf("Servidor Modbus RTU iniciado (ID=%d, baudrate=%d)\n", 
                  MODBUS_SERVER_ID, MODBUS_BAUDRATE);
    
    // Configurar puente MQTT
    mqttBridge.setBroker(MQTT_BROKER, MQTT_PORT);
    mqttBridge.setClientId(MQTT_CLIENT_ID);
    
    if (strlen(MQTT_USER) > 0) {
        mqttBridge.setCredentials(MQTT_USER, MQTT_PASS);
    }
    
    mqttBridge.setAutoReconnect(true);
    
    // Configurar topics de publicación (Modbus → MQTT)
    ModbusMQTTTopicConfig tempConfig;
    strcpy(tempConfig.topic, MQTT_TOPIC_TEMPERATURE);
    tempConfig.registerAddress = REG_TEMPERATURE;
    tempConfig.registerType = MODBUS_INPUT_REGISTER;
    tempConfig.publishOnChange = true;
    tempConfig.description = "Temperatura en °C (x10)";
    mqttBridge.addPublishTopic(tempConfig);
    
    ModbusMQTTTopicConfig humConfig;
    strcpy(humConfig.topic, MQTT_TOPIC_HUMIDITY);
    humConfig.registerAddress = REG_HUMIDITY;
    humConfig.registerType = MODBUS_INPUT_REGISTER;
    humConfig.publishInterval = 10000;  // Publicar cada 10 segundos
    humConfig.description = "Humedad relativa % (x10)";
    mqttBridge.addPublishTopic(humConfig);
    
    // Configurar topics de suscripción (MQTT → Modbus)
    ModbusMQTTTopicConfig setpointConfig;
    strcpy(setpointConfig.topic, MQTT_TOPIC_SETPOINT);
    setpointConfig.registerAddress = REG_SETPOINT;
    setpointConfig.registerType = MODBUS_HOLDING_REGISTER;
    setpointConfig.qos = MQTT_QOS_1;
    setpointConfig.retain = false;
    mqttBridge.addSubscribeTopic(setpointConfig);
    
    ModbusMQTTTopicConfig commandConfig;
    strcpy(commandConfig.topic, MQTT_TOPIC_COMMAND);
    commandConfig.registerAddress = 0;  // No asociado a registro
    commandConfig.registerType = MODBUS_HOLDING_REGISTER;
    commandConfig.qos = MQTT_QOS_1;
    mqttBridge.addSubscribeTopic(commandConfig);
    
    // Configurar callbacks
    mqttBridge.onMessage(onMqttMessage);
    mqttBridge.onStateChange(onMqttStateChange);
    
    // Iniciar puente MQTT
    if (mqttBridge.begin(&modbusServer)) {
        Serial.println("✓ Puente MQTT iniciado");
    } else {
        Serial.println("✗ Error al iniciar puente MQTT");
    }
    
    Serial.println("\n=== Configuración completada ===");
    Serial.printf("Topics de publicación:\n");
    Serial.printf("  - %s (temperatura)\n", MQTT_TOPIC_TEMPERATURE);
    Serial.printf("  - %s (humedad)\n", MQTT_TOPIC_HUMIDITY);
    Serial.printf("Topics de suscripción:\n");
    Serial.printf("  - %s (setpoint)\n", MQTT_TOPIC_SETPOINT);
    Serial.printf("  - %s (comandos)\n", MQTT_TOPIC_COMMAND);
    Serial.println();
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================
void loop() {
    // Procesar servidor Modbus
    modbusServer.task();
    
    // Procesar puente MQTT
    mqttBridge.process();
    
    // Leer sensores periódicamente (cada 5 segundos)
    if (millis() - lastSensorReadTime >= 5000) {
        lastSensorReadTime = millis();
        
        int16_t temperature, humidity;
        readSensors(&temperature, &humidity);
        
        // Actualizar registros Modbus
        modbusServer.Ireg(REG_TEMPERATURE, temperature);
        modbusServer.Ireg(REG_HUMIDITY, humidity);
        
        // Actualizar estado
        modbusServer.Hreg(REG_STATUS, systemReady ? 1 : 0);
        
        Serial.printf("Sensores: T=%.1f°C, H=%.1f%%\n", 
                      temperature/10.0, humidity/10.0);
        
        // Detectar cambios significativos para publicación inmediata
        if (abs(temperature - lastTemperature) > 5) {  // Cambio > 0.5°C
            mqttBridge.publishValue(MQTT_TOPIC_TEMPERATURE, temperature);
            mqttMessagesCount++;
            lastTemperature = temperature;
        }
        
        if (abs(humidity - lastHumidity) > 10) {  // Cambio > 1%
            mqttBridge.publishValue(MQTT_TOPIC_HUMIDITY, humidity);
            mqttMessagesCount++;
            lastHumidity = humidity;
        }
    }
    
    // Publicar estadísticas periódicamente (cada 60 segundos)
    static uint32_t lastStatsTime = 0;
    if (millis() - lastStatsTime >= 60000) {
        lastStatsTime = millis();
        
        if (systemReady) {
            char stats[128];
            snprintf(stats, sizeof(stats),
                     "{\"uptime\":%lu,\"modbus_req\":%lu,\"mqtt_msg\":%lu,\"errors\":%lu}",
                     millis()/1000, modbusRequestsCount, mqttMessagesCount, errorsCount);
            
            mqttBridge.publishMessage(MQTT_BASE_TOPIC "/stats", (uint8_t*)stats, strlen(stats));
            mqttMessagesCount++;
            
            Serial.printf("Estadísticas: %s\n", stats);
        }
    }
    
    // Pequeña pausa para estabilidad
    delay(50);
}

// ============================================================================
// NOTAS DE USO
// ============================================================================

/*
COMANDOS MQTT DISPONIBLES:

1. Cambiar setpoint:
   Topic: modbus/gateway001/control/setpoint
   Payload: "250"  (representa 25.0°C)

2. Solicitar estado:
   Topic: modbus/gateway001/command
   Payload: "STATUS"

3. Reiniciar gateway:
   Topic: modbus/gateway001/command
   Payload: "RESET"

TOPICS DE SUSCRIPCIÓN PARA MONITOREO:

- modbus/gateway001/sensor/temperature
  Publica temperatura actual cuando cambia > 0.5°C

- modbus/gateway001/sensor/humidity
  Publica humedad cada 10 segundos o cuando cambia > 1%

- modbus/gateway001/status
  Estado del gateway (connected, disconnected, rebooting)

- modbus/gateway001/stats
  Estadísticas cada 60 segundos

PRUEBA CON MOSQUITTO:

# Suscribirse a temperatura
mosquitto_sub -h test.mosquitto.org -t "modbus/gateway001/sensor/temperature"

# Publicar nuevo setpoint
mosquitto_pub -h test.mosquitto.org -t "modbus/gateway001/control/setpoint" -m "275"

# Solicitar estado
mosquitto_pub -h test.mosquitto.org -t "modbus/gateway001/command" -m "STATUS"

CONFIGURACIÓN PARA PRODUCCIÓN:

1. Cambiar broker a uno local o seguro:
   #define MQTT_BROKER "192.168.1.100"
   
2. Habilitar autenticación:
   #define MQTT_USER "mi_usuario"
   #define MQTT_PASS "mi_password_segura"

3. Usar TLS/SSL (puerto 8883):
   #define MQTT_PORT 8883
   
4. Configurar Last Will Testament para detección de fallos

5. Implementar watchdog hardware para recuperación automática
*/
