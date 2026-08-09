# TAREA 3.2: SOPORTE MODBUS ASCII

## Resumen de Implementación

Implementación completa del protocolo Modbus ASCII además de RTU, conmutables en runtime.

## Características Implementadas

- [x] Parsing correcto de tramas ASCII hex
- [x] Checksum LRC válido (Longitudinal Redundancy Check)
- [x] Conmutable entre RTU/ASCII en runtime
- [x] Compatible con especificación Modbus ASCII
- [x] Ejemplo de comunicación ASCII incluido

## Diferencias RTU vs ASCII

| Característica | RTU | ASCII |
|---------------|-----|-------|
| Formato | Binario | Texto hexadecimal |
| Checksum | CRC-16 | LRC |
| Eficiencia | Alta (~98%) | Media (~70%) |
| Debugging | Difícil | Fácil (legible) |
| Carácter inicio | - | `:` (0x3A) |
| Fin de trama | - | `\r\n` |

## Formato de Trama ASCII

```
:[Address][Function][Data][LRC]\r\n
Ejemplo: :010300000001F9\r\n
         :  = Inicio (0x3A)
         01 = Dirección esclavo
         03 = Función
         0000 = Registro inicial
         0001 = Cantidad de registros
         F9 = LRC
         \r\n = Fin de línea
```

## Cálculo LRC

```cpp
uint8_t calculateLRC(uint8_t* frame, uint8_t len) {
    uint8_t lrc = 0;
    for (uint8_t i = 0; i < len; i++) {
        lrc += frame[i];
    }
    return (~lrc + 1) & 0xFF;
}
```

## Archivo de Implementación

- `src/ModbusASCII.h` (691 líneas)

## Ejemplo de Uso

Ver `examples/Avanzados/Comunicacion-ASCII/`

```cpp
#include <ModbusASCII.h>

ModbusASCII mbAscii;

void setup() {
    Serial.begin(9600);
    mbAscii.begin(&Serial);
    mbAscii.setSlaveId(1);
}

void loop() {
    mbAscii.task();
}
```

## Tests Unitarios

Tests escritos en `tests/Phase3_Advanced/test_modbus_ascii.cpp`:
- test_lrc_calculation_basic
- test_ascii_frame_generation
- test_ascii_parsing
- test_invalid_frame_detection

## Criterios de Aceptación Cumplidos

- [x] Parsing correcto de tramas ASCII hex
- [x] Checksum LRC válido
- [x] Conmutable entre RTU/ASCII en runtime
- [x] Ejemplo de comunicación ASCII incluido

## Autor

Equipo de Desarrollo Modbus - 2024

## Fecha Completado

Agosto 2024
