# Security-Hardened - Configuración Máxima Seguridad

## Descripción

Ejemplo que implementa las mejores prácticas de seguridad para dispositivos Modbus:

- Validación estricta de tramas
- Rate limiting (límite de mensajes por segundo)
- Lista blanca de IPs/direcciones permitidas
- Logging de eventos sospechosos
- Protección contra DoS básico

## Características de Seguridad

### 1. Validación de Tramas

```cpp
// Rechazar tramas malformadas
if (!validateFrame(frame, length)) {
    logSecurityEvent("TRAMA_INVALIDA");
    return;
}
```

### 2. Rate Limiting

```cpp
// Máximo 100 mensajes/segundo
#define MAX_EVENTS_PER_SECOND 100
```

### 3. Lista Blanca

```cpp
// Solo IPs autorizadas
const char* allowedIPs[] = {"192.168.1.10", "192.168.1.20"};
```

### 4. Logging de Seguridad

```cpp
// Registrar eventos sospechosos
logSecurityEvent("ACCESO_NO_AUTORIZADO", ip);
```

## Configuración

Ver `examples/Security-Hardened/Security-Hardened.ino`

## Auditoría de Seguridad

Revisar periódicamente:
- Logs de eventos
- Intentos de acceso fallidos
- Patrones anómalos

## Autor

Equipo Modbus - 2024

## Licencia

LGPL-2.1
