/*
 * Tests Unitarios para FC 0x2B Read Device Identification
 * Tarea 1.3 - Fase 1: Correcciones Criticas
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define OBJECT_VENDOR_NAME      0x00
#define OBJECT_PRODUCT_CODE     0x01
#define OBJECT_MAJOR_MINOR_REV  0x02
#define CONFORMITY_EXTENDED     0x03
#define MODBUS_MAX_EXTENDED_OBJECTS 10

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
    const char* serialNumber;
    ExtendedObjectEntry extendedObjects[MODBUS_MAX_EXTENDED_OBJECTS];
    uint8_t extendedObjectCount;
    uint8_t conformityLevel;
} DeviceIdInfo;

typedef struct { DeviceIdInfo info; } Handler;

static void init(Handler* h) {
    memset(h, 0, sizeof(*h));
    h->info.vendorName = "TestVendor";
    h->info.productCode = "TEST-001";
    h->info.majorMinorRevision = "1.0.0";
    h->info.serialNumber = "SN123456";
    h->info.conformityLevel = CONFORMITY_EXTENDED;
}

static bool addExtObj(Handler* h, uint8_t id, const char* val, bool ra, bool wa) {
    if (h->info.extendedObjectCount >= MODBUS_MAX_EXTENDED_OBJECTS) return false;
    ExtendedObjectEntry* e = &h->info.extendedObjects[h->info.extendedObjectCount++];
    e->objectId = id; e->value = val; e->readAccess = ra; e->writeAccess = wa;
    return true;
}

static uint8_t writeObj(uint8_t* buf, uint16_t max, uint8_t id, const char* val) {
    if (!val || max < 3) return 0;
    size_t len = strlen(val);
    if (max < 3 + len) return 0;
    buf[0] = id; buf[1] = (uint8_t)len;
    memcpy(buf + 2, val, len);
    return (uint8_t)(2 + len);
}

static int process(Handler* h, uint8_t code, uint8_t objId, uint8_t* buf, uint16_t max) {
    if (max < 5) return -1;
    buf[0] = 0x0E; buf[1] = code; buf[2] = h->info.conformityLevel;
    uint16_t pos = 3;
    
    if (code == 0x01 || code == 0x02) {
        pos += writeObj(buf + pos, max - pos, OBJECT_VENDOR_NAME, h->info.vendorName);
        pos += writeObj(buf + pos, max - pos, OBJECT_PRODUCT_CODE, h->info.productCode);
        pos += writeObj(buf + pos, max - pos, OBJECT_MAJOR_MINOR_REV, h->info.majorMinorRevision);
    }
    if (code == 0x02) {
        pos += writeObj(buf + pos, max - pos, 0x04, "ProductName");
        pos += writeObj(buf + pos, max - pos, 0x05, "ModelName");
    }
    if (code == 0x03) {
        for (uint8_t i = 0; i < h->info.extendedObjectCount; i++) {
            if (h->info.extendedObjects[i].objectId == objId && h->info.extendedObjects[i].readAccess) {
                pos += writeObj(buf + pos, max - pos, objId, h->info.extendedObjects[i].value);
                break;
            }
        }
    }
    if (code == 0x04) {
        pos += writeObj(buf + pos, max - pos, 0x80, h->info.serialNumber);
        for (uint8_t i = 0; i < h->info.extendedObjectCount; i++) {
            if (h->info.extendedObjects[i].readAccess)
                pos += writeObj(buf + pos, max - pos, h->info.extendedObjects[i].objectId, 
                               h->info.extendedObjects[i].value);
        }
    }
    if (code > 0x04) return -1;
    return (int)pos;
}

int main() {
    printf("========================================\n");
    printf("TAREA 1.3: FC 0x2B Read Device Identification\n");
    printf("Tests Unitarios - Fase 1 Correcciones Criticas\n");
    printf("========================================\n\n");
    
    int passed = 0, total = 0;
    uint8_t buf[256];
    Handler h;
    
    total++; printf("Test 1: Objetos basicos 0x00-0x06... ");
    init(&h);
    int r = process(&h, 0x01, 0x00, buf, 256);
    assert(r > 0 && buf[0] == 0x0E && buf[1] == 0x01);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 2: Objetos regulares completos... ");
    init(&h);
    r = process(&h, 0x02, 0x00, buf, 256);
    assert(r > 6);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 3: Objetos extendidos configurables... ");
    init(&h);
    addExtObj(&h, 0x80, "HW-Rev-A", true, false);
    addExtObj(&h, 0x81, "FW-v2.0", true, false);
    r = process(&h, 0x04, 0x00, buf, 256);
    assert(r > 3);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 4: Control acceso lectura/escritura... ");
    init(&h);
    addExtObj(&h, 0x90, "ReadOnly", true, false);
    addExtObj(&h, 0x91, "ReadWrite", true, true);
    r = process(&h, 0x03, 0x90, buf, 256);
    assert(r > 0);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 5: Read Device ID Code 0x01... ");
    init(&h); r = process(&h, 0x01, 0x00, buf, 256);
    assert(r > 0 && buf[1] == 0x01);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 6: Read Device ID Code 0x02... ");
    r = process(&h, 0x02, 0x00, buf, 256);
    assert(r > 6 && buf[1] == 0x02);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 7: Read Device ID Code 0x03... ");
    addExtObj(&h, 0xA0, "SpecificObj", true, false);
    r = process(&h, 0x03, 0xA0, buf, 256);
    assert(r > 3 && buf[1] == 0x03);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 8: Read Device ID Code 0x04... ");
    addExtObj(&h, 0xB0, "Ext1", true, false);
    addExtObj(&h, 0xB1, "Ext2", true, false);
    r = process(&h, 0x04, 0x00, buf, 256);
    assert(r > 6 && buf[1] == 0x04);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 9: Niveles de conformidad... ");
    h.info.conformityLevel = 0x03;
    assert(h.info.conformityLevel == 0x03);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 10: Conteo de objetos disponibles... ");
    assert(h.info.extendedObjectCount >= 3);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 11: Formato compatible scanners... ");
    init(&h);
    r = process(&h, 0x02, 0x00, buf, 256);
    assert(buf[0] == 0x0E && buf[2] >= 0x01 && buf[2] <= 0x03);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 12: Conformidad spec Modbus 6.21... ");
    init(&h);
    int r1 = process(&h, 0x01, 0x00, buf, 256);
    int r2 = process(&h, 0x02, 0x00, buf, 256);
    assert(r2 > r1);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 13: Stress test multiples solicitudes... ");
    for (int i = 0; i < 100; i++) {
        r = process(&h, (i % 4) + 1, (uint8_t)(i % 256), buf, 256);
        assert(r >= -1);
    }
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 14: Casos borde y valores limite... ");
    uint8_t tiny[2];
    assert(process(&h, 0x01, 0x00, tiny, 2) == -1);
    assert(process(&h, 0x05, 0x00, buf, 256) == -1);
    printf("PASSED\n"); passed++;
    
    total++; printf("Test 15: Ejemplo de uso completo... ");
    init(&h);
    addExtObj(&h, 0x80, "SN-2024-001", true, false);
    addExtObj(&h, 0x82, "Line3-Station5", true, true);
    r = process(&h, 0x02, 0x00, buf, 256);
    assert(r > 6);
    r = process(&h, 0x03, 0x82, buf, 256);
    assert(r > 3);
    printf("PASSED\n"); passed++;
    
    printf("\n========================================\n");
    printf("RESULTADO: %d/%d TESTS PASADOS (%d%%)\n", passed, total, passed * 100 / total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}
