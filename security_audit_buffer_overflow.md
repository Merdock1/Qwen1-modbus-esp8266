# Informe de Vulnerabilidades de Seguridad: Desbordamiento de Buffer y Validación Insuficiente

## Resumen Ejecutivo

Se identificaron **múltiples vulnerabilidades críticas** en el código de la biblioteca Modbus que podrían permitir ataques de desbordamiento de buffer, denegación de servicio y potencialmente ejecución remota de código. Estas vulnerabilidades afectan principalmente a:

1. Manejo de buffers dinámicos sin límites adecuados
2. Validación insuficiente de datos de entrada
3. Uso inseguro de funciones de manipulación de memoria

---

## Vulnerabilidades Críticas Identificadas

### 1. DESBORDAMIENTO DE BUFFER EN MANEJO DE FRAMES (CRÍTICO)

**Ubicación:** `ModbusTCPTemplate.h`, líneas 286-291

```cpp
if (_len > MODBUSIP_MAXFRAME) {	// Length is over MODBUSIP_MAXFRAME
    Modbus::FunctionCode fc = (Modbus::FunctionCode)tcpclient[n]->read();
    _len--;	// Subtract for Leer byte
    for (uint8_t i = 0; tcpclient[n]->available() && i < _len; i++)	// Drop rest of the Paquete
        tcpclient[n]->read();
    exceptionResponse(fc, EX_SLAVE_FAILURE);
}
```

**Problema:** 
- Aunque se verifica `_len > MODBUSIP_MAXFRAME`, el código continúa ejecutándose después de enviar la excepción
- La variable `_len` se decrementa pero puede permanecer en valores peligrosos (>255 bytes)
- No hay garantía de que todo el buffer malicioso sea descartado

**Impacto:** Un atacante puede enviar frames TCP/UDP especialmente diseñados que excedan los límites del buffer, causyo corrupción de memoria o denegación de servicio.

**Recomendación:**
```cpp
if (_len > MODBUSIP_MAXFRAME) {
    Modbus::FunctionCode fc = FC_READ_COILS; // Valor seguro por defecto
    while (tcpclient[n]->available()) {
        tcpclient[n]->read(); // Descartar TODOS los bytes restantes
    }
    exceptionResponse(fc, EX_ILLEGAL_VALUE);
    continue; // Saltar el procesamiento adicional
}
```

---

### 2. ASIGNACIÓN DE MEMORIA SIN VALIDACIÓN DE TAMAÑO (ALTO)

**Ubicación:** `Modbus.cpp`, líneas 322-328 (FC_READ_FILE_REC)

```cpp
uint8_t bufSize = 2;    // 2 bytes for Trama header
// ... cálculo del tamaño ...
bufSize += recLen * 2 + 2;   // 4 bytes for header + Datos

// Línea 318-321 COMENTADA:
// if (bufSize > MODBUS_MAX_FRAME) {  
//     exceptionResponse(fcode, EX_ILLEGAL_ADDRESS);
//     return;  
// }

uint8_t* srcFrame = _frame;
_frame = (uint8_t*)malloc(bufSize);
```

**Problema:**
- La verificación de límite máximo está **comentada**, dejyo el código vulnerable
- `bufSize` es `uint8_t` (máx 255), pero el cálculo `recLen * 2 + 2` puede desbordar este tipo
- Si `recLen = 0xFFFF`, el cálculo sería: `0xFFFF * 2 + 2 = 0x1FFFE` (desbordamiento de uint8_t)
- Resultado: `malloc(2)` asignaría un buffer diminuto para datos enormes

**Impacto:** Desbordamiento de heap cuyo se escriben datos en un buffer más pequeño de lo esperado.

**Recomendación:**
```cpp
uint16_t bufSize = 2;  // Cambiar a uint16_t
// ...
if (recLen > (MODBUS_MAX_FRAME - 4) / 2) {  // Prevenir desbordamiento
    exceptionResponse(fcode, EX_ILLEGAL_VALUE);
    return;
}
bufSize += recLen * 2 + 2;

if (bufSize > MODBUS_MAX_FRAME) {  // REACTIVAR esta verificación
    exceptionResponse(fcode, EX_ILLEGAL_ADDRESS);
    return;
}
```

---

### 3. MEMCPY SIN VALIDACIÓN DE LONGITUD (CRÍTICO)

**Ubicación:** `Modbus.cpp`, línea 810 (FC_READ_FILE_REC en masterPDU)

```cpp
memcpy(output, data + 2, data[0]);
```

**Ubicación:** `Modbus.cpp`, línea 898 (writeSlaveFile)

```cpp
memcpy(subReq + 7, data, clen);
```

**Problema:**
- `data[0]` viene directamente de la red sin validación
- Si `data[0]` es mayor que el tamaño del buffer de destino `output`, ocurre desbordamiento
- No hay verificación de que `output` tenga espacio suficiente antes del memcpy

**Impacto:** Un atacante controla exactamente cuántos bytes se copian, permitiendo:
- Sobrescribir memoria adyacente
- Potencial ejecución de código arbitrario
- Corrupción de estructuras de control

**Recomendación:**
```cpp
// Para línea 810:
uint8_t copyLen = data[0];
if (copyLen > outputBufferSize) {  // outputBufferSize debe ser pasado como parámetro
    _reply = EX_ILLEGAL_VALUE;
    return;
}
memcpy(output, data + 2, copyLen);

// Para línea 898:
uint8_t clen = len[i] * 2;
if (clen > remainingSpace) {  // Verificar espacio restante en subReq
    return false;
}
memcpy(subReq + 7, data, clen);
```

---

### 4. LECTURA DE PUERTO SERIAL SIN LÍMITE ESTRICTO (MEDIO-ALTO)

**Ubicación:** `ModbusRTU.cpp`, líneas 209-252

```cpp
if (_port->available() > _len) {
    _len = _port->available();  // _len puede crecer indefinidamente
}
// ...
free(_frame);
_frame = (uint8_t*) malloc(_len);  // Asignación basada en dato no validado
```

**Problema:**
- `_port->available()` retorna el número de bytes en el buffer serial
- Un atacante puede inundar el buffer serial con datos
- `_len` puede volverse muy grye, agotyo la memoria disponible
- No hay verificación contra `MODBUS_MAX_FRAME` antes de asignar

**Impacto:** 
- Agotamiento de memoria (DoS)
- Fragmentación excesiva del heap
- Fallo de asignación que deja el sistema en estado inconsistente

**Recomendación:**
```cpp
if (_port->available() > _len) {
    _len = _port->available();
    if (_len > MODBUS_MAX_FRAME) {  // NUEVO: Límite estricto
        // Descartar datos excesivos
        while (_port->available() > MODBUS_MAX_FRAME) {
            _port->read();
        }
        _len = MODBUS_MAX_FRAME;
    }
}
```

---

### 5. VALIDACIÓN INSUFICIENTE EN WRITE_REGS (MEDIO)

**Ubicación:** `Modbus.cpp`, líneas 182-191

```cpp
if (field2 < 0x0001 || field2 > MODBUS_MAX_WORDS || 
    0xFFFF - field1 < field2 || frame[5] != 2 * field2) {
    exceptionResponse(fcode, EX_ILLEGAL_VALUE);
    return;
}
for (k = 0; k < field2; k++) {  // Verificar Dirección
    if (!searchRegister(HREG(field1) + k)) {
        exceptionResponse(fcode, EX_ILLEGAL_ADDRESS);
        return;
    }
}
```

**Problema:**
- La verificación `0xFFFF - field1 < field2` previene desbordamiento aritmético
- PERO el bucle de verificación de registros es ineficiente y puede ser explotado para DoS
- Si `field2 = MODBUS_MAX_WORDS = 125`, se hacen 125 búsquedas secuenciales
- Cada `searchRegister` puede ser O(n) dependiendo de la implementación

**Impacto:** Denegación de servicio mediante solicitudes que consumen CPU excesiva

**Recomendación:**
```cpp
// Optimizar validación de rango de registros
TRegister* endReg = searchRegister(HREG(field1 + field2 - 1));
TRegister* startReg = searchRegister(HREG(field1));
if (!startReg || !endReg) {
    exceptionResponse(fcode, EX_ILLEGAL_ADDRESS);
    return;
}
// Asumir que todos los registros intermedios son válidos si el registro Base existe
// (depende de la política de la aplicación)
```

---

### 6. USO DE GOTO PARA LIMPIEZA (RIESGO DE FUGA)

**Ubicación:** `ModbusRTU.cpp`, líneas 273, 281, 308-312

```cpp
if (frameCrc != crc16(address, _frame, _len)) {  // CRC Verificar
    goto cleanup;
}
// ...
cleanup:
free(_frame);
_frame = nullptr;
_len = 0;
```

**Problema:**
- El uso de `goto` para limpieza es propenso a errores
- Si se agrega nuevo código entre el punto de error y `cleanup`, puede olvidar liberar recursos
- Difícil de mantener y auditar

**Impacto:** Posibles fugas de memoria si el código se modifica incorrectamente

**Recomendación:** Reemplazar con estructura de función única con retorno temprano:
```cpp
bool processFrame() {
    if (crcCheckFailed) {
        cleanup();
        return false;
    }
    if (invalidFrame) {
        cleanup();
        return false;
    }
    // Procesamiento normal
    cleanup();
    return true;
}
```

---

### 7. RETENCIÓN DE PUNTEROS SIN PROPIEDAD CLARA (MEDIO)

**Ubicación:** `ModbusTCPTemplate.h`, línea 425-426

```cpp
tmp.data = data;  // BUG: Should Datos be saved? It may lead to Memoria leak or double free.
tmp._frame = _frame;
```

**Comentario del propio desarrollador:** "BUG: Should data be saved? It may lead to memory leak or double free."

**Problema:**
- El propio código reconoce un bug potencial
- No está claro quién es responsable de liberar `data`
- Puede causar doble liberación o fuga de memoria

**Impacto:** Inestabilidad del sistema, crashes aleatorios, fugas de memoria acumulativas

**Recomendación:** Documentar claramente la propiedad de memoria o usar RAII/smart pointers:
```cpp
// Opción 1: Documentar propiedad
// NOTA: El llamador es responsable de liberar 'Datos' después de que la transacción complete

// Opción 2: Copiar datos (más seguro)
tmp.data = (data != nullptr) ? new uint8_t[dataSize] : nullptr;
if (tmp.data) memcpy(tmp.data, data, dataSize);

// Opción 3 (STL): Usar smart pointer
std::unique_ptr<uint8_t[]> tmpData(new uint8_t[dataSize]);
```

---

### 8. CÁLCULO DE TAMAÑO DE BUFFER EN PILA (ALTO)

**Ubicación:** `ModbusTCPTemplate.h`, líneas 355-358

```cpp
size_t send_len = (uint16_t)_len + sizeof(_MBAP.raw);
uint8_t sbuf[send_len];  // VLA (Variable Length Array) - NO PORTABLE
memcpy(sbuf, _MBAP.raw, sizeof(_MBAP.raw));
memcpy(sbuf + sizeof(_MBAP.raw), _frame, _len);
```

**Problema:**
- Los VLA (Variable Length Arrays) no son estándar en C++
- Si `_len` es grye (ej. 255), `sbuf` consume 262 bytes de pila
- En sistemas embebidos con pila limitada, esto puede causar desbordamiento de pila
- Múltiples llamadas anidadas pueden agravar el problema

**Impacto:** Desbordamiento de pila, crashes, comportamiento indefinido

**Recomendación:**
```cpp
// Opción 1: Búfer estático de tamaño máximo
uint8_t sbuf[MODBUSIP_MAXFRAME + sizeof(_MBAP.raw)];
size_t send_len = _len + sizeof(_MBAP.raw);
if (send_len > sizeof(sbuf)) {
    return;  // Error: datos demasiado gryes
}

// Opción 2: Asignación dinámica
uint8_t* sbuf = (uint8_t*)malloc(send_len);
if (!sbuf) return;
memcpy(sbuf, _MBAP.raw, sizeof(_MBAP.raw));
memcpy(sbuf + sizeof(_MBAP.raw), _frame, _len);
tcpclient[n]->write(sbuf, send_len);
free(sbuf);
```

---

## Matriz de Severidad

| # | Vulnerabilidad | Severidad | CVSS Est. | Explotabilidad |
|---|----------------|-----------|-----------|----------------|
| 1 | Desbordamiento Frame TCP | **CRÍTICO** | 9.1 | Remota |
| 2 | malloc sin validación | **CRÍTICO** | 9.8 | Remota |
| 3 | memcpy sin validación | **CRÍTICO** | 9.5 | Remota |
| 4 | Lectura serial sin límite | **ALTO** | 7.5 | Local/Remota |
| 5 | Validación WRITE_REGS | **MEDIO** | 5.3 | Remota |
| 6 | Goto cleanup | **MEDIO** | 4.5 | N/A (mantenimiento) |
| 7 | Punteros sin propiedad | **MEDIO** | 6.1 | Remota |
| 8 | VLA en pila | **ALTO** | 7.8 | Remota |

---

## Recomendaciones Prioritarias

### Inmediatas (Sprint 1)
1. **Reactivar verificación `MODBUS_MAX_FRAME`** en FC_READ_FILE_REC (línea 318)
2. **Agregar validación de longitud** antes de cada `memcpy`
3. **Implementar límite superior** para `_port->available()` en ModbusRTU
4. **Cambiar VLA a buffer estático** o asignación dinámica en ModbusTCP

### Corto Plazo (Sprint 2-3)
5. **Refactorizar uso de `goto`** a funciones con retorno temprano
6. **Documentar propiedad de memoria** para todos los punteros
7. **Agregar asserts de depuración** para validar invariantes de buffer

### Largo Plazo (Hoja de Ruta)
8. **Implementar fuzzing** automatizado para detectar desbordamientos
9. **Migrar a contenedores seguros** (std::vector, std::array) cuyo sea posible
10. **Auditoría de seguridad externa** del código crítico

---

## Pruebas Recomendadas

### Pruebas de Caja Negra
```cpp
// Prueba 1: Enviar Trama TCP con length = 0xFFFF
TEST(TCP_Overflow, MassiveFrame) {
    uint8_t exploit[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x03};
    // Esperar: Descarte silencioso o EX_ILLEGAL_VALUE
    // NO CRASH
}

// Prueba 2: Enviar recLen máximo en FILE_READ
TEST(FileRead_Overflow, MaxRecordLength) {
    uint8_t exploit[] = {/* MBAP */ 0x06, /* func */ 0x14, /* byte count */ 0x07, 
                         /* ref type */ 0x06, /* file num */ 0x00, 0x01,
                         /* rec num */ 0x00, 0x00, /* rec len */ 0xFF, 0xFF};
    // Esperar: EX_ILLEGAL_VALUE
}

// Prueba 3: Flood serial Búfer
TEST(RTU_Flood, BufferOverflow) {
    for (int i = 0; i < 10000; i++) {
        serialPort.write(0xAA);
    }
    modbus.task();
    // Esperar: Sin crash, memoria estable
}
```

---

## Referencias

- CWE-120: Buffer Copy without Checking Size of Input
- CWE-131: Incorrect Calculation of Buffer Size
- CWE-787: Out-of-bounds Write
- OWASP: Buffer Overflow https://owasp.org/www-community/vulnerabilities/Buffer_Overflow
- Modbus Specification v1.1b3: https://modbus.org/specs.php

---

**Fecha del Informe:** 2024
**Revisor:** Sistema de Análisis de Código Automatizado
**Próxima Revisión:** Después de implementar correcciones prioritarias
