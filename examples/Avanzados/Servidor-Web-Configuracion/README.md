# Servidor Web de Configuración Modbus

## Descripción

Ejemplo avanzado que implementa un servidor web completo para configuración y monitoring de dispositivos Modbus. Permite:

- **Monitorizar en tiempo real**: Estado del sistema, estadísticas de comunicación
- **Ver/Editiar registros**: Coils, discrete inputs, holding registers, input registers
- **Configurar parámetros**: Red WiFi, ID esclavo, puerto Modbus
- **Estadísticas detalladas**: Mensajes, errores, uptime, calidad WiFi

## Hardware Requerido

- ESP32 o ESP8266 con WiFi
- Módulo RS485 MAX485 (opcional, para comunicación Modbus RTU)

## Conexiones RS485

### ESP32
```
MAX485    ESP32
------    -----
DI        GPIO17 (RX2)
RO        GPIO16 (TX2)
DE+RE     GPIO5
VCC       5V
GND       GND
```

### ESP8266
```
MAX485    ESP8266
------    -------
DI        GPIO1 (TX)
RO        GPIO3 (RX)
DE+RE     GPIO2
VCC       5V
GND       GND
```

## Configuración

1. **Editar credenciales WiFi** en el archivo `.ino`:
```cpp
const char* WIFI_SSID = "TuSSID";
const char* WIFI_PASSWORD = "TuPassword";
```

2. **Configurar parámetros Modbus** (opcional):
```cpp
#define MODBUS_SLAVE_ID 1
#define MODBUS_BAUDRATE 9600
```

## Instrucciones de Uso

### 1. Subir el Código
- Conectar ESP32/ESP8266 vía USB
- Seleccionar placa en Arduino IDE
- Subir sketch

### 2. Abrir Monitor Serial
- Configurar baudrate a 115200
- Reiniciar dispositivo
- Anotar dirección IP mostrada

### 3. Acceder desde Navegador
- Abrir navegador web
- Ir a `http://<IP-del-dispositivo>`
- Ejemplo: `http://192.168.1.100`

## Interfaz Web

### Página Principal
- Dashboard con estado del sistema
- Estadísticas rápidas (uptime, mensajes, errores)
- Acceso rápido a todas las secciones

### Registros Modbus
- **Coils**: Ver estado de salidas digitales (solo lectura en esta vista)
- **Discrete Inputs**: Ver entradas digitales (solo lectura)
- **Holding Registers**: Ver y editar registros de salida
- **Input Registers**: Ver registros de entrada (solo lectura)

### Configuración
- Nombre del dispositivo
- ID de esclavo Modbus (1-247)
- Puerto Modbus TCP
- Credenciales WiFi
- Autenticación web (opcional)

### Estadísticas
- Rendimiento Modbus (mensajes totales, errores, tasa de éxito)
- Información de red (RSSI, calidad WiFi, IP)
- Información del sistema (uptime, heap libre, temperatura CPU)

## API REST

El servidor expone endpoints JSON para integración:

### GET /api/data
Obtiene datos en tiempo real:
```json
{
  "uptime": 1234,
  "totalMessages": 5678,
  "totalErrors": 12,
  "wifiQuality": 85,
  "rssi": -65,
  "freeHeap": 123456,
  "cpuTemperature": 45.2
}
```

### POST /api/save
Guarda configuración:
```json
{
  "deviceName": "Mi-Dispositivo",
  "slaveId": 1,
  "modbusPort": 502,
  "ssid": "MiRed",
  "password": "MiPassword",
  "enableAuth": 0
}
```

### POST /api/write
Escribe en registro Modbus:
```json
{
  "type": 2,
  "addr": 10,
  "value": 1234
}
```

Tipos de registro:
- 0: Coils
- 1: Discrete Inputs
- 2: Holding Registers
- 3: Input Registers

## Características Técnicas

### Actualización en Tiempo Real
- AJAX polling cada 1 segundo
- Sin necesidad de refresh manual
- Datos actualizados automáticamente

### Persistencia
- Configuración guardada en SPIFFS/LittleFS
- Sobrevive a reinicios
- Archivo: `/modbus_config.json`

### Responsive Design
- Compatible con móvil, tablet y desktop
- CSS moderno con Flexbox/Grid
- Interfaz limpia y profesional

### Seguridad Básica
- Validación de inputs
- Autenticación opcional
- Contraseñas en texto plano (mejorar en producción)

## Limitaciones

- Solo para ESP32/ESP8266 (requiere WiFi y servidor web)
- Máximo 100 registros visibles por tipo
- Parseo JSON básico (recomendado usar ArduinoJson en producción)
- Sin HTTPS (solo HTTP)

## Mejoras Futuras

- [ ] WebSocket para actualizaciones push
- [ ] HTTPS/TLS para seguridad
- [ ] ArduinoJson para parseo robusto
- [ ] Autenticación JWT
- [ ] Gráficos de tendencias
- [ ] Exportar datos CSV
- [ ] Soporte multi-idioma

## Troubleshooting

### No se conecta a WiFi
- Verificar SSID y contraseña
- Comprobar cobertura de señal
- Reiniciar dispositivo

### Página no carga
- Verificar IP correcta
- Comprobar conexión de red
- Reiniciar servidor web

### Error al guardar configuración
- Verificar espacio en SPIFFS
- Formatear SPIFFS si es necesario

## Autor

Equipo de Desarrollo Modbus - 2024

## Licencia

LGPL-2.1
