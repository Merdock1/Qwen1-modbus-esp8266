# Gateway MQTT-Modbus

## Descripción

Este ejemplo implementa un **gateway IoT bidireccional** que conecta dispositivos Modbus con sistemas MQTT para aplicaciones de Industria 4.0 y monitoreo remoto.

## Características Principales

- ✅ **Publicación automática** de valores de sensores Modbus a topics MQTT
- ✅ **Suscripción a comandos** MQTT para controlar registros Modbus
- ✅ **Reconexión automática** ante fallos de WiFi o broker MQTT
- ✅ **Detección de cambios** para publicar solo cuando los valores varían significativamente
- ✅ **Estadísticas en tiempo real** del funcionamiento del gateway
- ✅ **Comandos remotos** (STATUS, RESET) vía MQTT
- ✅ **Soporte para múltiples formatos** de payload (JSON, plain text)

## Hardware Requerido

| Componente | Cantidad | Notas |
|------------|----------|-------|
| ESP8266 o ESP32 | 1 | Recomendado ESP32 por dual-core |
| Módulo RS485 MAX485 | 1 | Para comunicación Modbus RTU |
| Sensores (opcional) | - | DHT22, DS18B20, etc. |

## Conexiones

### Módulo RS485 MAX485

```
MAX485          ESP32
------          -----
VCC    ----->   5V o 3.3V
GND    ----->   GND
RO     ----->   GPIO 16 (RX2)
DI     ----->   GPIO 17 (TX2)
DE+RE  ----->   GPIO 2
A      ----->   Bus RS485 A/+
B      ----->   Bus RS485 B/-
```

### Esquema de Conexión

```
                    +------------------+
                    |     ESP32        |
                    |                  |
     RS485 Bus <----+ A  B             |
                    |     MAX485       |
                    |                  |
              RO --->+ DI         DE+RE+---> GPIO 2
                     |                  |
                     +------------------+
```

## Configuración

### 1. Credenciales WiFi

Editar las siguientes constantes en el código:

```cpp
#define STASSID "tu_red_wifi"
#define STAPSK "tu_password_wifi"
```

### 2. Configuración MQTT

```cpp
#define MQTT_BROKER "test.mosquitto.org"  // Cambiar a tu broker
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "modbus_gateway_001"
#define MQTT_USER ""  // Opcional
#define MQTT_PASS ""  // Opcional
```

### 3. Configuración Modbus

```cpp
#define MODBUS_SERIAL Serial2    // UART para RS485
#define MODBUS_DE_PIN 2          // Pin de control
#define MODBUS_BAUDRATE 9600     // Velocidad de comunicación
#define MODBUS_SERVER_ID 1       // ID del esclavo Modbus
```

## Topics MQTT

### Publicación (Gateway → Broker)

| Topic | Descripción | Frecuencia |
|-------|-------------|------------|
| `modbus/gateway001/sensor/temperature` | Temperatura actual | Cuando cambia > 0.5°C |
| `modbus/gateway001/sensor/humidity` | Humedad relativa | Cada 10s o cambio > 1% |
| `modbus/gateway001/status` | Estado del gateway | En eventos |
| `modbus/gateway001/stats` | Estadísticas | Cada 60s |

### Suscripción (Broker → Gateway)

| Topic | Payload | Acción |
|-------|---------|--------|
| `modbus/gateway001/control/setpoint` | Número entero (x10) | Actualiza setpoint |
| `modbus/gateway001/command` | "STATUS" o "RESET" | Consulta estado o reinicia |

## Comandos de Prueba

### Usando mosquitto_cli

```bash
# Suscribirse a temperatura
mosquitto_sub -h test.mosquitto.org -t "modbus/gateway001/sensor/temperature" -v

# Suscribirse a humedad
mosquitto_sub -h test.mosquitto.org -t "modbus/gateway001/sensor/humidity" -v

# Cambiar setpoint a 25.0°C
mosquitto_pub -h test.mosquitto.org -t "modbus/gateway001/control/setpoint" -m "250"

# Solicitar estado
mosquitto_pub -h test.mosquitto.org -t "modbus/gateway001/command" -m "STATUS"

# Reiniciar gateway (¡cuidado!)
mosquitto_pub -h test.mosquitto.org -t "modbus/gateway001/command" -m "RESET"
```

### Usando Node-RED

```json
[
  {
    "id": "mqtt-sub-temp",
    "type": "mqtt in",
    "name": "Temperatura",
    "topic": "modbus/gateway001/sensor/temperature",
    "broker": "mqtt-broker-node"
  },
  {
    "id": "mqtt-pub-setpoint",
    "type": "mqtt out",
    "name": "Setpoint",
    "topic": "modbus/gateway001/control/setpoint",
    "broker": "mqtt-broker-node"
  }
]
```

## Formato de Mensajes

### Temperatura/Humedad

```json
{"value": 255}  // Representa 25.5°C o 25.5%
```

### Estado

```json
{
  "status": "connected",
  "uptime": 3600,
  "modbus_requests": 150,
  "mqtt_messages": 45
}
```

### Estadísticas

```json
{
  "uptime": 3600,
  "modbus_req": 150,
  "mqtt_msg": 45,
  "errors": 2
}
```

## Implementación en Producción

### 1. Seguridad MQTT

```cpp
// Usar autenticación
#define MQTT_USER "gateway_user"
#define MQTT_PASS "password_seguro_generado"

// Usar TLS/SSL
#define MQTT_PORT 8883  // Puerto seguro
// Configurar certificados en mqttBridge.begin()
```

### 2. Last Will Testament

```cpp
// Configurar mensaje de despedida
mqttBridge.setLastWill(
    "modbus/gateway001/status",
    "{\"status\":\"offline\",\"reason\":\"unexpected_disconnect\"}",
    MQTT_QOS_1,
    true  // Retain
);
```

### 3. Watchdog Hardware

```cpp
#include <esp_task_wdt.h>

void setup() {
    // Configurar watchdog de 5 segundos
    esp_task_wdt_init(5, true);
    
    // Alimentar watchdog periódicamente
    esp_task_wdt_reset();
}

void loop() {
    // ... código principal ...
    
    // Alimentar watchdog
    esp_task_wdt_reset();
}
```

### 4. Logging Remoto

```cpp
// Enviar logs a servidor syslog
void sendLog(const char* level, const char* message) {
    char logMsg[256];
    snprintf(logMsg, sizeof(logMsg), 
             "{\"ts\":%lu,\"level\":\"%s\",\"msg\":\"%s\"}",
             millis()/1000, level, message);
    
    mqttBridge.publishMessage(
        MQTT_BASE_TOPIC "/log",
        (uint8_t*)logMsg,
        strlen(logMsg)
    );
}
```

## Solución de Problemas

### El gateway no se conecta a WiFi

1. Verificar credenciales WiFi
2. Comprobar señal WiFi (RSSI > -70dBm)
3. Reiniciar el ESP32
4. Verificar que el router no tenga MAC filtering

### No hay comunicación Modbus

1. Verificar conexiones RS485 (A/B no invertidos)
2. Confirmar baudrate coincide con dispositivo esclavo
3. Verificar ID de esclavo correcto
4. Usar analizador lógico para debug

### MQTT se desconecta frecuentemente

1. Verificar calidad de conexión WiFi
2. Reducir intervalo de keep-alive
3. Verificar capacidad del broker
4. Implementar backoff exponencial en reconexión

### Los valores no se publican

1. Verificar umbrales de cambio (threshold)
2. Comprobar que los sensores están leyendo correctamente
3. Revisar logs seriales para errores

## Métricas de Rendimiento

| Parámetro | Valor Típico |
|-----------|--------------|
| Latencia Modbus→MQTT | < 100ms |
| Consumo WiFi activo | ~80mA |
| Consumo WiFi sleep | ~20μA |
| Memoria RAM usada | ~45KB |
| Memoria FLASH usada | ~650KB |

## Extensiones Posibles

1. **Soporte Multi-drop**: Conectar múltiples dispositivos Modbus
2. **Data Logging**: Guardar históricos en SPIFFS/LittleFS
3. **OTA Updates**: Actualización remota de firmware
4. **Web Interface**: Configuración vía navegador
5. **Protocolos Adicionales**: BACnet, OPC-UA, etc.

## Licencia

LGPL-2.1 - Ver LICENSE.txt para detalles.

## Autor

Equipo de Desarrollo Modbus - 2024

## Referencias

- [Especificación Modbus](https://modbus.org/specs.php)
- [MQTT Protocol v3.1.1](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
- [ESP32 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
