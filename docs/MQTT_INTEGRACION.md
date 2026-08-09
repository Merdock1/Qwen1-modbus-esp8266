# Integración MQTT - Guía Completa

## Introducción

La biblioteca Modbus ahora incluye soporte nativo para integración MQTT, permitiendo crear puentes bidireccionales entre protocolos Modbus y MQTT para aplicaciones de IoT Industrial.

## Arquitectura

```
┌─────────────────┐         ┌──────────────────┐         ┌─────────────────┐
│  Dispositivos   │  RS485  │   Gateway ESP32  │  WiFi   │    Broker MQTT  │
│     Modbus      │ <-----> │  con ModbusMQTT  │ <-----> │                 │
│  (sensores PLC) │         │                  │         │  (Mosquitto,    │
└─────────────────┘         └──────────────────┘         │   AWS IoT, etc) │
                                                          └─────────────────┘
                                                                   │
                                                          ┌─────────────────┐
                                                          │  Aplicaciones   │
                                                          │  (Node-RED,     │
                                                          │   Dashboard,    │
                                                          │   Mobile App)   │
                                                          └─────────────────┘
```

## Características de ModbusMQTT

### Funcionalidades Principales

1. **Publicación Automática**
   - Detecta cambios en registros Modbus
   - Publica solo cuando hay variaciones significativas
   - Configurable por topic

2. **Suscripción Bidireccional**
   - Recibe comandos desde MQTT
   - Escribe en registros Modbus automáticamente
   - Validación de datos incorporada

3. **Gestión de Conexión**
   - Reconexión automática ante fallos
   - Last Will Testament para detección de desconexiones
   - Buffer de mensajes offline

4. **Formatos Flexibles**
   - Plain text: `"255"`
   - JSON: `{"value": 255}`
   - CSV: `255,60,1013`

## API Reference

### Clase ModbusMQTT

#### Métodos de Configuración

```cpp
// Configurar broker
void setBroker(const char* broker, uint16_t port = 1883);
void setCredentials(const char* user, const char* pass = nullptr);
void setClientId(const char* clientId);

// Configurar comportamiento
void setAutoReconnect(bool enable);
void setPublishFormat(ModbusMQTTPublishFormat format);
void includeTimestamp(bool enable);
void setBaseTopic(const char* base);

// Configurar Last Will
void setLastWill(const char* topic, const char* payload,
                 ModbusMQTTQoS qos = MQTT_QOS_0, bool retain = false);
```

#### Gestión de Topics

```cpp
// Agregar topic de publicación (Modbus → MQTT)
bool addPublishTopic(const ModbusMQTTTopicConfig& config);

// Agregar topic de suscripción (MQTT → Modbus)
bool addSubscribeTopic(const ModbusMQTTTopicConfig& config);

// Limpiar topics
void clearPublishTopics();
void clearSubscribeTopics();
```

#### Operaciones

```cpp
// Inicializar
bool begin(Modbus* modbus = nullptr);

// Procesar (llamar en loop)
void process();

// Operaciones manuales
bool publishValue(const char* topic, int16_t value,
                  ModbusMQTTQoS qos = MQTT_QOS_1, bool retain = false);
bool readRegister(uint16_t address, ModbusRegisterType type, int16_t* value);
bool writeRegister(uint16_t address, ModbusRegisterType type, int16_t value);

// Finalizar
void end();
```

#### Callbacks

```cpp
// Mensaje recibido
void onMessage(ModbusMQTTCallback callback, void* userData = nullptr);

// Cambio de estado
void onStateChange(ModbusMQTTStateCallback callback, void* userData = nullptr);
```

#### Estado y Estadísticas

```cpp
ModbusMQTTConnectionState getConnectionState() const;
bool isConnected() const;
void getStatistics(uint32_t* reconnectCount, 
                   uint32_t* messagesPublished,
                   uint32_t* messagesReceived);
void reconnect();
```

### Estructuras de Configuración

#### ModbusMQTTTopicConfig

```cpp
struct ModbusMQTTTopicConfig {
    char topic[MODBUS_MQTT_MAX_TOPIC_LENGTH];  // Topic MQTT
    uint16_t registerAddress;                   // Dirección Modbus
    ModbusRegisterType registerType;            // Tipo de registro
    ModbusMQTTQoS qos;                          // Calidad de servicio
    bool retain;                                // Retener mensaje
    bool publishOnChange;                       // Solo publicar si cambia
    uint32_t publishInterval;                   // Intervalo mínimo (ms)
    const char* description;                    // Descripción
};
```

#### Estados de Conexión

```cpp
enum ModbusMQTTConnectionState {
    MQTT_DISCONNECTED = 0,   // Desconectado
    MQTT_CONNECTING,         // Conectando
    MQTT_CONNECTED,          // Conectado
    MQTT_RECONNECTING,       // Reconectando
    MQTT_ERROR               // Error crítico
};
```

#### Calidad de Servicio (QoS)

```cpp
enum ModbusMQTTQoS {
    MQTT_QOS_0 = 0,  // Como mucho una vez
    MQTT_QOS_1 = 1,  // Al menos una vez
    MQTT_QOS_2 = 2   // Exactamente una vez
};
```

## Ejemplo de Uso Completo

```cpp
#include <ModbusMQTT.h>
#include <ModbusRTU.h>

ModbusRTU modbus;
ModbusMQTT mqttBridge;

void setup() {
    Serial.begin(115200);
    
    // Configurar Modbus
    Serial2.begin(9600, SERIAL_8N1);
    modbus.server(&Serial2, 2);
    modbus.serverId(1);
    modbus.addHreg(0, 100, 10);
    
    // Configurar MQTT
    mqttBridge.setBroker("broker.local", 1883);
    mqttBridge.setClientId("gateway_001");
    mqttBridge.setAutoReconnect(true);
    
    // Topic de temperatura (publicación)
    ModbusMQTTTopicConfig tempPub;
    strcpy(tempPub.topic, "factory/line1/temp");
    tempPub.registerAddress = 0;
    tempPub.registerType = MODBUS_HOLDING_REGISTER;
    tempPub.publishOnChange = true;
    mqttBridge.addPublishTopic(tempPub);
    
    // Topic de setpoint (suscripción)
    ModbusMQTTTopicConfig setpointSub;
    strcpy(setpointSub.topic, "factory/line1/setpoint");
    setpointSub.registerAddress = 5;
    setpointSub.registerType = MODBUS_HOLDING_REGISTER;
    setpointSub.qos = MQTT_QOS_1;
    mqttBridge.addSubscribeTopic(setpointSub);
    
    // Callbacks
    mqttBridge.onMessage([](const char* topic, const uint8_t* payload, 
                            size_t length, void* userData) {
        Serial.printf("MQTT [%s]: %.*s\n", topic, length, payload);
    });
    
    mqttBridge.onStateChange([](ModbusMQTTConnectionState state, void* userData) {
        if (state == MQTT_CONNECTED) {
            Serial.println("Conectado a MQTT");
        }
    });
    
    // Iniciar
    mqttBridge.begin(&modbus);
}

void loop() {
    modbus.task();
    mqttBridge.process();
    delay(50);
}
```

## Mejores Prácticas

### 1. Nomenclatura de Topics

```
✅ Recomendado:
- factory/area1/line1/sensor/temp
- building/floor3/room201/humidity
- device/{device_id}/status

❌ Evitar:
- temp1
- sensor
- data
```

### 2. QoS Adecuado

| Escenario | QoS Recomendado | Razón |
|-----------|-----------------|-------|
| Lectura de sensores | 0 | Pérdida ocasional aceptable |
| Comandos críticos | 1 | Confirmación necesaria |
| Configuración/Setup | 2 | Exactitud requerida |
| Alertas/Alarmas | 1 + Retain | No perder último estado |

### 3. Manejo de Errores

```cpp
void onMqttStateChange(ModbusMQTTConnectionState state, void* userData) {
    switch(state) {
        case MQTT_ERROR:
            // Log error
            Serial.println("Error MQTT - verificando configuración");
            
            // Intentar diagnóstico
            if (!WiFi.isConnected()) {
                WiFi.reconnect();
            }
            break;
            
        case MQTT_RECONNECTING:
            // Backoff exponencial
            static int retryCount = 0;
            delay(min(1000 * pow(2, retryCount), 30000));
            retryCount++;
            break;
            
        case MQTT_CONNECTED:
            retryCount = 0;  // Resetear contador
            break;
    }
}
```

### 4. Optimización de Publicaciones

```cpp
// Configurar umbrales inteligentes
tempConfig.publishOnChange = true;
// Solo publicar si cambio > 0.5°C (valor escalado x10 = 5)
tempConfig.publishInterval = 5000;  // Máximo cada 5 segundos

// Para variables estables
pressureConfig.publishOnChange = false;
pressureConfig.publishInterval = 60000;  // Cada minuto
```

## Seguridad

### 1. Autenticación

```cpp
// Credenciales seguras
mqttBridge.setCredentials("gateway_user", "password_fuerte_123!");

// Usar variables de entorno o almacenamiento seguro
#include <Preferences.h>
Preferences prefs;
prefs.begin("mqtt", true);
String user = prefs.getString("user");
String pass = prefs.getString("pass");
mqttBridge.setCredentials(user.c_str(), pass.c_str());
prefs.end();
```

### 2. TLS/SSL

```cpp
// Puerto seguro
#define MQTT_PORT 8883

// Configurar certificados (ESP32)
#include <WiFiClientSecure.h>
WiFiClientSecure secureClient;
secureClient.setCACert(root_ca_cert);

// Pasar cliente seguro al bridge
mqttBridge.setClient(&secureClient);
```

### 3. Last Will Testament

```cpp
// Configurar LWT antes de connect
mqttBridge.setLastWill(
    "factory/gateway001/status",
    "{\"status\":\"offline\",\"ts\":1234567890}",
    MQTT_QOS_1,
    true  // Retain para que nuevos suscriptores lo vean
);
```

## Troubleshooting

### Problema: No se conecta al broker

**Causas posibles:**
1. Credenciales incorrectas
2. Firewall bloqueando puerto 1883/8883
3. Broker caído o saturado

**Solución:**
```cpp
// Verificar estado
if (mqttBridge.getConnectionState() == MQTT_ERROR) {
    Serial.println("Error de conexión");
    // Verificar logs del broker
}
```

### Problema: Publicaciones no llegan

**Causas posibles:**
1. Topic mal formado
2. QoS incompatible
3. ACL del broker bloqueando

**Solución:**
```cpp
// Habilitar logging
mqttBridge.setLogLevel(MODBUS_LOG_DEBUG);

// Verificar formato de topic
Serial.printf("Publicando en: %s\n", topic);
```

### Problema: Alta latencia

**Causas posibles:**
1. WiFi débil
2. Broker sobrecargado
3. Demasiadas publicaciones

**Solución:**
```cpp
// Optimizar frecuencia
config.publishInterval = 10000;  // Reducir frecuencia
config.publishOnChange = true;   // Solo cambios

// Verificar RSSI
int rssi = WiFi.RSSI();
Serial.printf("RSSI: %d dBm\n", rssi);
if (rssi < -70) {
    // Mejorar ubicación o usar antenna externa
}
```

## Recursos Adicionales

- [Ejemplo Gateway-MQTT-Modbus](../examples/Avanzados/Gateway-MQTT-Modbus/)
- [Documentación MQTT Protocol](http://mqtt.org/documentation)
- [Mosquitto Broker](https://mosquitto.org/)

## Soporte

Para issues o preguntas, abrir un ticket en GitHub con:
- Versión de la biblioteca
- Plataforma (ESP32, ESP8266, etc.)
- Logs completos
- Código mínimo reproducible

---

*Documentación actualizada: 2024*
*Versión: 1.0.0*
