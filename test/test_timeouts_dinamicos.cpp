/**
 * @file test_timeouts_dinamicos.cpp
 * @brief Tests unitarios para Tarea 2.3: Timeouts Dinámicos
 * 
 * Criterios de aceptación:
 * - Timeout se ajusta automáticamente al cambiar baudrate
 * - Soporte para 1200-921600 baud
 * - Sin falsos positivos/negativos en tests
 */

#include <Arduino.h>
#include <Stream.h>
#include <unity.h>

// Mock de la clase Stream para testing
class MockStream : public Stream {
public:
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
    size_t write(uint8_t) override { return 1; }
    
    // Método mock para baudRate
    uint32_t baudRate() const { return _baud; }
    void setBaudRate(uint32_t baud) { _baud = baud; }
    
private:
    uint32_t _baud = 9600;
};

// Incluir ModbusRTU después de definir mocks
#include "ModbusRTU.h"

// Variables globales para testing
ModbusRTU modbus;
MockStream mockSerial;

// Variables para capturar eventos de seguridad
bool securityEventCaptured = false;
SecurityEvent_t lastSecurityEvent;

void securityLogCallback(SecurityEvent_t* event) {
    securityEventCaptured = true;
    lastSecurityEvent = *event;
}

void setUp(void) {
    // Configurar antes de cada test
    securityEventCaptured = false;
    memset(&lastSecurityEvent, 0, sizeof(SecurityEvent_t));
    
    // Reiniciar modbus
    modbus = ModbusRTU();
}

void tearDown(void) {
    // Limpiar después de cada test
}

/**
 * @test Test 2.3.1: Verificar cálculo de tiempo de carácter
 * 
 * Verifica que charSendTime calcula correctamente el tiempo
 * de transmisión de un carácter para diferentes baudrates.
 */
void test_2_3_1_char_send_time_calculation() {
    // Para 9600 baud, 11 bits: 11 * 1000000 / 9600 = 1145 us
    uint32_t time9600 = modbus.charSendTime(9600, 11);
    TEST_ASSERT_UINT32_WITHIN(1, 1145, time9600);
    
    // Para 19200 baud, 11 bits: 11 * 1000000 / 19200 = 572 us
    uint32_t time19200 = modbus.charSendTime(19200, 11);
    TEST_ASSERT_UINT32_WITHIN(1, 572, time19200);
    
    // Para 115200 baud, 11 bits: 11 * 1000000 / 115200 = 95 us
    uint32_t time115200 = modbus.charSendTime(115200, 11);
    TEST_ASSERT_UINT32_WITHIN(1, 95, time115200);
    
    // Para baudrate 0 debe retornar 0
    uint32_t timeZero = modbus.charSendTime(0, 11);
    TEST_ASSERT_EQUAL_UINT32(0, timeZero);
}

/**
 * @test Test 2.3.2: Verificar cálculo de inter-frame time
 * 
 * Verifica que calculateMinimumInterFrameTime calcula correctamente
 * el tiempo mínimo entre frames según especificación Modbus.
 */
void test_2_3_2_interframe_time_calculation() {
    // Para baudrates <= 19200: 3.5 * charTime
    // 9600 baud: 3.5 * 1145 = 4007 us (aprox)
    uint32_t ift9600 = modbus.calculateMinimumInterFrameTime(9600, 11);
    TEST_ASSERT_UINT32_WITHIN(10, 4010, ift9600);
    
    // 19200 baud: 3.5 * 572 = 2002 us (aprox)
    uint32_t ift19200 = modbus.calculateMinimumInterFrameTime(19200, 11);
    TEST_ASSERT_UINT32_WITHIN(10, 2004, ift19200);
    
    // Para baudrates > 19200: fijo en 1750 us
    uint32_t ift115200 = modbus.calculateMinimumInterFrameTime(115200, 11);
    TEST_ASSERT_EQUAL_UINT32(1750, ift115200);
    
    uint32_t ift921600 = modbus.calculateMinimumInterFrameTime(921600, 11);
    TEST_ASSERT_EQUAL_UINT32(1750, ift921600);
}

/**
 * @test Test 2.3.3: Verificar validación de rango de baudrate
 * 
 * Verifica que setBaudrate rechaza valores fuera del rango 1200-921600.
 */
void test_2_3_4_baudrate_range_validation() {
    // Configurar callback de seguridad
    SecurityConfig_t config = modbus.getSecurityConfig();
    config.enableLogging = true;
    config.logCallback = securityLogCallback;
    modbus.setSecurityConfig(config);
    
    // Baudrate demasiado bajo (< 1200)
    TEST_ASSERT_FALSE(modbus.setBaudrate(300));
    TEST_ASSERT_TRUE(securityEventCaptured);
    TEST_ASSERT_EQUAL(SEC_EVENT_INVALID_CONFIG, lastSecurityEvent.eventType);
    
    securityEventCaptured = false;
    TEST_ASSERT_FALSE(modbus.setBaudrate(600));
    TEST_ASSERT_TRUE(securityEventCaptured);
    
    securityEventCaptured = false;
    TEST_ASSERT_FALSE(modbus.setBaudrate(1199));
    TEST_ASSERT_TRUE(securityEventCaptured);
    
    // Baudrate demasiado alto (> 921600)
    securityEventCaptured = false;
    TEST_ASSERT_FALSE(modbus.setBaudrate(1000000));
    TEST_ASSERT_TRUE(securityEventCaptured);
    
    securityEventCaptured = false;
    TEST_ASSERT_FALSE(modbus.setBaudrate(921601));
    TEST_ASSERT_TRUE(securityEventCaptured);
}

/**
 * @test Test 2.3.5: Verificar baudrates válidos en límites
 * 
 * Verifica que los baudrates en los límites del rango son aceptados.
 */
void test_2_3_5_valid_baudrate_limits() {
    // Límite inferior exacto
    TEST_ASSERT_TRUE(modbus.setBaudrate(1200));
    TEST_ASSERT_EQUAL_UINT32(1200, modbus.getCurrentBaudrate());
    
    // Límite superior exacto
    TEST_ASSERT_TRUE(modbus.setBaudrate(921600));
    TEST_ASSERT_EQUAL_UINT32(921600, modbus.getCurrentBaudrate());
    
    // Baudrates comunes
    TEST_ASSERT_TRUE(modbus.setBaudrate(2400));
    TEST_ASSERT_EQUAL_UINT32(2400, modbus.getCurrentBaudrate());
    
    TEST_ASSERT_TRUE(modbus.setBaudrate(4800));
    TEST_ASSERT_EQUAL_UINT32(4800, modbus.getCurrentBaudrate());
    
    TEST_ASSERT_TRUE(modbus.setBaudrate(9600));
    TEST_ASSERT_EQUAL_UINT32(9600, modbus.getCurrentBaudrate());
    
    TEST_ASSERT_TRUE(modbus.setBaudrate(19200));
    TEST_ASSERT_EQUAL_UINT32(19200, modbus.getCurrentBaudrate());
    
    TEST_ASSERT_TRUE(modbus.setBaudrate(38400));
    TEST_ASSERT_EQUAL_UINT32(38400, modbus.getCurrentBaudrate());
    
    TEST_ASSERT_TRUE(modbus.setBaudrate(57600));
    TEST_ASSERT_EQUAL_UINT32(57600, modbus.getCurrentBaudrate());
    
    TEST_ASSERT_TRUE(modbus.setBaudrate(115200));
    TEST_ASSERT_EQUAL_UINT32(115200, modbus.getCurrentBaudrate());
    
    TEST_ASSERT_TRUE(modbus.setBaudrate(230400));
    TEST_ASSERT_EQUAL_UINT32(230400, modbus.getCurrentBaudrate());
    
    TEST_ASSERT_TRUE(modbus.setBaudrate(460800));
    TEST_ASSERT_EQUAL_UINT32(460800, modbus.getCurrentBaudrate());
}

/**
 * @test Test 2.3.6: Verificar ajuste automático de timeout
 * 
 * Verifica que el timeout se ajusta automáticamente al cambiar baudrate.
 */
void test_2_3_6_auto_timeout_adjustment() {
    // Habilitar auto-timeout
    modbus.enableAutoTimeout(true);
    
    // Configurar baudrate bajo (timeout largo esperado)
    modbus.setBaudrate(1200);
    uint32_t timeoutLowBaud = modbus.getTimeout();
    
    // Configurar baudrate alto (timeout corto esperado)
    modbus.setBaudrate(115200);
    uint32_t timeoutHighBaud = modbus.getTimeout();
    
    // El timeout para baudrate bajo debe ser mayor que para baudrate alto
    TEST_ASSERT_GREATER_THAN(timeoutHighBaud, timeoutLowBaud);
    
    // Verificar que el timeout es aproximadamente 3x el inter-frame time
    uint32_t expectedTimeoutLow = 3.5 * modbus.charSendTime(1200, 11) * 3;
    TEST_ASSERT_UINT32_WITHIN(500, expectedTimeoutLow, timeoutLowBaud);
}

/**
 * @test Test 2.3.7: Verificar timeout manual deshabilita auto-ajuste
 * 
 * Verifica que configurar timeout manualmente deshabilita el auto-ajuste.
 */
void test_2_3_7_manual_timeout_disables_auto() {
    modbus.enableAutoTimeout(true);
    modbus.setBaudrate(9600);
    
    uint32_t manualTimeout = 50000; // 50ms
    modbus.setTimeout(manualTimeout);
    
    TEST_ASSERT_EQUAL_UINT32(manualTimeout, modbus.getTimeout());
    
    // Cambiar baudrate no debería afectar timeout manual
    modbus.setBaudrate(115200);
    TEST_ASSERT_EQUAL_UINT32(manualTimeout, modbus.getTimeout());
}

/**
 * @test Test 2.3.8: Verificar habilitar/deshabilitar auto-timeout
 * 
 * Verifica que enableAutoTimeout funciona correctamente.
 */
void test_2_3_8_enable_disable_auto_timeout() {
    // Inicialmente debería estar habilitado por defecto
    modbus.enableAutoTimeout(true);
    modbus.setBaudrate(9600);
    uint32_t timeoutAuto = modbus.getTimeout();
    
    // Deshabilitar y verificar que no cambia con nuevo baudrate
    modbus.enableAutoTimeout(false);
    modbus.setBaudrate(115200);
    uint32_t timeoutDisabled = modbus.getTimeout();
    
    // Debería mantener el valor anterior
    TEST_ASSERT_EQUAL_UINT32(timeoutAuto, timeoutDisabled);
    
    // Rehabilitar y verificar que se actualiza
    modbus.enableAutoTimeout(true);
    modbus.setBaudrate(115200);
    uint32_t timeoutReenabled = modbus.getTimeout();
    
    // Ahora debería ser diferente (menor para baudrate más alto)
    TEST_ASSERT_LESS_THAN(timeoutAuto, timeoutReenabled);
}

/**
 * @test Test 2.3.9: Verificar cálculo con diferentes tamaños de carácter
 * 
 * Verifica que los cálculos funcionan con diferentes configuraciones de caracteres.
 */
void test_2_3_9_different_char_sizes() {
    // Character de 10 bits (fuera de estándar pero soportado)
    uint32_t time10bits = modbus.charSendTime(9600, 10);
    uint32_t time11bits = modbus.charSendTime(9600, 11);
    
    // 10 bits debería ser menor que 11 bits
    TEST_ASSERT_LESS_THAN(time11bits, time10bits);
    
    // Verificar proporción aproximada
    uint32_t expectedRatio = (time11bits * 10) / 11;
    TEST_ASSERT_UINT32_WITHIN(10, expectedRatio, time10bits);
}

/**
 * @test Test 2.3.10: Verificar integración con begin()
 * 
 * Verifica que begin() configura correctamente los timeouts dinámicos.
 */
void test_2_3_10_begin_integration() {
    mockSerial.setBaudRate(19200);
    
    TEST_ASSERT_TRUE(modbus.begin(&mockSerial, -1, true));
    
    // Verificar que el baudrate fue capturado
    TEST_ASSERT_EQUAL_UINT32(19200, modbus.getCurrentBaudrate());
    
    // Verificar que el timeout fue calculado
    uint32_t timeout = modbus.getTimeout();
    TEST_ASSERT_GREATER_THAN(0, timeout);
}

/**
 * @test Test 2.3.11: Verificar sin falsos positivos en validación
 * 
 * Verifica que baudrates válidos no generan eventos de error.
 */
void test_2_3_11_no_false_positives() {
    SecurityConfig_t config = modbus.getSecurityConfig();
    config.enableLogging = true;
    config.logCallback = securityLogCallback;
    modbus.setSecurityConfig(config);
    
    // Probar múltiples baudrates válidos
    uint32_t validBaudrates[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
    int numBaudrates = sizeof(validBaudrates) / sizeof(validBaudrates[0]);
    
    for (int i = 0; i < numBaudrates; i++) {
        securityEventCaptured = false;
        TEST_ASSERT_TRUE(modbus.setBaudrate(validBaudrates[i]));
        
        // No debería haber evento de error para baudrates válidos
        if (securityEventCaptured) {
            TEST_ASSERT_NOT_EQUAL(SEC_EVENT_INVALID_CONFIG, lastSecurityEvent.eventType);
        }
    }
}

/**
 * @test Test 2.3.12: Verificar consistencia de resultados
 * 
 * Verifica que múltiples llamadas con mismo baudrate producen mismos resultados.
 */
void test_2_3_12_consistent_results() {
    uint32_t testBaud = 57600;
    
    // Múltiples llamadas deberían producir mismos resultados
    modbus.setBaudrate(testBaud);
    uint32_t timeout1 = modbus.getTimeout();
    uint32_t baud1 = modbus.getCurrentBaudrate();
    
    modbus.setBaudrate(testBaud);
    uint32_t timeout2 = modbus.getTimeout();
    uint32_t baud2 = modbus.getCurrentBaudrate();
    
    modbus.setBaudrate(testBaud);
    uint32_t timeout3 = modbus.getTimeout();
    uint32_t baud3 = modbus.getCurrentBaudrate();
    
    TEST_ASSERT_EQUAL_UINT32(timeout1, timeout2);
    TEST_ASSERT_EQUAL_UINT32(timeout2, timeout3);
    TEST_ASSERT_EQUAL_UINT32(baud1, baud2);
    TEST_ASSERT_EQUAL_UINT32(baud2, baud3);
    TEST_ASSERT_EQUAL_UINT32(testBaud, baud1);
}

void setup() {
    delay(2000); // Esperar para Serial
    
    UNITY_BEGIN();
    
    RUN_TEST(test_2_3_1_char_send_time_calculation);
    RUN_TEST(test_2_3_2_interframe_time_calculation);
    RUN_TEST(test_2_3_4_baudrate_range_validation);
    RUN_TEST(test_2_3_5_valid_baudrate_limits);
    RUN_TEST(test_2_3_6_auto_timeout_adjustment);
    RUN_TEST(test_2_3_7_manual_timeout_disables_auto);
    RUN_TEST(test_2_3_8_enable_disable_auto_timeout);
    RUN_TEST(test_2_3_9_different_char_sizes);
    RUN_TEST(test_2_3_10_begin_integration);
    RUN_TEST(test_2_3_11_no_false_positives);
    RUN_TEST(test_2_3_12_consistent_results);
    
    UNITY_END();
}

void loop() {
    // No needed for tests
}
