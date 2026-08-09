/*
    Modbus Library for Arduino
    Security Constants - Phase 1 Hardening
    
    Based on analysis of Modbus specification documents and security audit
*/
#pragma once

// Frame validation constants
#define MODBUS_MIN_FRAME_LEN 3          // Minimum valid frame: func + crc(2) or slaveId + func + crc(2)
#define MODBUS_MAX_PDU_LEN 253          // Max PDU size per Modbus spec (256 - overhead)
#define MODBUS_SAFE_MALLOC_SIZE 512     // Safety limit for dynamic allocation to prevent DoS
#define MODBUS_MAX_BUFFER_LEN 256       // Maximum allowed buffer length for any Modbus frame

// Security validation macros
#define MODBUS_VALIDATE_FRAME_LEN(len) \
    (((len) >= MODBUS_MIN_FRAME_LEN) && ((len) <= MODBUS_MAX_BUFFER_LEN))

#define MODBUS_VALIDATE_MALLOC_SIZE(len) \
    (((len) > 0) && ((len) <= MODBUS_SAFE_MALLOC_SIZE))

#define MODBUS_VALIDATE_PDU_LEN(len) \
    (((len) > 0) && ((len) <= MODBUS_MAX_PDU_LEN))
