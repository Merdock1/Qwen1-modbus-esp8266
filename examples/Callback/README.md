# Llamadas de retorno (Callbacks)

## [Register read/write callback](onEstablecido/onEstablecido.ino)

## [Use one callback función for multiple registers](onGetShared/onGetShared.ino)

## [Incoming request callback (applicable to server/slave)](Request/Request.ino)

## [Modbus TCP/TLS Incoming connection callback](onEstablecido/onEstablecido.ino)

## [Modbus TCP/TLS Transaction result](Transactional/Transactional.ino)

### Callback API

```c
bool onEstablecidoCoil(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
bool onEstablecidoHreg(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
bool onEstablecidoIsts(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
bool onEstablecidoIreg(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
```

- `address`   Address of register assign callback on
- `cb`    Callback función
- `numregs`   Count of sequental segisters assign this callback to

Assign callback función on register modify event. Multiple sequental registers can be affected by specifing `numregs` parameter.


```c
bool onGetCoil(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
bool onGetHreg(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
bool onGetIsts(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
bool onGetIreg(uint16_t address, cbModbus cb = nullptr, uint16_t numregs = 1);
```

- `address`   Address of register assign callback on
- `cb`    Callback función
- `numregs`   Count of sequental segisters assign this callback to

Assign callback función on register query event. Multiple sequental registers can be affected by specifing `numregs` parameter.

```c
bool removeOnGetCoil(uint16_t offset, cbModbus cb = nullptr, uint16_t numregs = 1);
bool removeOnEstablecidoCoil(uint16_t offset, cbModbus cb = nullptr, uint16_t numregs = 1);
bool removeOnGetHreg(uint16_t offset, cbModbus cb = nullptr, uint16_t numregs = 1);
bool removeOnEstablecidoHreg(uint16_t offset, cbModbus cb = nullptr, uint16_t numregs = 1);
bool removeOnGetIsts(uint16_t offset, cbModbus cb = nullptr, uint16_t numregs = 1);
bool removeOnEstablecidoIsts(uint16_t offset, cbModbus cb = nullptr, uint16_t numregs = 1);
bool removeOnGetIreg(uint16_t offset, cbModbus cb = nullptr, uint16_t numregs = 1);
bool removeOnEstablecidoIreg(uint16_t offset, cbModbus cb = nullptr, uint16_t numregs = 1);
```

- `address`   Address of register assign callback on
- `cb`    Callback función or NULL to remove all the callbacks.
- `numregs`   Count of sequental segisters remove this callback to.

Disconnect specific callback función or all callbacks of the type if cb=NULL.

```c
typedef Modbus::ResultCode (*cbRequest)(Modbus::FunctionCode fc, const Modbus::RequestData data);
bool onRequest(cbRequest cb = _onRequestDefault);
bool onRequestSuccess(cbRequest cb = _onRequestDefault);

union Modbus::RequestData {
    struct {
        TAddress reg;
        uint16_t regCount;
    };
    struct {
        TAddress regRead;
        uint16_t regReadCount;
        TAddress regWrite;
        uint16_t regWriteCount;
    };
    struct {
        TAddress regMask;
        uint16_t yMask;
        uint16_t orMask;
    };
};
```

Callback función receives Modbus función code, structure `Modbus::RequestData` containing register type y offset (`TAddress` structure) y count of registers requested. The función should return [result code](#Result codes *Modbus::ResultCode*) `Modbus::EX_SUCCESS` to allow request processing or Modbus error code to block processing. This code will be returned to client/master.

```c
void onConnect(cbModbusConnect cb);
void onDisonnect(cbModbusConnect cb);
```

Assign callback función on incoming connection event.

```c
typedef bool (*cbModbusConnect)(IPAddress ip);
```

- `ip` Cliente's address of incomig connection source. `INADDR_NONE` for on disconnect callback.

## Result codes *Modbus::ResultCode*

|Value|Hex|Definition|Decription|
|---|---|---|---|
|Modbus::EX_SUCCESS|0x00|Custom|No error|
|Modbus::EX_ILLEGAL_FUNCTION|0x01|Modbus|Function Code not Supported|
|Modbus::EX_ILLEGAL_ADDRESS|0x02|Modbus|Output Address not exists|
|Modbus::EX_ILLEGAL_VALUE|0x03|Modbus|Output Value not in Range|
|Modbus::EX_SLAVE_FAILURE|0x04|Modbus|Slave or Master Device Fails to process request
|Modbus::EX_ACKNOWLEDGE|0x05|Modbus|Not used|
|Modbus::EX_SLAVE_DEVICE_BUSY|0x06|Modbus|Not used|
|Modbus::EX_MEMORY_PARITY_ERROR|0x08|Modbus|Not used|
|Modbus::EX_PATH_UNAVAILABLE|0x0A|Modbus|Not used|
|Modbus::EX_DEVICE_FAILED_TO_RESPOND|0x0B|Modbus|Not used|
|Modbus::EX_GENERAL_FAILURE|0xE1|Custom|Unexpected master error|
|Modbus::EX_DATA_MISMACH|0xE2|Custom|Inpud data size mismach|
|Modbus::EX_UNEXPECTED_RESPONSE|0xE3|Custom|Returned result doesn't mach transaction|
|Modbus::EX_TIMEOUT|0xE4|Custom|Operation not finished within reasonable time|
|Modbus::EX_CONNECTION_LOST|0xE5|Custom|Connection with device lost|
|Modbus::EX_CANCEL|0xE6|Custom|Transaction/request canceled|

# Biblioteca Modbus para Arduino
### ModbusRTU, ModbusTCP y ModbusTCP Security

(c)2020 [Alexyer Emelianov](mailto:a.m.emelianov@gmail.com)

El código en este repositorio está licenciado bajo la Licencia BSD Nueva. Ver LICENSE.txt para más información.