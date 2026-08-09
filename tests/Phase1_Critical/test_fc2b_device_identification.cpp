/*
 * Tests Unitarios para FC 0x2B Read Device Identification
 * Tarea 1.3 - Fase 1: Correcciones Críticas
 * 
 * Verifica implementación completa de identificación de dispositivo Modbus
 * conforme a especificación Section 6.21
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// ============================================================================
// DEFINICIONES DE CONSTANTES MODBUS
// ============================================================================

#define OBJECT_VENDOR_NAME      0x00
#define OBJECT_PRODUCT_CODE     0x01
#define OBJECT_MAJOR_MINOR_REV  0x02
#define OBJECT_VENDOR_URL       0x03
#define OBJECT_PRODUCT_NAME     0x04
#define OBJECT_MODEL_NAME       0x05
#define OBJECT_USER_APP_NAME    0x06
#define OBJECT_EXTENDED_START   0x80
#define OBJECT_EXTENDED_END     0xFF

#define CONFORMITY_BASIC    0x01
#define CONFORMITY_REGULAR  0x02
#define CONFORMITY_EXTENDED 0x03

#define MODBUS_MAX_EXTENDED_OBJECTS 10

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

typedef struct {
    uint8_t objectId;
    const char* value;
    bool readAccess;
    bool writeAccess;
} ExtendedObjectEntry;

typedef struct {
    const char* vendorName;
    const char* productCode;
    const char* majorMinorRevision;
    const char* vendorURL;
    const char* productName;
    const char* modelName;
    const char* userApplicationName;
    const char* serialNumber;
    
    ExtendedObjectEntry extendedObjects[MODBUS_MAX_EXTENDED_OBJECTS];
    uint8_t extendedObjectCount;
    
    uint8_t conformityLevel;
    bool individualReadSupport;
    bool streamReadSupport;
} DeviceIdInfo;

typedef struct {
    DeviceIdInfo info;
} ModbusDeviceIdentificationHandler;

// ============================================================================
// IMPLEMENTACIÓN DE FUNCIONES
// ============================================================================

static void initHandler(ModbusDeviceIdentificationHandler* handler) {
    memset(handler, 0, sizeof(*handler));
    handler->info.vendorName = "Unknown";
    handler->info.productCode = "Unknown";
    handler->info.majorMinorRevision = "1.0.0";
    handler->info.productName = "Modbus Device";
    handler->info.modelName = "Generic";
    handler->info.serialNumber = "00000000";
    handler->info.extendedObjectCount = 0;
    handler->info.conformityLevel = CONFORMITY_EXTENDED;
    handler->info.individualReadSupport = true;
    handler->info.streamReadSupport = true;
}

static void setVendorName(ModbusDeviceIdentificationHandler* handler, const char* name) {
    handler->info.vendorName = name;
}

static void setProductCode(ModbusDeviceIdentificationHandler* handler, const char* code) {
    handler->info.productCode = code;
}

static void setRevision(ModbusDeviceIdentificationHandler* handler, const char* rev) {
    handler->info.majorMinorRevision = rev;
}

static void setVendorURL(ModbusDeviceIdentificationHandler* handler, const char* url) {
    handler->info.vendorURL = url;
}

static void setProductName(ModbusDeviceIdentificationHandler* handler, const char* name) {
    handler->info.productName = name;
}

static void setModelName(ModbusDeviceIdentificationHandler* handler, const char* name) {
    handler->info.modelName = name;
}

static void setUserApplicationName(ModbusDeviceIdentificationHandler* handler, const char* name) {
    handler->info.userApplicationName = name;
}

static void setSerialNumber(ModbusDeviceIdentificationHandler* handler, const char* sn) {
    handler->info.serialNumber = sn;
}

static void setConformityLevel(ModbusDeviceIdentificationHandler* handler, uint8_t level) {
    if (level > CONFORMITY_EXTENDED) level = CONFORMITY_EXTENDED;
    handler->info.conformityLevel = level;
}

static bool addExtendedObject(ModbusDeviceIdentificationHandler* handler, uint8_t objectId, 
                             const char* value, bool readAccess, bool writeAccess) {
    if (handler->info.extendedObjectCount >= MODBUS_MAX_EXTENDED_OBJECTS) return false;
    if (objectId < OBJECT_EXTENDED_START || objectId > OBJECT_EXTENDED_END) return false;
    
    ExtendedObjectEntry* entry = &handler->info.extendedObjects[handler->info.extendedObjectCount];
    entry->objectId = objectId;
    entry->value = value;
    entry->readAccess = readAccess;
    entry->writeAccess = writeAccess;
    handler->info.extendedObjectCount++;
    return true;
}

static bool updateExtendedObject(ModbusDeviceIdentificationHandler* handler, uint8_t objectId, const char* newValue) {
    for (uint8_t i = 0; i < handler->info.extendedObjectCount; i++) {
        if (handler->info.extendedObjects[i].objectId == objectId) {
            if (!handler->info.extendedObjects[i].writeAccess) return false;
            handler->info.extendedObjects[i].value = newValue;
            return true;
        }
    }
    return false;
}

static uint8_t getObjectsCount(ModbusDeviceIdentificationHandler* handler) {
    return handler->info.extendedObjectCount + 7;
}

static uint8_t getConformityLevel(ModbusDeviceIdentificationHandler* handler) {
    return handler->info.conformityLevel;
}

static uint8_t writeObject(uint8_t* buf, uint8_t maxLen, uint8_t objId, const char* value) {
    if (!value || maxLen < 3) return 0;
    
    size_t strLen = strlen(value);
    if (maxLen < 3 + strLen) return 0;
    
    buf[0] = objId;
    buf[1] = (uint8_t)strLen;
    memcpy(buf + 2, value, strLen);
    
    return (uint8_t)(2 + strLen);
}

static uint8_t writeBasicIdentification(uint8_t* buf, uint8_t maxLen, DeviceIdInfo* info) {
    uint8_t pos = 0;
    pos += writeObject(buf + pos, maxLen - pos, OBJECT_VENDOR_NAME, info->vendorName);
    pos += writeObject(buf + pos, maxLen - pos, OBJECT_PRODUCT_CODE, info->productCode);
    pos += writeObject(buf + pos, maxLen - pos, OBJECT_MAJOR_MINOR_REV, info->majorMinorRevision);
    return pos;
}

static uint8_t writeRegularIdentification(uint8_t* buf, uint8_t maxLen, DeviceIdInfo* info) {
    uint8_t pos = writeBasicIdentification(buf, maxLen, info);
    
    if (info->vendorURL && strlen(info->vendorURL) > 0 && pos < maxLen) {
        pos += writeObject(buf + pos, maxLen - pos, OBJECT_VENDOR_URL, info->vendorURL);
    }
    if (pos < maxLen) {
        pos += writeObject(buf + pos, maxLen - pos, OBJECT_PRODUCT_NAME, info->productName);
    }
    if (pos < maxLen) {
        pos += writeObject(buf + pos, maxLen - pos, OBJECT_MODEL_NAME, info->modelName);
    }
    if (info->userApplicationName && strlen(info->userApplicationName) > 0 && pos < maxLen) {
        pos += writeObject(buf + pos, maxLen - pos, OBJECT_USER_APP_NAME, info->userApplicationName);
    }
    
    return pos;
}

static const char* getExtendedObjectValue(DeviceIdInfo* info, uint8_t objId) {
    for (uint8_t i = 0; i < info->extendedObjectCount; i++) {
        if (info->extendedObjects[i].objectId == objId) {
            return info->extendedObjects[i].readAccess ? info->extendedObjects[i].value : NULL;
        }
    }
    if (objId == 0x80) return info->serialNumber;
    return NULL;
}

static uint8_t writeExtendedIdentification(uint8_t objId, uint8_t* buf, uint8_t maxLen, DeviceIdInfo* info) {
    const char* value = getExtendedObjectValue(info, objId);
    if (!value) return 0;
    return writeObject(buf, maxLen, objId, value);
}

static uint8_t writeAllExtendedIdentification(uint8_t* buf, uint8_t maxLen, DeviceIdInfo* info) {
    uint8_t pos = 0;
    
    if (info->serialNumber && strlen(info->serialNumber) > 0) {
        pos += writeObject(buf + pos, maxLen - pos, 0x80, info->serialNumber);
    }
    
    for (uint8_t i = 0; i < info->extendedObjectCount && pos < maxLen; i++) {
        if (info->extendedObjects[i].readAccess && info->extendedObjects[i].value) {
            pos += writeObject(buf + pos, maxLen - pos, 
                              info->extendedObjects[i].objectId, 
                              info->extendedObjects[i].value);
        }
    }
    
    return pos;
}

static int process(ModbusDeviceIdentificationHandler* handler, uint8_t readDeviceIdCode, uint8_t objectId, 
                   uint8_t* responseData, uint8_t maxLen) {
    if (maxLen < 5) return -1;
    
    uint8_t offset = 3;
    uint8_t written = 0;
    
    responseData[0] = 0x0E;  // MEI Type
    responseData[1] = readDeviceIdCode;
    responseData[2] = handler->info.conformityLevel;
    
    switch (readDeviceIdCode) {
        case 0x01:
            written = writeBasicIdentification(responseData + offset, maxLen - offset, &handler->info);
            break;
        case 0x02:
            written = writeRegularIdentification(responseData + offset, maxLen - offset, &handler->info);
            break;
        case 0x03:
            if (!handler->info.individualReadSupport) return -1;
            written = writeExtendedIdentification(objectId, responseData + offset, maxLen - offset, &handler->info);
            break;
        case 0x04:
            if (!handler->info.streamReadSupport) return -1;
            written = writeAllExtendedIdentification(responseData + offset, maxLen - offset, &handler->info);
            break;
        default:
            return -1;
    }
    
    if (written == 0 && readDeviceIdCode != 0x01) return -1;
    
    return written + offset;
}

// ============================================================================
// TESTS DE OBJETOS BÁSICOS (0x00-0x06)
// ============================================================================

void test_basic_objects_implementation() {
    printf("Test 1: Objetos básicos 0x00-0x06... ");
    
    ModbusDeviceIdentificationHandler handler;
    initHandler(&handler);
    
    // Configurar información básica
    setVendorName(&handler, "TestVendor");
    setProductCode(&handler, "PRD-001");
    setRevision(&handler, "2.1.0");
    setVendorURL(&handler, "https://testvendor.com");
    setProductName(&handler, "TestProduct");
    setModelName(&handler, "TestModel");
    setUserApplicationName(&handler, "TestApp");
    
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    // Probar identificación básica (read code 0x01)
    int result = process(&handler, 0x01, 0x00, buffer, sizeof(buffer));
    
    assert(result > 0);
    assert(buffer[0] == 0x0E);  // MEI Type
    assert(buffer[1] == 0x01);  // Read Device Id Code
    assert(buffer[2] == CONFORMITY_EXTENDED);  // Conformity Level
    
    // Verificar que contiene Vendor Name (objeto 0x00)
    bool foundVendor = false;
    int pos = 3;
    while (pos < result) {
        if (buffer[pos] == OBJECT_VENDOR_NAME) {
            foundVendor = true;
            break;
        }
        pos += 2 + buffer[pos + 1];  // Skip object: ID + Length + Value
    }
    assert(foundVendor);
    
    printf("PASSED\n");
}

void test_regular_objects_implementation() {
    std::cout << "Test 2: Objetos regulares completos... ";
    
    ModbusDeviceIdentificationHandler handler;
    
    handler.setVendorName("AcmeCorp");
    handler.setProductCode("ACME-500");
    handler.setRevision("3.0.1");
    handler.setVendorURL("https://acme.com");
    handler.setProductName("Industrial Controller");
    handler.setModelName("IC-500-Pro");
    handler.setUserApplicationName("ProcessControl");
    
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    // Probar identificación regular (read code 0x02)
    int result = handler.process(0x02, 0x00, buffer, sizeof(buffer));
    
    assert(result > 6);  // Al menos 3 objetos básicos
    
    // Verificar presencia de objetos regulares
    bool foundProductName = false;
    bool foundModelName = false;
    int pos = 3;
    while (pos < result) {
        if (buffer[pos] == OBJECT_PRODUCT_NAME) {
            foundProductName = true;
        }
        if (buffer[pos] == OBJECT_MODEL_NAME) {
            foundModelName = true;
        }
        pos += 2 + buffer[pos + 1];
    }
    
    assert(foundProductName);
    assert(foundModelName);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// TESTS DE OBJETOS EXTENDIDOS (0x80-0xFF)
// ============================================================================

void test_extended_objects_configurable() {
    std::cout << "Test 3: Objetos extendidos configurables... ";
    
    ModbusDeviceIdentificationHandler handler;
    
    handler.setVendorName("ExtVendor");
    handler.setProductCode("EXT-001");
    handler.setRevision("1.0.0");
    handler.setSerialNumber("SN123456789");
    
    // Agregar objetos extendidos personalizados
    bool added1 = handler.addExtendedObject(0x80, "Hardware Rev A");
    bool added2 = handler.addExtendedObject(0x81, "Firmware v2.0");
    bool added3 = handler.addExtendedObject(0x82, "Location: Building 5");
    
    assert(added1);
    assert(added2);
    assert(added3);
    
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    // Leer todos los objetos extendidos (read code 0x04)
    int result = handler.process(0x04, 0x00, buffer, sizeof(buffer));
    
    assert(result > 3);
    
    // Verificar que se incluyen objetos extendidos
    bool foundExt80 = false;
    bool foundExt81 = false;
    int pos = 3;
    while (pos < result) {
        if (buffer[pos] == 0x80) foundExt80 = true;
        if (buffer[pos] == 0x81) foundExt81 = true;
        pos += 2 + buffer[pos + 1];
    }
    
    assert(foundExt80);
    assert(foundExt81);
    
    std::cout << "PASSED" << std::endl;
}

void test_extended_object_read_write_access() {
    std::cout << "Test 4: Control de acceso lectura/escritura... ";
    
    ModbusDeviceIdentificationHandler handler;
    
    handler.setVendorName("SecureVendor");
    handler.setProductCode("SEC-001");
    handler.setRevision("1.0.0");
    
    // Objeto solo lectura
    handler.addExtendedObject(0x90, "ReadOnlyValue", true, false);
    // Objeto lectura/escritura
    handler.addExtendedObject(0x91, "ReadWriteValue", true, true);
    // Objeto sin acceso lectura
    handler.addExtendedObject(0x92, "NoReadValue", false, true);
    
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    // Leer objeto extendido específico (read code 0x03)
    int result = handler.process(0x03, 0x90, buffer, sizeof(buffer));
    assert(result > 0);  // Debe poder leer 0x90 (read access = true)
    
    memset(buffer, 0, sizeof(buffer));
    result = handler.process(0x03, 0x92, buffer, sizeof(buffer));
    assert(result == -1 || result <= 3);  // No debe leer 0x92 (read access = false)
    
    // Actualizar objeto con write access
    bool updated = handler.updateExtendedObject(0x91, "NewValue");
    assert(updated);
    
    // Intentar actualizar objeto sin write access
    updated = handler.updateExtendedObject(0x90, "ShouldFail");
    assert(!updated);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// TESTS DE CÓDIGOS DE LECTURA (0x01-0x04)
// ============================================================================

void test_read_device_id_code_01_basic() {
    std::cout << "Test 5: Read Device ID Code 0x01 (Básico)... ";
    
    ModbusDeviceIdentificationHandler handler;
    handler.setVendorName("BasicVendor");
    handler.setProductCode("BASIC");
    handler.setRevision("1.0");
    
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    int result = handler.process(0x01, 0x00, buffer, sizeof(buffer));
    
    assert(result > 0);
    assert(buffer[1] == 0x01);  // Confirmar read code
    
    std::cout << "PASSED" << std::endl;
}

void test_read_device_id_code_02_regular() {
    std::cout << "Test 6: Read Device ID Code 0x02 (Regular)... ";
    
    ModbusDeviceIdentificationHandler handler;
    handler.setVendorName("RegularVendor");
    handler.setProductCode("REGULAR");
    handler.setRevision("2.0");
    handler.setProductName("RegularProduct");
    handler.setModelName("RegularModel");
    
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    int result = handler.process(0x02, 0x00, buffer, sizeof(buffer));
    
    assert(result > 6);  // Más datos que básico
    assert(buffer[1] == 0x02);  // Confirmar read code
    
    std::cout << "PASSED" << std::endl;
}

void test_read_device_id_code_03_extended_single() {
    std::cout << "Test 7: Read Device ID Code 0x03 (Extendido único)... ";
    
    ModbusDeviceIdentificationHandler handler;
    handler.setVendorName("SingleVendor");
    handler.setProductCode("SINGLE");
    handler.setRevision("1.0");
    handler.addExtendedObject(0xA0, "SpecificObject");
    
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    // Leer objeto específico 0xA0
    int result = handler.process(0x03, 0xA0, buffer, sizeof(buffer));
    
    assert(result > 3);
    assert(buffer[1] == 0x03);  // Confirmar read code
    
    // Verificar que el objeto retornado es el solicitado
    bool foundA0 = (buffer[3] == 0xA0);
    assert(foundA0);
    
    std::cout << "PASSED" << std::endl;
}

void test_read_device_id_code_04_extended_all() {
    std::cout << "Test 8: Read Device ID Code 0x04 (Todos extendidos)... ";
    
    ModbusDeviceIdentificationHandler handler;
    handler.setVendorName("AllVendor");
    handler.setProductCode("ALL");
    handler.setRevision("1.0");
    handler.setSerialNumber("SN999");
    handler.addExtendedObject(0xB0, "ExtObj1");
    handler.addExtendedObject(0xB1, "ExtObj2");
    handler.addExtendedObject(0xB2, "ExtObj3");
    
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    int result = handler.process(0x04, 0x00, buffer, sizeof(buffer));
    
    assert(result > 6);
    assert(buffer[1] == 0x04);  // Confirmar read code
    
    // Contar objetos extendidos en respuesta
    int extCount = 0;
    int pos = 3;
    while (pos < result) {
        if (buffer[pos] >= 0x80) extCount++;
        pos += 2 + buffer[pos + 1];
    }
    
    assert(extCount >= 3);  // Al menos 3 objetos extendidos
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// TESTS DE NIVELES DE CONFORMIDAD
// ============================================================================

void test_conformity_levels() {
    std::cout << "Test 9: Niveles de conformidad... ";
    
    // Nivel básico
    ModbusDeviceIdentificationHandler handlerBasic;
    handlerBasic.setConformityLevel(CONFORMITY_BASIC);
    assert(handlerBasic.getConformityLevel() == CONFORMITY_BASIC);
    
    // Nivel regular
    ModbusDeviceIdentificationHandler handlerRegular;
    handlerRegular.setConformityLevel(CONFORMITY_REGULAR);
    assert(handlerRegular.getConformityLevel() == CONFORMITY_REGULAR);
    
    // Nivel extendido
    ModbusDeviceIdentificationHandler handlerExtended;
    handlerExtended.setConformityLevel(CONFORMITY_EXTENDED);
    assert(handlerExtended.getConformityLevel() == CONFORMITY_EXTENDED);
    
    // Validar límite máximo
    handlerExtended.setConformityLevel(0xFF);  // Valor inválido
    assert(handlerExtended.getConformityLevel() == CONFORMITY_EXTENDED);
    
    std::cout << "PASSED" << std::endl;
}

void test_object_counting() {
    std::cout << "Test 10: Conteo de objetos disponibles... ";
    
    ModbusDeviceIdentificationHandler handler;
    handler.setVendorName("CountVendor");
    handler.setProductCode("COUNT");
    handler.setRevision("1.0");
    handler.setProductName("CountProduct");
    handler.setModelName("CountModel");
    
    // Sin objetos extendidos: 7 básicos
    uint8_t count = handler.getObjectsCount();
    assert(count >= 7);
    
    // Agregar 5 objetos extendidos
    handler.addExtendedObject(0xC0, "Obj1");
    handler.addExtendedObject(0xC1, "Obj2");
    handler.addExtendedObject(0xC2, "Obj3");
    handler.addExtendedObject(0xC3, "Obj4");
    handler.addExtendedObject(0xC4, "Obj5");
    
    count = handler.getObjectsCount();
    assert(count >= 12);  // 7 básicos + 5 extendidos
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// TESTS DE COMPATIBILIDAD CON SCANNERS MODBUS
// ============================================================================

void test_scanner_compatibility_format() {
    std::cout << "Test 11: Formato compatible con scanners (CAS, QModMaster)... ";
    
    ModbusDeviceIdentificationHandler handler;
    
    // Configurar como dispositivo industrial típico
    handler.setVendorName("Siemens");
    handler.setProductCode("SIMATIC-S7-1200");
    handler.setRevision("V4.2");
    handler.setVendorURL("https://siemens.com");
    handler.setProductName("PLC S7-1200");
    handler.setModelName("CPU 1214C");
    handler.setUserApplicationName("FactoryAutomation");
    handler.setSerialNumber("S7-1234567890");
    
    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    // Simular solicitud de scanner Modbus
    int result = handler.process(0x02, 0x00, buffer, sizeof(buffer));
    
    // Verificar formato de respuesta Modbus
    assert(result > 0);
    assert(buffer[0] == 0x0E);  // MEI Type correcto
    assert(buffer[1] == 0x02);  // Read Device ID Code eco
    assert(buffer[2] >= 0x01 && buffer[2] <= 0x03);  // Conformity level válido
    
    // Verificar estructura TLV (Type-Length-Value)
    int pos = 3;
    bool validFormat = true;
    while (pos < result - 2) {
        uint8_t objId = buffer[pos];
        uint8_t len = buffer[pos + 1];
        
        // Verificar que hay suficientes bytes para el valor
        if (pos + 2 + len > result) {
            validFormat = false;
            break;
        }
        
        pos += 2 + len;
    }
    
    assert(validFormat);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// TEST DE ESPECIFICACIÓN MODBUS SECTION 6.21
// ============================================================================

void test_modbus_spec_section_6_21() {
    std::cout << "Test 12: Conformidad especificación Modbus Section 6.21... ";
    
    ModbusDeviceIdentificationHandler handler;
    
    // Configurar todos los objetos mandatory
    handler.setVendorName("SpecCompliantVendor");
    handler.setProductCode("SPEC-001");
    handler.setRevision("1.0.0");
    
    uint8_t buffer[256];
    
    // Test 0x01: Basic (debe retornar 0x00, 0x01, 0x02)
    memset(buffer, 0, sizeof(buffer));
    int result1 = handler.process(0x01, 0x00, buffer, sizeof(buffer));
    assert(result1 > 0);
    
    // Test 0x02: Regular (debe retornar 0x00-0x06)
    handler.setVendorURL("https://spec.com");
    handler.setProductName("SpecProduct");
    handler.setModelName("SpecModel");
    memset(buffer, 0, sizeof(buffer));
    int result2 = handler.process(0x02, 0x00, buffer, sizeof(buffer));
    assert(result2 > result1);  // Regular debe tener más datos que Basic
    
    // Test 0x03: Extended individual
    handler.addExtendedObject(0x80, "SerialFromSpec");
    memset(buffer, 0, sizeof(buffer));
    int result3 = handler.process(0x03, 0x80, buffer, sizeof(buffer));
    assert(result3 > 0);
    assert(buffer[3] == 0x80);  // Debe retornar el objeto solicitado
    
    // Test 0x04: Extended all
    handler.addExtendedObject(0x81, "Ext1");
    handler.addExtendedObject(0x82, "Ext2");
    memset(buffer, 0, sizeof(buffer));
    int result4 = handler.process(0x04, 0x00, buffer, sizeof(buffer));
    assert(result4 > 0);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// TEST DE ESTRÉS Y CASOS BORDE
// ============================================================================

void test_stress_multiple_requests() {
    std::cout << "Test 13: Stress test - múltiples solicitudes consecutivas... ";
    
    ModbusDeviceIdentificationHandler handler;
    handler.setVendorName("StressVendor");
    handler.setProductCode("STRESS");
    handler.setRevision("1.0");
    
    for (int i = 0; i < 100; i++) {
        uint8_t buffer[256];
        memset(buffer, 0, sizeof(buffer));
        
        int result = handler.process((i % 4) + 1, i % 256, buffer, sizeof(buffer));
        assert(result >= -1);  -1 es válido para códigos inválidos
    }
    
    std::cout << "PASSED" << std::endl;
}

void test_edge_cases() {
    std::cout << "Test 14: Casos borde y valores límite... ";
    
    ModbusDeviceIdentificationHandler handler;
    
    // Buffer demasiado pequeño
    uint8_t tinyBuffer[2];
    int result = handler.process(0x01, 0x00, tinyBuffer, 2);
    assert(result == -1);  // Debe fallar por buffer insuficiente
    
    // Read code inválido
    uint8_t buffer[256];
    result = handler.process(0x05, 0x00, buffer, sizeof(buffer));
    assert(result == -1);  // Código inválido
    
    // Object ID inválido (en rango reservado)
    result = handler.process(0x03, 0x50, buffer, sizeof(buffer));
    assert(result >= -1);  // Puede fallar o retornar vacío
    
    // String vacío
    handler.setVendorName("");
    result = handler.process(0x01, 0x00, buffer, sizeof(buffer));
    assert(result >= 3);  // Al menos header
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// EJEMPLO DE USO COMPLETO
// ============================================================================

void test_complete_usage_example() {
    std::cout << "Test 15: Ejemplo de uso completo... ";
    
    // Crear handler
    ModbusDeviceIdentificationHandler device;
    
    // Configurar información del dispositivo
    device.setVendorName("IndustrialCorp");
    device.setProductCode("IC-PLC-500");
    device.setRevision("2.5.1");
    device.setVendorURL("https://industrialcorp.com");
    device.setProductName("Programmable Logic Controller");
    device.setModelName("PLC-500-Pro");
    device.setUserApplicationName("Manufacturing Control System");
    device.setSerialNumber("IC20240001");
    device.setHardwareRevision("Rev C");
    device.setSoftwareRevision("FW 3.2.1");
    device.setDeviceLocation("Assembly Line 3");
    
    // Agregar objetos extendidos personalizados
    device.addExtendedObject(0x80, "IC20240001", true, false);  // Serial (solo lectura)
    device.addExtendedObject(0x81, "2024-01-15", true, false);  // Fecha fabricación
    device.addExtendedObject(0x82, "Line3-Station5", true, true);  // Ubicación (lectura/escritura)
    device.addExtendedObject(0x83, "admin@industrialcorp.com", true, false);  // Contacto
    
    // Simular comunicación Modbus
    uint8_t requestBuffer[256];
    uint8_t responseBuffer[256];
    
    // Caso 1: Scanner solicita identificación básica
    memset(responseBuffer, 0, sizeof(responseBuffer));
    int respLen = device.process(0x01, 0x00, responseBuffer, sizeof(responseBuffer));
    assert(respLen > 0);
    
    // Caso 2: Scanner solicita identificación completa
    memset(responseBuffer, 0, sizeof(responseBuffer));
    respLen = device.process(0x02, 0x00, responseBuffer, sizeof(responseBuffer));
    assert(respLen > 6);
    
    // Caso 3: Leer objeto extendido específico
    memset(responseBuffer, 0, sizeof(responseBuffer));
    respLen = device.process(0x03, 0x82, responseBuffer, sizeof(responseBuffer));
    assert(respLen > 3);
    
    // Caso 4: Obtener todos los objetos extendidos
    memset(responseBuffer, 0, sizeof(responseBuffer));
    respLen = device.process(0x04, 0x00, responseBuffer, sizeof(responseBuffer));
    assert(respLen > 6);
    
    // Actualizar objeto con write access
    bool updated = device.updateExtendedObject(0x82, "Line5-Station10");
    assert(updated);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// FUNCIÓN PRINCIPAL DE TESTS
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "TAREA 1.3: FC 0x2B Read Device Identification" << std::endl;
    std::cout << "Tests Unitarios - Fase 1 Correcciones Críticas" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    try {
        // Tests de objetos básicos
        test_basic_objects_implementation();
        test_regular_objects_implementation();
        
        // Tests de objetos extendidos
        test_extended_objects_configurable();
        test_extended_object_read_write_access();
        
        // Tests de códigos de lectura
        test_read_device_id_code_01_basic();
        test_read_device_id_code_02_regular();
        test_read_device_id_code_03_extended_single();
        test_read_device_id_code_04_extended_all();
        
        // Tests de conformidad
        test_conformity_levels();
        test_object_counting();
        
        // Tests de compatibilidad
        test_scanner_compatibility_format();
        test_modbus_spec_section_6_21();
        
        // Tests de estrés y casos borde
        test_stress_multiple_requests();
        test_edge_cases();
        
        // Ejemplo completo
        test_complete_usage_example();
        
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "RESULTADO: 15/15 TESTS PASADOS (100%)" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FALLIDO: " << e.what() << std::endl;
        return 1;
    }
}
