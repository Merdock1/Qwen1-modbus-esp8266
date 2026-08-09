# Referencia Completa de API Modbus

## Índice

1. [Clases Principales](#clases-principales)
2. [Funciones Básicas (FC 0x01-0x10)](#funciones-básicas)
3. [Funciones Avanzadas (FC 0x14-0x17)](#funciones-avanzadas)
4. [Diagnóstico (FC 0x08)](#diagnóstico)
5. [Identificación de Dispositivo (FC 0x2B)](#identificación-de-dispositivo)
6. [Modbus ASCII](#modbus-ascii)
7. [Integración MQTT](#integración-mqtt)
8. [Servidor Web](#servidor-web)

---

## Clases Principales

### ModbusRTU

Clase base para comunicación Modbus RTU sobre puerto serial.

#### Constructor

```cpp
ModbusRTU();
```

#### Métodos Principales

##### begin()

Inicializa la comunicación Modbus RTU.

```cpp
bool begin(HardwareSerial* port, int16_t dirPin = -1);
bool begin(SoftwareSerial* port, int16_t dirPin = -1);
```

**Parámetros:**
- `port`: Puntero al objeto Serial (HardwareSerial o SoftwareSerial)
- `dirPin`: Pin de control de dirección RS485 (opcional, -1 si no se usa)

**Retorna:** `true` si éxito

**Ejemplo:**
```cpp
ModbusRTU mb;
void setup() {
  Serial.begin(9600);
  mb.begin(&Serial, 3);  // Pin 3 para control direccional
}
```

---

##### setSlaveId() / setMasterId()

Establece el ID del dispositivo.

```cpp
void setSlaveId(uint8_t slaveId);
void setMasterId(uint8_t masterId);
```

**Parámetros:**
- `slaveId`: ID de esclavo (1-247)
- `masterId`: ID de maestro (usualmente 1)

**Notas:**
- ID 0 es broadcast (solo escritura)
- IDs 248-255 reservados

---

##### task()

Procesa las comunicaciones Modbus. Debe llamarse en loop().

```cpp
void task();
```

**Ejemplo:**
```cpp
void loop() {
  mb.task();
  delay(10);
}
```

---

##### addCoil() / addIsts() / addHreg() / addIreg()

Añade registros a la tabla de datos.

```cpp
bool addCoil(int address, uint16_t num = 1);
bool addIsts(int address, uint16_t num = 1);
bool addHreg(int address, uint16_t num = 1);
bool addIreg(int address, uint16_t num = 1);
```

**Parámetros:**
- `address`: Dirección inicial (offset 0-based)
- `num`: Número de registros a añadir

**Retorna:** `true` si éxito

**Ejemplo:**
```cpp
mb.addCoil(0, 10);      // 10 bobinas desde dirección 0
mb.addHreg(100, 50);    // 50 registros de retención desde 100
```

---

##### Coil() / Ists() / Hreg() / Ireg()

Accede a valores de registros individuales.

```cpp
bool Coil(int address);
uint16_t Ists(int address);
uint16_t Hreg(int address);
uint16_t Ireg(int address);

// Escritura
bool Coil(int address, bool value);
bool Hreg(int address, uint16_t value);
bool Ireg(int address, uint16_t value);
```

**Parámetros:**
- `address`: Dirección del registro
- `value`: Valor a escribir

**Retorna:** Valor actual o `true` si escritura exitosa

**Ejemplo:**
```cpp
bool estado = mb.Coil(0);           // Leer bobina 0
mb.Hreg(10, 1234);                  // Escribir 1234 en registro 10
uint16_t valor = mb.Ireg(5);        // Leer registro de entrada 5
```

---

##### readCoil() / readIsts() / readHreg() / readIreg() (Cliente)

Lee registros desde un servidor remoto.

```cpp
uint16_t readCoil(uint8_t slaveId, int address, uint16_t num, FrameInfo* frame = nullptr);
uint16_t readIsts(uint8_t slaveId, int address, uint16_t num, FrameInfo* frame = nullptr);
uint16_t readHreg(uint8_t slaveId, int address, uint16_t num, FrameInfo* frame = nullptr);
uint16_t readIreg(uint8_t slaveId, int address, uint16_t num, FrameInfo* frame = nullptr);
```

**Parámetros:**
- `slaveId`: ID del servidor remoto
- `address`: Dirección inicial
- `num`: Número de registros
- `frame`: Información de trama (opcional)

**Retorna:** ID de transacción (>0) o código de error

**Ejemplo:**
```cpp
uint16_t transId = mb.readHreg(2, 0, 10);  // Leer 10 regs del esclavo 2
if (transId > 0) {
  Serial.println("Lectura iniciada");
}
```

---

##### writeCoil() / writeHreg() / writeMultipleCoils() / writeMultipleHregs()

Escribe registros en servidor remoto.

```cpp
uint16_t writeCoil(uint8_t slaveId, int address, bool value);
uint16_t writeHreg(uint8_t slaveId, int address, uint16_t value);
uint16_t writeMultipleCoils(uint8_t slaveId, int address, uint16_t num, bool* value);
uint16_t writeMultipleHregs(uint8_t slaveId, int address, uint16_t num, uint16_t* value);
```

**Retorna:** ID de transacción o código de error

**Ejemplo:**
```cpp
bool coils[] = {true, false, true, false};
mb.writeMultipleCoils(2, 0, 4, coils);
```

---

### ModbusTCP

Clase para comunicación Modbus TCP sobre Ethernet/WiFi.

#### Constructor

```cpp
ModbusTCP();
```

#### Métodos Principales

##### begin()

```cpp
bool begin();
bool begin(uint16_t port);
```

**Parámetros:**
- `port`: Puerto TCP (por defecto 502)

**Ejemplo (ESP8266/ESP32):**
```cpp
#include <ModbusTCP.h>
#include <WiFi.h>

ModbusTCP mb;

void setup() {
  WiFi.begin("SSID", "password");
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  mb.begin();  // Puerto 502 por defecto
}
```

---

##### server() / client()

Configura como servidor o cliente.

```cpp
void server(uint16_t port = 502);
bool client(IPAddress ip, uint16_t port = 502);
bool client(const char* host, uint16_t port = 502);
```

**Ejemplo:**
```cpp
mb.server(502);  // Servidor en puerto 502
mb.client("192.168.1.100", 502);  // Cliente hacia IP específica
```

---

##### isConnected()

Verifica estado de conexión.

```cpp
bool isConnected();
```

**Retorna:** `true` si conectado

---

### ModbusAdvanced

Clase para funciones avanzadas (diagnóstico, identificación).

#### Constructor

```cpp
ModbusAdvanced(Modbus* modbus);
```

#### Métodos Principales

##### begin()

```cpp
void begin();
```

---

##### sendDiagnostic()

Envía comando de diagnóstico.

```cpp
uint16_t sendDiagnostic(uint8_t slaveId, uint16_t subFunction, uint16_t data);
```

**Sub-funciones soportadas:**
- `0x0000`: Echo
- `0x0001`: Restart Communications Option
- `0x0002`: Return Diagnostic Register
- `0x0003`: Change ASCII Input Delimiter
- `0x0004`: Force Listen Only Mode
- `0x000A`: Clear Counters and Diagnostic Register
- `0x000B`: Return Bus Message Count
- `0x000C`: Return Bus Communication Error Count
- `0x000D`: Return Exception Error Count
- `0x000E`: Return Slave Message Count
- `0x000F`: Return Slave No Response Count
- `0x0010`: Return Slave NAK Count
- `0x0011`: Return Slave Busy Count
- `0x0012`: Return Bus Character Overrun Count
- `0x0013`: I Am Ready
- `0x0014`: Reset Counters

**Ejemplo:**
```cpp
advanced.sendDiagnostic(2, 0x0000, 0x1234);  // Echo test
```

---

##### setDeviceIdentification()

Configura identificación de dispositivo.

```cpp
void setDeviceIdentification(uint8_t objectId, const char* value, bool writable = false);
```

**Objetos básicos (0x00-0x06):**
- `0x00`: VendorName
- `0x01`: ProductCode
- `0x02`: MajorMinorRevision
- `0x03`: VendorUrl
- `0x04`: ProductName
- `0x05`: ModelName
- `0x06`: UserApplicationName

**Objetos extendidos (0x80-0xFF):** Personalizables

**Ejemplo:**
```cpp
advanced.setDeviceIdentification(0x00, "MiEmpresa S.L.");
advanced.setDeviceIdentification(0x04, "Controlador Modbus v1.0");
advanced.setDeviceIdentification(0x80, "Dato Personalizado", true);  // Extendido, escribible
```

---

### ModbusASCII

Clase para comunicación Modbus ASCII.

#### Constructor

```cpp
ModbusASCII();
```

#### Métodos Principales

##### begin()

```cpp
bool begin(HardwareSerial* port, int16_t dirPin = -1);
```

**Nota:** El checksum LRC se calcula automáticamente.

**Ejemplo:**
```cpp
ModbusASCII mbAscii;
void setup() {
  Serial.begin(9600);
  mbAscii.begin(&Serial);
}
```

---

##### setMode()

Cambia entre modos RTU y ASCII en runtime.

```cpp
void setMode(MODBUS_MODE mode);
```

**Valores:**
- `MODBUS_RTU`: Modo RTU (binario)
- `MODBUS_ASCII`: Modo ASCII (hexadecimal)

**Ejemplo:**
```cpp
mb.setMode(MODBUS_ASCII);  // Cambiar a ASCII
mb.setMode(MODBUS_RTU);    // Volver a RTU
```

---

### ModbusMQTT

Clase para integración MQTT bidireccional.

#### Constructor

```cpp
ModbusMQTT(Modbus* modbus, PubSubClient* mqttClient);
```

#### Configuración

```cpp
struct ModbusMQTTConfig {
  const char* broker;           // Broker MQTT
  uint16_t port;                // Puerto (1883 por defecto)
  const char* clientId;         // ID de cliente único
  const char* user;             // Usuario (opcional)
  const char* password;         // Contraseña (opcional)
  const char* publishTopic;     // Topic para publicar cambios Modbus
  const char* subscribeTopic;   // Topic para recibir comandos
  uint32_t publishInterval;     // Intervalo de publicación (ms)
  bool autoReconnect;           // Reconexión automática
};
```

#### Métodos Principales

##### begin()

```cpp
bool begin(ModbusMQTTConfig config);
```

**Ejemplo:**
```cpp
#include <ModbusMQTT.h>
#include <PubSubClient.h>

ModbusRTU mb;
PubSubClient mqttClient(wifiClient);
ModbusMQTT mqttBridge(&mb, &mqttClient);

ModbusMQTTConfig config = {
  .broker = "test.mosquitto.org",
  .port = 1883,
  .clientId = "modbus-device-001",
  .publishTopic = "modbus/data",
  .subscribeTopic = "modbus/command",
  .publishInterval = 5000,
  .autoReconnect = true
};

void setup() {
  mb.begin(&Serial);
  mqttBridge.begin(config);
}

void loop() {
  mb.task();
  mqttBridge.loop();
}
```

---

##### registerMapping()

Registra mapeo entre registros Modbus y topics MQTT.

```cpp
void registerMapping(uint8_t modbusType, int address, const char* mqttField);
```

**Tipos Modbus:**
- `COIL`: Bobinas
- `IST`: Entradas discretas
- `HREG`: Registros de retención
- `IREG`: Registros de entrada

**Ejemplo:**
```cpp
mqttBridge.registerMapping(COIL, 0, "relay1");
mqttBridge.registerMapping(HREG, 10, "temperature");
mqttBridge.registerMapping(HREG, 11, "humidity");
```

---

### ModbusWebServer

Clase para servidor web de configuración.

#### Constructor

```cpp
ModbusWebServer(Modbus* modbus);
```

#### Configuración

```cpp
struct ModbusWebConfig {
  char ssid[32];              // SSID WiFi
  char password[64];          // Contraseña WiFi
  char deviceName[32];        // Nombre del dispositivo
  uint16_t modbusPort;        // Puerto Modbus TCP
  uint8_t modbusSlaveId;      // ID esclavo Modbus
  bool enableAuth;            // Autenticación web
  char webUser[16];           // Usuario web
  char webPassword[32];       // Contraseña web
};
```

#### Estadísticas

```cpp
struct ModbusWebStats {
  uint32_t uptime;              // Tiempo activo (segundos)
  uint32_t totalMessages;       // Mensajes procesados
  uint32_t totalErrors;         // Errores totales
  uint32_t activeConnections;   // Conexiones activas
  float cpuTemperature;         // Temperatura CPU
  int16_t rssi;                 // Intensidad WiFi
  uint32_t freeHeap;            // Memoria libre
  uint8_t wifiQuality;          // Calidad WiFi (0-100%)
};
```

#### Métodos Principales

##### begin()

```cpp
bool begin(const char* ssid = nullptr, const char* password = nullptr);
```

**Ejemplo:**
```cpp
#include <ModbusWebConfig.h>

ModbusTCP mb;
ModbusWebServer webServer(&mb);

void setup() {
  mb.begin();
  
  if (webServer.begin("MiRed", "contraseña")) {
    Serial.println("Servidor web iniciado");
    Serial.print("URL: http://");
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  mb.task();
  webServer.handleClient();
}
```

---

##### handleClient()

Procesa solicitudes HTTP. Llamar en loop().

```cpp
void handleClient();
```

---

##### getStats()

Obtiene estadísticas actuales.

```cpp
const ModbusWebStats& getStats() const;
```

---

## Códigos de Error

| Código | Descripción |
|--------|-------------|
| 0x01 | Función ilegal |
| 0x02 | Dirección de datos ilegal |
| 0x03 | Valor ilegal |
| 0x04 | Falla en esclavo |
| 0x05 | Confirmación |
| 0x06 | Ocupado |
| 0x08 | Paridad |
| 0x0A | Gateway no disponible |
| 0x0B | Gateway timeout |

---

## Constantes y Definiciones

### Tipos de Registro

```cpp
#define COIL      0   // Bobinas (lectura/escritura, 1 bit)
#define IST       1   // Discrete Inputs (solo lectura, 1 bit)
#define HREG      2   // Holding Registers (lectura/escritura, 16 bits)
#define IREG      3   // Input Registers (solo lectura, 16 bits)
```

### Funciones Modbus

```cpp
#define MB_FC_READ_COILS              0x01
#define MB_FC_READ_DISCRETE_INPUTS    0x02
#define MB_FC_READ_HOLDING_REGISTERS  0x03
#define MB_FC_READ_INPUT_REGISTERS    0x04
#define MB_FC_WRITE_COIL              0x05
#define MB_FC_WRITE_REGISTER          0x06
#define MB_FC_WRITE_MULTIPLE_COILS    0x0F
#define MB_FC_WRITE_MULTIPLE_REGISTERS 0x10
#define MB_FC_READ_FILE_RECORD        0x14
#define MB_FC_WRITE_FILE_RECORD       0x15
#define MB_FC_MASK_WRITE_REGISTER     0x16
#define MB_FC_READ_WRITE_REGISTERS    0x17
#define MB_FC_DIAGNOSTICS             0x08
#define MB_FC_DEVICE_IDENTIFICATION   0x2B
```

---

## Ejemplos de Uso Completo

### Servidor RTU con Callbacks

```cpp
#include <ModbusRTU.h>

#define LED_PIN 2

ModbusRTU mb;

bool coils[10];
uint16_t hregs[20];

// Callback para escritura en bobina
bool onCoilWrite(TRegister* reg, uint16_t val) {
  Serial.print("Bobina ");
  Serial.print(reg->address);
  Serial.print(" escrita: ");
  Serial.println(val ? "ON" : "OFF");
  
  if (reg->address == 0) {
    digitalWrite(LED_PIN, val ? HIGH : LOW);
  }
  return true;  // Permitir escritura
}

// Callback para lectura de registro
uint16_t onHregRead(TRegister* reg) {
  if (reg->address == 0) {
    return analogRead(A0);  // Leer sensor
  }
  return reg->value;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  
  mb.begin(&Serial);
  mb.setSlaveId(1);
  
  mb.addCoil(0, 10);
  mb.addHreg(0, 20);
  
  // Registrar callbacks
  mb.onWriteCoil(onCoilWrite);
  mb.onReadHreg(onHregRead);
}

void loop() {
  mb.task();
  delay(10);
}
```

### Cliente TCP con Reconexión

```cpp
#include <ModbusTCP.h>
#include <WiFi.h>

const char* ssid = "MiRed";
const char* password = "MiPassword";

ModbusTCP mb;

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  mb.begin();
  mb.client("192.168.1.100", 502);
}

void loop() {
  if (!mb.isConnected()) {
    Serial.println("Reconectando...");
    mb.client("192.168.1.100", 502);
    delay(2000);
    return;
  }
  
  uint16_t res = mb.readHreg(1, 0, 10);
  if (res > 0) {
    Serial.println("Lectura completada");
  } else {
    Serial.print("Error: ");
    Serial.println(mb.getError());
  }
  
  mb.task();
  delay(1000);
}
```

---

## Notas Importantes

1. **Offsets Base 0**: Las direcciones en esta biblioteca son base 0. Un registro configurado como 100 debe referenciarse como 100 en ScadaBR pero como 101 en CAS Modbus Scanner (que usa base 1).

2. **Timeouts**: Los timeouts se calculan automáticamente según baudrate en RTU. En TCP, el timeout por defecto es 1000ms.

3. **Memoria**: En dispositivos AVR, usar `#define MODBUS_STATIC_BUFFER` antes de incluir la biblioteca para evitar fragmentación de heap.

4. **Broadcast**: El ID 0 es broadcast. Solo usar para funciones de escritura (0x05, 0x06, 0x0F, 0x10). No esperar respuesta.

5. **Múltiples Instancias**: Se pueden crear múltiples instancias Modbus (ej. RTU + TCP simultáneos). Cada una requiere su propio `task()`.

---

**Última actualización**: Versión 4.3.0
**Documentación en español completa**
