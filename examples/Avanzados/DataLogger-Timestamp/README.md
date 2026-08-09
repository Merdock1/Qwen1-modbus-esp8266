# DataLogger con Timestamp

## Descripción

Sistema de registro de datos (data logger) que lee registros Modbus periódicamente y los guarda con timestamp en una SD card. Ideal para:

- Monitorización ambiental (temperatura, humedad, presión)
- Registro de consumo energético
- Tracking de variables industriales
- Análisis histórico de procesos

## Hardware Requerido

- ESP32 (con RTC interno)
- Lector SD card módulo
- Dispositivo Modbus slave (sensor, medidor, PLC)
- RS485 MAX485

## Conexiones

### ESP32 + SD Card + RS485

```
SD Card     ESP32
-------     -----
CS          GPIO5
MOSI        GPIO23
MISO        GPIO19
SCK         GPIO18
VCC         3.3V
GND         GND

MAX485      ESP32
------      -----
DI          GPIO17 (RX2)
RO          GPIO16 (TX2)
DE+RE       GPIO4
VCC         5V
GND         GND
```

## Configuración

### 1. WiFi para NTP

Editar credenciales en el sketch:

```cpp
const char* WIFI_SSID = "TuSSID";
const char* WIFI_PASSWORD = "TuPassword";
```

### 2. Zona Horaria

Ajustar offset según ubicación:

```cpp
const long gmtOffset_sec = -3600;    // UTC-1 (ej: Canarias)
const int daylightOffset_sec = 0;    // Sin DST
```

### 3. Parámetros Modbus

```cpp
#define SLAVE_ADDRESS 10        // Dirección del slave
#define FIRST_REGISTER 0        // Primer registro a leer
#define REGISTER_COUNT 10       // Cantidad de registros
```

### 4. Intervalo de Logging

```cpp
#define LOG_INTERVAL 60         // Segundos entre lecturas
```

## Formato de Archivo CSV

El archivo `modbus_log.csv` generado tiene este formato:

```csv
Timestamp,Registro0,Registro1,Registro2,Registro3,Registro4,Registro5,Registro6,Registro7,Registro8,Registro9
2024-01-15 10:30:00,250,320,280,290,300,310,320,330,340,350
2024-01-15 10:31:00,252,322,281,291,301,311,321,331,341,351
```

Compatible con:
- Microsoft Excel
- LibreOffice Calc
- Google Sheets
- Python pandas
- MATLAB

## Características

### Timestamp Preciso
- Sincronización NTP vía WiFi
- Formato ISO 8601: `YYYY-MM-DD HH:MM:SS`
- Respaldo con RTC interno si no hay WiFi

### Gestión de SD Card
- Detección automática
- Creación de archivo si no existe
- Append mode (no sobrescribe datos)
- Cabecera CSV automática

### Modo Fallback
- Si no hay SD: muestra datos en Serial
- Si no hay WiFi: usa reloj interno
- Funciona offline después de configuración inicial

## Uso Típico

1. **Configurar** parámetros en el sketch
2. **Subir** código al ESP32
3. **Insertar** SD card formateada (FAT32)
4. **Conectar** dispositivo Modbus
5. **Esperar** sincronización NTP
6. **Verificar** logs en Serial Monitor

## Visualización de Datos

### Excel / Sheets

1. Abrir `modbus_log.csv`
2. Seleccionar columna Timestamp
3. Insertar gráfico de líneas
4. Personalizar ejes y etiquetas

### Python (pandas + matplotlib)

```python
import pandas as pd
import matplotlib.pyplot as plt

# Leer CSV
df = pd.read_csv('modbus_log.csv', parse_dates=['Timestamp'])

# Plot
plt.figure(figsize=(12, 6))
plt.plot(df['Timestamp'], df['Registro0'], label='Sensor 1')
plt.plot(df['Timestamp'], df['Registro1'], label='Sensor 2')
plt.xlabel('Tiempo')
plt.ylabel('Valor')
plt.legend()
plt.grid(True)
plt.show()
```

## Optimizaciones

### Para Mayor Duración de Batería

```cpp
#define LOG_INTERVAL 3600       // 1 hora
// Usar deep sleep entre lecturas
esp_sleep_enable_timer_wakeup(LOG_INTERVAL * 1000000ULL);
esp_deep_sleep_start();
```

### Para Más Registros

```cpp
#define REGISTER_COUNT 50       // Aumentar cantidad
uint16_t registerValues[REGISTER_COUNT];
```

### Para SPIFFS en lugar de SD

```cpp
#include <SPIFFS.h>
// Reemplazar SD.begin() por SPIFFS.begin(true)
// Usar File = SPIFFS.open()
```

## Troubleshooting

### SD Card No Detectada

- Verificar conexiones CS/MOSI/MISO/SCK
- Probar otra SD card (máx 32GB para FAT32)
- Formatear como FAT32

### Timestamp Incorrecto

- Verificar conexión WiFi
- Comprobar offsets de zona horaria
- Esperar sincronización completa (~5s)

### Error Lectura Modbus

- Verificar dirección slave
- Comprobar cableado RS485 (A/B invertidos?)
- Confirmar baudrate correcto

### Archivo CSV Vacío

- Verificar espacio en SD card
- Comprobar permisos de escritura
- Reiniciar después de formatear

## Ejemplos de Aplicación

### 1. Monitor de Temperatura

```cpp
#define SLAVE_ADDRESS 1         // Sensor temperatura
#define FIRST_REGISTER 0        // Registro de temperatura
#define REGISTER_COUNT 1        // Solo 1 registro
#define LOG_INTERVAL 300        // Cada 5 minutos
```

### 2. Medidor Energético

```cpp
#define SLAVE_ADDRESS 5         // Medidor energía
#define FIRST_REGISTER 0        // Voltaje
#define REGISTER_COUNT 6        // V, I, P, Q, S, PF
#define LOG_INTERVAL 60         // Cada minuto
```

### 3. Estación Meteorológica

```cpp
#define SLAVE_ADDRESS 10        // Estación
#define FIRST_REGISTER 100      // Temp exterior
#define REGISTER_COUNT 8        // Temp, Hum, Pres, Viento, etc.
#define LOG_INTERVAL 600        // Cada 10 minutos
```

## Limitaciones

- Requiere WiFi inicial para NTP (opcional)
- SD card ocupa espacio físico
- Máximo ~2GB para FAT32
- Sin compresión de datos

## Mejoras Futuras

- [ ] Compresión gzip de logs antiguos
- [ ] Upload automático a cloud (AWS IoT, Thingspeak)
- [ ] Alertas por email/SMS si valores fuera de rango
- [ ] Dashboard web integrado
- [ ] Soporte para múltiples slaves

## Autor

Equipo de Desarrollo Modbus - 2024

## Licencia

LGPL-2.1
