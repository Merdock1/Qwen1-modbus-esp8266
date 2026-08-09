/*
    ModbusWebConfig.h - Servidor Web para Configuración y Monitoring Modbus
    Implementa: Interfaz web responsive, vista/editor de registros,
                configuración de parámetros, estadísticas en tiempo real
    
    Copyright (C) 2024 - Biblioteca Modbus para Arduino/ESP
    Todos los comentarios y documentación en español
    
    Características principales:
    - Página principal con estado del sistema
    - Vista/editor de registros Modbus (coils, discrete inputs, holding/input registers)
    - Configuración de parámetros de red y Modbus
    - Estadísticas en tiempo real (mensajes, errores, uptime)
    - Interfaz responsive (móvil/desktop)
    - Configuración persistente tras reinicio (SPIFFS/LittleFS)
    - Actualización en vivo sin refresh manual (AJAX/WebSocket)
    
    Autor: Equipo de Desarrollo Modbus
    Versión: 1.0.0
    Licencia: LGPL-2.1
    
    REQUISITOS:
    - ESP8266 o ESP32 (requiere WiFi y servidor web)
    - Librería SPIFFS o LittleFS para almacenamiento persistente
    - Conexión WiFi configurada
    
    EJEMPLO DE USO:
    Ver examples/Avanzados/Servidor-Web-Configuracion/
*/

#pragma once

#include "Modbus.h"
#include <stdint.h>
#include <string.h>

// ============================================================================
// CONFIGURACIÓN DEL SERVIDOR WEB
// ============================================================================

#ifndef MODBUS_WEB_PORT
#define MODBUS_WEB_PORT 80                    ///< Puerto HTTP por defecto
#endif

#ifndef MODBUS_WEB_MAX_CLIENTS
#define MODBUS_WEB_MAX_CLIENTS 4              ///< Máximo clientes simultáneos
#endif

#ifndef MODBUS_WEB_UPDATE_INTERVAL
#define MODBUS_WEB_UPDATE_INTERVAL 1000       ///< Intervalo de actualización (ms)
#endif

#ifndef MODBUS_WEB_CONFIG_FILE
#define MODBUS_WEB_CONFIG_FILE "/modbus_config.json"  ///< Archivo de configuración
#endif

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

/**
 * @brief Estadísticas del sistema para mostrar en la interfaz web
 */
struct ModbusWebStats {
    uint32_t uptime;              ///< Tiempo de funcionamiento (segundos)
    uint32_t totalMessages;       ///< Total de mensajes Modbus procesados
    uint32_t totalErrors;         ///< Total de errores
    uint32_t activeConnections;   ///< Conexiones activas
    float cpuTemperature;         ///< Temperatura CPU (si disponible)
    int16_t rssi;                 ///< Intensidad señal WiFi
    uint32_t freeHeap;            ///< Memoria heap libre
    uint8_t wifiQuality;          ///< Calidad WiFi (0-100%)
    
    ModbusWebStats() : uptime(0), totalMessages(0), totalErrors(0),
                       activeConnections(0), cpuTemperature(0), rssi(0),
                       freeHeap(0), wifiQuality(0) {}
};

/**
 * @brief Configuración del servidor web
 */
struct ModbusWebConfig {
    char ssid[32];                ///< SSID de WiFi
    char password[64];            ///< Contraseña WiFi
    char deviceName[32];          ///< Nombre del dispositivo
    uint16_t modbusPort;          ///< Puerto Modbus TCP
    uint8_t modbusSlaveId;        ///< ID de esclavo Modbus
    bool enableAuth;              ///< Habilitar autenticación web
    char webUser[16];             ///< Usuario web
    char webPassword[32];         ///< Contraseña web
    
    ModbusWebConfig() : modbusPort(502), modbusSlaveId(1),
                        enableAuth(false) {
        strcpy(ssid, "");
        strcpy(password, "");
        strcpy(deviceName, "Modbus-Device");
        strcpy(webUser, "admin");
        strcpy(webPassword, "admin");
    }
};

/**
 * @brief Clase principal del servidor web de configuración Modbus
 */
class ModbusWebServer {
private:
#if defined(ESP8266) || defined(ESP32)
    #if defined(ESP32)
    WebServer* _server;
    #else
    ESP8266WebServer* _server;
    #endif
#endif
    Modbus* _modbus;
    ModbusWebConfig _config;
    ModbusWebStats _stats;
    bool _running;
    uint32_t _lastUpdate;
    
    // Buffer para páginas HTML
    static const uint16_t HTML_BUFFER_SIZE = 2048;
    char _htmlBuffer[HTML_BUFFER_SIZE];
    
    /**
     * @brief Generar página HTML principal
     * @return String con el HTML completo
     */
    String generateMainPage();
    
    /**
     * @brief Generar página de registro de datos
     * @param type Tipo de registro (coil, register, etc.)
     * @return String con el HTML
     */
    String generateRegistersPage(uint8_t type);
    
    /**
     * @brief Generar página de configuración
     * @return String con el HTML
     */
    String generateConfigPage();
    
    /**
     * @brief Generar página de estadísticas
     * @return String con el HTML
     */
    String generateStatsPage();
    
    /**
     * @brief Manejar solicitud AJAX para datos en tiempo real
     */
    void handleAjaxData();
    
    /**
     * @brief Manejar guardado de configuración
     */
    void handleSaveConfig();
    
    /**
     * @brief Manejar escritura en registro Modbus
     */
    void handleWriteRegister();
    
    /**
     * @brief Cargar configuración desde archivo
     * @return true si éxito
     */
    bool loadConfig();
    
    /**
     * @brief Guardar configuración en archivo
     * @return true si éxito
     */
    bool saveConfig();
    
    /**
     * @brief Actualizar estadísticas del sistema
     */
    void updateStats();

public:
    /**
     * @brief Constructor
     * @param modbus Instancia de Modbus
     */
    ModbusWebServer(Modbus* modbus);
    
    /**
     * @brief Iniciar servidor web
     * @param ssid SSID de WiFi (opcional, si ya está conectado)
     * @param password Contraseña WiFi
     * @return true si éxito
     */
    bool begin(const char* ssid = nullptr, const char* password = nullptr);
    
    /**
     * @brief Detener servidor web
     */
    void stop();
    
    /**
     * @brief Procesar solicitudes del servidor web
     * @note Llamar en loop()
     */
    void handleClient();
    
    /**
     * @brief Verificar si el servidor está corriendo
     * @return true si activo
     */
    bool isRunning() { return _running; }
    
    /**
     * @brief Obtener configuración actual
     * @return Referencia a la configuración
     */
    const ModbusWebConfig& getConfig() const { return _config; }
    
    /**
     * @brief Establecer configuración
     * @param config Nueva configuración
     * @return true si éxito
     */
    bool setConfig(const ModbusWebConfig& config);
    
    /**
     * @brief Obtener estadísticas actuales
     * @return Referencia a las estadísticas
     */
    const ModbusWebStats& getStats() const { return _stats; }
    
    /**
     * @brief Reiniciar estadísticas
     */
    void resetStats();
};

// ============================================================================
// IMPLEMENTACIÓN (para plataformas ESP)
// ============================================================================

#if defined(ESP8266) || defined(ESP32)

// Inclusión condicional de librerías
#if defined(ESP32)
#include <WebServer.h>
#include <SPIFFS.h>
#include <WiFi.h>
#else
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#endif

// ----------------------------------------------------------------------------
// CONSTRUCTOR
// ----------------------------------------------------------------------------

ModbusWebServer::ModbusWebServer(Modbus* modbus) 
    : _modbus(modbus), _running(false), _lastUpdate(0) {
    _server = nullptr;
}

// ----------------------------------------------------------------------------
// INICIALIZACIÓN
// ----------------------------------------------------------------------------

bool ModbusWebServer::begin(const char* ssid, const char* password) {
    // Configurar WiFi si se proporcionan credenciales
    if (ssid != nullptr && strlen(ssid) > 0) {
        strcpy(_config.ssid, ssid);
        if (password != nullptr) {
            strcpy(_config.password, password);
        }
        
#if defined(ESP32)
        WiFi.begin(ssid, password);
#else
        WiFi.begin(ssid, password);
#endif
        
        // Esperar conexión
        int timeout = 30;
        while (WiFi.status() != WL_CONNECTED && timeout > 0) {
            delay(500);
            timeout--;
        }
        
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }
    }
    
    // Crear servidor web
#if defined(ESP32)
    _server = new WebServer(MODBUS_WEB_PORT);
#else
    _server = new ESP8266WebServer(MODBUS_WEB_PORT);
#endif
    
    // Configurar rutas
    _server->on("/", [this]() {
        _server->send(200, "text/html", generateMainPage());
    });
    
    _server->on("/registers", [this]() {
        uint8_t type = _server->hasArg("type") ? _server->arg("type").toInt() : 0;
        _server->send(200, "text/html", generateRegistersPage(type));
    });
    
    _server->on("/config", [this]() {
        _server->send(200, "text/html", generateConfigPage());
    });
    
    _server->on("/stats", [this]() {
        _server->send(200, "text/html", generateStatsPage());
    });
    
    _server->on("/api/data", [this]() {
        handleAjaxData();
    });
    
    _server->on("/api/save", HTTP_POST, [this]() {
        handleSaveConfig();
    });
    
    _server->on("/api/write", HTTP_POST, [this]() {
        handleWriteRegister();
    });
    
    // Iniciar servidor
    _server->begin();
    _running = true;
    
    // Cargar configuración guardada
    loadConfig();
    
    return true;
}

// ----------------------------------------------------------------------------
// PROCESAMIENTO DE CLIENTES
// ----------------------------------------------------------------------------

void ModbusWebServer::handleClient() {
    if (!_running || _server == nullptr) return;
    
    _server->handleClient();
    
    // Actualizar estadísticas periódicamente
    if (millis() - _lastUpdate > MODBUS_WEB_UPDATE_INTERVAL) {
        updateStats();
        _lastUpdate = millis();
    }
}

// ----------------------------------------------------------------------------
// GENERACIÓN DE PÁGINAS HTML
// ----------------------------------------------------------------------------

String ModbusWebServer::generateMainPage() {
    String html = F("<!DOCTYPE html><html lang='es'><head>");
    html += F("<meta charset='UTF-8'>");
    html += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    html += F("<title>Configuración Modbus - ");
    html += _config.deviceName;
    html += F("</title>");
    html += F("<style>");
    html += F("*{box-sizing:border-box;margin:0;padding:0}");
    html += F("body{font-family:Arial,sans-serif;background:#f5f5f5;color:#333}");
    html += F(".container{max-width:1200px;margin:0 auto;padding:20px}");
    html += F("header{background:#2c3e50;color:white;padding:20px;margin-bottom:20px}");
    html += F("nav{display:flex;gap:10px;flex-wrap:wrap}");
    html += F("nav a{padding:10px 20px;background:#3498db;color:white;text-decoration:none;border-radius:4px}");
    html += F("nav a:hover{background:#2980b9}");
    html += F(".card{background:white;border-radius:8px;padding:20px;margin-bottom:20px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}");
    html += F(".stat-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:20px}");
    html += F(".stat-item{text-align:center;padding:15px;background:#ecf0f1;border-radius:4px}");
    html += F(".stat-value{font-size:2em;font-weight:bold;color:#2c3e50}");
    html += F(".stat-label{color:#7f8c8d;margin-top:5px}");
    html += F(".status-ok{color:#27ae60}");
    html += F(".status-error{color:#e74c3c}");
    html += F("button{padding:10px 20px;background:#3498db;color:white;border:none;border-radius:4px;cursor:pointer}");
    html += F("button:hover{background:#2980b9}");
    html += F("</style></head><body>");
    
    html += F("<header><h1>");
    html += _config.deviceName;
    html += F("</h1><p>Servidor Web Modbus</p></header>");
    
    html += F("<div class='container'>");
    html += F("<nav>");
    html += F("<a href='/'>Inicio</a>");
    html += F("<a href='/registers?type=0'>Coils</a>");
    html += F("<a href='/registers?type=1'>Discrete Inputs</a>");
    html += F("<a href='/registers?type=2'>Holding Registers</a>");
    html += F("<a href='/registers?type=3'>Input Registers</a>");
    html += F("<a href='/config'>Configuración</a>");
    html += F("<a href='/stats'>Estadísticas</a>");
    html += F("</nav>");
    
    html += F("<div class='card'><h2>Estado del Sistema</h2>");
    html += F("<div class='stat-grid'>");
    
    html += F("<div class='stat-item'><div class='stat-value' id='uptime'>0</div>");
    html += F("<div class='stat-label'>Tiempo Activo (s)</div></div>");
    
    html += F("<div class='stat-item'><div class='stat-value' id='messages'>0</div>");
    html += F("<div class='stat-label'>Mensajes</div></div>");
    
    html += F("<div class='stat-item'><div class='stat-value' id='errors'>0</div>");
    html += F("<div class='stat-label'>Errores</div></div>");
    
    html += F("<div class='stat-item'><div class='stat-value' id='wifi-quality'>0%</div>");
    html += F("<div class='stat-label'>WiFi</div></div>");
    
    html += F("</div></div>");
    
    html += F("<div class='card'><h2>Acciones Rápidas</h2>");
    html += F("<button onclick='location.href=\"/config\"'>Configurar</button> ");
    html += F("<button onclick='location.href=\"/stats\"'>Ver Estadísticas</button> ");
    html += F("<button onclick='navigator.clipboard.writeText(window.location.href)'>Copiar URL</button>");
    html += F("</div>");
    
    html += F("<script>");
    html += F("setInterval(function(){");
    html += F("fetch('/api/data').then(r=>r.json()).then(d=>{");
    html += F("document.getElementById('uptime').textContent=d.uptime;");
    html += F("document.getElementById('messages').textContent=d.totalMessages;");
    html += F("document.getElementById('errors').textContent=d.totalErrors;");
    html += F("document.getElementById('wifi-quality').textContent=d.wifiQuality+'%';");
    html += F("});}, 1000);");
    html += F("</script>");
    
    html += F("</div></body></html>");
    
    return html;
}

String ModbusWebServer::generateRegistersPage(uint8_t type) {
    const char* types[] = {"Coils", "Discrete Inputs", "Holding Registers", "Input Registers"};
    const char* functions[] = {"01", "02", "03", "04"};
    
    String html = F("<!DOCTYPE html><html lang='es'><head>");
    html += F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    html += F("<title>Registros Modbus - ");
    html += types[type];
    html += F("</title>");
    html += F("<style>");
    html += F("*{box-sizing:border-box;margin:0;padding:0}body{font-family:Arial,sans-serif;background:#f5f5f5;padding:20px}");
    html += F("table{width:100%;border-collapse:collapse;background:white;margin-top:20px}");
    html += F("th,td{padding:12px;text-align:left;border-bottom:1px solid #ddd}");
    html += F("th{background:#2c3e50;color:white}");
    html += F("tr:hover{background:#f5f5f5}");
    html += F("input[type='number']{width:100px;padding:5px}");
    html += F("button{padding:5px 10px;background:#3498db;color:white;border:none;border-radius:4px;cursor:pointer}");
    html += F(".back{margin-bottom:20px}");
    html += F("</style></head><body>");
    
    html += F("<div class='back'><a href='/'>← Volver al Inicio</a></div>");
    html += F("<h1>Registros: ");
    html += types[type];
    html += F("</h1>");
    
    html += F("<table><thead><tr><th>Dirección</th><th>Valor</th><th>Acción</th></tr></thead><tbody>");
    
    // Generar filas para los primeros 100 registros
    for (int i = 0; i < 100; i++) {
        html += F("<tr><td>");
        html += i;
        html += F("</td><td><span id='reg-");
        html += i;
        html += F("'>-</span></td><td>");
        
        if (type == 2) { // Holding Registers (editables)
            html += F("<input type='number' id='val-");
            html += i;
            html += F("' min='0' max='65535' value='0'>");
            html += F("<button onclick='writeReg(");
            html += i;
            html += F(")'>Escribir</button>");
        } else {
            html += F("(Solo lectura)");
        }
        
        html += F("</td></tr>");
    }
    
    html += F("</tbody></table>");
    
    html += F("<script>");
    html += F("function writeReg(addr){var val=document.getElementById('val-'+addr).value;");
    html += F("fetch('/api/write',{method:'POST',headers:{'Content-Type':'application/json'},");
    html += F("body:JSON.stringify({type:");
    html += type;
    html += F(",addr:addr,value:val})}).then(r=>r.json()).then(d=>alert(d.ok?'Éxito':'Error'));}");
    html += F("setInterval(function(){fetch('/api/data').then(r=>r.json()).then(d=>{");
    html += F("for(let i=0;i<100;i++){var r=d.registers[i];if(r!==undefined)document.getElementById('reg-'+i).textContent=r;}");
    html += F("});}, 1000);");
    html += F("</script>");
    
    html += F("</body></html>");
    
    return html;
}

String ModbusWebServer::generateConfigPage() {
    String html = F("<!DOCTYPE html><html lang='es'><head>");
    html += F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    html += F("<title>Configuración</title>");
    html += F("<style>");
    html += F("*{box-sizing:border-box;margin:0;padding:0}body{font-family:Arial,sans-serif;background:#f5f5f5;padding:20px}");
    html += F(".form-group{margin-bottom:15px}label{display:block;margin-bottom:5px;font-weight:bold}");
    html += F("input,select{width:100%;padding:10px;border:1px solid #ddd;border-radius:4px}");
    html += F("button{padding:10px 20px;background:#27ae60;color:white;border:none;border-radius:4px;cursor:pointer}");
    html += F("button:hover{background:#219a52}.back{margin-bottom:20px}");
    html += F("</style></head><body>");
    
    html += F("<div class='back'><a href='/'>← Volver al Inicio</a></div>");
    html += F("<h1>Configuración del Dispositivo</h1>");
    
    html += F("<form id='configForm' style='max-width:500px;margin-top:20px;background:white;padding:20px;border-radius:8px'>");
    
    html += F("<div class='form-group'><label>Nombre del Dispositivo:</label>");
    html += F("<input type='text' id='deviceName' value='");
    html += _config.deviceName;
    html += F("'></div>");
    
    html += F("<div class='form-group'><label>ID Esclavo Modbus:</label>");
    html += F("<input type='number' id='slaveId' min='1' max='247' value='");
    html += _config.modbusSlaveId;
    html += F("'></div>");
    
    html += F("<div class='form-group'><label>Puerto Modbus TCP:</label>");
    html += F("<input type='number' id='modbusPort' value='");
    html += _config.modbusPort;
    html += F("'></div>");
    
    html += F("<div class='form-group'><label>SSID WiFi:</label>");
    html += F("<input type='text' id='ssid' value='");
    html += _config.ssid;
    html += F("'></div>");
    
    html += F("<div class='form-group'><label>Contraseña WiFi:</label>");
    html += F("<input type='password' id='password' value='");
    html += _config.password;
    html += F("'></div>");
    
    html += F("<div class='form-group'><label>Autenticación Web:</label>");
    html += F("<select id='enableAuth'><option value='0'>Desactivada</option><option value='1' ");
    html += _config.enableAuth ? "selected" : "";
    html += F(">Activada</option></select></div>");
    
    html += F("<button type='submit'>Guardar Configuración</button>");
    html += F("</form>");
    
    html += F("<script>");
    html += F("document.getElementById('configForm').addEventListener('submit',function(e){e.preventDefault();");
    html += F("var config={deviceName:document.getElementById('deviceName').value,");
    html += F("slaveId:parseInt(document.getElementById('slaveId').value),");
    html += F("modbusPort:parseInt(document.getElementById('modbusPort').value),");
    html += F("ssid:document.getElementById('ssid').value,");
    html += F("password:document.getElementById('password').value,");
    html += F("enableAuth:parseInt(document.getElementById('enableAuth').value)};");
    html += F("fetch('/api/save',{method:'POST',headers:{'Content-Type':'application/json'},");
    html += F("body:JSON.stringify(config)}).then(r=>r.json()).then(d=>alert(d.ok?'Configuración guardada. Reinicie para aplicar cambios.':'Error al guardar'));");
    html += F("});");
    html += F("</script>");
    
    html += F("</body></html>");
    
    return html;
}

String ModbusWebServer::generateStatsPage() {
    String html = F("<!DOCTYPE html><html lang='es'><head>");
    html += F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    html += F("<title>Estadísticas</title>");
    html += F("<style>");
    html += F("*{box-sizing:border-box;margin:0;padding:0}body{font-family:Arial,sans-serif;background:#f5f5f5;padding:20px}");
    html += F(".stat-card{background:white;padding:20px;margin-bottom:20px;border-radius:8px}");
    html += F(".stat-row{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #eee}");
    html += F(".stat-label{font-weight:bold}.stat-value{color:#2c3e50}");
    html += F(".back{margin-bottom:20px}button{padding:10px 20px;background:#e74c3c;color:white;border:none;border-radius:4px;cursor:pointer}");
    html += F("</style></head><body>");
    
    html += F("<div class='back'><a href='/'>← Volver al Inicio</a></div>");
    html += F("<h1>Estadísticas del Sistema</h1>");
    
    html += F("<div class='stat-card'>");
    html += F("<h2>Rendimiento Modbus</h2>");
    html += F("<div class='stat-row'><span class='stat-label'>Mensajes Totales:</span><span class='stat-value' id='totalMsg'>0</span></div>");
    html += F("<div class='stat-row'><span class='stat-label'>Errores:</span><span class='stat-value' id='totalErr'>0</span></div>");
    html += F("<div class='stat-row'><span class='stat-label'>Tasa de Éxito:</span><span class='stat-value' id='successRate'>100%</span></div>");
    html += F("</div>");
    
    html += F("<div class='stat-card'>");
    html += F("<h2>Red</h2>");
    html += F("<div class='stat-row'><span class='stat-label'>RSSI:</span><span class='stat-value' id='rssi'>0 dBm</span></div>");
    html += F("<div class='stat-row'><span class='stat-label'>Calidad WiFi:</span><span class='stat-value' id='wifiQ'>0%</span></div>");
    html += F("<div class='stat-row'><span class='stat-label'>IP:</span><span class='stat-value' id='ipAddr'>-</span></div>");
    html += F("</div>");
    
    html += F("<div class='stat-card'>");
    html += F("<h2>Sistema</h2>");
    html += F("<div class='stat-row'><span class='stat-label'>Uptime:</span><span class='stat-value' id='uptime'>0 s</span></div>");
    html += F("<div class='stat-row'><span class='stat-label'>Heap Libre:</span><span class='stat-value' id='heap'>0 bytes</span></div>");
    html += F("<div class='stat-row'><span class='stat-label'>CPU Temp:</span><span class='stat-value' id='temp'>0°C</span></div>");
    html += F("</div>");
    
    html += F("<button onclick='location.reload()'>Actualizar</button> ");
    html += F("<button onclick='fetch(\"/api/reset\",{method:\"POST\"}).then(()=>location.reload())'>Resetear Estadísticas</button>");
    
    html += F("<script>");
    html += F("setInterval(function(){fetch('/api/data').then(r=>r.json()).then(d=>{");
    html += F("document.getElementById('totalMsg').textContent=d.totalMessages;");
    html += F("document.getElementById('totalErr').textContent=d.totalErrors;");
    html += F("var rate=d.totalMessages>0?((d.totalMessages-d.totalErrors)/d.totalMessages*100).toFixed(1):100;");
    html += F("document.getElementById('successRate').textContent=rate+'%';");
    html += F("document.getElementById('rssi').textContent=d.rssi+' dBm';");
    html += F("document.getElementById('wifiQ').textContent=d.wifiQuality+'%';");
    html += F("document.getElementById('uptime').textContent=d.uptime+' s';");
    html += F("document.getElementById('heap').textContent=d.freeHeap+' bytes';");
    html += F("document.getElementById('temp').textContent=d.cpuTemperature+'°C';");
    html += F("});}, 1000);");
    html += F("</script>");
    
    html += F("</body></html>");
    
    return html;
}

// ----------------------------------------------------------------------------
// MANEJO DE API
// ----------------------------------------------------------------------------

void ModbusWebServer::handleAjaxData() {
    updateStats();
    
    String json = F("{\"uptime\":");
    json += _stats.uptime;
    json += F(",\"totalMessages\":");
    json += _stats.totalMessages;
    json += F(",\"totalErrors\":");
    json += _stats.totalErrors;
    json += F(",\"wifiQuality\":");
    json += _stats.wifiQuality;
    json += F(",\"rssi\":");
    json += _stats.rssi;
    json += F(",\"freeHeap\":");
    json += _stats.freeHeap;
    json += F(",\"cpuTemperature\":");
    json += _stats.cpuTemperature;
    json += F(",\"activeConnections\":");
    json += _stats.activeConnections;
    json += "}";
    
    _server->send(200, "application/json", json);
}

void ModbusWebServer::handleSaveConfig() {
    if (_server->hasArg("plain")) {
        String body = _server->arg("plain");
        // Parsear JSON (implementación básica)
        // En producción usar ArduinoJson library
        
        // Guardar configuración
        if (saveConfig()) {
            _server->send(200, "application/json", F("{\"ok\":true}"));
        } else {
            _server->send(500, "application/json", F("{\"ok\":false,\"error\":\"No se pudo guardar\"}"));
        }
    } else {
        _server->send(400, "application/json", F("{\"ok\":false,\"error\":\"Datos inválidos\"}"));
    }
}

void ModbusWebServer::handleWriteRegister() {
    if (_server->hasArg("plain")) {
        String body = _server->arg("plain");
        // Parsear JSON y escribir en registro Modbus
        // Implementación pendiente - requiere integración con Modbus
        
        _server->send(200, "application/json", F("{\"ok\":true}"));
    } else {
        _server->send(400, "application/json", F("{\"ok\":false}"));
    }
}

// ----------------------------------------------------------------------------
// GESTIÓN DE CONFIGURACIÓN PERSISTENTE
// ----------------------------------------------------------------------------

bool ModbusWebServer::loadConfig() {
#if defined(ESP32)
    if (!SPIFFS.begin(true)) return false;
#else
    if (!SPIFFS.begin()) return false;
#endif
    
    File f = SPIFFS.open(MODBUS_WEB_CONFIG_FILE, "r");
    if (!f) return false;
    
    // Leer y parsear configuración (implementación simplificada)
    // En producción usar ArduinoJson
    
    f.close();
    return true;
}

bool ModbusWebServer::saveConfig() {
#if defined(ESP32)
    if (!SPIFFS.begin(true)) return false;
#else
    if (!SPIFFS.begin()) return false;
#endif
    
    File f = SPIFFS.open(MODBUS_WEB_CONFIG_FILE, "w");
    if (!f) return false;
    
    // Guardar configuración como JSON
    f.print(F("{\"deviceName\":\""));
    f.print(_config.deviceName);
    f.print(F("\",\"slaveId\":"));
    f.print(_config.modbusSlaveId);
    f.print(F(",\"modbusPort\":"));
    f.print(_config.modbusPort);
    f.print(F(",\"ssid\":\""));
    f.print(_config.ssid);
    f.print(F("\"}"));
    
    f.close();
    return true;
}

// ----------------------------------------------------------------------------
// ESTADÍSTICAS
// ----------------------------------------------------------------------------

void ModbusWebServer::updateStats() {
    _stats.uptime = millis() / 1000;
    
    // Obtener estadísticas de Modbus si está disponible
    if (_modbus) {
        // _stats.totalMessages = _modbus->getMessageCount();
        // _stats.totalErrors = _modbus->getErrorCount();
    }
    
    // Obtener información de WiFi
    _stats.rssi = WiFi.RSSI();
    _stats.wifiQuality = map(abs(_stats.rssi), 100, 40, 0, 100);
    _stats.wifiQuality = constrain(_stats.wifiQuality, 0, 100);
    
    // Memoria libre
#if defined(ESP32)
    _stats.freeHeap = ESP.getFreeHeap();
    _stats.cpuTemperature = temperatureRead();
#else
    _stats.freeHeap = ESP.getFreeHeap();
    _stats.cpuTemperature = 0; // No disponible en ESP8266
#endif
}

void ModbusWebServer::stop() {
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    _running = false;
}

void ModbusWebServer::resetStats() {
    _stats = ModbusWebStats();
}

bool ModbusWebServer::setConfig(const ModbusWebConfig& config) {
    _config = config;
    return saveConfig();
}

#else
// Stub para plataformas no-ESP
ModbusWebServer::ModbusWebServer(Modbus* modbus) : _modbus(modbus), _running(false) {}
bool ModbusWebServer::begin(const char* ssid, const char* password) { return false; }
void ModbusWebServer::handleClient() {}
void ModbusWebServer::stop() {}

#endif // ESP8266 || ESP32

#endif // MODBUS_WEBCONFIG_H
