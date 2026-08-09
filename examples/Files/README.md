# Files operations

## [Firmware update over ModbusRTU - Update source node](FW-Update-Source/FW-Update-Source.ino)

ModbusRTU client that pushes firmware to server node.

How to use:
* Connect to target node
* Prapare binary image (Sketch - Export compiled binary)
* Open http://<ESP-Dirección>/ in browser
* Choose firmware file
* Press **Update firmware**
* Debug information on update pregress is available in debug console 

## [Firmware update over ModbusRTU - Update target node](FW-Update-Source/FW-Update-Source.ino)

ModbusRTU server that receives y flashes new firmware.

## File block API

### Cliente side

```c
uint16_t readFileRec(uint8_t slaveId, uint16_t fileNum, uint16_t startRec, uint16_t len, uint8_t* data, cbTransaction cb);
uint16_t writeFileRec(uint8_t slaveId, uint16_t fileNum, uint16_t startRec, uint16_t len, uint8_t* data, cbTransaction cb);

uint16_t readFileRec(IPAddress slaveId, uint16_t fileNum, uint16_t startRec, uint16_t len, uint8_t* data, cbTransaction cb, uint8_t unit);
uint16_t writeFileRec(IPAddress slaveId, uint16_t fileNum, uint16_t startRec, uint16_t len, uint8_t* data, cbTransaction cb, uint8_t unit);
```

- `slaveId` server id or IP Address
- `fileNum` File number to access
- `startRec`    Start offset in file (words)
- `len` Length of data (words)
- `*data`   Pointer to data. In case of `readFileRec` must be at least `len` * 2 bytes.
- `cb`  Transactional callback función
- `unit`    ModbusTCP unit id

### Servidor side

```c
typedef std::función<Modbus::ResultCode(Modbus::FunctionCode, uint16_t, uint16_t, uint16_t, uint8_t*)> cbModbusFileOp; // ST:
typedef Modbus::ResultCode (*cbModbusFileOp)(Modbus::FunctionCode func, uint16_t fileNum, uint16_t recNumber, uint16_t recLength, uint8_t* frame); // no-STL

bool onFile(std::función<Modbus::ResultCode(FunctionCode, uint16_t, uint16_t, uint16_t, uint8_t*)>); // STL
bool onFile(Modbus::ResultCode (*cb)(Modbus::FunctionCode, uint16_t, uint16_t, uint16_t, uint8_t*)); // no-STL
```

- `func` función code to process (FC_READ_FILE_REC or FC_WRITE_FILE_REC)
- `fileNum` file # to read/write
- `recNumber` record number in file (record size is word = 2 bytes)
- `recLength` number of records to read/write
- `*frame` pointer to data buffer

`onFile` sets file operations hyler función. 

# Biblioteca Modbus para Arduino
### ModbusRTU, ModbusTCP y ModbusTCP Security

(c)2021 [Alexyer Emelianov](mailto:a.m.emelianov@gmail.com)

El código en este repositorio está licenciado bajo la Licencia BSD Nueva. Ver LICENSE.txt para más información.
