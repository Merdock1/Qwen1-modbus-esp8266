# Ejemplo: Comunicación Modbus ASCII

## Descripción
Este ejemplo demuestra el uso del protocolo **Modbus ASCII** para comunicación serial, una alternativa más legible al formato RTU binario.

## Características
- ✅ Configuración básica de esclavo Modbus ASCII
- ✅ Soporte para Coils, Discrete Inputs, Input Registers y Holding Registers
- ✅ Parsing automático de tramas ASCII hexadecimales
- ✅ Validación LRC (Longitudinal Redundancy Check)
- ✅ Actualización dinámica de registros
- ✅ Comentarios detallados en español

## Hardware Requerido
- Arduino Uno/Nano/Leonardo **O** ESP8266 **O** ESP32
- Convertidor RS485 (MAX485, SP3485, etc.) - opcional para comunicación directa USB-TTL

## Conexiones

### Sin RS485 (USB-TTL directo para testing)
```
Arduino/ESP     USB-TTL
-----------     -------
TX (GPIO1)  ->  RX
RX (GPIO3)  ->  TX
GND         ->  GND
```

### Con RS485 (comunicación industrial)
```
Arduino/ESP     MAX485      Terminal
-----------     ------      --------
TX (GPIO1)  ->  DI          -
RX (GPIO3)  ->  RO          -
GPIO2       ->  DE+RE       -
GND         ->  GND       -> GND
            -  A         ->  A (+)
            -  B         ->  B (-)
```

**Nota:** Agregar resistencia de terminación 120Ω entre A y B en los extremos del bus.

## Configuración

### Parámetros Modbus ASCII
- **Baudrate:** 9600 bps (configurable)
- **Data bits:** 8
- **Parity:** None
- **Stop bits:** 1
- **Slave ID:** 1

### Formato de Trama
```
:[Dirección][Función][Datos][LRC]\r\n
```

Ejemplo: `:010300000001F9\r\n`
- `:` = Carácter de inicio (0x3A)
- `01` = Slave ID
- `03` = Función (Read Holding Registers)
- `0000` = Registro inicial
- `0001` = Cantidad de registros
- `F9` = LRC checksum
- `\r\n` = Fin de línea

## Pruebas con Software

### Opción 1: QModMaster (Recomendado)
1. Descargar e instalar QModMaster
2. Configurar conexión:
   - Mode: **ASCII** (¡importante!)
   - Port: COMx (asignado a tu USB-RS485)
   - Baudrate: 9600
   - Data: 8, Parity: None, Stop: 1
   - Slave ID: 1

3. Probar funciones:
   - Read Holding Registers (03): Addr 0, Count 10
   - Read Input Registers (04): Addr 0, Count 10
   - Read Coils (01): Addr 0, Count 10
   - Write Single Register (06): Addr 0, Value 1234

### Opción 2: Terminal Serial
Usar PuTTY, screen o terminal similar:
```bash
# Linux/Mac
screen /dev/ttyUSB0 9600

# Enviar comando manualmente
echo -ne ":010300000001F9\r\n" > /dev/ttyUSB0
```

### Opción 3: Python Script
```python
import serial
import time

ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1)

# Leer 1 Holding Register desde dirección 0
comando = b':010300000001F9\r\n'
ser.write(comando)
time.sleep(0.1)
respuesta = ser.readall()
print(f"Respuesta: {respuesta}")
```

## Funciones Soportadas

| Código | Función | Descripción |
|--------|---------|-------------|
| 0x01 | Read Coils | Leer salidas digitales |
| 0x02 | Read Discrete Inputs | Leer entradas digitales |
| 0x03 | Read Holding Registers | Leer registros escribibles |
| 0x04 | Read Input Registers | Leer registros solo lectura |
| 0x05 | Write Single Coil | Escribir una salida digital |
| 0x06 | Write Single Register | Escribir un registro |
| 0x0F | Write Multiple Coils | Escribir múltiples salidas |
| 0x10 | Write Multiple Registers | Escribir múltiples registros |

## Solución de Problemas

### No hay respuesta
1. Verificar conexiones (TX/RX invertidos?)
2. Confirmar baudrate (9600)
3. Verificar Slave ID es 1
4. Asegurar que `mb.task()` se llama en loop()

### Errores LRC
1. Verificar modo ASCII (no RTU) en software maestro
2. Confirmar configuración 8N1
3. Verificar ruido eléctrico (agregar terminación 120Ω)

### Timeouts frecuentes
1. Modo ASCII es ~2x más lento que RTU - aumentar timeout
2. Verificar que `task()` se llama frecuentemente
3. Reducir baudrate si hay muchos errores

## Comparación: ASCII vs RTU

| Característica | ASCII | RTU |
|---------------|-------|-----|
| Formato | Texto hexadecimal | Binario |
| Eficiencia | ~50% (2 chars por byte) | 100% |
| Velocidad | Más lento | Más rápido |
| Debugging | Fácil (texto plano) | Requiere analyzer |
| Legibilidad | Alta | Baja |
| Uso recomendado | Testing, debugging | Producción |

## Recursos Adicionales
- [Documentación API](../../../docs/API_ES.md)
- [Especificación Modbus ASCII](https://modbus.org/docs/Modbus_ASCII.pdf)
- [QModMaster](http://qmodmaster.sourceforge.net/)
- [Calculadora LRC online](https://www.simplymodbus.ca/LRC.htm)

## Licencia
BSD New License - Ver LICENSE.txt para detalles.

## Autor
Modbus Library Team, 2024
