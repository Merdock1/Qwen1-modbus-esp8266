# Descripción de la Librería Modbus para Arduino

## Resumen Ejecutivo

Esta es la implementación más completa del protocolo Modbus para Arduino, desarrollada originalmente por André Sarmento Barbosa y mantenida actualmente por Alexyer Emelianov. La librería proporciona una implementación robusta del protocolo Modbus para automatización industrial y domótica.

**Versión Actual:** 4.1.0  
**Licencia:** BSD New Licencia  
**Repositorio:** https://github.com/emelianov/modbus-esp8266  
**Contacto:** a.m.emelianov@gmail.com

---

## Características Principales

### Protocoloos Soportados

1. **Modbus RTU** - Comunicación serial sobre RS-485
2. **Modbus TCP** - Comunicación sobre Ethernet/WiFi
3. **Modbus TCP Security (TLS)** - Comunicación segura con cifrado

### Funciones Modbus Implementadas

| Código | Función | Descripción |
|--------|---------|-------------|
| 0x01 | Leer Bobinas (Read Coils) | Lectura de salidas discretas |
| 0x02 | Leer Estado de Entradas (Read Discrete Inputs) | Lectura de entradas discretas |
| 0x03 | Leer Registros de Retención | Lectura de registros de salida |
| 0x04 | Leer Registros de Entrada | Lectura de registros de entrada |
| 0x05 | Escribir Bobina Individual | Escritura de una salida discreta |
| 0x06 | Escribir Registro Individual | Escritura de un registro de salida |
| 0x0F | Escribir Múltiples Bobinas | Escritura múltiple de salidas discretas |
| 0x10 | Escribir Múltiples Registros | Escritura múltiple de registros de salida |
| 0x14 | Leer Registro de Archivo | Lectura de archivo |
| 0x15 | Escribir Registro de Archivo | Escritura de archivo |
| 0x16 | Enmascarar Escritura de Registro | Escritura con máscara de registro |
| 0x17 | Read/Escribir Múltiples Registros | Lectura/escritura combinada |

### Arquitectura

- **Diseño basado en callbacks** para manejo asíncrono de transacciones
- **Soporte multi-instancia**: Puede operar simultáneamente como:
  - Múltiples servidores Modbus RTU
  - Múltiples clientes Modbus RTU
  - Múltiples servidores Modbus TCP
  - Múltiples clientes Modbus TCP
  - Servidores/clientes Modbus TLS
- **Independiente de STL**: Puede compilarse sin la biblioteca estándar de C++ para plataformas con recursos limitados

---

## Plataformas Soportadas

### Compatibilidad General

**Arquitecturas:** Todas las plataformas Arduino (`architectures=*`)

La librería está diseñada para ser portable y funcionar en cualquier placa compatible con el ecosistema Arduino.

### Plataformas Específicas Probadas y Certificadas

#### 1. **ESP8266**
- **Soporte completo**: Clientee/Servidor Modbus TCP
- **Soporte completo**: Clientee/Servidor Modbus TLS (Security)
- **Soporte completo**: Clientee/Servidor Modbus RTU
- **Características especiales**:
  - WiFi integrado
  - Hasta 8 conexiones TCP simultáneas
  - Soporte para actualizaciones de firmware over Modbus

#### 2. **ESP32**
- **Soporte completo**: Clientee/Servidor Modbus TCP
- **Soporte parcial**: Clientee Modbus TLS (Servidor en desarrollo)
- **Soporte completo**: Clientee/Servidor Modbus RTU
- **Características especiales**:
  - WiFi y Ethernet integrado
  - Multithreading para acceso concurrente
  - SoftwareSerial para puertos seriales adicionales
  - Timeout de conexión configurable
  - Hasta 8 clientes TCP simultáneos
  - Soporte nativo para nombres DNS

#### 3. **Arduino con Ethernet Shield**
- **Soporte completo**: Clientee/Servidor Modbus TCP
- **Hardware soportado**:
  - WizNet W5x00 (W5100, W5200, W5500)
  - ENC28J60
- **Librerías Ethernet**: Versión 2+ (v1 no soportada)

#### 4. **Teknic ClearCore**
- **Soporte mediante ArduinoWrapper**
- Ejemplos específicos disponibles
- Controladores industriales programables

#### 5. **Otras Placas Arduino**
- Arduino Uno/Nano/Mega
- Arduino Due
- Arduino Zero
- Arduino Leonardo
- Cualquier placa con soporte Arduino Core

---

## Chips y Hardware Soportado

### Microcontroladores

| Familia | Modelos | Estado |
|---------|---------|--------|
| **Espressif** | ESP8266 (ESP-01, ESP-12, NodeMCU, Wemos D1) | ✅ Completo |
| **Espressif** | ESP32, ESP32-S2, ESP32-C3 | ✅ Completo |
| **Atmel AVR** | ATmega328P, ATmega2560, ATmega32U4 | ✅ Completo |
| **Atmel SAM** | SAMD21, SAMD51 | ✅ Completo |
| **Teknic** | ClearCore | ✅ Mediante wrapper |

### Transceptores RS-485

| Modelo | Velocidad Máxima | Notas |
|--------|------------------|-------|
| MAX485 | 115200 bps | Recomendado |
| SP3485 | 115200 bps | Compatible |
| SN75176 | 115200 bps | Compatible |
| XY-017 | 9600 bps | Limitado |
| XY-485 | 9600 bps | Limitado |

### Chips Ethernet

| Chip | Librería | Estado |
|------|----------|--------|
| W5100 | Ethernet v2 | ✅ Soportado |
| W5200 | Ethernet v2 | ✅ Soportado |
| W5500 | Ethernet v2 | ✅ Soportado |
| ENC28J60 | EthernetENC | ✅ Soportado |

---

## Requisitos del Sistema

### Memoria

- **Sin STL**: Menor footprint de memoria, recomendado para placas con <64KB RAM
- **Con STL**: Funcionalidad completa, requiere ~64KB+ RAM disponible
- **Límite de registros**: Vector limitado a 4000 registros (ESP8266/ESP32)

### Configuración de Compilación

```cpp
// En ModbusConfiguración.h o antes de incluir la librería

// Usar STL (recomendado para ESP32/ESP8266)
#define MODBUS_USE_STL

// Usar asignación estática de buffers (roadmap v4.2.0)
// #define MODBUS_STATIC_BUFFER

// Tiempo de espera de conexión (ESP32)
#define MODBUSIP_CONNECTION_TIMEOUT 1000

// Máximo de clientes TCP
#define MODBUSIP_MAX_CLIENTS 8  // ESP32
#define MODBUSIP_MAX_CLIENTS 4  // ESP8266
```

---

## Casos de Uso Típicos

### 1. Automatización Industrial
- Lectura de sensores remotos
- Control de actuadores
- Monitoreo de variables de proceso
- Integración con SCADA (ScadaBR, CAS Modbus Scanner)

### 2. Domótica
- Control de iluminación
- Gestión de climatización
- Monitoreo de consumo energético
- Integración con sistemas de seguridad

### 3. Puentes y Gateways
- Bridge Modbus RTU ↔ Modbus TCP
- Conversión de protocolos
- Concentradores de datos

### 4. Actualizaciones Remotas
- Firmware update over Modbus (ESP8266/ESP32)
- Transferencia de archivos
- Gestión de configuraciones

---

## Ejemplos Incluidos

```
examples/
├── RTU/                    # Modbus Serial (RS-485)
│   ├── master/            # Clientee simple
│   ├── slave/             # Servidor simple
│   ├── masterSync/        # Clientee síncrono
│   └── ESP32-Concurent/   # Acceso multihilo (ESP32)
├── TCP-ESP/               # Modbus TCP para ESP8266/ESP32
│   ├── client/            # Clientee básico
│   ├── clientSync/        # Clientee bloqueante
│   └── server/            # Servidor
├── TCP-Ethernet/          # Modbus TCP con shield Ethernet
├── TLS/                   # Modbus TCP Security
│   ├── client/            # Clientee seguro
│   └── server/            # Servidor seguro (ESP8266)
├── Bridge/                # Puentes RTU↔TCP
├── Callback/              # Uso avanzado de callbacks
├── Files/                 # Operaciones con archivos
└── ClearCore/             # Ejemplos Teknic ClearCore
```

---

## Consideraciones Importantes

### Offset de Registros
- **La librería usa offsets 0-based**
- ScadaBR: Usa 0-based (compatible directamente)
- CAS Modbus Scanner: Usa 1-based (requiere ajuste +1)

### Velocidades de Comunicación
- **RS-485**: Hasta 115200 bps (depende del transceptor)
- **TCP**: Limitado por la red y plataforma
- **Recomendación**: MAX485 para altas velocidades

### Limitaciones Conocidas
1. Una sola conexión por dirección IP (cliente TCP)
2. Servidor TLS solo ESP8266 (ESP32 en desarrollo)
3. No soporta cambio dinámico entre modo cliente/servidor
4. XY-017/XY-485 limitados a 9600 bps

---

## Hoja de Ruta (Próximas Versiones)

### Versión 4.2.0
- [ ] Cálculo alternativo de CRC (menor uso de memoria)
- [ ] Asignación estática de buffers para Modbus RTU
- [ ] Limitación de tamaño de buffer/paquete
- [ ] Validación adicional de respuestas
- [ ] Liberación de registros globales y callbacks

### Versión 4.3.0
- [ ] Servidor TLS para ESP32
- [ ] Pruebas completos para TLS ESP32
- [ ] ModbusAsyncTCP
- [ ] API extendida para comyos Modbus personalizados
- [ ] Ejemplos básicos de operaciones con archivos

---

## Recursos Adicionales

### Documentación Oficial
- [Especificación Modbus Application Protocolo V1.1b3](https://modbus.org/docs/Modbus_Application_Protocolo_V1_1b3.pdf)
- [Guía de Implementación Modbus Messaging on TCP/IP](http://www.modbus.org/docs/Modbus_Messaging_Implementation_Guide_V1_0b.pdf)
- [Especificación Modbus over Serial Line V1.02](http://www.modbus.org/docs/Modbus_over_serial_line_V1_02.pdf)
- [Especificación Modbus/TCP Security](https://modbus.org/docs/MB-TCP-Seguridad-v21_2018-07-24.pdf)

### Enlaces de Interés
- [Wikipedia - Modbus](http://pt.wikipedia.org/wiki/Modbus)
- [Modbus Organization](https://modbus.org)
- [Repositorio GitHub](https://github.com/emelianov/modbus-esp8266)

---

## Historial de Versiones Recientes

### v4.1.1 (Actual)
- ✅ Corrección de código de error en registros inexistentes
- ✅ Fix de posible fuga de memoria en ModbusTCP
- ✅ Extensiones en cbEnable/cbDisable
- ✅ CMakeLists.txt para ESP-IDF
- ✅ Ejemplos TCP-to-RTU corregidos

### v4.1.0
- ✅ Procesamiento de frames Modbus raw
- ✅ Control preciso de intervalo inter-frame RTU
- ✅ Puente ModbusRTU a ModbusTCP Servidor
- ✅ Respuesta a múltiples IDs desde un dispositivo
- ✅ Control de pin de dirección para Stream
- ✅ Soporte para SoftwareSerial en ESP32
- ✅ Construcción sin dependencia STL (configurable)
- ✅ Control de acceso por callback para funciones individuales

### v4.0.0
- ✅ Soporte para todas las placas Arduino
- ✅ ModbusTLS: ESP8266 Clientee/Servidor y ESP32 Cliente
- ✅ Soporte Ethernet: WizNet W5x00, ENC28J60
- ✅ Funciones 0x14, 0x15, 0x16, 0x17
- ✅ Actualización de firmware over Modbus
- ✅ Renombrado Master/Slave → Clientee/Servidor

---

**Última actualización:** Agosto 2024  
**Mantenedor:** Alexyer Emelianov
