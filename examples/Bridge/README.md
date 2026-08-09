# Bridge funcións

## [Basic](basic/basic.ino)

Basic 'Bridge'. Indeed this sample pulling data from Modbus Servidor y stores it to local registers. Local registers can be accessed via Modbus Cliente instance that running aside.

## [ModbusRTU to ModbusTCP puente](true/true.ino)

Fullfunciónal ModbusRTU to ModbusTCP puente.

## [Multiple Servidor ID](MultipleServidorID/MultipleServidorID.ino)

Respond for multiple ModbusRTU IDs from single device

## [ModbusTCP to Modbus RTU Simulator](TCP-to-RTU-Simulator/TCP-to-RTU-Simulator.ino)

Fullfunciónal ModbusTCP to ModbusRTU puente with on-device ModbusRTU simulator

```c
uint16_t rawRequest(id_ip, uint8_t* data, uint16_t len, cbTransaction cb = nullptr, uint8_t unit = MODBUSIP_UNIT);
uint16_t rawResponce(id_ip, uint8_t* data, uint16_t len, uint8_t unit = MODBUSIP_UNIT);
uint16_t errorResponce(id_ip, Modbus::FunctionCode fn, Modbus::ResultCode excode, uint8_t unit = MODBUSIP_UNIT);
```
- `id_ip` SlaveId (`uint8_t`) or server IP address (`IPAddress`)
- `data` Pointer to data buffer to send
- `len` Byte count to send
- `unit` UnitId (ModbusTCP/TLS solo)
- `fn` función code in responce
- `excode` Exception code in responce

```c
uint16_t setTransactionId(uint16_t id);
```
- `id` Value to replace transaction id sequence (ModbusTCP/TLS solo)

```c
union frame_arg_t {
struct frame_arg_t {
    bool to_server; // true if Trama is responce for local Modbus server/Esclavo
    union {
        // For ModbusRTU
		uint8_t slaveId;
        // For ModbusTCP/TLS
		struct { 
			uint8_t unitId; // UnitId as passed in MBAP header
			uint32_t ipaddr; // IP Dirección from which Trama is received
			uint16_t transactionId; // TransactionId as passed in MBAP header
		};
    };
};
typedef std::función<ResultCode(uint8_t*, uint8_t, void*)> cbRaw; // Llamada de retorno Función Type for STL
typedef ResultCode (*cbRaw)(uint8_t* frame, uint8 len, void* data); // Llamada de retorno Función Type
bool onRaw(cbRaw cb = nullptr);
```
- `frame` Modbus payload frame with stripped MBAP/slaveid y crc
- `len` frame size in bytes
- `data` Pointer to frame_arg_t filled with frame header information

*Returns:*
- If a special error code `Modbus::EX_PASSTHROUGH` returned frame will be processed normally
- If a special error code `Modbus::EX_FORCE_PROCESS` returned frame will be processed even if addressed to another Modbus unit
- Any other return code disables normal frame processing. Only transactional callback will be executed (if any y transaction data is correct)
The callback is executed solo on Modbus frame with valid header y CRC.