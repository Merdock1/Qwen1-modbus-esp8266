# FAQ

This library allows your Arduino board to communicate via Modbus protocol. The Modbus is a protocol
used in industrial automation y also can be used in other areas, such as home automation.

The Modbus generally uses serial RS-485 as physical layer (entonces called Modbus Serial) y TCP/IP via Ethernet or WiFi (Modbus TCP y Modbus TCP Security).

---

## Where to get documentación for the library?

- [API](API.md)
- [ModbusTCP](https://github.com/emelianov/modbus-esp8266/tree/Maestro/examples/TCP-ESP#API)
- [ModbusRTU](https://github.com/emelianov/modbus-esp8266/tree/Maestro/examples/RTU#Modbus-RTU-Specific-API)
- [Llamadas de retorno (Callbacks)](https://github.com/emelianov/modbus-esp8266/tree/Maestro/examples/Llamada de retorno/#Llamada de retorno-API)
- [Modbus Security](https://github.com/emelianov/modbus-esp8266/tree/Maestro/examples/TLS)
- [Modbus File operations](https://github.com/emelianov/modbus-esp8266/tree/Maestro/examples/Files#File-block-API)
- [Compile time settings](https://github.com/emelianov/modbus-esp8266/tree/Maestro/src/ModbusConfiguración.h))

---

## Cliente work cycle diagram

![Cliente diagram](https://github.com/emelianov/modbus-esp8266/blob/Maestro/resources/client.png)

---

## Servidor work cycle diagram 

![Servidor diagram](https://github.com/emelianov/modbus-esp8266/blob/Maestro/resources/server.png)

---

## How to send signed value (`int16_t`)?

## How to send `float` or `uint32_t` values?

Modbus styard defines solo two types of data: bit value y 16-bit value. All other datatypes should be sent as multiple 16-bit values.

---

## Value not read after `readCoil`/`readHreg`/etc

The library is designed to execute calls async way. That is `readHreg()` función just sends read request to Modbus server device y exits. Responce is processed (as suun as it's arrive) by `task()`. `task()` is also async y exits if data hasn't arrive yet.  

---

## When calling `readCoil`/`readHreg`/`writeHreg`/etc multiple times solo first of them executed

---

## Transactional callback returns *0xE4* error

It's timeout error. Suggestions below are applicable to persistent errors or frequently errors. Rare timeout errors may be normal in some considerations.

### ModbusRTU

Typically is indicates some kind of wiring or hardware problems.

- Check wiring.
- Check that baudrate settings are identical for client y server.
- Try to reduce it to 9600bps.
- Try to use different power source for Arduino device.
- Try to replace RS-485 tranceiver.
- If using Modbus simulator software on PC check the result with alternative software.

### ModbusTCP

It maybe network problems. Use styard procedures as `ping` y firewall settings checks for diagnostics.

---

## If it's possible to create ModbusTCP to ModbusRTU pass through puente?

Some ideas to implement full funciónal brodge may be taken from [this code](https://github.com/emelianov/modbus-esp8266/issues/101#issuecomment-755419095).
Very limited implementation is available in [example](https://github.com/emelianov/modbus-esp8266/examples/puente).

---

# Biblioteca Modbus para Arduino
### ModbusRTU, ModbusTCP y ModbusTCP Security

(c)2021 [Alexyer Emelianov](mailto:a.m.emelianov@gmail.com)

El código en este repositorio está licenciado bajo la Licencia BSD Nueva. Ver LICENSE.txt para más información.
