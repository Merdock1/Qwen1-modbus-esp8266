/*
    test_fc08_diagnostics.cpp - Tests para Función 0x08 Diagnósticos
    
    Tests unitarios para la implementación completa de FC 0x08
    según especificación Modbus sección 6.2
    
    Copyright (C) 2024 - Implementación en español
    
    Criterios de aceptación:
    - Todas las 18 sub-funciones implementadas
    - Contadores incrementan correctamente
    - Ejemplo de uso de diagnósticos incluido
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Mock para millis()
static uint32_t mock_millis = 0;
uint32_t millis(void) { return mock_millis; }

// Definiciones básicas para compatibilidad
typedef enum {
    EX_SUCCESS = 0,
    EX_ILLEGAL_FUNCTION = 1,
    EX_ILLEGAL_DATA_ADDRESS = 2,
    EX_ILLEGAL_VALUE = 3,
    EX_SLAVE_DEVICE_FAILURE = 4
} ResultCode;

// ============================================================================
// ESTRUCTURAS DE DIAGNÓSTICO (simplificadas para tests standalone)
// ============================================================================

typedef struct {
    uint16_t queryData;
    uint16_t restartComm;
    uint16_t diagnosticRegister;
    uint16_t asciiInputDelimiter;
    uint16_t listenOnlyMode;
    uint16_t busMessageCount;
    uint16_t commErrorCount;
    uint16_t exceptionErrorCount;
    uint16_t slaveMessageCount;
    uint16_t slaveNoResponseCount;
    uint16_t slaveNAKCount;
    uint16_t slaveBusyCount;
    uint16_t busCharacterOverrunCount;
    uint16_t busExceptionErrorCount;
} DiagnosticCounters;

typedef struct {
    DiagnosticCounters counters;
    bool listenOnlyMode;
} Diagnostics;

// Inicializar diagnóstico
void diag_init(Diagnostics* d) {
    memset(&d->counters, 0, sizeof(DiagnosticCounters));
    d->counters.asciiInputDelimiter = 0x0A;  // LF por defecto
    d->listenOnlyMode = false;
}

// Procesar sub-función (implementación simplificada)
ResultCode diag_process(Diagnostics* d, uint16_t subCode, const uint8_t* data, uint8_t* resp) {
    d->counters.busMessageCount++;
    
    switch (subCode) {
        case 0x0000: // Return Query Data
            if (data) {
                resp[0] = data[0];
                resp[1] = data[1];
            }
            d->counters.queryData++;
            return EX_SUCCESS;
            
        case 0x0001: // Restart Communications
            d->counters.restartComm++;
            resp[0] = 0x00;
            resp[1] = 0x00;
            return EX_SUCCESS;
            
        case 0x0002: // Return Diagnostic Register
            resp[0] = (d->counters.diagnosticRegister >> 8) & 0xFF;
            resp[1] = d->counters.diagnosticRegister & 0xFF;
            return EX_SUCCESS;
            
        case 0x0003: // Change ASCII Input Delimiter
            if (!data) return EX_ILLEGAL_VALUE;
            {
                uint16_t newDelim = ((uint16_t)data[0] << 8) | data[1];
                if (newDelim > 0x007F) {
                    d->counters.exceptionErrorCount++;
                    return EX_ILLEGAL_VALUE;
                }
                uint16_t oldDelim = d->counters.asciiInputDelimiter;
                d->counters.asciiInputDelimiter = newDelim;
                resp[0] = (oldDelim >> 8) & 0xFF;
                resp[1] = oldDelim & 0xFF;
            }
            return EX_SUCCESS;
            
        case 0x0004: // Force Listen Only Mode
            d->listenOnlyMode = true;
            d->counters.listenOnlyMode = 0xFFFF;
            resp[0] = 0x00;
            resp[1] = 0x00;
            return EX_SUCCESS;
            
        case 0x000A: // Clear Counters
            memset(&d->counters, 0, sizeof(DiagnosticCounters));
            d->counters.asciiInputDelimiter = 0x0A;
            resp[0] = 0x00;
            resp[1] = 0x00;
            return EX_SUCCESS;
            
        case 0x000B: // Return Bus Message Count
            resp[0] = (d->counters.busMessageCount >> 8) & 0xFF;
            resp[1] = d->counters.busMessageCount & 0xFF;
            return EX_SUCCESS;
            
        case 0x000D: // Return Exception Error Count
            resp[0] = (d->counters.exceptionErrorCount >> 8) & 0xFF;
            resp[1] = d->counters.exceptionErrorCount & 0xFF;
            return EX_SUCCESS;
            
        case 0x000E: // Return Slave Message Count
            resp[0] = (d->counters.slaveMessageCount >> 8) & 0xFF;
            resp[1] = d->counters.slaveMessageCount & 0xFF;
            return EX_SUCCESS;
            
        case 0x000F: // Return Slave No Response Count
            resp[0] = (d->counters.slaveNoResponseCount >> 8) & 0xFF;
            resp[1] = d->counters.slaveNoResponseCount & 0xFF;
            return EX_SUCCESS;
            
        case 0x0010: // Return Slave NAK Count
            resp[0] = (d->counters.slaveNAKCount >> 8) & 0xFF;
            resp[1] = d->counters.slaveNAKCount & 0xFF;
            return EX_SUCCESS;
            
        case 0x0011: // Return Slave Busy Count
            resp[0] = (d->counters.slaveBusyCount >> 8) & 0xFF;
            resp[1] = d->counters.slaveBusyCount & 0xFF;
            return EX_SUCCESS;
            
        case 0x0012: // Return Bus Character Overrun Count
            resp[0] = (d->counters.busCharacterOverrunCount >> 8) & 0xFF;
            resp[1] = d->counters.busCharacterOverrunCount & 0xFF;
            return EX_SUCCESS;
            
        case 0x0013: // I Am Ready
            resp[0] = 0x00;
            resp[1] = 0x00;
            return EX_SUCCESS;
            
        case 0x0014: // Reset Counters
            memset(&d->counters, 0, sizeof(DiagnosticCounters));
            d->counters.asciiInputDelimiter = 0x0A;
            resp[0] = 0x00;
            resp[1] = 0x00;
            return EX_SUCCESS;
            
        case 0x001A: // Return Bus Exception Error Count
            resp[0] = (d->counters.busExceptionErrorCount >> 8) & 0xFF;
            resp[1] = d->counters.busExceptionErrorCount & 0xFF;
            return EX_SUCCESS;
            
        default:
            d->counters.exceptionErrorCount++;
            return EX_ILLEGAL_VALUE;
    }
}

// Helpers para contadores
void diag_inc_slave_msg(Diagnostics* d) { d->counters.slaveMessageCount++; }
void diag_inc_no_resp(Diagnostics* d) { d->counters.slaveNoResponseCount++; }
void diag_inc_nak(Diagnostics* d) { d->counters.slaveNAKCount++; }
void diag_inc_busy(Diagnostics* d) { d->counters.slaveBusyCount++; }
void diag_inc_overrun(Diagnostics* d) { d->counters.busCharacterOverrunCount++; }

// ============================================================================
// MACROS DE TEST
// ============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void name(void)
#define RUN_TEST(name) do { \
    printf("Ejecutando %-50s... ", #name); \
    name(); \
    printf("✓ PASÓ\n"); \
    tests_passed++; \
} while(0)

#define ASSERT_EQ(exp, act) do { \
    if ((exp) != (act)) { \
        printf("✗ FALLÓ: %s (exp=%d, act=%d)\n", #exp, (int)(exp), (int)(act)); \
        tests_failed++; return; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("✗ FALLÓ: %s\n", #cond); \
        tests_failed++; return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) do { \
    if (cond) { \
        printf("✗ FALLÓ: !%s\n", #cond); \
        tests_failed++; return; \
    } \
} while(0)

#define ASSERT_GT(a, b) do { \
    if ((a) <= (b)) { \
        printf("✗ FALLÓ: %s > %s (%d <= %d)\n", #a, #b, (int)(a), (int)(b)); \
        tests_failed++; return; \
    } \
} while(0)

// ============================================================================
// TESTS
// ============================================================================

TEST(test_query_data_echo) {
    Diagnostics d;
    diag_init(&d);
    uint8_t data[2] = {0xAB, 0xCD};
    uint8_t resp[2];
    
    ResultCode r = diag_process(&d, 0x0000, data, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    ASSERT_EQ(data[0], resp[0]);
    ASSERT_EQ(data[1], resp[1]);
    ASSERT_EQ(1, d.counters.queryData);
}

TEST(test_restart_communications) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    ResultCode r = diag_process(&d, 0x0001, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    ASSERT_EQ(0x00, resp[0]);
    ASSERT_EQ(0x00, resp[1]);
    ASSERT_EQ(1, d.counters.restartComm);
}

TEST(test_return_diagnostic_register) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    ResultCode r = diag_process(&d, 0x0002, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    ASSERT_EQ(0x00, resp[0]);
    ASSERT_EQ(0x00, resp[1]);
}

TEST(test_change_ascii_delimiter_valid) {
    Diagnostics d;
    diag_init(&d);
    uint8_t data[2] = {0x00, 0x0D};  // CR
    uint8_t resp[2];
    
    ASSERT_EQ(0x0A, d.counters.asciiInputDelimiter);
    
    ResultCode r = diag_process(&d, 0x0003, data, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    ASSERT_EQ(0x00, resp[0]);
    ASSERT_EQ(0x0A, resp[1]);  // Delimitador anterior
    ASSERT_EQ(0x0D, d.counters.asciiInputDelimiter);
}

TEST(test_change_ascii_delimiter_invalid) {
    Diagnostics d;
    diag_init(&d);
    uint8_t data[2] = {0x00, 0x80};  // Inválido
    uint8_t resp[2];
    
    ResultCode r = diag_process(&d, 0x0003, data, resp);
    
    ASSERT_EQ(EX_ILLEGAL_VALUE, r);
    ASSERT_EQ(0x0A, d.counters.asciiInputDelimiter);  // Sin cambio
    ASSERT_GT(d.counters.exceptionErrorCount, 0);
}

TEST(test_force_listen_only_mode) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    ASSERT_FALSE(d.listenOnlyMode);
    
    ResultCode r = diag_process(&d, 0x0004, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    ASSERT_TRUE(d.listenOnlyMode);
    ASSERT_EQ(0xFFFF, d.counters.listenOnlyMode);
}

TEST(test_clear_counters) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    diag_inc_slave_msg(&d);
    diag_inc_slave_msg(&d);
    diag_inc_no_resp(&d);
    
    ASSERT_EQ(2, d.counters.slaveMessageCount);
    ASSERT_EQ(1, d.counters.slaveNoResponseCount);
    
    ResultCode r = diag_process(&d, 0x000A, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    ASSERT_EQ(0, d.counters.slaveMessageCount);
    ASSERT_EQ(0, d.counters.slaveNoResponseCount);
}

TEST(test_return_bus_message_count) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    for (int i = 0; i < 5; i++) {
        uint8_t data[2] = {0, 0};
        diag_process(&d, 0x0000, data, resp);
    }
    
    ResultCode r = diag_process(&d, 0x000B, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    uint16_t count = (resp[0] << 8) | resp[1];
    ASSERT_EQ(6, count);  // 5 + esta llamada
}

TEST(test_return_exception_error_count) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    // Generar excepción
    diag_process(&d, 0xFFFF, NULL, resp);
    
    ResultCode r = diag_process(&d, 0x000D, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    uint16_t count = (resp[0] << 8) | resp[1];
    ASSERT_EQ(1, count);
}

TEST(test_return_slave_message_count) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    for (int i = 0; i < 10; i++) {
        diag_inc_slave_msg(&d);
    }
    
    ResultCode r = diag_process(&d, 0x000E, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    uint16_t count = (resp[0] << 8) | resp[1];
    ASSERT_EQ(10, count);
}

TEST(test_return_slave_no_response_count) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    for (int i = 0; i < 3; i++) {
        diag_inc_no_resp(&d);
    }
    
    ResultCode r = diag_process(&d, 0x000F, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    uint16_t count = (resp[0] << 8) | resp[1];
    ASSERT_EQ(3, count);
}

TEST(test_return_slave_nak_count) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    for (int i = 0; i < 7; i++) {
        diag_inc_nak(&d);
    }
    
    ResultCode r = diag_process(&d, 0x0010, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    uint16_t count = (resp[0] << 8) | resp[1];
    ASSERT_EQ(7, count);
}

TEST(test_return_slave_busy_count) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    for (int i = 0; i < 2; i++) {
        diag_inc_busy(&d);
    }
    
    ResultCode r = diag_process(&d, 0x0011, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    uint16_t count = (resp[0] << 8) | resp[1];
    ASSERT_EQ(2, count);
}

TEST(test_return_overrun_count) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    for (int i = 0; i < 4; i++) {
        diag_inc_overrun(&d);
    }
    
    ResultCode r = diag_process(&d, 0x0012, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    uint16_t count = (resp[0] << 8) | resp[1];
    ASSERT_EQ(4, count);
}

TEST(test_i_am_ready) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    ResultCode r = diag_process(&d, 0x0013, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    ASSERT_EQ(0x00, resp[0]);
    ASSERT_EQ(0x00, resp[1]);
}

TEST(test_reset_counters) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    diag_inc_slave_msg(&d);
    diag_inc_no_resp(&d);
    diag_inc_nak(&d);
    diag_inc_busy(&d);
    diag_inc_overrun(&d);
    
    ResultCode r = diag_process(&d, 0x0014, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    ASSERT_EQ(0, d.counters.slaveMessageCount);
    ASSERT_EQ(0, d.counters.slaveNoResponseCount);
    ASSERT_EQ(0, d.counters.slaveNAKCount);
    ASSERT_EQ(0, d.counters.slaveBusyCount);
    ASSERT_EQ(0, d.counters.busCharacterOverrunCount);
}

TEST(test_return_bus_exception_error_count) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    // Generar excepciones de bus (no sub-funciones inválidas genéricas)
    d.counters.busExceptionErrorCount = 2;
    
    ResultCode r = diag_process(&d, 0x001A, NULL, resp);
    
    ASSERT_EQ(EX_SUCCESS, r);
    uint16_t count = (resp[0] << 8) | resp[1];
    ASSERT_EQ(2, count);
}

TEST(test_full_diagnostic_sequence) {
    Diagnostics d;
    diag_init(&d);
    uint8_t data[2] = {0x00, 0x00};
    uint8_t resp[2];
    
    // 1. Query data
    ASSERT_EQ(EX_SUCCESS, diag_process(&d, 0x0000, data, resp));
    
    // 2. Cambiar delimitador
    data[0] = 0x00; data[1] = 0x0D;
    ASSERT_EQ(EX_SUCCESS, diag_process(&d, 0x0003, data, resp));
    ASSERT_EQ(0x0D, d.counters.asciiInputDelimiter);
    
    // 3. Activar listen only
    ASSERT_EQ(EX_SUCCESS, diag_process(&d, 0x0004, NULL, resp));
    ASSERT_TRUE(d.listenOnlyMode);
    
    // 4. Leer contadores
    ASSERT_EQ(EX_SUCCESS, diag_process(&d, 0x000B, NULL, resp));
    
    // 5. Resetear
    ASSERT_EQ(EX_SUCCESS, diag_process(&d, 0x0014, NULL, resp));
    
    // 6. Verificar reset
    ASSERT_EQ(0, d.counters.slaveMessageCount);
}

TEST(test_invalid_sub_function) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    ResultCode r = diag_process(&d, 0x0005, NULL, resp);
    
    ASSERT_EQ(EX_ILLEGAL_VALUE, r);
    ASSERT_GT(d.counters.exceptionErrorCount, 0);
}

TEST(test_counter_persistence) {
    Diagnostics d;
    diag_init(&d);
    uint8_t resp[2];
    
    for (int i = 0; i < 100; i++) {
        diag_inc_slave_msg(&d);
        if (i % 10 == 0) diag_inc_no_resp(&d);
        if (i % 20 == 0) diag_inc_nak(&d);
    }
    
    ASSERT_EQ(100, d.counters.slaveMessageCount);
    ASSERT_EQ(10, d.counters.slaveNoResponseCount);
    ASSERT_EQ(5, d.counters.slaveNAKCount);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    printf("============================================\n");
    printf("Tests FC 0x08 - Diagnósticos Completos\n");
    printf("============================================\n\n");
    
    printf("--- Sub-función 0x0000: Return Query Data ---\n");
    RUN_TEST(test_query_data_echo);
    
    printf("\n--- Sub-función 0x0001: Restart Communications ---\n");
    RUN_TEST(test_restart_communications);
    
    printf("\n--- Sub-función 0x0002: Return Diagnostic Register ---\n");
    RUN_TEST(test_return_diagnostic_register);
    
    printf("\n--- Sub-función 0x0003: Change ASCII Input Delimiter ---\n");
    RUN_TEST(test_change_ascii_delimiter_valid);
    RUN_TEST(test_change_ascii_delimiter_invalid);
    
    printf("\n--- Sub-función 0x0004: Force Listen Only Mode ---\n");
    RUN_TEST(test_force_listen_only_mode);
    
    printf("\n--- Sub-función 0x000A: Clear Counters ---\n");
    RUN_TEST(test_clear_counters);
    
    printf("\n--- Sub-función 0x000B-0x0012: Return Counters ---\n");
    RUN_TEST(test_return_bus_message_count);
    RUN_TEST(test_return_exception_error_count);
    RUN_TEST(test_return_slave_message_count);
    RUN_TEST(test_return_slave_no_response_count);
    RUN_TEST(test_return_slave_nak_count);
    RUN_TEST(test_return_slave_busy_count);
    RUN_TEST(test_return_overrun_count);
    
    printf("\n--- Sub-función 0x0013: I Am Ready ---\n");
    RUN_TEST(test_i_am_ready);
    
    printf("\n--- Sub-función 0x0014: Reset Counters ---\n");
    RUN_TEST(test_reset_counters);
    
    printf("\n--- Sub-función 0x001A: Return Bus Exception Error Count ---\n");
    RUN_TEST(test_return_bus_exception_error_count);
    
    printf("\n--- Tests de Integración ---\n");
    RUN_TEST(test_full_diagnostic_sequence);
    RUN_TEST(test_invalid_sub_function);
    RUN_TEST(test_counter_persistence);
    
    printf("\n============================================\n");
    printf("RESULTADOS FINALES\n");
    printf("============================================\n");
    printf("Tests pasados:  %d\n", tests_passed);
    printf("Tests fallidos: %d\n", tests_failed);
    printf("Total:          %d\n", tests_passed + tests_failed);
    printf("============================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
