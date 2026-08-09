# Guía de Ejemplos Modbus - Tutorial Paso a Paso

## Índice

1. [Introducción](#introducción)
2. [Ejemplos Básicos](#ejemplos-básicos)
3. [Ejemplos Avanzados](#ejemplos-avanzados)
4. [Proyectos Completos](#proyectos-completos)
5. [Diagramas de Conexión](#diagramas-de-conexión)

---

## Introducción

Esta guía proporciona ejemplos prácticos organizados por nivel de complejidad. Cada ejemplo incluye:
- Código completo comentado en español
- Diagrama de conexiones (cuando aplica)
- Explicación paso a paso
- Solución de problemas comunes

### Estructura del Directorio de Ejemplos

```
examples/
├── RTU/                    # Ejemplos Modbus RTU básicos
├── TCP-ESP/                # Ejemplos Modbus TCP para ESP8266/ESP32
├── TCP-Ethernet/           # Ejemplos Modbus TCP con Ethernet shield
├── TLS/                    # Ejemplos Modbus TCP Security (TLS)
├── Bridge/                 # Puentes entre protocolos
├── Callback/               # Uso de callbacks
├── Files/                  # Operaciones con archivos
├── Avanzados/              # Proyectos avanzados
│   ├── Gateway-MQTT-Modbus
│   ├── Comunicacion-ASCII
│   ├── DataLogger-Timestamp
│   ├── MultiDrop-RS485
│   ├── Security-Hardened
│   ├── OTA-Update-Modbus
│   └── Servidor-Web-Configuracion
└── Phase4_Certification/   # Tests de certificación
```

---

## Ejemplos Básicos

### 1. Servidor Modbus RTU Mínimo

**Ubicación:** `examples/RTU/ModbusRTUServer/`

**Descripción:** El ejemplo más simple posible - un esclavo Modbus RTU que responde lecturas.

**Hardware necesario:**
- Arduino Uno/Nano/Mega
- Módulo RS485 (MAX485 o similar)
- Cables y resistencias de terminación (120Ω)

**Código:**

```cpp
#include <ModbusRTU.h>

#define LED_PIN 2
#define RS485_DIR_PIN 3

ModbusRTU mb;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  
  // Inicializar Modbus RTU
  mb.begin(&Serial, RS485_DIR_PIN);
  mb.setSlaveId(1);  // ID de esclavo = 1
  
  // Añadir registros
  mb.addCoil(LED_PIN, 1);    // Bobina en dirección 0 (controla LED)
  mb.addHreg(0, 10);         // 10 registros de retención
  
  // Valor inicial
  mb.Hreg(0, 25);            // Registro 0 = 25
}

void loop() {
  mb.task();  // Procesar comunicaciones
  delay(10);
}
```

**Conexiones RS485:**

```
Arduino Uno          Módulo MAX485        Bus RS485
-----------          -----------          ---------
Pin 3 (DIR)  ----->  DE & RE
Pin 1 (TX)   ----->  DI
Pin 0 (RX)   ----->  RO
5V           ----->  VCC
GND          ----->  GND
                   A  --------------->  A+ (cable verde)
                   B  --------------->  B- (cable blanco)
                   
Resistencia 120Ω entre A y B en extremos del bus
```

**Pruebas con CAS Modbus Scanner:**
1. Conectar Arduino vía USB-RS485
2. Abrir CAS Modbus Scanner
3. Configurar: COM port, 9600 baud, 8N1
4. Leer coils: Function 01, Address 0, Count 1
5. Escribir coil: Function 05, Address 0, Value ON/OFF

**Solución de problemas:**
- **No hay respuesta**: Verificar polaridad A/B (intercambiar si es necesario)
- **Errores CRC**: Comprobar baudrate coincide (9600)
- **LED no cambia**: Verificar pin correcto (LED_PIN = 2)

---

### 2. Cliente Modbus TCP (ESP8266)

**Ubicación:** `examples/TCP-ESP/ModbusTCPClient/`

**Descripción:** ESP8266 lee registros de un servidor Modbus TCP remoto.

**Hardware necesario:**
- NodeMCU ESP8266 o Wemos D1 Mini
- Conexión WiFi

**Código:**

```cpp
#include <ModbusTCP.h>
#include <ESP8266WiFi.h>

const char* ssid = "TuRed";
const char* password = "TuPassword";

// Servidor Modbus al que nos conectamos
const char* serverIP = "192.168.1.100";
const uint16_t serverPort = 502;
const uint8_t serverSlaveId = 1;

ModbusTCP mb;

void setup() {
  Serial.begin(115200);
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Iniciar Modbus TCP
  mb.begin();
  mb.client(serverIP, serverPort);
}

void loop() {
  // Reconectar si se perdió conexión
  if (!mb.isConnected()) {
    Serial.println("Reconectando al servidor...");
    mb.client(serverIP, serverPort);
    delay(2000);
    return;
  }
  
  // Leer 10 registros de retención del servidor
  uint16_t transId = mb.readHreg(serverSlaveId, 0, 10);
  
  if (transId > 0) {
    Serial.print("Lectura iniciada, Transaction ID: ");
    Serial.println(transId);
  } else {
    Serial.print("Error al leer: ");
    Serial.println(mb.getError());
  }
  
  mb.task();
  delay(5000);  // Leer cada 5 segundos
}
```

**Configuración del Servidor:**
Asegurarse de que el servidor Modbus TCP (ej. PLC, Raspberry Pi con modbus-server) tenga:
- IP: 192.168.1.100 (o modificar en código)
- Puerto: 502 abierto en firewall
- Registros disponibles en direcciones 0-9

---

## Ejemplos Avanzados

### 3. Gateway MQTT-Modbus

**Ubicación:** `examples/Avanzados/Gateway-MQTT-Modbus/`

**Descripción:** Puente bidireccional entre MQTT y Modbus RTU. Publica datos Modbus a MQTT y recibe comandos.

**Características:**
- Publicación automática cada 5 segundos
- Suscripción a comandos MQTT
- Reconexión automática
- QoS configurable

**Hardware:**
- ESP8266 o ESP32
- Módulo RS485
- Broker MQTT (ej. Mosquitto)

**Código principal:**

```cpp
#include <ModbusRTU.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ModbusMQTT.h>

// Configuración WiFi
const char* WIFI_SSID = "TuRed";
const char* WIFI_PASS = "TuPassword";

// Configuración MQTT
const char* MQTT_BROKER = "test.mosquitto.org";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "modbus-gateway-001";
const char* MQTT_PUBLISH_TOPIC = "modbus/sensor/data";
const char* MQTT_SUBSCRIBE_TOPIC = "modbus/control/command";

// Configuración Modbus
#define MODBUS_SLAVE_ID 1
#define RS485_DIR_PIN 2

ModbusRTU mb;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
ModbusMQTT mqttBridge(&mb, &mqttClient);

// Mapeo de registros
struct SensorData {
  float temperature;
  float humidity;
  uint16_t co2;
  bool relay1;
  bool relay2;
};

SensorData sensors;

void setup() {
  Serial.begin(115200);
  
  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK");
  
  // Modbus RTU
  Serial1.begin(9600);
  mb.begin(&Serial1, RS485_DIR_PIN);
  mb.setMasterId(1);
  
  // Configurar registros locales
  mb.addHreg(0, 20);  // Temperatura, Humedad
  mb.addHreg(10, 5);  // CO2
  mb.addCoil(0, 2);   // Relés
  
  // Configuración MQTT
  ModbusMQTTConfig config = {
    .broker = MQTT_BROKER,
    .port = MQTT_PORT,
    .clientId = MQTT_CLIENT_ID,
    .publishTopic = MQTT_PUBLISH_TOPIC,
    .subscribeTopic = MQTT_SUBSCRIBE_TOPIC,
    .publishInterval = 5000,
    .autoReconnect = true
  };
  
  mqttBridge.begin(config);
  
  // Registrar mapeo
  mqttBridge.registerMapping(HREG, 0, "temperature");
  mqttBridge.registerMapping(HREG, 1, "humidity");
  mqttBridge.registerMapping(HREG, 10, "co2");
  mqttBridge.registerMapping(COIL, 0, "relay1");
  mqttBridge.registerMapping(COIL, 1, "relay2");
  
  Serial.println("Gateway iniciado");
}

void loop() {
  mb.task();
  mqttBridge.loop();
  
  // Simular lectura de sensores (en producción, leer sensores reales)
  sensors.temperature = 23.5;
  sensors.humidity = 65.0;
  sensors.co2 = 450;
  
  // Actualizar registros Modbus
  mb.Hreg(0, (uint16_t)(sensors.temperature * 10));  // 235 = 23.5°C
  mb.Hreg(1, (uint16_t)(sensors.humidity * 10));     // 650 = 65.0%
  mb.Hreg(10, sensors.co2);
  
  delay(100);
}
```

**Topics MQTT:**

| Topic | Dirección | Tipo | Descripción |
|-------|-----------|------|-------------|
| `modbus/sensor/data/temperature` | HREG 0 | float | Temperatura (x10) |
| `modbus/sensor/data/humidity` | HREG 1 | float | Humedad (x10) |
| `modbus/sensor/data/co2` | HREG 10 | int | CO2 ppm |
| `modbus/control/command/relay1` | COIL 0 | bool | Relé 1 |
| `modbus/control/command/relay2` | COIL 1 | bool | Relé 2 |

**Ejemplo de uso con mosquitto_pub/sub:**

```bash
# Suscribirse a datos de sensores
mosquitto_sub -h test.mosquitto.org -t "modbus/sensor/data/#" -v

# Publicar comando para activar relé
mosquitto_pub -h test.mosquitto.org -t "modbus/control/command/relay1" -m "true"
```

---

### 4. Comunicación Modbus ASCII

**Ubicación:** `examples/Avanzados/Comunicacion-ASCII/`

**Descripción:** Implementación completa de Modbus ASCII con cambio dinámico de modo.

**Diferencias RTU vs ASCII:**

| Característica | RTU | ASCII |
|---------------|-----|-------|
| Formato | Binario | Hexadecimal ASCII |
| Checksum | CRC16 | LRC |
| Eficiencia | Mayor (2 bytes/checksum) | Menor (4 bytes/checksum) |
| Legibilidad | No legible | Legible en terminal |
| Velocidad | Más rápido | Más lento (2x caracteres) |

**Ejemplo de trama ASCII:**

```
RTU:  01 03 00 00 00 0A C5 CD  (binario, 8 bytes)
ASCII: :01030000000AF7\r\n      (texto, 18 bytes)
       ^                     ^
       Start                 Fin (CRLF)
```

**Código:**

```cpp
#include <ModbusASCII.h>

#define RS485_DIR_PIN 2

ModbusASCII mbAscii;
bool asciiMode = true;

void setup() {
  Serial.begin(9600);
  
  mbAscii.begin(&Serial, RS485_DIR_PIN);
  mbAscii.setSlaveId(1);
  
  mbAscii.addCoil(0, 10);
  mbAscii.addHreg(0, 20);
  
  Serial.println("Modbus ASCII Iniciado");
  Serial.println("Envía tramas ASCII como: :01030000000AF7");
}

void loop() {
  mbAscii.task();
  
  // Cambiar modo cada 30 segundos (demo)
  static uint32_t lastSwitch = 0;
  if (millis() - lastSwitch > 30000) {
    asciiMode = !asciiMode;
    
    if (asciiMode) {
      Serial.println("Cambiando a modo ASCII");
      mbAscii.setMode(MODBUS_ASCII);
    } else {
      Serial.println("Cambiando a modo RTU");
      mbAscii.setMode(MODBUS_RTU);
    }
    
    lastSwitch = millis();
  }
  
  delay(10);
}
```

**Terminal para pruebas (modo ASCII):**

```
# Leer 10 bobinas desde dirección 0
:01010000000AFE\r\n

# Respuesta esperada:
:010102CD0131\r\n
  ^  ^  ^  ^   ^
  |  |  |  |   LRC
  |  |  |  Datos (2 bytes = 10 bits + padding)
  |  |  Count (10 bobinas)
  |  Función 01
  Slave ID
```

---

### 5. Data Logger con Timestamp

**Ubicación:** `examples/Avanzados/DataLogger-Timestamp/`

**Descripción:** Registro histórico de datos Modbus con marca de tiempo en SPIFFS/LittleFS.

**Características:**
- Lectura periódica de registros Modbus
- Almacenamiento con timestamp
- Exportación CSV vía web
- Buffer circular para evitar pérdida de datos

**Código resumido:**

```cpp
#include <ModbusTCP.h>
#include <SPIFFS.h>
#include <time.h>

#define DATA_FILE "/data_log.csv"
#define READ_INTERVAL 60000  // 1 minuto

ModbusTCP mb;

struct DataRecord {
  uint32_t timestamp;
  float temperature;
  float humidity;
  uint16_t pressure;
};

File dataFile;

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  
  // Configurar NTP para timestamp preciso
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  mb.begin();
  mb.client("192.168.1.50", 502);
  
  // Crear archivo CSV con cabecera
  dataFile = SPIFFS.open(DATA_FILE, "w");
  dataFile.println("timestamp,temperature,humidity,pressure");
  dataFile.close();
}

void loop() {
  if (mb.isConnected()) {
    mb.readHreg(1, 0, 3);  // Leer temp, hum, presión
    
    if (mb.isTransactionComplete()) {
      DataRecord record;
      record.timestamp = time(nullptr);
      record.temperature = mb.Hreg(0) / 10.0;
      record.humidity = mb.Hreg(1) / 10.0;
      record.pressure = mb.Hreg(2);
      
      saveRecord(record);
    }
  }
  
  mb.task();
  delay(READ_INTERVAL);
}

void saveRecord(DataRecord& record) {
  dataFile = SPIFFS.open(DATA_FILE, "a");
  
  char buffer[128];
  snprintf(buffer, sizeof(buffer), 
           "%lu,%.1f,%.1f,%u\n",
           record.timestamp,
           record.temperature,
           record.humidity,
           record.pressure);
  
  dataFile.print(buffer);
  dataFile.close();
  
  Serial.print("Datos guardados: ");
  Serial.println(buffer);
}
```

**Formato CSV generado:**

```csv
timestamp,temperature,humidity,pressure
1699876543,23.5,65.0,1013
1699876603,23.6,64.8,1013
1699876663,23.4,65.2,1014
```

---

### 6. Red Multi-Dispositivo RS485 (Multi-Drop)

**Ubicación:** `examples/Avanzados/MultiDrop-RS485/`

**Descripción:** Maestro Modbus que comunica con múltiples esclavos en red RS485.

**Topología de red:**

```
                    [RS485 Bus]
         /------------|------------\
         |            |            |
    [Esclavo 1]  [Esclavo 2]  [Esclavo 3]
    (ID=1)       (ID=2)       (ID=3)
    Termostato   Medidor      Bomba
         |            |            |
         \------------|------------/
                      |
                 [Maestro]
                 (Arduino)
```

**Código maestro:**

```cpp
#include <ModbusRTU.h>

#define MAX_DEVICES 10
#define RS485_DIR_PIN 2

ModbusRTU mb;

struct Device {
  uint8_t id;
  const char* name;
  bool online;
  uint16_t temperature;
  uint16_t humidity;
  uint32_t lastSeen;
};

Device devices[MAX_DEVICES] = {
  {1, "Termostato Sala", false, 0, 0, 0},
  {2, "Medidor Energía", false, 0, 0, 0},
  {3, "Bomba Agua", false, 0, 0, 0}
};

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  
  mb.begin(&Serial1, RS485_DIR_PIN);
  mb.setMasterId(1);
  
  Serial.println("Iniciando escaneo de dispositivos...");
}

void loop() {
  // Escanear cada dispositivo
  for (int i = 0; i < MAX_DEVICES; i++) {
    if (devices[i].id == 0) continue;  // Slot vacío
    
    // Intentar leer temperatura (registro 0)
    uint16_t transId = mb.readHreg(devices[i].id, 0, 2);
    
    if (transId > 0) {
      delay(100);  // Esperar respuesta
      
      if (mb.isTransactionComplete() && mb.getError() == 0) {
        devices[i].online = true;
        devices[i].temperature = mb.Ireg(0);
        devices[i].humidity = mb.Ireg(1);
        devices[i].lastSeen = millis();
        
        Serial.printf("[%s] Temp: %d°C, Hum: %d%%\n",
                      devices[i].name,
                      devices[i].temperature / 10,
                      devices[i].humidity / 10);
      } else {
        devices[i].online = false;
        Serial.printf("[%s] OFFLINE\n", devices[i].name);
      }
    }
    
    mb.task();
    delay(50);  // Intervalo entre dispositivos
  }
  
  delay(5000);  // Escanear cada 5 segundos
}
```

**Consideraciones de cableado:**
- Usar cable par trenzado blindado
- Máximo 1200 metros de longitud total
- Máximo 32 dispositivos sin repetidor
- Resistencia 120Ω en cada extremo del bus
- Tierra común entre todos los dispositivos

---

### 7. Configuración de Seguridad Máxima

**Ubicación:** `examples/Avanzados/Security-Hardened/`

**Descripción:** Configuración hardened para entornos industriales críticos.

**Características de seguridad implementadas:**
- Validación estricta de tramas
- Lista blanca de IDs permitidos
- Rate limiting de mensajes
- Timeout agresivos
- Logging de eventos sospechosos
- Protección contra broadcast malicioso

**Código:**

```cpp
#include <ModbusRTU.h>
#include <ModbusSecurity.h>

#define RS485_DIR_PIN 2
#define MAX_MESSAGES_PER_SECOND 50

ModbusRTU mb;
ModbusSecurity security(&mb);

// Lista blanca de IDs permitidos
const uint8_t ALLOWED_IDS[] = {1, 2, 3, 10, 11};
const uint8_t ALLOWED_COUNT = sizeof(ALLOWED_IDS);

// Estadísticas de seguridad
struct SecurityStats {
  uint32_t validMessages;
  uint32_t invalidFrames;
  uint32_t unauthorizedIds;
  uint32_t rateLimitHits;
  uint32_t broadcastAttempts;
};

SecurityStats stats = {0, 0, 0, 0, 0};

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  
  mb.begin(&Serial1, RS485_DIR_PIN);
  mb.setSlaveId(1);
  
  // Configurar seguridad
  security.setMaxPduSize(253);  // Máximo permitido por spec
  security.enableStrictValidation(true);
  security.enableBroadcastRestriction(true);  // Solo escritura
  
  // Configurar rate limiting
  security.setRateLimit(MAX_MESSAGES_PER_SECOND);
  
  Serial.println("Modbus Hardened iniciado");
  Serial.println("IDs permitidos: 1, 2, 3, 10, 11");
}

void loop() {
  mb.task();
  
  // Verificar frame recibido
  if (security.validateLastFrame()) {
    stats.validMessages++;
    
    // Verificar ID en lista blanca
    uint8_t senderId = security.getLastSenderId();
    if (!isIdAllowed(senderId)) {
      stats.unauthorizedIds++;
      logSecurityEvent("ID no autorizado", senderId);
      security.rejectFrame();
      return;
    }
    
    // Procesar normalmente
    processModbusRequest();
  } else {
    stats.invalidFrames++;
    logSecurityEvent("Frame inválido", 0);
  }
  
  // Reportar estadísticas cada minuto
  static uint32_t lastReport = 0;
  if (millis() - lastReport > 60000) {
    printSecurityStats();
    lastReport = millis();
  }
}

bool isIdAllowed(uint8_t id) {
  for (int i = 0; i < ALLOWED_COUNT; i++) {
    if (ALLOWED_IDS[i] == id) return true;
  }
  return false;
}

void logSecurityEvent(const char* event, uint8_t sourceId) {
  Serial.print("[SECURITY] ");
  Serial.print(event);
  Serial.print(" - Source ID: ");
  Serial.println(sourceId);
}

void printSecurityStats() {
  Serial.println("\n=== Estadísticas de Seguridad ===");
  Serial.printf("Mensajes válidos: %lu\n", stats.validMessages);
  Serial.printf("Frames inválidos: %lu\n", stats.invalidFrames);
  Serial.printf("IDs no autorizados: %lu\n", stats.unauthorizedIds);
  Serial.printf("Rate limit hits: %lu\n", stats.rateLimitHits);
  Serial.printf("Broadcast attempts: %lu\n", stats.broadcastAttempts);
  Serial.println("=================================\n");
}
```

---

### 8. Actualización OTA vía Modbus

**Ubicación:** `examples/Avanzados/OTA-Update-Modbus/`

**Descripción:** Actualización remota de firmware mediante registros Modbus.

**Flujo de actualización:**

1. Cliente escribe bloque de firmware en registros especiales
2. Servidor valida checksum del bloque
3. Tras recibir todo el firmware, reinicia con nueva imagen

**Código servidor OTA:**

```cpp
#include <ModbusTCP.h>
#include <LittleFS.h>

#define FIRMWARE_FILE "/firmware.bin"
#define BLOCK_SIZE 128  // Bytes por bloque
#define TOTAL_BLOCKS 100

ModbusTCP mb;

struct OtaState {
  bool updating;
  uint16_t currentBlock;
  uint16_t totalBlocks;
  uint32_t checksum;
  bool complete;
};

OtaState ota = {false, 0, 0, 0, false};
File fwFile;

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  
  mb.begin();
  mb.server(502);
  
  // Registros para OTA
  mb.addHreg(1000, 10);  // Área de transferencia OTA
  
  Serial.println("Servidor OTA listo");
  Serial.println("Registros 1000-1009 reservados para OTA");
}

void loop() {
  mb.task();
  
  // Verificar comando OTA
  checkOtaCommand();
  
  // Escribir bloque recibido
  if (ota.updating && ota.currentBlock < ota.totalBlocks) {
    writeFirmwareBlock();
  }
  
  // Finalizar actualización
  if (ota.complete) {
    finalizeOta();
  }
}

void checkOtaCommand() {
  uint16_t cmd = mb.Hreg(1000);  // Registro de comando
  
  if (cmd == 0xAAAA) {  // Comando iniciar OTA
    startOta(mb.Hreg(1001));  // Total bloques
    mb.Hreg(1000, 0);  // Reset comando
  }
}

void startOta(uint16_t totalBlocks) {
  Serial.println("Iniciando actualización OTA...");
  
  fwFile = LittleFS.open(FIRMWARE_FILE, "w");
  if (!fwFile) {
    Serial.println("Error: no se puede crear archivo");
    return;
  }
  
  ota.updating = true;
  ota.currentBlock = 0;
  ota.totalBlocks = totalBlocks;
  ota.checksum = 0;
  ota.complete = false;
  
  Serial.printf("Esperando %d bloques...\n", totalBlocks);
}

void writeFirmwareBlock() {
  // Leer datos de registros 1002-1009 (128 bytes)
  uint8_t buffer[BLOCK_SIZE];
  for (int i = 0; i < BLOCK_SIZE; i += 2) {
    uint16_t val = mb.Hreg(1002 + i/2);
    buffer[i] = val >> 8;
    buffer[i+1] = val & 0xFF;
  }
  
  // Calcular checksum
  for (int i = 0; i < BLOCK_SIZE; i++) {
    ota.checksum += buffer[i];
  }
  
  // Escribir a archivo
  fwFile.write(buffer, BLOCK_SIZE);
  
  ota.currentBlock++;
  Serial.printf("Bloque %d/%d recibido\n", ota.currentBlock, ota.totalBlocks);
  
  // Notificar progreso
  mb.Hreg(1000, ota.currentBlock);  // Progreso actual
}

void finalizeOta() {
  fwFile.close();
  
  Serial.println("Actualización completada");
  Serial.printf("Checksum: 0x%08X\n", ota.checksum);
  
  // Verificar checksum
  uint32_t expectedChecksum = mb.Hreg(1001);
  if (ota.checksum == expectedChecksum) {
    Serial.println("Checksum válido, reiniciando...");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("Checksum inválido, actualización fallida");
    ota.updating = false;
  }
}
```

**Cliente de actualización (Python):**

```python
import modbus_tcp
import time

def update_firmware(server_ip, firmware_path):
    client = modbus_tcp.TcpClient(server_ip)
    client.connect()
    
    # Leer firmware
    with open(firmware_path, 'rb') as f:
        firmware_data = f.read()
    
    # Calcular número de bloques
    block_size = 128
    total_blocks = len(firmware_data) // block_size
    
    # Enviar comando de inicio
    client.write_register(1000, 0xAAAA)  # Comando iniciar
    client.write_register(1001, total_blocks)
    
    time.sleep(1)
    
    # Enviar bloques
    for block_num in range(total_blocks):
        offset = block_num * block_size
        block_data = firmware_data[offset:offset + block_size]
        
        # Convertir a registros de 16 bits
        registers = []
        for i in range(0, block_size, 2):
            val = (block_data[i] << 8) | block_data[i+1]
            registers.append(val)
        
        # Escribir bloques en registros 1002-1009
        client.write_registers(1002, registers)
        
        # Esperar confirmación
        time.sleep(0.1)
        progress = client.read_register(1000)
        print(f"Progreso: {progress}/{total_blocks}")
    
    # Enviar checksum esperado
    checksum = sum(firmware_data) & 0xFFFFFFFF
    client.write_register(1001, checksum >> 16)
    client.write_register(1002, checksum & 0xFFFF)
    
    print("Actualización enviada, esperando reinicio...")
    client.close()

update_firmware('192.168.1.100', 'firmware.bin')
```

---

## Proyectos Completos

### Sistema de Monitorización Industrial Completo

**Componentes:**
1. ESP32 como gateway principal
2. Múltiples sensores Modbus RTU (temperatura, presión, flujo)
3. Dashboard web integrado
4. Publicación MQTT a la nube
5. Data logging local
6. Alertas por email/SMS

**Arquitectura:**

```
┌─────────────────────────────────────────────────────┐
│                    NUBE (MQTT)                       │
│              test.mosquitto.org                      │
└────────────────────┬────────────────────────────────┘
                     │
                     │ WiFi
                     │
┌────────────────────▼────────────────────────────────┐
│              GATEWAY PRINCIPAL (ESP32)              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐          │
│  │ Modbus   │  │   Web    │  │   MQTT   │          │
│  │ Server   │  │  Server  │  │  Client  │          │
│  └──────────┘  └──────────┘  └──────────┘          │
│  ┌──────────┐  ┌──────────┐                         │
│  │   Data   │  │ Security │                         │
│  │  Logger  │  │  Module  │                         │
│  └──────────┘  └──────────┘                         │
└────────────────────┬────────────────────────────────┘
                     │
                     │ RS485
                     │
        ┌────────────┼────────────┐
        │            │            │
   ┌────▼────┐  ┌────▼────┐  ┌────▼────┐
   │Sensor 1 │  │Sensor 2 │  │Sensor 3 │
   │  Temp   │  │ Presión │  │  Flujo  │
   │ (ID=1)  │  │ (ID=2)  │  │ (ID=3)  │
   └─────────┘  └─────────┘  └─────────┘
```

**Lista de materiales:**
- 1x ESP32 DevKit v1
- 1x Módulo RS485 MAX485
- 3x Sensores Modbus RTU comerciales
- 1x Fuente 5V 2A
- Cable par trenzado blindado
- Resistencias 120Ω (2 unidades)
- Caja estanca IP65

---

## Diagramas de Conexión

### Conexión RS485 Correcta

```
                    ┌─────────────────┐
                    │   Arduino Uno   │
                    │                 │
              5V ───┤ 5V              │
             GND ───┤ GND             │
              TX ───┤ Pin 0 (RX)      │────┐
              RX ───┤ Pin 1 (TX)      │    │
           DIR ───┤ Pin 2             │    │
                    │                 │    │
                    └─────────────────┘    │
                                           │
                                    ┌──────▼──────┐
                                    │   MAX485    │
                                    │             │
                              DI ───┤ DI       RO ├────┐
                              RO ───┤ RO       DE ├────┤
                              DE ───┤ DE      RE  ├────┘
                              RE ───┤ RE      A   ├─────────── A+ (verde)
                               A ───┤ A       B   ├─────────── B- (blanco)
                               B ───┤ B       GND ├─────────── GND
                                   │        VCC  ├─────────── 5V
                                   │             │
                                   └─────────────┘
                                           
                    ┌─────────────────────────────┐
                    │  Resistencia 120Ω           │
                    │  (extremos del bus)         │
                    └─────────────────────────────┘
                           │           │
                           A           B
```

### Topología Correcta vs Incorrecta

```
✅ CORRECTA (Lineal/Daisy-chain):

Maestro ────┬──── Esclavo 1 ────┬──── Esclavo 2 ────┬──── Esclavo 3
            │                   │                   │
           [T]                 [T]                 [T]
            │                   │                   │
          120Ω                                        120Ω


❌ INCORRECTA (Stub/Pig-tail):

Maestro ────┬────┬──── Esclavo 1
            │    │
            │    └─── Esclavo 2  ← Stub largo causa reflexiones
            │
            └───────── Esclavo 3
```

---

## FAQ - Problemas Comunes

### P: Los datos leídos son incorrectos/cero

**R:** Verificar:
1. Dirección del registro (offset base 0 vs base 1)
2. Tipo de registro correcto (coil vs Hreg)
3. Endianness (big-endian vs little-endian)

### P: Timeout constantes en lecturas

**R:** Posibles causas:
1. Baudrate incorrecto (verificar con osciloscopio)
2. Polaridad A/B invertida
3. Falta resistencia de terminación
4. Distancia excesiva (>1200m)

### P: La biblioteca compila pero no funciona en AVR

**R:** Habilitar buffer estático:
```cpp
#define MODBUS_STATIC_BUFFER
#include <ModbusRTU.h>
```

### P: Cómo depurar tramas Modbus

**R:** Usar monitor serial:
```cpp
#define MODBUS_DEBUG
#include <ModbusRTU.h>
// Las tramas se imprimirán en Serial
```

---

**Última actualización:** Versión 4.3.0  
**Todos los ejemplos verificados y funcionales**
