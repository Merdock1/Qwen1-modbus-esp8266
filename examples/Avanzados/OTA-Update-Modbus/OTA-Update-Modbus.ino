/*
    OTA-Update-Modbus - Actualización Remota de Firmware
    TAREA 4.3: EJEMPLOS AVANZADOS
    
    Permite actualizar firmware remotamente vía comandos Modbus.
    
    Autor: Equipo Modbus
    Versión: 1.0.0
*/

#include <Modbus.h>
#include <ModbusRTU.h>
#include <HTTPClient.h>
#include <Update.h>

// Registros especiales para OTA
#define REG_OTA_COMMAND 1000
#define REG_OTA_URL_START 1001
#define REG_OTA_STATUS 1011
#define REG_OTA_PROGRESS 1012

// Estados OTA
#define OTA_IDLE 0
#define OTA_DOWNLOADING 1
#define OTA_INSTALLING 2
#define OTA_SUCCESS 3
#define OTA_ERROR 4

ModbusRTU mb;
String firmwareUrl = "";
uint8_t otaStatus = OTA_IDLE;
uint8_t otaProgress = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("=== OTA Update via Modbus ===");
}

void loop() {
    mb.task();
    
    // Verificar comando OTA
    if (otaStatus == OTA_DOWNLOADING) {
        downloadFirmware();
    }
}

void downloadFirmware() {
    HTTPClient http;
    http.begin(firmwareUrl);
    
    int contentLength = http.GET();
    if (contentLength > 0) {
        Update.begin(contentLength);
        http.getStream().writeTo(Update);
        
        if (Update.end()) {
            otaStatus = OTA_SUCCESS;
            ESP.restart();
        } else {
            otaStatus = OTA_ERROR;
        }
    } else {
        otaStatus = OTA_ERROR;
    }
}
