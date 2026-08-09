# OTA-Update-Modbus - Actualización Remota de Firmware

## Descripción

Ejemplo que permite actualizar el firmware del dispositivo ESP32/ESP8266 remotamente mediante comandos Modbus:

- Descarga de firmware desde URL HTTP/HTTPS
- Verificación de checksum/integridad
- Rollback automático si falla actualización
- Notificación de estado vía registros Modbus

## Flujo de Actualización

1. Master escribe URL del firmware en registro especial
2. Device descarga y verifica firmware
3. Device reinicia en modo bootloader
4. Nuevo firmware se instala
5. Device notifica resultado vía Modbus

## Registros Especiales

| Registro | Función |
|----------|---------|
| 1000     | Comando (1=Iniciar OTA) |
| 1001-1010| URL del firmware |
| 1011     | Estado (0=Idle, 1=Downloading, 2=Installing, 3=Success, 4=Error) |
| 1012     | Progreso (0-100%) |

## Requisitos

- ESP32 con soporte OTA
- Conexión WiFi o Ethernet
- Servidor HTTP con firmware

## Autor

Equipo Modbus - 2024

## Licencia

LGPL-2.1
