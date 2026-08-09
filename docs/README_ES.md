# Biblioteca Modbus para Arduino - Guía de Inicio Rápido

## Descripción General

Esta biblioteca proporciona una implementación completa del protocolo Modbus para Arduino, incluyendo soporte para Modbus RTU, Modbus TCP y Modbus TCP Security (TLS).

### Características Principales

- **Soporte Multiplataforma**: Compatible con todas las placas Arduino (AVR, ESP8266, ESP32, SAMD, etc.)
- **Múltiples Protocolos**: 
  - Modbus RTU sobre Serial/RS485
  - Modbus TCP sobre Ethernet/WiFi
  - Modbus TCP Security (TLS) para ESP8266/ESP32
- **Funciones Modbus Completas**: 0x01-0x17 incluyendo diagnóstico avanzado
- **Arquitectura Flexible**: Servidor, Cliente o ambos simultáneamente
- **Optimizado para Dispositivos Limitados**: Buffer pool estático para AVR

## Instalación

### Método 1: Gestor de Bibliotecas de Arduino

1. Abra el IDE de Arduino
2. Vaya a `Programa` → `Incluir Librería` → `Gestionar Bibliotecas...`
3. Busque "Modbus"
4. Seleccione la biblioteca e instale

### Método 2: Instalación Manual

1. Descargue el código fuente desde GitHub
2. Extraiga el archivo ZIP
3. Copie la carpeta `modbus-esp8266` a su carpeta `libraries` de Arduino
4. Reinicie el IDE de Arduino

## Requisitos del Sistema

### Hardware Soportado

| Plataforma | Modbus RTU | Modbus TCP | Modbus TLS |
|------------|-----------|-----------|-----------|
| Arduino Uno/Nano (AVR) | ✅ | ❌ | ❌ |
| Arduino Leonardo/Micro | ✅ | ❌ | ❌ |
| Arduino Mega | ✅ | Con Ethernet Shield | ❌ |
| ESP8266 (NodeMCU, Wemos) | ✅ | ✅ | ✅ |
| ESP32 | ✅ | ✅ | ✅ |
| Arduino Due (SAM) | ✅ | Con Ethernet Shield | ❌ |
| Teensy | ✅ | ✅ (con adaptador) | ❌ |

### Dependencias de Software

- **Arduino IDE** 1.8.19 o superior (o PlatformIO)
- Para ESP8266: Core ESP8266 2.7.0+
- Para ESP32: Core ESP32 1.0.4+
- Para Ethernet: Biblioteca Ethernet 2.0.0+

## Primeros Pasos

### Ejemplo 1: Servidor Modbus RTU Básico

```cpp
#include <ModbusRTU.h>

#define LED_PIN 2
#define RS485_DIR_PIN 3

// Tabla de registros
bool coils[10];
uint16_t holdingRegs[20];

ModbusRTU mb;

void setup() {
  Serial.begin(9600);
  
  // Configurar dirección RS485
  pinMode(RS485_DIR_PIN, OUTPUT);
  
  // Inicializar Modbus con ID=1
  mb.begin(&Serial, RS485_DIR_PIN);
  mb.setSlaveId(1);
  
  // Mapear registros
  mb.addCoil(LED_PIN, 10);           // 10 bobinas empezando en LED_PIN
  mb.addHreg(0, 20);                 // 20 registros de retención
  
  // Establecer valor inicial
  mb.Hreg(0, 1234);
}

void loop() {
  mb.task();  // Procesar comunicaciones Modbus
  delay(10);
}
```

### Ejemplo 2: Cliente Modbus TCP (ESP8266/ESP32)

```cpp
#include <ModbusTCP.h>
#include <WiFi.h>  // o <ESP8266WiFi.h>

const char* ssid = "tu_red";
const char* password = "tu_contraseña";

ModbusTCP mb;

void setup() {
  Serial.begin(115200);
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  
  // Iniciar Modbus TCP
  mb.begin();
  
  // Conectar al servidor Modbus (IP: 192.168.1.100, Puerto: 502)
  mb.client("192.168.1.100", 502);
}

void loop() {
  mb.task();
  
  // Leer 10 bobinas del servidor (ID=1, dirección=0)
  if (mb.isConnected()) {
    mb.readCoil(1, 0, 10);
  }
  
  delay(1000);
}
```

### Ejemplo 3: Usar Función de Diagnóstico (FC 0x08)

```cpp
#include <ModbusRTU.h>
#include <ModbusAdvanced.h>

ModbusRTU mb;
ModbusAdvanced advanced(&mb);

void setup() {
  Serial.begin(9600);
  mb.begin(&Serial);
  mb.setMasterId(1);
  
  advanced.begin();
}

void loop() {
  mb.task();
  advanced.task();
  
  // Enviar comando de diagnóstico (sub-función 0x0000 - Echo)
  static uint32_t lastDiag = 0;
  if (millis() - lastDiag > 5000) {
    advanced.sendDiagnostic(2, 0x0000, 0x1234);  // ID=2, Echo
    lastDiag = millis();
  }
  
  delay(10);
}
```

## Configuración Avanzada

### Timeouts Dinámicos

La biblioteca calcula automáticamente los timeouts basados en el baudrate:

```cpp
ModbusRTU mb;

void setup() {
  Serial.begin(115200);
  mb.begin(&Serial);
  
  // El timeout se ajusta automáticamente
  // Para 115200 baud: ~3.5ms por carácter
  // Para 9600 baud: ~35ms por carácter
}
```

### Buffer Pool Estático (para AVR)

Para dispositivos con RAM limitada como Arduino Uno:

```cpp
#define MODBUS_STATIC_BUFFER  // Definir antes de incluir la biblioteca
#include <ModbusRTU.h>

// Esto usa memoria estática en lugar de malloc/free
// Ideal para sistemas embebidos críticos
```

### Optimización CRC para AVR

La tabla CRC se almacena en FLASH para ahorrar RAM:

```cpp
// Automáticamente habilitado para plataformas AVR
// Ahorra 512 bytes de RAM
// 40% más rápido que cálculo en tiempo real
```

## Solución de Problemas

### Problema: No hay comunicación Modbus RTU

**Causas posibles:**
1. Verificar cableado RS485 (A/B correctos)
2. Comprobar baudrate coincide en todos los dispositivos
3. Verificar ID de esclavo único
4. Revisar polaridad A/B (intercambiar si es necesario)

### Problema: Timeout en lecturas TCP

**Soluciones:**
1. Verificar conectividad de red (ping)
2. Confirmar puerto 502 abierto en firewall
3. Comprobar IP y puerto correctos
4. Aumentar timeout si la red es lenta: `mb.setTimeout(5000);`

### Problema: Fugas de memoria en ESP8266

**Solución:**
- Actualizar a versión 4.1.1+ (corrección implementada)
- Usar buffer pool estático si es posible
- Monitorear con `ESP.getFreeHeap()`

## Recursos Adicionales

- [Documentación API Completa](API_ES.md)
- [Ejemplos Detallados](EJEMPLOS_ES.md)
- [Guía de Seguridad](SEGURIDAD_ES.md)
- [Especificación Modbus Oficial](https://modbus.org/specs.php)

## Soporte y Contribuciones

- **Reportar Issues**: https://github.com/emelianov/modbus-esp8266/issues
- **Discusiones**: https://github.com/emelianov/modbus-esp8266/discussions
- **Email**: a.m.emelianov@gmail.com

## Licencia

Licencia BSD Nueva. Ver LICENSE.txt para detalles completos.

---

**Nota**: Esta guía está actualizada para la versión 4.3.0 de la biblioteca. Algunas características pueden variar en versiones anteriores.
