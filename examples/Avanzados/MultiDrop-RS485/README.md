# Red Multi-Drop RS485

## Descripción

Ejemplo avanzado que implementa una red Modbus RTU multi-dispositivo con gestión inteligente de slaves. Ideal para:

- Sistemas con múltiples sensores/actuadores
- Monitorización de estado de red en tiempo real
- Detección automática de fallos
- Aplicaciones industriales con alta disponibilidad

## Características Clave

- **Polling Round-Robin**: Consulta cíclica a todos los slaves
- **Detección de Fallos**: Marca slaves offline tras múltiples errores
- **Reintentos Automáticos**: Hasta 3 intentos antes de declarar fallo
- **Estadísticas en Tiempo Real**: Éxitos, fallos, última respuesta
- **Callbacks Asíncronos**: Respuesta y error manejados por eventos

## Hardware Requerido

- ESP32 o Arduino Mega (con múltiples UART)
- Módulo RS485 MAX485
- 2+ dispositivos Modbus slave

## Conexiones Típicas

```
         [RS485 Bus]
        /     |     \
   Slave1  Slave2  Slave3
   (ID=1)  (ID=2)  (ID=3)
      |       |       |
      +-------+-------+
              |
           [Master]
           (ESP32)
```

### Cableado RS485

```
Dispositivo    Terminal    Color Típico
-----------    --------    -------------
               A (+)       Verde
               B (-)       Rojo
               GND         Negro
```

**Importante**: Todos los dispositivos deben compartir la misma referencia GND.

## Configuración

### Lista de Slaves

```cpp
const uint8_t slaveIds[MAX_SLAVES] = {1, 2, 3, 4, 5};
const uint8_t slaveCount = 5;
```

### Parámetros de Comunicación

```cpp
#define SLAVE_TIMEOUT 500      // ms antes de timeout
#define MAX_RETRIES 3          // Reintentos antes de offline
#define POLL_INTERVAL 100      // ms entre consultas
```

## Estructura de Datos

```cpp
struct SlaveStatus {
    uint8_t id;                    // ID del slave
    bool online;                   // Estado actual
    uint16_t registers[5];         // Valores leídos
    uint32_t successCount;         // Total éxitos
    uint32_t failCount;            // Total fallos
    uint32_t lastResponseTime;     // ms desde última respuesta
    uint8_t consecutiveFails;      // Fallos consecutivos
};
```

## Salida Típica

```
========== ESTADO DE RED ==========
ID     Online   Éxitos     Fallos     Última Resp.  
------------------------------------------
1      ✓        150        2          234         
   Regs: 250 320 280 290 300
2      ✓        148        3          456         
   Regs: 180 190 200 210 220
3      ✗        45         89         15234       
======================================
```

## Mejores Prácticas

### 1. Terminación de Línea

Colocar resistencias de 120Ω en los extremos del bus:

```
[120Ω]----[Slave1]----[Slave2]----[Slave3]----[120Ω]
```

### 2. Velocidad de Comunicación

Para redes largas (>100m) o muchos slaves:

```cpp
#define MODBUS_BAUDRATE 9600    // Más lento pero más fiable
```

### 3. Timeout Ajustado

Calcular timeout basado en cantidad de slaves:

```cpp
// Timeout mínimo = (slaves * registros * 10ms)
#define SLAVE_TIMEOUT (slaveCount * REGISTROS_POR_SLAVE * 10)
```

## Troubleshooting

### Slave No Responde

1. Verificar ID correcto
2. Comprobar cableado A/B
3. Medir continuidad GND
4. Probar baudrate más bajo

### Muchos Timeouts

1. Reducir POLL_INTERVAL
2. Aumentar SLAVE_TIMEOUT
3. Disminuir baudrate
4. Verificar terminación de línea

### Lecturas Incorrectas

1. Verificar endianness (big-endian Modbus)
2. Confirmar dirección de registro
3. Chequear tipo de dato (uint16 vs int16)

## Aplicaciones Típicas

### 1. Sistema de Climatización

- Slave 1: Termostato zona 1
- Slave 2: Termostato zona 2
- Slave 3: Caldera central
- Slave 4: Bomba circulación

### 2. Monitor Energético

- Slave 1: Medidor entrada principal
- Slave 2: Medidor iluminación
- Slave 3: Medidor HVAC
- Slave 4: Medidor enchufes

### 3. Control de Acceso

- Slave 1: Lector puerta principal
- Slave 2: Lector puerta secundaria
- Slave 3: Cerradura eléctrica
- Slave 4: Sensor apertura/cierre

## Autor

Equipo de Desarrollo Modbus - 2024

## Licencia

LGPL-2.1
