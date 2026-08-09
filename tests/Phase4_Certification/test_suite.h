/**
 * @file test_suite.h
 * @brief Phase 4: Certification and Validation - Test Suite Framework
 * @version 4.2.0
 * @date 2024-08-09
 * 
 * @copyright Copyright (c) 2024
 * 
 * Comprehensive test suite for Modbus library certification
 * Includes: Unit tests, Integration tests, Stress tests, Compliance validation
 */

#ifndef MODBUS_TEST_SUITE_H
#define MODBUS_TEST_SUITE_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

// Test result codes
typedef enum {
    TEST_PASSED = 0,
    TEST_FAILED = 1,
    TEST_SKIPPED = 2,
    TEST_ERROR = 3
} TestResult_t;

// Test categories per Phase 4 requirements
typedef enum {
    CAT_UNIT_TESTS = 0,      // Individual function testing
    CAT_INTEGRATION = 1,     // Component interaction testing
    CAT_STRESS = 2,         // Load and endurance testing
    CAT_COMPLIANCE = 3,     // Modbus specification compliance
    CAT_SECURITY = 4,       // Security validation (Phase 1-3 features)
    CAT_PERFORMANCE = 5     // Performance benchmarks (Phase 3 optimizations)
} TestCategory_t;

// Test case structure
typedef struct {
    const char* name;
    TestCategory_t category;
    TestResult_t (*testFunction)(void);
    bool executed;
    TestResult_t result;
    uint32_t executionTimeMs;
    const char* description;
} TestCase_t;

// Test suite statistics
typedef struct {
    uint16_t totalTests;
    uint16_t passed;
    uint16_t failed;
    uint16_t skipped;
    uint16_t errors;
    uint32_t totalExecutionTimeMs;
    uint8_t coveragePercent;
} TestStats_t;

// Compliance validation structures
typedef struct {
    const char* specification;
    const char* section;
    bool required;
    bool implemented;
    bool validated;
    const char* evidence;
} ComplianceItem_t;

// Stress test configuration
typedef struct {
    uint32_t durationSeconds;
    uint32_t framesPerSecond;
    uint8_t loadPercent;
    bool enableRandomErrors;
    bool logAllFrames;
} StressTestConfig_t;

// Function prototypes
void testSuite_init(void);
TestResult_t testSuite_runAll(void);
TestResult_t testSuite_runCategory(TestCategory_t category);
void testSuite_printReport(void);
TestStats_t testSuite_getStats(void);

// Individual test declarations
TestResult_t test_crc_calculation(void);
TestResult_t test_frame_validation(void);
TestResult_t test_buffer_pool_allocation(void);
TestResult_t test_security_logging(void);
TestResult_t test_rate_limiting(void);
TestResult_t test_pdu_length_validation(void);
TestResult_t test_slave_id_handling(void);
TestResult_t test_timeout_management(void);
TestResult_t test_memory_safety(void);
TestResult_t test_performance_crc_lookup(void);
TestResult_t test_performance_pool_hit_rate(void);
TestResult_t test_stress_continuous_operation(void);
TestResult_t test_compliance_modbus_rtu_spec(void);
TestResult_t test_compliance_function_codes(void);
TestResult_t test_integration_rtU_serial(void);
TestResult_t test_integration_security_performance(void);

// Helper macros
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        Serial.print("FAIL: "); \
        Serial.println(message); \
        return TEST_FAILED; \
    }

#define TEST_ASSERT_EQUAL(expected, actual, message) \
    if ((expected) != (actual)) { \
        Serial.printf("FAIL: %s - Expected %d, got %d\n", message, (int)(expected), (int)(actual)); \
        return TEST_FAILED; \
    }

#define TEST_LOG_INFO(message) Serial.println(message)
#define TEST_LOG_ERROR(message) Serial.print("ERROR: "); Serial.println(message)

#endif // MODBUS_TEST_SUITE_H
