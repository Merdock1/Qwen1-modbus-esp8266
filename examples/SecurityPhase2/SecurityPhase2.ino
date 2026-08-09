/*
 * Example: Modbus RTU Security Logging - Phase 2
 * 
 * This example demonstrates how to use the Phase 2 security features:
 * - Security event logging
 * - Rate limiting
 * - Strict validation
 * - DoS protection
 * 
 * Hardware:
 * - Arduino ESP32/ESP8266 or any Arduino with Serial support
 * - RS485 module connected to Serial1 (or hardware serial)
 */

#include <ModbusRTU.h>
#include "ModbusSecurity.h"

// Create Modbus RTU instance
ModbusRTU mb;

// Security event counter
struct {
    uint32_t frameTooSmall;
    uint32_t frameTooLarge;
    uint32_t pduViolation;
    uint32_t mallocFailure;
    uint32_t crcMismatch;
    uint32_t slaveIdMismatch;
    uint32_t timeout;
    uint32_t securityPassed;
} securityStats = {0};

// Security Event Callback Function
// This function is called whenever a security event occurs
void securityLogCallback(const SecurityEvent_t* event) {
    // Increment statistics
    switch (event->eventType) {
        case SEC_EVENT_FRAME_TOO_SMALL:
            securityStats.frameTooSmall++;
            break;
        case SEC_EVENT_FRAME_TOO_LARGE:
            securityStats.frameTooLarge++;
            break;
        case SEC_EVENT_PDU_LENGTH_VIOLATION:
            securityStats.pduViolation++;
            break;
        case SEC_EVENT_MALLOC_FAILURE:
            securityStats.mallocFailure++;
            break;
        case SEC_EVENT_CRC_MISMATCH:
            securityStats.crcMismatch++;
            break;
        case SEC_EVENT_SLAVE_ID_MISMATCH:
            securityStats.slaveIdMismatch++;
            break;
        case SEC_EVENT_TIMEOUT:
            securityStats.timeout++;
            break;
        case SEC_EVENT_SECURITY_CHECK_PASSED:
            securityStats.securityPassed++;
            break;
        default:
            break;
    }
    
    // Print security event to Serial
    Serial.print("[SECURITY] ");
    Serial.print(micros() / 1000);
    Serial.print("ms - ");
    
    // Print severity
    switch (event->severity) {
        case SEC_SEVERITY_INFO:
            Serial.print("[INFO] ");
            break;
        case SEC_SEVERITY_WARNING:
            Serial.print("[WARNING] ");
            break;
        case SEC_SEVERITY_ERROR:
            Serial.print("[ERROR] ");
            break;
        case SEC_SEVERITY_CRITICAL:
            Serial.print("[CRITICAL] ");
            break;
    }
    
    // Print event type
    switch (event->eventType) {
        case SEC_EVENT_FRAME_TOO_SMALL:
            Serial.print("Frame too small");
            break;
        case SEC_EVENT_FRAME_TOO_LARGE:
            Serial.print("Frame too large");
            break;
        case SEC_EVENT_PDU_LENGTH_VIOLATION:
            Serial.print("PDU length violation");
            break;
        case SEC_EVENT_MALLOC_FAILURE:
            Serial.print("Memory allocation failure");
            break;
        case SEC_EVENT_CRC_MISMATCH:
            Serial.print("CRC mismatch");
            break;
        case SEC_EVENT_SLAVE_ID_MISMATCH:
            Serial.print("Slave ID mismatch");
            break;
        case SEC_EVENT_SECURITY_CHECK_PASSED:
            Serial.print("Security check passed");
            break;
        default:
            Serial.print("Unknown event");
            break;
    }
    
    Serial.print(" | SlaveID: ");
    Serial.print(event->slaveId);
    Serial.print(" | FuncCode: 0x");
    Serial.print(event->functionCode, HEX);
    Serial.print(" | FrameLen: ");
    Serial.print(event->frameLength);
    Serial.print(" | Desc: ");
    Serial.println(event->description);
}

// Register callback for holding registers
uint16_t onSetHreg(TRegister* reg, uint16_t val) {
    Serial.print("HREG ");
    Serial.print(reg->address.address);
    Serial.print(" set to ");
    Serial.println(val);
    return val;
}

uint16_t onGetHreg(TRegister* reg, uint16_t val) {
    return val;
}

void setup() {
    // Initialize Serial communication
    Serial.begin(115200);
    while (!Serial);
    
    Serial.println();
    Serial.println("=== Modbus RTU Security Phase 2 Example ===");
    Serial.println();
    
    // Initialize RS485 serial (adjust pins for your board)
    #if defined(ESP32)
        Serial1.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
    #elif defined(ESP8266)
        Serial1.begin(9600);
    #else
        Serial1.begin(9600);
    #endif
    
    // Initialize Modbus RTU as slave with ID=1
    mb.begin(&Serial1);
    mb.slave(1);
    
    // Add some holding registers for testing
    mb.addReg(HREG(0), 100);  // HREG 0 = 100
    mb.addReg(HREG(1), 200);  // HREG 1 = 200
    mb.addReg(HREG(10), 0);   // HREG 10 = writable
    
    // Setup callbacks
    mb.onSet(HREG(10), onSetHreg);
    mb.onGet(HREG(10), onGetHreg);
    
    // ============================================
    // PHASE 2 SECURITY CONFIGURATION
    // ============================================
    
    // Create security configuration
    SecurityConfig_t securityConfig = SECURITY_CONFIG_DEFAULT;
    
    // Enable security logging
    securityConfig.enableLogging = true;
    
    // Set the security log callback
    securityConfig.logCallback = securityLogCallback;
    
    // Enable strict Modbus compliance validation
    securityConfig.enableStrictValidation = true;
    
    // Enable DoS attack protection
    securityConfig.enableDoSProtection = true;
    
    // Enable rate limiting (optional, uncomment to enable)
    // securityConfig.enableRateLimiting = true;
    // securityConfig.maxEventsPerSecond = 100;  // Max 100 frames per second
    
    // Apply security configuration
    mb.setSecurityConfig(securityConfig);
    
    Serial.println("Security Configuration Applied:");
    Serial.print("  - Logging: ");
    Serial.println(securityConfig.enableLogging ? "ENABLED" : "DISABLED");
    Serial.print("  - Strict Validation: ");
    Serial.println(securityConfig.enableStrictValidation ? "ENABLED" : "DISABLED");
    Serial.print("  - DoS Protection: ");
    Serial.println(securityConfig.enableDoSProtection ? "ENABLED" : "DISABLED");
    Serial.print("  - Rate Limiting: ");
    Serial.println(securityConfig.enableRateLimiting ? "ENABLED" : "DISABLED");
    if (securityConfig.enableRateLimiting) {
        Serial.print("  - Max Events/sec: ");
        Serial.println(securityConfig.maxEventsPerSecond);
    }
    Serial.println();
    Serial.println("Modbus RTU Slave Ready (ID=1, Baud=9600)");
    Serial.println("Waiting for Modbus requests...");
    Serial.println();
}

uint32_t lastStatsTime = 0;

void loop() {
    // Process Modbus RTU communications
    mb.task();
    
    // Print statistics every 10 seconds
    if (millis() - lastStatsTime >= 10000) {
        Serial.println();
        Serial.println("=== Security Statistics ===");
        Serial.print("Frames passed security: ");
        Serial.println(securityStats.securityPassed);
        Serial.print("CRC mismatches: ");
        Serial.println(securityStats.crcMismatch);
        Serial.print("Slave ID mismatches: ");
        Serial.println(securityStats.slaveIdMismatch);
        Serial.print("Frames too small: ");
        Serial.println(securityStats.frameTooSmall);
        Serial.print("Frames too large: ");
        Serial.println(securityStats.frameTooLarge);
        Serial.print("PDU violations: ");
        Serial.println(securityStats.pduViolation);
        Serial.print("Memory allocation failures: ");
        Serial.println(securityStats.mallocFailure);
        Serial.println("===========================");
        Serial.println();
        
        lastStatsTime = millis();
    }
    
    delay(1);  // Small delay to prevent watchdog triggers
}

/*
 * Testing the Security Features:
 * 
 * 1. Normal Operation:
 *    - Send valid Modbus requests to read/write registers
 *    - Observe "Security check passed" messages
 * 
 * 2. CRC Mismatch Test:
 *    - Send a frame with incorrect CRC
 *    - Observe "CRC mismatch" error message
 * 
 * 3. Slave ID Mismatch Test:
 *    - Send request to different slave ID
 *    - Observe "Slave ID mismatch" warning
 * 
 * 4. Frame Size Attack Test:
 *    - Send oversized frame (>512 bytes)
 *    - Observe "Frame too large" critical message
 * 
 * 5. Rate Limiting Test (if enabled):
 *    - Send rapid successive requests (>100/sec)
 *    - Observe dropped frames when limit exceeded
 * 
 * Expected Output Example:
 * 
 * [SECURITY] 1234ms - [CRITICAL] Frame too large | SlaveID: 1 | FuncCode: 0x00 | FrameLen: 600 | Desc: Frame exceeds safe malloc limit
 * [SECURITY] 1235ms - [ERROR] CRC mismatch | SlaveID: 1 | FuncCode: 0x03 | FrameLen: 8 | Desc: CRC validation failed
 * [SECURITY] 1236ms - [INFO] Security check passed | SlaveID: 1 | FuncCode: 0x03 | FrameLen: 8 | Desc: Security validation passed
 */
