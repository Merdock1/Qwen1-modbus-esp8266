# Biblioteca Modbus para Arduino
### ModbusRTU, ModbusTCP y ModbusTCP Security

For detailes on the library usage visit [documentación](documentación) section.

## Características

* Soporta todas las plataformas Arduino
* Opera en cualquier combinación de múltiples instancias de
  * [Servidor Modbus RTU](examples/RTU)
  * [Clientee Modbus RTU](examples/RTU)
  * Servidor Modbus TCP for [ESP8266/ESP32](examples/TCP-ESP) y [Biblioteca Ethernet](examples/TCP-Ethernet)
  * Clientee Modbus TCP for [ESP8266/ESP32](examples/TCP-ESP) y [Biblioteca Ethernet](examples/TCP-Ethernet)
  * [Servidor MODBUS/TCP Security (ESP8266)](examples/TLS)
  * [Clientee MODBUS/TCP Security (ESP8266/ESP32)](examples/TLS)
* Funciones Modbus soportadas:
  * 0x01 - Leer Bobinas (Read Coils)
  * 0x02 - Leer Estado de Entradas (Read Discrete Inputs) (Leer Entradas Discretas)
  * 0x03 - Leer Registros de Retención
  * 0x04 - Leer Registros de Entrada
  * 0x05 - Escribir Bobina Individual
  * 0x06 - Escribir Registro Individual
  * 0x0F - Escribir Múltiples Bobinas
  * 0x10 - Escribir Múltiples Registros
  * 0x14 - Leer Registro de Archivo
  * 0x15 - Escribir Registro de Archivo
  * 0x16 - Enmascarar Escritura de Registro
  * 0x17 - Leer/Escribir múltiplos registros
* [Llamadas de retorno (Callbacks)](examples/Callback) diseño basado en
* Ejemplos complejos de la vida real:
  * [ESP8266/ESP32 actualización de firmware sobre Modbus](examples/Files)
  * [ModbusRTU to ModbusTCP puente](examples/Bridge)

## Notas

1. Los desplazamientos (offsets) para los registros son base 0. Tenga cuidado al configurar su sistema supervisorio o su software de prueba. Por ejemplo, in [ScadaBR](http://www.scadabr.com.br) los desplazamientos son base 0, entonces, un Registro configurado como 100 en la biblioteca se configura como 100 in ScadaBR. Por otro lado, in the [CAS Modbus Scanner](http://www.chipkin.com/products/software/modbus-software/cas-modbus-scanner/) los desplazamientos son base 1, por lo que un Registro configurado como 100 en la biblioteca debería ser 101 en este software.
2. Los transceptores RS-485 basados en MAX-485 funcionan al menos hasta 115200. XY-017/XY-485 funcionan solo hasta 9600 por alguna razón.

Para más información sobre Modbus ver:

* [Modbus (De Wikipedia, la enciclopedia libre)](http://pt.wikipedia.org/wiki/Modbus)
* [ESPECIFICACIÓN DEL PROTOCOLO DE APLICACIÓN MODBUS V1.1b3](https://modbus.org/docs/Modbus_Application_Protocolo_V1_1b3.pdf)
* [GUÍA DE IMPLEMENTACIÓN DE MENSAJERÍA MODBUS EN TCP/IP V1.0b](http://www.modbus.org/docs/Modbus_Messaging_Implementation_Guide_V1_0b.pdf)
* [Especificación y Guía de Implementación MODBUS sobre Línea Serial V1.02](http://www.modbus.org/docs/Modbus_over_serial_line_V1_02.pdf)
* [Especificación del Protocoloo de Seguridad MODBUS/TCP](https://modbus.org/docs/MB-TCP-Seguridad-v21_2018-07-24.pdf)

## Últimos Cambios

```diff
// 4.1.1
+ Protocolo: Corregida respuesta de código de error incorrecto en registro inexistente
+ ModbusTCP: Corregida posible fuga de memoria
+ API: cbEnable/cbDisable funcionalidad extendida
+ ESP-IDF: CMakeList.txt añadido
+ Ejemplos: TCP-to-RTU corregido
// 4.1.0
+ API: Funcionalidad de procesamiento de trama Modbus sin formato (Raw)
+ ModbusRTU: Control preciso del intervalo entre tramas
+ Ejemplos: Puente verdadero de Servidor ModbusRTU a ModbusTCP
+ Ejemplos: ModbusRTU responde a múltiples ID desde un único dispositivo
+ ModbusRTU: Añadido control de pin de dirección para Stream
+ STL: Añadida limitación de conteo de Registros al límite vectorial of 4000 (para ESP8266 y ESP32)
+ Configuración: Añadido MODBUSIP_CONNECTION_TIMEOUT (ESP32 solo)
+ Configuración: Establecido MODBUSIP_MAX_CLIENTS = 8 for ESP32
+ ModbusTCP: Hacer opcional el uso de nombres DNS
+ ModbusRTU: Añadida característica opcional de control separado de pines RE/DE
+ API: Eliminar soporte de la biblioteca Ethernet v1
+ Ejemplos: Añadidos ejemplos Teknic ClearCore ArduinoWrapper
+ Ejemplos: Añadido ejemplo ModbusTCP a ModbusRTU
+ ModbusRTU: Característica opcional de retardo adicional de flush
// 4.0.0
+ Soporte para todas las placas Arduino
+ ModbusTLS: ESP8266 Clientee/Servidor y ESP32 Cliente
+ ModbusTCP: ModbusEthernet - Soporte para bibliotecas Ethernet WizNet W5x00, ENC28J60
+ 0x14 - Leer Registro de Archivos función
+ 0x15 - Escribir Registro de Archivos función
+ Ejemplos: Ejemplo completamente funcional de actualización de FW sobre Modbus
+ 0x16 - Escribir Registro con Máscara+ Prueba: 0x16
+ 0x17 - Leer/Escribir Registros
+ ModbusRTU: Soporte SoftwareSerial ESP32
+ Compilación sin dependencia STL (conmutable)
+ API: ModbusIP => ModbusTCP
+ API: Callback de control de acceso para función Modbus individual
+ API: Master/Slave => Clientee/Servidor según [NOTA DE PRENSA](https://modbus.org/docs/Cliente-ServidorPR-07-2020-final.docx.pdf)
+ Gran cantidad de refactorización de código y pequeñas correcciones
```

## Hoja de Ruta

```diff
// 4.2.0
- API: Cálculo alternativo de CRC (reducción de huella de memoria)
- ModbusRTU: Asignación estática de buffer
- Prueba: Precisión de trama según especificación
- Soporte de limitación de tamaño de buffer/paquete
- Slave/Servidor: slavePDU usa salida temprana por retorno donde sea posible
- Master/Clientee: Verificar tamaño de trama contra datos de cabecera donde sea posible
- Master/Clientee: Validación adicional de datos de respuesta
- Liberar registros globales y callbacks al eliminar la última instancia Modbus
- Prueba: funciones push/pull
- ModbusTCP: Refactorizar conexión por nombre DNS (usyo implementación nativa para ESP32, etc)
// 4.3.0
- ModbusTLS: ESP32 Servidor
- Prueba: TLS ESP32 Servidor
- Prueba: TLS ESP32 Cliente
- Ejemplos: Extensión de rol de prueba de certificado TLS y Alt-Name
- Ejemplos: Añadir explicación de ejemplo TLS
- ModbusTCP: ModbusAsyncTCP
- API: Extender API para permitir comyos Modbus personalizados
- Ejemplos: Operaciones básicas de archivo
- Ejemplos: Revisión
```
## Contribuciones

https://github.com/emelianov/modbus-esp8266

a.m.emelianov@gmail.com

Versión original:

https://github.com/yresarmento/modbus-esp8266

https://github.com/yresarmento/modbus-arduino

prof (at) yresarmento (dot) com

## Licencia

El código en este repositorio está licenciado bajo la Licencia BSD Nueva. Ver LICENSE.txt para más información.
