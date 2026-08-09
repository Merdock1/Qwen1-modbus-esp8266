# Guía de Hardening y Seguridad para Modbus

## Índice

1. [Introducción a la Seguridad Modbus](#introducción)
2. [Vulnerabilidades Comunes](#vulnerabilidades-comunes)
3. [Hardening de Configuración](#hardening-de-configuración)
4. [Validación de Tramas](#validación-de-tramas)
5. [Protección contra DoS](#protección-contra-dos)
6. [Seguridad en Red](#seguridad-en-red)
7. [Modbus TCP Security (TLS)](#modbus-tcp-security)
8. [Auditoría y Logging](#auditoría-y-logging)
9. [Checklist de Seguridad](#checklist-de-seguridad)

---

## Introducción

### ¿Por qué es importante la seguridad en Modbus?

El protocolo Modbus fue diseñado en 1979 para entornos industriales aislados. **No incluye características de seguridad nativas**:

- ❌ Sin autenticación
- ❌ Sin cifrado
- ❌ Sin integridad de datos
- ❌ IDs fáciles de spoofear

En entornos modernos conectados a Internet, esto representa riesgos significativos:

```
┌─────────────────────────────────────────────────────┐
│              ESCENARIOS DE ATAQUE                    │
├─────────────────────────────────────────────────────┤
│ • Lectura no autorizada de datos sensibles          │
│ • Escritura maliciosa de registros (sabotaje)       │
│ • Denegación de servicio (DoS)                      │
│ • Man-in-the-middle (MITM)                          │
│ • Replay attacks                                    │
│ • Escaneo de red industrial                         │
└─────────────────────────────────────────────────────┘
```

### Modelo de Amenazas

```
        INTERNET
           │
           ▼
    ┌──────────────┐
    │   Firewall   │ ← Primera línea de defensa
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │     DMZ      │ ← Zona desmilitarizada
    │  (Gateway    │
    │   Seguro)    │
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │   Red OT     │ ← Red operacional tecnológica
    │  ┌────────┐  │
    │  │ PLCs   │  │
    │  │ RTUs   │  │
    │  │ Sensores│ │
    │  └────────┘  │
    └──────────────┘
```

---

## Vulnerabilidades Comunes

### 1. Broadcast Malicioso

**Descripción:** El ID 0 (broadcast) permite escribir en TODOS los dispositivos simultáneamente.

**Ataque típico:**
```
Trama: 00 0F 0000 0010 02 FF00 CRC
       │  │  │    │    │  │    │
       │  │  │    │    │  │    CRC
       │  │  │    │    │  Valor ON (todas las bobinas)
       │  │  │    │    Count (16 bobinas)
       │  │  │    Address 0
       │  │  Función 0x0F (Write Multiple Coils)
       │  Broadcast ID = TODOS los dispositivos
```

**Impacto:** Apagado masivo de equipos, activación de alarmas falsas.

**Mitigación:**
```cpp
#include <ModbusSecurity.h>

ModbusRTU mb;
ModbusSecurity security(&mb);

void setup() {
  // Rechazar broadcast completamente
  security.enableBroadcastRestriction(true);
  
  // O permitir solo funciones de escritura específicas
  security.setAllowedBroadcastFunctions({0x05, 0x06});
}
```

---

### 2. Inyección de Tramas

**Descripción:** Envío de tramas malformadas para causar comportamientos inesperados.

**Ejemplos de tramas maliciosas:**

```cpp
// PDU excesivamente larga (>253 bytes, viola especificación)
uint8_t maliciousFrame[] = {
  0x01, 0x03, 0x00, 0x00, 
  0xFF, 0xFF,  // Count = 65535 (¡inválido!)
  // ... 65KB de datos basura
};

// Función inexistente
uint8_t invalidFunction[] = {
  0x01, 0x7F,  // Función 0x7F no existe
  0x00, 0x00,
  0x00, 0x01
};

// Dirección de registro fuera de rango
uint8_t outOfRange[] = {
  0x01, 0x03,
  0xFF, 0xFF,  // Address 65535
  0x00, 0x01,  // Count 1
  CRC
};
```

**Mitigación con validación estricta:**
```cpp
void setup() {
  security.setMaxPduSize(253);  // Límite especificación
  security.enableStrictValidation(true);
  security.validateFunctionCodes(true);
  security.validateRegisterRanges(true);
}
```

---

### 3. Rate Limiting Ausente

**Descripción:** Un atacante puede enviar miles de mensajes por segundo, saturando el dispositivo.

**Síntomas:**
- CPU al 100%
- Timeouts en operaciones legítimas
- Reinicios por watchdog
- Pérdida de paquetes válidos

**Solución:**
```cpp
#define MAX_MESSAGES_PER_SEC 50

void setup() {
  security.setRateLimit(MAX_MESSAGES_PER_SEC);
  
  // Configurar acción ante violación
  security.onRateLimitExceeded([]() {
    Serial.println("[SECURITY] Rate limit excedido - bloqueando temporalmente");
    delay(1000);  // Penalización de 1 segundo
  });
}
```

---

### 4. IDs No Autorizados

**Descripción:** Cualquier dispositivo puede hacerse pasar por un ID válido.

**Escenario de ataque:**
```
Dispositivo legítimo (ID=5) ────┐
                                 ├─── Bus RS485
Atacante (también ID=5) ─────────┘
                                 │
                                 ▼
                        Maestro recibe respuestas
                        de ambos → confusión
```

**Defensa con lista blanca:**
```cpp
const uint8_t ALLOWED_IDS[] = {1, 2, 3, 5, 10, 11, 20};
const size_t ALLOWED_COUNT = sizeof(ALLOWED_IDS);

bool isIdAllowed(uint8_t id) {
  for (size_t i = 0; i < ALLOWED_COUNT; i++) {
    if (ALLOWED_IDS[i] == id) return true;
  }
  return false;
}

// En el callback de recepción
void onMessageReceived() {
  uint8_t senderId = mb.getSenderId();
  
  if (!isIdAllowed(senderId)) {
    Serial.printf("[ALERTA] ID no autorizado: %d\n", senderId);
    security.rejectFrame();
    logSecurityEvent("UNAUTHORIZED_ID", senderId);
    return;
  }
  
  // Procesar mensaje legítimo
  processMessage();
}
```

---

## Hardening de Configuración

### Configuración Segura Mínima

```cpp
#include <ModbusRTU.h>
#include <ModbusSecurity.h>

ModbusRTU mb;
ModbusSecurity security(&mb);

struct SecurityConfig {
  // Lista blanca de IDs permitidos
  const uint8_t allowedIds[10] = {1, 2, 3, 4, 5, 10, 11, 12, 20, 100};
  const size_t allowedCount = 10;
  
  // Funciones permitidas por tipo de registro
  const uint8_t allowedFunctions[] = {
    0x01,  // Read Coils
    0x02,  // Read Discrete Inputs
    0x03,  // Read Holding Registers
    0x04,  // Read Input Registers
    0x05,  // Write Single Coil
    0x06,  // Write Single Register
    0x0F,  // Write Multiple Coils
    0x10   // Write Multiple Registers
  };
  
  // Rango válido de registros
  const uint16_t minRegister = 0;
  const uint16_t maxRegister = 1000;
  
  // Rate limiting
  const uint16_t maxMessagesPerSec = 50;
  
  // Timeout agresivo para reducir ventana de ataque
  const uint16_t timeoutMs = 500;
};

void applySecurityConfig() {
  // Validación estricta
  security.enableStrictValidation(true);
  security.setMaxPduSize(253);
  
  // Restricciones de broadcast
  security.enableBroadcastRestriction(true);
  
  // Rate limiting
  security.setRateLimit(SecurityConfig::maxMessagesPerSec);
  
  // Timeout
  mb.setTimeout(SecurityConfig::timeoutMs);
  
  Serial.println("Configuración de seguridad aplicada");
}
```

### Hardening para Entornos Críticos

```cpp
// Para SCADA, energía, agua, etc.

#define CRITICAL_ENVIRONMENT

#ifdef CRITICAL_ENVIRONMENT

// Configuración ultra-estricta
#define MAX_MESSAGES_PER_SEC 20      // Muy conservador
#define TIMEOUT_MS 300               // Timeout corto
#define MAX_RETRY_ATTEMPTS 2         // Pocos reintentos
#define ENABLE_CRYPTO_CHECKSUM       // Si disponible

void setupCriticalSecurity() {
  // Lista blanca EXPLÍCITA (denegar por defecto)
  security.setDefaultPolicy(DENY_ALL);
  
  // Añadir reglas explícitas
  security.allowId(1);   // Maestro principal
  security.allowId(2);   // Maestro backup
  security.allowId(10);  // HMI autorizado
  
  // Registrar todas las funciones críticas
  security.allowFunction(0x03, 0, 100);   // Leer regs 0-100
  security.allowFunction(0x06, 50, 50);   // Escribir solo reg 50
  security.allowFunction(0x10, 0, 10);    // Escribir regs 0-10
  
  // Habilitar logging forense
  security.enableLogging(true);
  security.setLogLevel(LOG_ALL);
  
  // Acción ante anomalías
  security.onSecurityEvent([](SecurityEvent event) {
    logToSecureStorage(event);
    triggerAlarm();
    
    // En caso de ataque sostenido, entrar en modo seguro
    if (event.count > 10) {
      enterSafeMode();
    }
  });
}

#endif
```

---

## Validación de Tramas

### Implementación de Validador Personalizado

```cpp
class FrameValidator {
public:
  struct ValidationResult {
    bool isValid;
    const char* reason;
    uint8_t errorCode;
  };
  
  static ValidationResult validate(const uint8_t* frame, size_t length) {
    ValidationResult result;
    
    // 1. Verificar longitud mínima
    if (length < 8) {  // ID + Func + Address + Count + CRC
      result.isValid = false;
      result.reason = "Trama demasiado corta";
      result.errorCode = 0x02;  // Illegal data address
      return result;
    }
    
    // 2. Extraer campos
    uint8_t slaveId = frame[0];
    uint8_t function = frame[1];
    uint16_t address = (frame[2] << 8) | frame[3];
    uint16_t count = (frame[4] << 8) | frame[5];
    
    // 3. Validar ID (0 = broadcast, 1-247 = válido, 248-255 = reservado)
    if (slaveId > 247) {
      result.isValid = false;
      result.reason = "ID de esclavo reservado";
      result.errorCode = 0x01;  // Illegal function
      return result;
    }
    
    // 4. Validar función
    if (!isValidFunction(function)) {
      result.isValid = false;
      result.reason = "Función no soportada";
      result.errorCode = 0x01;
      return result;
    }
    
    // 5. Validar longitud PDU máxima (253 bytes según spec)
    size_t pduLength = length - 2;  // Excluyendo CRC
    if (pduLength > 253) {
      result.isValid = false;
      result.reason = "PDU excede máximo especificación";
      result.errorCode = 0x02;
      return result;
    }
    
    // 6. Validar count razonable
    if (count == 0 || count > 125) {  // 125 coils, 123 regs máx
      result.isValid = false;
      result.reason = "Count inválido";
      result.errorCode = 0x02;
      return result;
    }
    
    // 7. Validar dirección + count no excede espacio
    if (address + count > MAX_REGISTER_ADDRESS) {
      result.isValid = false;
      result.reason = "Dirección fuera de rango";
      result.errorCode = 0x02;
      return result;
    }
    
    // 8. Validar CRC
    uint16_t receivedCrc = (frame[length-1] << 8) | frame[length-2];
    uint16_t calculatedCrc = calculateCrc(frame, length-2);
    if (receivedCrc != calculatedCrc) {
      result.isValid = false;
      result.reason = "CRC inválido";
      result.errorCode = 0x04;  // Slave device failure
      return result;
    }
    
    // Todo OK
    result.isValid = true;
    result.reason = "OK";
    result.errorCode = 0;
    return result;
  }
  
private:
  static bool isValidFunction(uint8_t func) {
    return func == 0x01 || func == 0x02 || func == 0x03 || 
           func == 0x04 || func == 0x05 || func == 0x06 ||
           func == 0x0F || func == 0x10 || func == 0x11 ||
           func == 0x14 || func == 0x15 || func == 0x16 ||
           func == 0x17 || func == 0x2B;
  }
  
  static uint16_t calculateCrc(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
      crc ^= data[i];
      for (uint8_t j = 0; j < 8; j++) {
        if (crc & 0x0001) {
          crc >>= 1;
          crc ^= 0xA001;
        } else {
          crc >>= 1;
        }
      }
    }
    return crc;
  }
};

// Uso en el loop
void loop() {
  mb.task();
  
  if (mb.frameReceived()) {
    auto result = FrameValidator::validate(mb.getLastFrame(), mb.getFrameLength());
    
    if (!result.isValid) {
      Serial.printf("[VALIDADOR] %s (Error 0x%02X)\n", result.reason, result.errorCode);
      security.rejectFrame();
      return;
    }
    
    // Procesar trama válida
    processValidFrame();
  }
}
```

---

## Protección contra DoS

### Estrategias de Defensa en Profundidad

```cpp
class DosProtection {
private:
  struct Stats {
    uint32_t messagesLastSecond;
    uint32_t consecutiveErrors;
    uint32_t lastResetTime;
    bool blocked;
    uint32_t blockUntil;
  };
  
  Stats stats = {0, 0, 0, false, 0};
  
  static const uint32_t MAX_MSG_PER_SEC = 50;
  static const uint32_t MAX_CONSECUTIVE_ERRORS = 10;
  static const uint32_t BLOCK_DURATION_MS = 5000;
  
public:
  bool allowProcessing() {
    uint32_t now = millis();
    
    // Reset contador por segundo
    if (now - stats.lastResetTime > 1000) {
      stats.messagesLastSecond = 0;
      stats.consecutiveErrors = 0;
      stats.lastResetTime = now;
    }
    
    // Verificar bloqueo activo
    if (stats.blocked) {
      if (now > stats.blockUntil) {
        stats.blocked = false;
        Serial.println("[DoS] Bloqueo levantado");
      } else {
        return false;  // Aún bloqueado
      }
    }
    
    // Rate limiting
    stats.messagesLastSecond++;
    if (stats.messagesLastSecond > MAX_MSG_PER_SEC) {
      triggerBlock("Rate limit excedido");
      return false;
    }
    
    return true;
  }
  
  void reportError() {
    stats.consecutiveErrors++;
    
    if (stats.consecutiveErrors > MAX_CONSECUTIVE_ERRORS) {
      triggerBlock("Demasiados errores consecutivos");
    }
  }
  
  void reportSuccess() {
    stats.consecutiveErrors = 0;
  }
  
private:
  void triggerBlock(const char* reason) {
    stats.blocked = true;
    stats.blockUntil = millis() + BLOCK_DURATION_MS;
    
    Serial.printf("[DoS] BLOQUEO ACTIVADO: %s\n", reason);
    Serial.printf("       Duración: %lu ms\n", BLOCK_DURATION_MS);
    
    // Log forense
    logSecurityEvent("DOS_ATTACK_DETECTED", reason);
    
    // Notificar sistema externo si está disponible
    notifySecuritySystem();
  }
};

// Integración
DosProtection dos;

void loop() {
  if (!dos.allowProcessing()) {
    delay(100);  // Esperar antes de reintentar
    return;
  }
  
  mb.task();
  
  if (mb.getError() != 0) {
    dos.reportError();
  } else {
    dos.reportSuccess();
  }
}
```

---

## Seguridad en Red

### Segmentación de Red Industrial

```
┌─────────────────────────────────────────────────────────┐
│                    NIVEL 5: Enterprise                   │
│                  (Oficinas, ERP, Cloud)                  │
│                            │                             │
│                       ═════════                          │
│                       Firewall NG                         │
│                       ═════════                          │
│                            │                             │
├────────────────────────────┼─────────────────────────────┤
│                    NIVEL 4: IT Corporate                   │
│                  (Servidores, Historian)                  │
│                            │                             │
│                       ═════════                          │
│                    DMZ Firewall                           │
│                       ═════════                          │
│                            │                             │
├────────────────────────────┼─────────────────────────────┤
│              NIVEL 3: Operations (OT/ICS)                 │
│            (HMI, SCADA, Engineering Workstation)          │
│                            │                             │
│                       ═════════                          │
│                    OT Firewall                            │
│                       ═════════                          │
│                            │                             │
├────────────────────────────┼─────────────────────────────┤
│                NIVEL 2: Control Network                   │
│              (PLCs, RTUs, Controladores)                  │
│                            │                             │
│                    Switches Industriales                  │
│                            │                             │
├────────────────────────────┼─────────────────────────────┤
│                NIVEL 1: Field Devices                     │
│           (Sensores, Actuadores, VFDs)                    │
│                            │                             │
│                    Bus de Campo (RS485)                   │
│                            │                             │
└────────────────────────────┴─────────────────────────────┘
```

### Reglas de Firewall Recomendadas

```bash
# Firewall perimetral (nivel 4-3)
# Permitir SOLO tráfico esencial desde IT hacia OT

# Permitir SCADA hacia PLCs (puerto 502 Modbus TCP)
iptables -A FORWARD -s 10.0.4.0/24 -d 10.0.3.0/24 -p tcp --dport 502 -j ACCEPT

# Permitir respuesta
iptables -A FORWARD -s 10.0.3.0/24 -d 10.0.4.0/24 -m state --state ESTABLISHED -j ACCEPT

# Denegar TODO lo demás por defecto
iptables -A FORWARD -j DROP

# Logging de intentos de conexión no autorizados
iptables -A FORWARD -p tcp --dport 502 -j LOG --log-prefix "MODBUS_UNAUTH: "
```

### VLANs para Aislamiento

```yaml
# Configuración switch industrial (ejemplo Cisco)

# VLAN 10: Management
vlan 10
  name MGMT
  interface range GigabitEthernet0/1-4
    switchport access vlan 10

# VLAN 20: SCADA/HMI
vlan 20
  name SCADA
  interface range GigabitEthernet0/5-10
    switchport access vlan 20

# VLAN 30: PLCs/Control
vlan 30
  name CONTROL
  interface range GigabitEthernet0/11-20
    switchport access vlan 30

# VLAN 40: Field Devices
vlan 40
  name FIELD
  interface range GigabitEthernet0/21-48
    switchport access vlan 40

# Inter-VLAN routing SOLO donde sea necesario
interface Vlan20
  ip address 10.0.20.1 255.255.255.0
  ip access-group SCADA-IN in

interface Vlan30
  ip address 10.0.30.1 255.255.255.0
  ip access-group CONTROL-IN in
```

---

## Modbus TCP Security (TLS)

### Implementación con ESP8266/ESP32

```cpp
#include <ModbusTLS.h>
#include <WiFiClientSecure.h>

// Certificados (generar con OpenSSL)
const char* CA_CERT = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDXTCCAkWgAwIBAgIJAJC1HiIAZAiUMA0Gcg...
-----END CERTIFICATE-----
)EOF";

const char* SERVER_CERT = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDXTCCAkWgAwIBAgIJAJC1HiIAZAiUMA0Gcg...
-----END CERTIFICATE-----
)EOF";

const char* SERVER_KEY = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEA0Z3VS5JJcds3xfn/ygWyF8PbnGy...
-----END RSA PRIVATE KEY-----
)EOF";

ModbusTLS mb;
WiFiClientSecure secureClient;

void setup() {
  // Configurar certificados
  secureClient.setCACert(CA_CERT);
  secureClient.setCertificate(SERVER_CERT);
  secureClient.setPrivateKey(SERVER_KEY);
  
  // Conectar con TLS
  if (secureClient.connect("modbus-server.local", 802)) {
    Serial.println("Conexión TLS establecida");
    
    // Verificar certificado del servidor
    if (secureClient.verifyCertChain()) {
      Serial.println("Certificado verificado ✓");
    } else {
      Serial.println("⚠ ALERTA: Certificado no verificado");
      return;
    }
  }
  
  mb.begin(&secureClient);
}

void loop() {
  mb.task();
  
  // Las comunicaciones ahora están cifradas con TLS 1.2+
  // Protegido contra:
  // - Eavesdropping
  // - Man-in-the-middle
  // - Replay attacks
}
```

### Generación de Certificados

```bash
#!/bin/bash
# generate_certs.sh

# 1. Crear CA privada
openssl genrsa -out ca.key 2048
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt \
  -subj "/C=ES/ST=Madrid/L=Madrid/O=MiEmpresa/CN=ModbusCA"

# 2. Crear certificado de servidor
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr \
  -subj "/C=ES/ST=Madrid/L=Madrid/O=MiEmpresa/CN=modbus-server.local"

# 3. Firmar certificado con CA
openssl x509 -req -days 365 -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt

# 4. Convertir formatos si es necesario
openssl pkcs12 -export -out server.pfx -inkey server.key -in server.crt

echo "Certificados generados:"
ls -la *.crt *.key *.csr
```

---

## Auditoría y Logging

### Sistema de Logging Forense

```cpp
class SecurityLogger {
public:
  enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_ALL
  };
  
  struct SecurityEvent {
    uint32_t timestamp;
    LogLevel level;
    const char* eventType;
    uint8_t sourceId;
    const char* description;
    uint32_t counter;
  };
  
private:
  static const int MAX_EVENTS = 100;
  SecurityEvent events[MAX_EVENTS];
  int eventIndex = 0;
  
  // Almacenamiento persistente (SPIFFS, SD, EEPROM)
  File logFile;
  
public:
  bool begin() {
    // Inicializar almacenamiento
    if (!SPIFFS.begin()) return false;
    
    logFile = SPIFFS.open("/security.log", "a");
    if (!logFile) return false;
    
    // Escribir cabecera
    if (logFile.size() == 0) {
      logFile.println("timestamp,level,event_type,source_id,description");
    }
    
    return true;
  }
  
  void logEvent(LogLevel level, const char* eventType, 
                uint8_t sourceId, const char* description) {
    SecurityEvent event;
    event.timestamp = millis();
    event.level = level;
    event.eventType = eventType;
    event.sourceId = sourceId;
    event.description = description;
    event.counter = getEventCount(eventType);
    
    // Guardar en buffer circular
    events[eventIndex] = event;
    eventIndex = (eventIndex + 1) % MAX_EVENTS;
    
    // Escribir a archivo
    logToFile(event);
    
    // Notificar si es crítico
    if (level >= LOG_CRITICAL) {
      triggerAlert(event);
    }
    
    // Imprimir en serial (para debugging)
    printEvent(event);
  }
  
  // Métodos de consulta
  void getRecentEvents(SecurityEvent* outEvents, int count) {
    // Devolver últimos N eventos
  }
  
  int getEventCount(const char* eventType) {
    int count = 0;
    for (int i = 0; i < MAX_EVENTS; i++) {
      if (strcmp(events[i].eventType, eventType) == 0) {
        count++;
      }
    }
    return count;
  }
  
  void exportLogs(Stream& output) {
    // Exportar logs para análisis externo
    for (int i = 0; i < MAX_EVENTS; i++) {
      SecurityEvent& e = events[i];
      output.printf("%lu,%d,%s,%d,%s\n",
                    e.timestamp, e.level, e.eventType, 
                    e.sourceId, e.description);
    }
  }
  
private:
  void logToFile(const SecurityEvent& event) {
    if (!logFile) return;
    
    logFile.printf("%lu,%d,%s,%d,%s\n",
                   event.timestamp, event.level, 
                   event.eventType, event.sourceId,
                   event.description);
    logFile.flush();
  }
  
  void printEvent(const SecurityEvent& event) {
    const char* levelStr[] = {"INFO", "WARN", "ERROR", "CRIT"};
    
    Serial.printf("[%s] %s: ID=%d - %s (Count: %lu)\n",
                  levelStr[event.level],
                  event.eventType,
                  event.sourceId,
                  event.description,
                  event.counter);
  }
  
  void triggerAlert(const SecurityEvent& event) {
    // Enviar alerta a sistema externo
    // Email, SMS, MQTT, webhook, etc.
    Serial.println("🚨 ALERTA DE SEGURIDAD ACTIVADA 🚨");
  }
};

// Tipos de eventos a loguear
#define EVT_UNAUTHORIZED_ID     "UNAUTHORIZED_ID"
#define EVT_INVALID_FRAME       "INVALID_FRAME"
#define EVT_RATE_LIMIT          "RATE_LIMIT_EXCEEDED"
#define EVT_BROADCAST_BLOCKED   "BROADCAST_BLOCKED"
#define EVT_DOS_DETECTED        "DOS_ATTACK_DETECTED"
#define EVT_CERT_INVALID        "CERTIFICATE_INVALID"
#define EVT_AUTH_FAILURE        "AUTHENTICATION_FAILURE"
#define EVT_CONFIG_CHANGE       "CONFIGURATION_CHANGED"

// Uso
SecurityLogger logger;

void setup() {
  logger.begin();
  logger.logEvent(LOG_INFO, "SYSTEM_START", 0, "Sistema iniciado");
}

void onSecurityViolation(uint8_t sourceId, const char* reason) {
  logger.logEvent(LOG_CRITICAL, EVT_UNAUTHORIZED_ID, 
                  sourceId, reason);
}
```

---

## Checklist de Seguridad

### ✅ Checklist de Implementación

#### Configuración Básica
- [ ] Lista blanca de IDs configurada
- [ ] Funciones no utilizadas deshabilitadas
- [ ] Rango de registros limitado
- [ ] Broadcast restringido o eliminado
- [ ] Timeout configurado (<1000ms)

#### Validación de Tramas
- [ ] Longitud PDU máxima validada (≤253 bytes)
- [ ] Códigos de función validados
- [ ] Direcciones de registro validadas
- [ ] Count máximo validado
- [ ] CRC verificado

#### Protección DoS
- [ ] Rate limiting implementado (≤50 msg/s)
- [ ] Bloqueo tras errores consecutivos
- [ ] Timeout de reconexión exponencial
- [ ] Buffer overflow protegido

#### Seguridad de Red
- [ ] Firewall configurado (solo puerto 502)
- [ ] VLANs segmentadas
- [ ] Acceso remoto restringido
- [ ] SNMP community cambiado
- [ ] Puertos de debug cerrados

#### Autenticación y Cifrado
- [ ] TLS habilitado (si soportado)
- [ ] Certificados válidos instalados
- [ ] Verificación de cadena de confianza
- [ ] Cipher suites fuertes (AES-256)

#### Logging y Monitoreo
- [ ] Logging de eventos de seguridad
- [ ] Alertas configuradas para anomalías
- [ ] Retención de logs ≥90 días
- [ ] Monitoreo en tiempo real activo
- [ ] Procedimiento de respuesta a incidentes

#### Físico
- [ ] Puertos de consola protegidos
- [ ] Gabinete cerrado con llave
- [ ] Acceso físico restringido
- [ ] CCTV en áreas críticas

### ✅ Checklist de Auditoría Periódica

**Diario:**
- [ ] Revisar alertas de seguridad
- [ ] Verificar logs de acceso no autorizado
- [ ] Confirmar backups de configuración

**Semanal:**
- [ ] Analizar patrones de tráfico anómalos
- [ ] Revisar intentos de conexión fallidos
- [ ] Actualizar listas blancas si es necesario

**Mensual:**
- [ ] Prueba de penetración básica
- [ ] Revisión de reglas de firewall
- [ ] Actualización de firmware/parches
- [ ] Rotación de credenciales

**Anual:**
- [ ] Auditoría de seguridad completa
- [ ] Renovación de certificados TLS
- [ ] Revisión de arquitectura de red
- [ ] Training de personal en seguridad

---

## Recursos Adicionales

### Herramientas de Testing

| Herramienta | Propósito | URL |
|------------|-----------|-----|
| ModScan32 | Cliente Modbus testing | www.wintriss.com |
| CAS Modbus Scanner | Scanner avanzado | www.chipkin.com |
| Wireshark | Análisis de tramas | wireshark.org |
| Nmap | Escaneo de red | nmap.org |
| Metasploit | Penetration testing | metasploit.com |

### Estándares y Guías

- **IEC 62351**: Seguridad en protocolos de power systems
- **NIST SP 800-82**: Guía de seguridad ICS/SCADA
- **ISA/IEC 62443**: Seguridad en automatización industrial
- **CISA Binding Operational Directive 22-01**: Reducir superficie de ataque

### Referencias de Ataques Conocidos

- **CVE-2018-7790**: Modicon PLC vulnerability
- **CVE-2019-6803**: WAGO PLC Modbus TCP flaw
- **CVE-2020-5682**: Mitsubishi Electric Modbus issue
- **Stuxnet**: Ataque famoso usando Modbus (2010)

---

**Última actualización:** Versión 4.3.0  
**Documento revisado por equipo de seguridad**
