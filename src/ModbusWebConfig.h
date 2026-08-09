/**
 * @file ModbusWebConfig.h
 * @brief Módulo de configuración web segura para dispositivos Modbus
 * @author Ingeniero de Software Senior
 * @version 1.0.0
 * @date 2024
 * 
 * Este módulo proporciona una interfaz web para configuración de dispositivos
 * Modbus, implementando validaciones estrictas de URLs y parámetros para
 * prevenir vulnerabilidades como buffer overflows e inyección de código.
 * Cumple con estándares IEC 62443 para seguridad industrial.
 */

#ifndef MODBUS_WEBCONFIG_H
#define MODBUS_WEBCONFIG_H

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WebServer.h>
#elif defined(ESP32)
#include <WebServer.h>
#endif
#include "Modbus.h"

// ============================================================================
// CONSTANTES Y CONFIGURACIÓN DE SEGURIDAD
// ============================================================================

/** @brief Longitud máxima de URL permitida (limitada por seguridad) */
#define WEB_URL_MAX_LEN 128

/** @brief Longitud máxima de parámetros GET/POST */
#define WEB_PARAM_MAX_LEN 64

/** @brief Longitud máxima de valores de formulario */
#define WEB_FORM_VALUE_MAX_LEN 128

/** @brief Número máximo de parámetros por solicitud */
#define WEB_MAX_PARAMS 10

/** @brief Timeout para sesiones web (ms) */
#define WEB_SESSION_TIMEOUT 300000

/** @brief Macro para sanitización de strings en URLs */
#define SANITIZE_URL_CHAR(c) (((c) >= 'a' && (c) <= 'z') || \
                              ((c) >= 'A' && (c) <= 'Z') || \
                              ((c) >= '0' && (c) <= '9') || \
                              (c) == '-' || (c) == '_' || (c) == '/' || (c) == '?')

/** @brief Macro para copia segura de strings con validación de límites */
#define SAFE_STRNCPY(dest, src, max_size) do { \
    strncpy((dest), (src), (max_size) - 1); \
    (dest)[(max_size) - 1] = '\0'; \
} while(0)

/** @brief Macro para concatenación segura de strings */
#define SAFE_STRNCAT(dest, src, max_size) do { \
    strncat((dest), (src), (max_size) - strlen((dest)) - 1); \
    (dest)[(max_size) - 1] = '\0'; \
} while(0)

/** @brief Macro para sprintf seguro con validación de longitud */
#define SAFE_SNPRINTF(dest, max_size, format, ...) do { \
    snprintf((dest), (max_size), (format), ##__VA_ARGS__); \
    (dest)[(max_size) - 1] = '\0'; \
} while(0)

// ============================================================================
// TIPOS Y ENUMERACIONES
// ============================================================================

/**
 * @enum WebAuthLevel
 * @brief Niveles de autenticación para acceso web
 */
enum WebAuthLevel {
    WEB_AUTH_NONE = 0,      ///< Sin autenticación (solo lectura)
    WEB_AUTH_USER = 1,      ///< Autenticación de usuario básico
    WEB_AUTH_ADMIN = 2      ///< Autenticación de administrador
};

/**
 * @struct WebConfigCredentials
 * @brief Credenciales de autenticación web con buffers seguros
 */
struct WebConfigCredentials {
    char username[32];      ///< Nombre de usuario (limitado)
    char password[32];      ///< Contraseña (limitada)
    WebAuthLevel level;     ///< Nivel de acceso
    
    /**
     * @brief Constructor con inicialización segura
     */
    WebConfigCredentials() : level(WEB_AUTH_NONE) {
        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));
    }
    
    /**
     * @brief Valida las credenciales
     * @return true si credenciales válidas, false en caso contrario
     */
    bool validate() const {
        return (strlen(username) > 0 && strlen(username) < sizeof(username) &&
                strlen(password) >= 8 && strlen(password) < sizeof(password));
    }
};

/**
 * @struct WebRequestParam
 * @brief Parámetro de solicitud web validado
 */
struct WebRequestParam {
    char key[WEB_PARAM_MAX_LEN];        ///< Clave del parámetro
    char value[WEB_FORM_VALUE_MAX_LEN]; ///< Valor del parámetro
    bool isValid;                       ///< Flag de validación
    
    /**
     * @brief Constructor por defecto
     */
    WebRequestParam() : isValid(false) {
        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
    }
    
    /**
     * @brief Establece clave con validación
     * @param newKey Clave a establecer
     * @return true si éxito, false si excede longitud
     */
    bool setKey(const char* newKey) {
        if (newKey == nullptr || strlen(newKey) >= WEB_PARAM_MAX_LEN) {
            return false;
        }
        // Sanitizar clave (solo caracteres alfanuméricos y guión bajo)
        size_t len = strlen(newKey);
        for (size_t i = 0; i < len && i < WEB_PARAM_MAX_LEN - 1; i++) {
            if (SANITIZE_URL_CHAR(newKey[i]) || newKey[i] == '_') {
                key[i] = newKey[i];
            } else {
                key[i] = '_'; // Reemplazar caracteres inválidos
            }
        }
        key[WEB_PARAM_MAX_LEN - 1] = '\0';
        return true;
    }
    
    /**
     * @brief Establece valor con validación de longitud
     * @param newValue Valor a establecer
     * @return true si éxito, false si excede longitud
     */
    bool setValue(const char* newValue) {
        if (newValue == nullptr || strlen(newValue) >= WEB_FORM_VALUE_MAX_LEN) {
            return false;
        }
        SAFE_STRNCPY(value, newValue, WEB_FORM_VALUE_MAX_LEN);
        isValid = true;
        return true;
    }
};

// ============================================================================
// CLASE PRINCIPAL: ModbusWebConfig
// ============================================================================

#ifdef ESP8266
typedef ESP8266WebServer WebServerType;
#elif defined(ESP32)
typedef WebServer WebServerType;
#else
#error "Plataforma no soportada para ModbusWebConfig"
#endif

/**
 * @class ModbusWebConfig
 * @brief Servidor web seguro para configuración de dispositivos Modbus
 * 
 * Esta clase proporciona una interfaz web para configurar parámetros
 * de dispositivos Modbus, implementando validaciones estrictas en todas
 * las entradas HTTP para prevenir vulnerabilidades de seguridad.
 * 
 * Características de seguridad:
 * - Validación de longitud de URLs y parámetros
 * - Sanitización de inputs contra inyección
 * - Autenticación básica con niveles de acceso
 * - Timeouts de sesión para prevenir ataques
 * - Gestión segura de memoria sin fugas
 * 
 * @note Todos los handlers validan sus parámetros antes de procesar
 */
class ModbusWebConfig {
private:
    WebServerType* server;              ///< Servidor web subyacente
    Modbus* modbusInstance;             ///< Instancia Modbus asociada
    uint16_t serverPort;                ///< Puerto del servidor web
    bool running;                       ///< Estado del servidor
    uint32_t lastActivityTime;          ///< Última actividad de sesión
    WebConfigCredentials adminCreds;    ///< Credenciales de administrador
    char* responseBuffer;               ///< Buffer para respuestas (RAII)
    size_t responseBufferSize;          ///< Tamaño del buffer de respuesta
    
    /**
     * @brief Libera recursos de memoria asignados dinámicamente
     * Método privado llamado en destructor y stop
     */
    void freeResources() {
        if (responseBuffer != nullptr) {
            delete[] responseBuffer;
            responseBuffer = nullptr;
            responseBufferSize = 0;
        }
        if (server != nullptr) {
            delete server;
            server = nullptr;
        }
    }
    
    /**
     * @brief Valida y sanitiza una URL entrante
     * @param url URL a validar
     * @param maxLen Longitud máxima permitida
     * @return true si URL válida, false en caso contrario
     */
    bool validateURL(const char* url, size_t maxLen);
    
    /**
     * @brief Extrae y valida parámetros de una solicitud HTTP
     * @param params Array de parámetros para llenar
     * @param maxParams Número máximo de parámetros
     * @return Número de parámetros válidos extraídos
     */
    int extractParams(WebRequestParam* params, int maxParams);
    
    /**
     * @brief Verifica autenticación para el nivel requerido
     * @param requiredLevel Nivel de autenticación requerido
     * @return true si autenticado, false en caso contrario
     */
    bool checkAuth(WebAuthLevel requiredLevel);
    
    /**
     * @brief Genera página HTML de login segura
     */
    void handleLogin();
    
    /**
     * @brief Procesa formulario de login
     */
    void handleLoginSubmit();
    
    /**
     * @brief Genera página principal de configuración
     */
    void handleMainPage();
    
    /**
     * @brief Procesa actualización de parámetros Modbus
     */
    void handleUpdateParams();
    
    /**
     * @brief Maneja solicitudes de estado del dispositivo
     */
    void handleStatus();
    
    /**
     * @brief Maneja solicitudes no encontradas (404)
     */
    void handleNotFound();

public:
    /**
     * @brief Constructor de la clase ModbusWebConfig
     * Inicializa todos los punteros a nullptr para gestión RAII segura
     */
    ModbusWebConfig();
    
    /**
     * @brief Destructor con liberación garantizada de recursos
     * Sigue patrón RAII para evitar fugas de memoria
     */
    ~ModbusWebConfig();
    
    /**
     * @brief Elimina copia y asignación para prevenir problemas de memoria
     */
    ModbusWebConfig(const ModbusWebConfig&) = delete;
    ModbusWebConfig& operator=(const ModbusWebConfig&) = delete;
    
    /**
     * @brief Inicializa el servidor web con configuración segura
     * @param mbInstance Puntero a instancia Modbus existente
     * @param port Puerto para el servidor web (por defecto 80)
     * @param adminUser Usuario administrador
     * @param adminPass Contraseña administrador (mínimo 8 caracteres)
     * @return true si inicialización exitosa, false en caso de error
     * 
     * @pre mbInstance debe ser un puntero válido
     * @pre adminPass debe tener al menos 8 caracteres
     * @post El servidor web queda listo para aceptar conexiones
     */
    bool begin(Modbus* mbInstance, uint16_t port = 80,
               const char* adminUser = "admin",
               const char* adminPass = "modbus123");
    
    /**
     * @brief Inicia el servidor web
     * @return true si éxito, false si ya estaba corriendo
     */
    bool start();
    
    /**
     * @brief Detiene el servidor web y libera recursos
     */
    void stop();
    
    /**
     * @brief Verifica si el servidor está corriendo
     * @return true si corriendo, false en caso contrario
     */
    bool isRunning() const { return running; }
    
    /**
     * @brief Loop principal para procesar solicitudes HTTP
     * Debe llamarse periódicamente en el loop() de Arduino
     */
    void loop();
    
    /**
     * @brief Obtiene número de clientes conectados actualmente
     * @return Número de clientes activos
     */
    uint8_t getActiveClients() const;
    
    /**
     * @brief Restablece configuración web a valores de fábrica
     * @note Esto detiene el servidor y requiere reinicio para volver a usar
     */
    void reset();
    
    /**
     * @brief Establece timeout de sesión web
     * @param timeoutMs Timeout en milisegundos
     */
    void setSessionTimeout(uint32_t timeoutMs);
    
    /**
     * @brief Cambia credenciales de administrador
     * @param newUser Nuevo nombre de usuario
     * @param newPass Nueva contraseña
     * @return true si éxito, false si validación falla
     */
    bool changeAdminCredentials(const char* newUser, const char* newPass);
};

#endif // MODBUS_WEBCONFIG_H
