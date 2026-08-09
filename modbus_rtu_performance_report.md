# Informe de Rendimiento y Optimización - Protocoloo Modbus RTU

## Resumen Ejecutivo

Este informe analiza la implementación del protocolo **Modbus RTU** en la biblioteca modbus-esp8266 (v4.1.0), identificyo áreas de mejora en rendimiento, optimización de código y oportunidades de refactorización.

---

## 1. Análisis de Rendimiento Actual

### 1.1 Métricas de Tiempo Crítico

| Parámetro | Valor Actual | Impacto |
|-----------|--------------|---------|
| `MODBUSRTU_TIMEOUT_US` | 1,000,000 µs (1 seg) | Timeout para respuestas maestras |
| `MODBUSRTU_MAX_READ_US` | 100,000 µs (100 ms) | Límite máximo de lectura slave |
| `MODBUSRTU_REDE_SWITCH_US` | 1,000 µs (1 ms) | Retardo cambio RE/DE |
| Inter-frame time (>19200 baud) | 1,750 µs fijo | Espera entre frames |
| Inter-frame time (≤19200 baud) | 3.5 × tiempo carácter | Calculado dinámicamente |

### 1.2 Puntos Críticos de Rendimiento Identificados

#### 🔴 CRÍTICO: Bucle de Lectura Serial sin Límite Estricto

**Ubicación:** `ModbusRTU.cpp:209-233`

```cpp
void ModbusRTUTemplate::task() {
    if (_port->available() > _len) {
        _len = _port->available();  // ⚠️ PROBLEMA: Disponible() puede crecer indefinidamente
        t = micros();
    }
    // ...
    while (micros() - t < _t) {
        if (_port->available() > _len) {
            _len = _port->available();  // ⚠️ Sin validación de límite máximo
            t = micros();
        }
        if (micros() - taskStart > MODBUSRTU_MAX_READ_US) {
            return;  // ✅ Única protección existente
        }
    }
}
```

**Problema:** 
- `_port->available()` devuelve bytes en buffer serial, pero no hay límite máximo explícito
- En caso de ruido o flood de datos, `_len` puede crecer hasta agotar memoria RAM
- La asignación `malloc(_len)` en línea 252 falla silenciosamente si `_len` es muy grye

**Impacto:** 
- Agotamiento de memoria en microcontroladores con RAM limitada (Arduino Uno: 2KB, ESP8266: 80KB)
- Posible crash del sistema por `malloc()` fallido

**Recomendación:**
```cpp
#define MODBUSRTU_MAX_FRAME_LEN 256  // Añadir constante de configuración

if (_port->available() > _len) {
    uint8_t available = _port->available();
    if (available > MODBUSRTU_MAX_FRAME_LEN) {
        // Descartar datos excesivos
        while (_port->available()) _port->read();
        _len = 0;
        return;
    }
    _len = available;
    t = micros();
}
```

---

#### 🟡 ALTO: Uso de `goto` para Limpieza de Recursos

**Ubicación:** `ModbusRTU.cpp:273, 281, 308-312`

```cpp
if (frameCrc != crc16(address, _frame, _len)) {
    goto cleanup;  // ⚠️ Patrón propenso a errores
}
// ...
if (!valid_frame && _reply != EX_FORCE_PROCESS) {
    goto cleanup;
}
// ...
cleanup:
free(_frame);
_frame = nullptr;
_len = 0;
if (isMaster) cleanup();
```

**Problema:**
- Aunque funcional, el uso de `goto` dificulta el mantenimiento
- Riesgo de fugas de memoria si se agregan nuevos puntos de salida sin actualizar la etiqueta `cleanup`
- Práctica desaconsejada en C++ moderno

**Recomendación:**
```cpp
bool ModbusRTUTemplate::processFrame() {
    // Validar CRC
    if (frameCrc != crc16(address, _frame, _len)) {
        releaseFrame();
        return false;
    }
    
    // Procesar Trama
    // ...
    
    releaseFrame();
    return true;
}

inline void ModbusRTUTemplate::releaseFrame() {
    free(_frame);
    _frame = nullptr;
    _len = 0;
}
```

---

#### 🟡 ALTO: Asignación de Memoria en Cada Frame

**Ubicación:** `ModbusRTU.cpp:251-258`

```cpp
free(_frame);  // Just in case
_frame = (uint8_t*) malloc(_len);
if (!_frame) {
    for (uint8_t i=0 ; i < _len ; i++) _port->read();
    _len = 0;
    if (isMaster) cleanup();
    return;
}
```

**Problema:**
- `malloc()` y `free()` en cada frame recibido causa:
  - Fragmentación de heap en sistemas embebidos
  - Overhead de tiempo de ejecución (~10-50 µs por operación)
  - Posible fallo en sistemas con heap fragmentado

**Recomendación:**
```cpp
// Opción 1: Búfer estático pre-asignado
static uint8_t _staticBuffer[MODBUSRTU_MAX_FRAME_LEN];
_frame = _staticBuffer;

// Opción 2: Pool de buffers reutilizable
class ModbusRTUTemplate {
    uint8_t _bufferPool[2][MODBUSRTU_MAX_FRAME_LEN];
    uint8_t _currentBufferIndex;
    
    uint8_t* acquireBuffer(uint8_t size) {
        if (size > MODBUSRTU_MAX_FRAME_LEN) return nullptr;
        _currentBufferIndex = !_currentBufferIndex;
        return _bufferPool[_currentBufferIndex];
    }
};
```

---

#### 🟠 MEDIO: Cálculo de CRC con Acceso a PROGMEM

**Ubicación:** `ModbusRTU.cpp:32-44`

```cpp
uint16_t ModbusRTUTemplate::crc16(uint8_t address, uint8_t* frame, uint8_t pduLen) {
    uint8_t i = 0xFF ^ address;
    uint16_t val = pgm_read_word(_auchCRC + i);  // ⚠️ Acceso a flash en cada iteración
    uint8_t CRCHi = 0xFF ^ highByte(val);
    uint8_t CRCLo = lowByte(val);
    while (pduLen--) {
        i = CRCHi ^ *frame++;
        val = pgm_read_word(_auchCRC + i);  // ⚠️ 2 accesos a flash por byte
        CRCHi = CRCLo ^ highByte(val);
        CRCLo = lowByte(val);
    }
    return (CRCHi << 8) | CRCLo;
}
```

**Análisis:**
- ✅ **Ventaja:** Tabla CRC en PROGMEM ahorra RAM (~512 bytes)
- ⚠️ **Desventaja:** `pgm_read_word()` es más lento que acceso a RAM (2-4 ciclos adicionales)
- Para ESP32/ESP8266: impacto mínimo (flash mapeada en espacio de direcciones)
- Para AVR (Uno/Nano): impacto significativo (~15-20% más lento)

**Recomendación por Plataforma:**

```cpp
#if defined(ESP32) || defined(ESP8266)
    // Mantener en PROGMEM (sin impacto significativo)
    static const uint16_t _auchCRC[] PROGMEM = {...};
    val = pgm_read_word(_auchCRC + i);
    
#elif defined(ARDUINO_ARCH_AVR)
    // Opción 1: Usar algoritmo bit-a-bit sin tabla (ahorra 512B flash)
    return crc16_alt(address, frame, pduLen);
    
    // Opción 2: Tabla en RAM para máximo rendimiento
    static const uint16_t _auchCRC[] = {...};  // Sin PROGMEM
    val = _auchCRC[i];  // Acceso directo
    
#else
    // ARM Cortex-M (Due, Zero, RP2040): mantener en PROGMEM
    static const uint16_t _auchCRC[] PROGMEM = {...};
    val = pgm_read_word(_auchCRC + i);
#endif
```

---

#### 🟠 MEDIO: Delay Fijo en Cambio RE/DE

**Ubicación:** `ModbusRTU.cpp:142-150, 163-178`

```cpp
#if !defined(ESP32)
    delayMicroseconds(MODBUSRTU_REDE_SWITCH_US);  // 1000 µs fijos
#endif
```

**Problema:**
- 1ms es excesivo para la mayoría de transceptores RS-485
- MAX485: typical 100ns, maximum 500ns
- SN75176: typical 200ns
- Retardo innecesario reduce throughput ~1-2%

**Recomendación:**
```cpp
// Configurable según hardware
#ifndef MODBUSRTU_REDE_SWITCH_US
    #if defined(FAST_TRANSCEIVER)  // Usuario define si usa transceptores rápidos
        #define MODBUSRTU_REDE_SWITCH_US 100  // 100 µs suficiente
    #else
        #define MODBUSRTU_REDE_SWITCH_US 500  // 500 µs conservador
    #endif
#endif

// Eliminar dependencia de plataforma
digitalWrite(_txEnablePin, _direct?HIGH:LOW);
delayMicroseconds(MODBUSRTU_REDE_SWITCH_US);
```

---

#### 🟢 BAJO: Lectura Byte-a-Byte vs readBytes()

**Ubicación:** `ModbusRTU.cpp:259-269`

```cpp
for (uint8_t i=0 ; i < _len ; i++) {
    _frame[i] = _port->read();
    #if defined(MODBUSRTU_DEBUG)
    Serial.print(_frame[i], HEX);
    Serial.print(" ");
    #endif
}
//_port->readBytes(_frame, _len);  // ← Comentado
```

**Análisis:**
- ✅ Bucle manual permite debug byte-a-byte
- ⚠️ `readBytes()` sería más eficiente (llamada única vs N llamadas)
- Diferencia de rendimiento: ~5-10 µs para frame típico (10 bytes)

**Recomendación:**
```cpp
#if defined(MODBUSRTU_DEBUG)
    for (uint8_t i=0 ; i < _len ; i++) {
        _frame[i] = _port->read();
        Serial.print(_frame[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
#else
    size_t read = _port->readBytes(_frame, _len);  // Más eficiente
    if (read != _len) {
        // Manejar Tiempo de espera de lectura
        _len = read;
    }
#endif
```

---

#### 🟢 BAJO: Verificación de SlaveId Ineficiente

**Ubicación:** `ModbusRTU.cpp:236-249`

```cpp
address = _port->read();  // first byte of Trama = Dirección
_len--;
if (isMaster && _slaveId == 0) {
    valid_frame = false;
}
if (address != MODBUSRTU_BROADCAST && address != _slaveId) {
    valid_frame = false;
}
if (!valid_frame && !_cbRaw) {
    for (uint8_t i=0 ; i < _len ; i++) _port->read();  // ⚠️ Leer byte-a-byte para descartar
    _len = 0;
    if (isMaster) cleanup();
    return;
}
```

**Problema:**
- Se lee todo el frame a memoria antes de validar SlaveId
- Si SlaveId es inválido, se descarta frame completo después de leerlo

**Recomendación (Optimización Temprana):**
```cpp
// Leer solo primer byte (SlaveId)
address = _port->read();
_len--;

// Validación temprana antes de leer resto del Trama
bool valid_slave = (isMaster && _slaveId != 0) || 
                   (address == MODBUSRTU_BROADCAST || address == _slaveId);

if (!valid_slave && !_cbRaw) {
    // Descartar resto del Trama inmediatamente
    while (_port->available()) _port->read();
    _len = 0;
    if (isMaster) cleanup();
    return;
}

// Solo leer Trama completo si SlaveId es válido
// ... resto del procesamiento
```

**Beneficio:** Ahorra ~50-100 µs por frame inválido + reduce uso de CPU.

---

## 2. Optimizaciones Específicas por Plataforma

### 2.1 ESP32

**Características:**
- Dual-core, FreeRTOS, 80-240 MHz
- Flash mapeada en espacio de direcciones

**Optimizaciones Recomendadas:**

```cpp
// 1. Usar segundo core para tarea Modbus
void ModbusRTUTemplate::begin(Stream* port, ...) {
    // ...
    xTaskCreatePinnedToCore(
        taskWrapper,      // Función de tarea
        "ModbusRTU",      // Nombre
        4096,             // Pila size
        this,             // Parámetro
        5,                // Prioridad
        &_taskHyle,     // Hyle
        1                 // Core 1 (no bloquear WiFi en Core 0)
    );
}

// 2. Reducir vTaskDelay(0) innecesarios
#if defined(ESP32)
    // Mantener solo en puntos críticos de bloqueo
    if (_port->available() == 0) {
        vTaskDelay(1);  // Ceder CPU cuyo no hay datos
    }
#endif
```

### 2.2 ESP8266

**Características:**
- Single-core, 80-160 MHz
- watchdog integrado (WDT)

**Optimizaciones Recomendadas:**

```cpp
// 1. Alimentar watchdog en bucles largos
void ModbusRTUTemplate::task() {
    while (micros() - t < _t) {
        if (_port->available() > _len) {
            // ...
        }
        yield();  // ✅ Alimentar WDT
        if (micros() - taskStart > MODBUSRTU_MAX_READ_US) {
            return;
        }
    }
}

// 2. Evitar alocações gryes durante WiFi activo
// Usar Búfer estático en lugar de malloc
```

### 2.3 AVR (Uno, Nano, Mega)

**Características:**
- 16 MHz, 2-8 KB RAM, 32 KB Flash
- Sin MMU, sin RTOS

**Optimizaciones Recomendadas:**

```cpp
// 1. Usar CRC bit-a-bit para ahorrar 512B de tabla
uint16_t crc16(uint8_t address, uint8_t* frame, uint8_t pduLen) {
    return crc16_alt(address, frame, pduLen);  // Implementación sin tabla
}

// 2. Limitar estrictamente tamaño de Búfer
#define MODBUSRTU_MAX_FRAME_LEN 128  // AVR tiene RAM limitada

// 3. Usar interrupciones serial si disponible
// (requiere modificación mayor de arquitectura)
```

### 2.4 ARM Cortex-M (Due, Zero, RP2040)

**Características:**
- 48-200 MHz, ample RAM
- Hardware UART con FIFO

**Optimizaciones Recomendadas:**

```cpp
// 1. Aprovechar FIFO hardware con lectura por lotes
#if defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_RP2040)
    if (_port->available() >= 8) {  // Umbral mínimo de Trama válido
        _len = min(_port->available(), MODBUSRTU_MAX_FRAME_LEN);
        t = micros();
    }
#endif

// 2. Usar DMA si disponible (RP2040)
// Requiere soporte de núcleo Arduino
```

---

## 3. Matriz de Optimización

| Optimización | Impacto | Complejidad | Prioridad |
|--------------|---------|-------------|-----------|
| Límite estricto en `_port->available()` | Alto | Baja | 🔴 CRÍTICA |
| Buffer estático vs malloc | Alto | Media | 🟡 ALTA |
| Validación temprana de SlaveId | Medio | Baja | 🟡 ALTA |
| Refactorizar `goto` | Medio | Media | 🟡 ALTA |
| CRC optimizado por plataforma | Medio | Media | 🟠 MEDIA |
| Reducir delay RE/DE | Bajo | Baja | 🟠 MEDIA |
| Usar `readBytes()` | Bajo | Baja | 🟢 BAJA |
| Tarea en core separado (ESP32) | Medio | Alta | 🟢 BAJA |

---

## 4. Recomendaciones de Implementación

### 4.1 Cambios Inmediatos (Sprint 1)

1. **Agregar constante de límite de frame:**
```cpp
// ModbusConfiguración.h
#ifndef MODBUSRTU_MAX_FRAME_LEN
#define MODBUSRTU_MAX_FRAME_LEN 256
#endif

// ModbusRTU.cpp
if (_port->available() > MODBUSRTU_MAX_FRAME_LEN) {
    while (_port->available()) _port->read();
    _len = 0;
    return;
}
```

2. **Validación temprana de SlaveId:**
```cpp
address = _port->read();
_len--;

if ((isMaster && _slaveId == 0) || 
    (address != MODBUSRTU_BROADCAST && address != _slaveId)) {
    if (!_cbRaw) {
        while (_port->available()) _port->read();
        _len = 0;
        return;
    }
}
```

3. **Reducir delay RE/DE:**
```cpp
// ModbusConfiguración.h
#undef MODBUSRTU_REDE_SWITCH_US
#define MODBUSRTU_REDE_SWITCH_US 200  // 200 µs suficiente
```

### 4.2 Mejoras a Mediano Plazo (Sprint 2-3)

1. **Pool de buffers reutilizable:**
```cpp
class ModbusRTUTemplate {
protected:
    uint8_t _rxBuffer[MODBUSRTU_MAX_FRAME_LEN];
    uint8_t _txBuffer[MODBUSRTU_MAX_FRAME_LEN];
    bool _rxBufferInUse = false;
    
    uint8_t* acquireRxBuffer(uint8_t minSize) {
        if (_rxBufferInUse || minSize > MODBUSRTU_MAX_FRAME_LEN) {
            return nullptr;
        }
        _rxBufferInUse = true;
        return _rxBuffer;
    }
    
    void releaseRxBuffer() {
        _rxBufferInUse = false;
    }
};
```

2. **Refactorización de `cleanup()`:**
```cpp
void ModbusRTUTemplate::releaseFrame() {
    if (_frame) {
        free(_frame);
        _frame = nullptr;
    }
    _len = 0;
}

void ModbusRTUTemplate::task() {
    // ...
    if (crc_invalid) {
        releaseFrame();
        return;
    }
    // ...
    releaseFrame();
}
```

### 4.3 Optimizaciones Avanzadas (Futuro)

1. **Soporte para DMA (RP2040, ESP32-S3):**
   - Transferencia directa UART → RAM sin CPU
   - Reducción de overhead de interrupciones

2. **Modo sleep entre frames:**
   - Entrar en deep sleep durante inter-frame time
   - Wake-up por actividad UART

3. **Precomputación de CRC para frames comunes:**
   - Cache de CRC para requests frecuentes
   - Útil en aplicaciones con patrones de comunicación repetitivos

---

## 5. Benchmarks Estimados

### Escenario: Frame RTU típico (10 bytes) a 9600 baud

| Operación | Tiempo Actual | Tiempo Optimizado | Mejora |
|-----------|---------------|-------------------|--------|
| Lectura completa frame | ~1,200 µs | ~1,150 µs | 4% |
| Cálculo CRC (AVR) | ~180 µs | ~150 µs (bit-a-bit) | 17% |
| Cálculo CRC (ESP32) | ~40 µs | ~35 µs | 12% |
| malloc/free | ~30 µs | 0 µs (buffer estático) | 100% |
| Delay RE/DE | 1,000 µs | 200 µs | 80% |
| **Total por frame** | **~2,450 µs** | **~1,535 µs** | **~37%** |

### Throughput Máximo Teórico

| Configuración | Frames/seg | Bytes/seg |
|---------------|------------|-----------|
| Actual (9600 baud) | ~408 | ~4,080 |
| Optimizado (9600 baud) | ~651 | ~6,510 |
| Optimizado (115200 baud) | ~2,800 | ~28,000 |

---

## 6. Conclusiones

La implementación actual de Modbus RTU es **funcional y robusta**, pero presenta oportunidades significativas de optimización:

### Fortalezas:
- ✅ Correcta implementación del protocolo Modbus RTU
- ✅ Soporte multi-plataforma bien estructurado
- ✅ Manejo adecuado de timeouts y limpieza de recursos
- ✅ Flexibilidad mediante callbacks y configuración

### Áreas de Mejora Prioritarias:
1. 🔴 **Seguridad de memoria**: Agregar límites estrictos a lecturas seriales
2. 🟡 **Rendimiento**: Eliminar malloc/free en ruta crítica
3. 🟡 **Mantenibilidad**: Refactorizar uso de `goto`
4. 🟠 **Eficiencia**: Optimizar delays y validaciones tempranas

### Impacto Esperado:
- **37% reducción** en tiempo de procesamiento por frame
- **60% aumento** en throughput máximo
- **Mejor estabilidad** en sistemas con RAM limitada
- **Código más mantenible** y portable

---

## 7. Pruebas Recomendadas

Antes de implementar cambios, ejecutar:

```bash
# Prueba de stress de memoria
test_memory_stress.ino:
  - Enviar 10,000 frames consecutivos
  - Monitorear heap free
  - Verificar ausencia de fragmentation

# Prueba de rendimiento
test_throughput.ino:
  - Medir tiempo entre request y response
  - Comparar frames/segundo antes/después
  
# Prueba de compatibilidad
test_compatibility.ino:
  - Comunicar con dispositivos Modbus reales
  - Verificar CRC correcto en todos los casos
```

---

**Documento generado:** 2024
**Versión biblioteca analizada:** 4.1.0
**Archivos revisados:** ModbusRTU.h, ModbusRTU.cpp, ModbusConfiguración.h
