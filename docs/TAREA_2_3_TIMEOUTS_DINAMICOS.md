# Tarea 2.3: Timeouts Dinámicos - Documentación

## Descripción General

Esta implementación añade soporte para **timeouts dinámicos** que se ajustan automáticamente según el baudrate configurado, mejorando la fiabilidad de la comunicación Modbus RTU en diferentes configuraciones de velocidad serial.

## Archivos Modificados

- `src/ModbusRTU.h` - Declaración de nuevas APIs y variables de estado
- `src/ModbusRTU.cpp` - Implementación de lógica de cálculo de timeouts

## Características Implementadas

### 1. Cálculo Automático de Timeouts

El sistema calcula automáticamente los timeouts basándose en el baudrate real utilizando la fórmula:

```
Timeout = Inter-Frame Time × Multiplicador de Seguridad
Inter-Frame Time = 3.5 × Tiempo de Carácter (para baudrates ≤ 19200)
Inter-Frame Time = 1750 µs (para baudrates > 19200)
Tiempo de Carácter = (Bits por Carácter × 1,000,000) / Baudrate
```

### 2. Rango de Baudrates Soportado

La implementación soporta el rango completo especificado en los criterios de aceptación:
- **Mínimo:** 1200 baud
- **Máximo:** 921600 baud
- **Validación:** Rechaza automáticamente valores fuera de rango

### 3. API Pública Nueva

#### Métodos Principales

```cpp
/**
 * @brief Configura el baudrate y calcula automáticamente los timeouts
 * @param baud Baudrate deseado (1200-921600)
 * @return true si exitoso, false si baudrate inválido
 */
bool setBaudrate(uint32_t baud);

/**
 * @brief Obtiene el baudrate actual configurado
 * @return Baudrate actual en bps
 */
uint32_t getCurrentBaudrate() const;

/**
 * @brief Habilita o deshabilita el cálculo automático de timeouts
 * @param enable true para habilitar, false para deshabilitar
 */
void enableAutoTimeout(bool enable);

/**
 * @brief Configura timeout personalizado (deshabilita auto-ajuste)
 * @param timeout_us Timeout en microsegundos
 */
void setTimeout(uint32_t timeout_us);

/**
 * @brief Obtiene el timeout actual configurado
 * @return Timeout en microsegundos
 */
uint32_t getTimeout() const;
```

#### Métodos de Cálculo (públicos para testing)

```cpp
/**
 * @brief Calcula el tiempo de transmisión de un carácter
 * @param baud Baudrate del puerto
 * @param char_bits Tamaño de carácter en bits (default 11)
 * @return Tiempo en microsegundos
 */
uint32_t charSendTime(uint32_t baud, uint8_t char_bits = 11);

/**
 * @brief Calcula el tiempo de inter-frame mínimo
 * @param baud Baudrate del puerto serial
 * @param char_bits Tamaño de carácter (default 11 bits)
 * @return Tiempo en microsegundos
 */
uint32_t calculateMinimumInterFrameTime(uint32_t baud, uint8_t char_bits = 11);

/**
 * @brief Establece manualmente el tiempo de inter-frame
 * @param t_us Tiempo en microsegundos
 */
void setInterFrameTime(uint32_t t_us);
```

## Variables Internas Nuevas

```cpp
uint32_t _currentBaudrate = 0;    // Baudrate actualmente configurado
uint32_t _timeoutBase = 0;        // Timeout base calculado
uint32_t _charTime = 0;           // Tiempo de carácter calculado
bool _autoTimeoutEnabled = true;  // Estado del auto-ajuste de timeout
```

## Constantes Definidas

```cpp
#define MODBUS_MIN_BAUDRATE 1200           // Baudrate mínimo válido
#define MODBUS_MAX_BAUDRATE 921600         // Baudrate máximo válido
#define MODBUS_TIMEOUT_MULTIPLIER 3        // Margen de seguridad para timeout
```

## Ejemplos de Uso

### Ejemplo 1: Configuración Básica con Auto-Timeout

```cpp
#include <ModbusRTU.h>

ModbusRTU modbus;

void setup() {
    Serial.begin(9600);
    
    // El baudrate se detecta automáticamente si está disponible
    modbus.begin(&Serial, -1, true);
    
    // El timeout se calcula automáticamente basado en el baudrate
    // Para 9600 baud: timeout ≈ 12030 µs (3.5 × 1145 × 3)
    
    modbus.slave(1);
}

void loop() {
    modbus.task();
}
```

### Ejemplo 2: Cambio Dinámico de Baudrate

```cpp
#include <ModbusRTU.h>

ModbusRTU modbus;

void setup() {
    Serial.begin(9600);
    modbus.begin(&Serial, -1, true);
    modbus.slave(1);
    
    // Operar a 9600 baud inicialmente
    Serial.println("Operando a 9600 baud");
}

void cambiarABaudrateAlto() {
    // Cambiar a baudrate más alto para mayor velocidad
    if (modbus.setBaudrate(115200)) {
        Serial.println("Cambiado a 115200 baud exitosamente");
        Serial.print("Nuevo timeout: ");
        Serial.print(modbus.getTimeout());
        Serial.println(" µs");
    }
}

void cambiarABaudrateBajo() {
    // Cambiar a baudrate bajo para mayor distancia/ruido
    if (modbus.setBaudrate(2400)) {
        Serial.println("Cambiado a 2400 baud exitosamente");
        Serial.print("Nuevo timeout: ");
        Serial.print(modbus.getTimeout());
        Serial.println(" µs");
    }
}

void loop() {
    modbus.task();
}
```

### Ejemplo 3: Timeout Manual Personalizado

```cpp
#include <ModbusRTU.h>

ModbusRTU modbus;

void setup() {
    Serial.begin(9600);
    modbus.begin(&Serial, -1, true);
    modbus.slave(1);
    
    // Deshabilitar auto-timeout y configurar valor personalizado
    modbus.setTimeout(50000);  // 50ms fijos
    
    // Este timeout NO cambiará aunque se modifique el baudrate
    modbus.setBaudrate(115200);
    
    Serial.print("Timeout manual: ");
    Serial.print(modbus.getTimeout());
    Serial.println(" µs");
}

void loop() {
    modbus.task();
}
```

### Ejemplo 4: Rehabilitar Auto-Timeout

```cpp
#include <ModbusRTU.h>

ModbusRTU modbus;

void setup() {
    Serial.begin(9600);
    modbus.begin(&Serial, -1, true);
    modbus.slave(1);
    
    // Configurar timeout manual temporalmente
    modbus.setTimeout(100000);  // 100ms
    
    // ... operaciones especiales que requieren timeout largo ...
    
    // Rehabilitar auto-timeout
    modbus.enableAutoTimeout(true);
    
    // Ahora el timeout se recalculará con el próximo setBaudrate
    modbus.setBaudrate(19200);
    
    Serial.print("Timeout auto-recalculado: ");
    Serial.print(modbus.getTimeout());
    Serial.println(" µs");
}

void loop() {
    modbus.task();
}
```

### Ejemplo 5: Validación de Baudrate con Logging

```cpp
#include <ModbusRTU.h>

ModbusRTU modbus;

void securityLogCallback(SecurityEvent_t* event) {
    Serial.print("Evento de seguridad: ");
    Serial.println(event->description);
}

void setup() {
    Serial.begin(9600);
    
    // Configurar logging de seguridad
    SecurityConfig_t config = modbus.getSecurityConfig();
    config.enableLogging = true;
    config.logCallback = securityLogCallback;
    modbus.setSecurityConfig(config);
    
    modbus.begin(&Serial, -1, true);
    modbus.slave(1);
    
    // Intentar baudrate inválido (< 1200)
    if (!modbus.setBaudrate(600)) {
        Serial.println("Baudrate rechazado - fuera de rango válido");
    }
    
    // Intentar baudrate válido
    if (modbus.setBaudrate(57600)) {
        Serial.println("Baudrate aceptado");
    }
}

void loop() {
    modbus.task();
}
```

## Tabla de Valores de Timeout por Baudrate

| Baudrate | Tiempo Carácter (µs) | Inter-Frame (µs) | Timeout (µs) |
|----------|---------------------|------------------|--------------|
| 1200     | 9166                | 32083            | 96250        |
| 2400     | 4583                | 16041            | 48125        |
| 4800     | 2291                | 8020             | 24062        |
| 9600     | 1145                | 4010             | 12030        |
| 19200    | 572                 | 2004             | 6012         |
| 38400    | 286                 | 1750*            | 5250         |
| 57600    | 190                 | 1750*            | 5250         |
| 115200   | 95                  | 1750*            | 5250         |
| 230400   | 47                  | 1750*            | 5250         |
| 460800   | 23                  | 1750*            | 5250         |
| 921600   | 11                  | 1750*            | 5250         |

\* Para baudrates > 19200, el inter-frame time es fijo en 1750 µs según especificación Modbus

## Criterios de Aceptación Cumplidos

✅ **Timeout se ajusta automáticamente al cambiar baudrate**
- Implementado mediante `setBaudrate()` que recalcula todos los timeouts

✅ **Soporte para 1200-921600 baud**
- Validación de rango implementada
- Retorna `false` para valores fuera de rango
- Genera evento de seguridad cuando logging está habilitado

✅ **Sin falsos positivos/negativos en tests**
- Tests unitarios exhaustivos incluidos
- Validación precisa de límites
- Consistencia verificada en múltiples llamadas

## Compatibilidad hacia Atrás

La implementación mantiene **compatibilidad total** con código existente:

1. **Método `setBaudrate()` sobrecargado**: Ahora retorna `bool` pero el comportamiento anterior se mantiene
2. **Valores por defecto conservadores**: `_autoTimeoutEnabled = true` por defecto
3. **API existente preservada**: `setInterFrameTime()`, `calculateMinimumInterFrameTime()`, `charSendTime()` continúan funcionando igual

## Testing

Los tests unitarios están ubicados en `test/test_timeouts_dinamicos.cpp` e incluyen:

- Test 2.3.1: Cálculo de tiempo de carácter
- Test 2.3.2: Cálculo de inter-frame time
- Test 2.3.4: Validación de rango de baudrate
- Test 2.3.5: Baudrates válidos en límites
- Test 2.3.6: Ajuste automático de timeout
- Test 2.3.7: Timeout manual deshabilita auto-ajuste
- Test 2.3.8: Habilitar/deshabilitar auto-timeout
- Test 2.3.9: Diferentes tamaños de carácter
- Test 2.3.10: Integración con begin()
- Test 2.3.11: Sin falsos positivos
- Test 2.3.12: Consistencia de resultados

## Notas de Implementación

### Multiplicador de Seguridad

Se utiliza un multiplicador de **3x** sobre el inter-frame time para el timeout base. Esto proporciona un margen adecuado para:
- Variaciones en la precisión del reloj
- Retardos de procesamiento
- Ruido en la línea de comunicación

El multiplicador puede ajustarse modificando `MODBUS_TIMEOUT_MULTIPLIER` en `ModbusRTU.cpp`.

### Integración con Buffer Pool

El cálculo de timeout funciona independientemente del buffer pool, permitiendo optimizaciones de rendimiento adicionales sin interferencia.

### Logging de Seguridad

Cuando se configura un callback de logging, el sistema reporta:
- Intentos de configurar baudrates fuera de rango (`SEC_EVENT_INVALID_CONFIG`)
- Cambios exitosos de configuración (`SEC_EVENT_CONFIG_CHANGED`)

## Referencias

- Especificación Modbus RTU: https://modbus.org/specs.php
- Tarea 2.3: FASE 2 - OPTIMIZACIÓN DE RENDIMIENTO
- Documentación relacionada: `docs/API_ES.md` (pendiente de actualización completa)
