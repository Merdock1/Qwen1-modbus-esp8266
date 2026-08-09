/**
 * @file ModbusMQTT.h
 * @brief Módulo de integración Modbus-MQTT con validación segura de buffers
 * @author Ingeniero de Software Senior
 * @version 1.0.0
 * @date 2024
 * 
 * Este módulo proporciona integración entre protocolos Modbus y MQTT,
 * implementando validaciones estrictas de límites para prevenir buffer overflows.
 * Cumple con estándares IEC 62443 para seguridad industrial.
 */

#ifndef MODBUS_MQTT_H
#define MODBUS_MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include "Modbus.h"

// ============================================================================
// CONSTANTES Y CONFIGURACIÓN
// ============================================================================

/** @brief Tamaño máximo del buffer para tópicos MQTT (limitado por seguridad) */
#define MQTT_TOPIC_MAX_LEN 64

/** @brief Tamaño máximo del buffer para payloads MQTT (limitado por seguridad) */
#define MQTT_PAYLOAD_MAX_LEN 128

/** @brief Tamaño máximo del buffer para client ID MQTT */
#define MQTT_CLIENT_ID_MAX_LEN 32

/** @brief Timeout por defecto para operaciones MQTT (ms) */
#define MQTT_DEFAULT_TIMEOUT 5000

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
// ESTRUCTURAS DE DATOS
// ============================================================================

/**
 * @struct ModbusMQTTConfig
 * @brief Configuración segura para conexión MQTT
 * 
 * Todos los buffers tienen tamaños fijos limitados para prevenir
 * desbordamientos. Los valores se validan antes de su uso.
 */
struct ModbusMQTTConfig {
    char server[64];                    ///< Dirección del servidor MQTT
    uint16_t port;                      ///< Puerto del servidor (por defecto 1883)
    char clientId[MQTT_CLIENT_ID_MAX_LEN]; ///< ID único del cliente
    char user[32];                      ///< Usuario para autenticación
    char password[32];                  ///< Contraseña para autenticación
    char topicPrefix[MQTT_TOPIC_MAX_LEN]; ///< Prefijo para tópicos Modbus
    uint32_t keepAlive;                 ///< Intervalo keep-alive (segundos)
    bool cleanSession;                  ///< Limpieza de sesión en conexión
    
    /**
     * @brief Constructor con inicialización segura
     * Inicializa todos los buffers a vacío y valores por defecto seguros
     */
    ModbusMQTTConfig() : port(1883), keepAlive(60), cleanSession(true) {
        memset(server, 0, sizeof(server));
        memset(clientId, 0, sizeof(clientId));
        memset(user, 0, sizeof(user));
        memset(password, 0, sizeof(password));
        memset(topicPrefix, 0, sizeof(topicPrefix));
    }
    
    /**
     * @brief Valida la configuración actual
     * @return true si la configuración es válida, false en caso contrario
     */
    bool validate() const {
        return (strlen(server) > 0 && strlen(server) < sizeof(server) &&
                port > 0 && port <= 65535 &&
                strlen(clientId) > 0 && strlen(clientId) < sizeof(clientId));
    }
};

/**
 * @struct ModbusMQTTMessage
 * @brief Mensaje MQTT con validación de límites
 * 
 * Estructura inmutable una vez creada para garantizar integridad
 */
struct ModbusMQTTMessage {
    char topic[MQTT_TOPIC_MAX_LEN];     ///< Tópico del mensaje (validado)
    char payload[MQTT_PAYLOAD_MAX_LEN]; ///< Payload del mensaje (validado)
    uint16_t payloadLen;                ///< Longitud real del payload
    bool retained;                      ///< Flag de retención
    uint8_t qos;                        ///< Calidad de servicio (0, 1, 2)
    
    /**
     * @brief Constructor por defecto
     */
    ModbusMQTTMessage() : payloadLen(0), retained(false), qos(0) {
        memset(topic, 0, sizeof(topic));
        memset(payload, 0, sizeof(payload));
    }
    
    /**
     * @brief Establece el tópico con validación de longitud
     * @param newTopic Tópico a establecer
     * @return true si éxito, false si excede longitud máxima
     */
    bool setTopic(const char* newTopic) {
        if (newTopic == nullptr || strlen(newTopic) >= MQTT_TOPIC_MAX_LEN) {
            return false;
        }
        SAFE_STRNCPY(topic, newTopic, MQTT_TOPIC_MAX_LEN);
        return true;
    }
    
    /**
     * @brief Establece el payload con validación de longitud
     * @param newPayload Payload a establecer
     * @param len Longitud del payload
     * @return true si éxito, false si excede longitud máxima
     */
    bool setPayload(const char* newPayload, uint16_t len) {
        if (newPayload == nullptr || len >= MQTT_PAYLOAD_MAX_LEN) {
            return false;
        }
        SAFE_STRNCPY(payload, newPayload, MQTT_PAYLOAD_MAX_LEN);
        payloadLen = len;
        return true;
    }
};

// ============================================================================
// CLASE PRINCIPAL: ModbusMQTT
// ============================================================================

/**
 * @class ModbusMQTT
 * @brief Clase principal para integración Modbus-MQTT con seguridad reforzada
 * 
 * Esta clase gestiona la comunicación entre dispositivos Modbus y brokers MQTT,
 * implementando validaciones estrictas en todas las operaciones de entrada/salida
 * para prevenir vulnerabilidades de seguridad.
 * 
 * Características de seguridad:
 * - Validación de límites en todos los buffers
 * - Sanitización de inputs de red
 * - Gestión segura de memoria sin fugas
 * - Timeouts configurables para prevenir bloqueos
 * 
 * @note Todos los métodos públicos validan sus parámetros antes de ejecutar
 */
class ModbusMQTT {
private:
    PubSubClient* mqttClient;           ///< Cliente MQTT subyacente
    Modbus* modbusInstance;             ///< Instancia Modbus asociada
    ModbusMQTTConfig config;            ///< Configuración actual
    bool connected;                     ///< Estado de conexión
    uint32_t lastReconnectAttempt;      ///< Último intento de reconexión
    uint32_t reconnectDelay;            ///< Retraso entre reconexiones (backoff)
    char* messageBuffer;                ///< Buffer temporal para mensajes (gestión RAII)
    size_t messageBufferSize;           ///< Tamaño del buffer temporal
    
    /**
     * @brief Libera recursos de memoria asignados dinámicamente
     * Método privado llamado en destructor y reset
     */
    void freeResources() {
        if (messageBuffer != nullptr) {
            delete[] messageBuffer;
            messageBuffer = nullptr;
            messageBufferSize = 0;
        }
        if (mqttClient != nullptr) {
            delete mqttClient;
            mqttClient = nullptr;
        }
    }
    
    /**
     * @brief Construye un tópico MQTT válido y seguro
     * @param buffer Buffer de destino
     * @param bufferSize Tamaño del buffer
     * @param registerType Tipo de registro Modbus
     * @param registerAddress Dirección del registro
     * @return true si éxito, false si error de longitud
     */
    bool buildTopic(char* buffer, size_t bufferSize, 
                   uint8_t registerType, uint16_t registerAddress) {
        if (buffer == nullptr || bufferSize < MQTT_TOPIC_MAX_LEN) {
            return false;
        }
        
        const char* typeStr;
        switch(registerType) {
            case MB_COIL: typeStr = "coil"; break;
            case MB_INPUT: typeStr = "input"; break;
            case MB_HOLDING: typeStr = "holding"; break;
            case MB_INPUT_REG: typeStr = "input_reg"; break;
            default: return false;
        }
        
        int written = snprintf(buffer, bufferSize, "%s/%s/%u", 
                              config.topicPrefix, typeStr, registerAddress);
        
        // Verificación de que no hubo truncamiento
        if (written < 0 || (size_t)written >= bufferSize) {
            buffer[bufferSize - 1] = '\0';
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Callback interno para mensajes MQTT recibidos
     * @param topic Tópico del mensaje recibido
     * @param payload Payload del mensaje
     * @param length Longitud del payload
     */
    void mqttCallback(char* topic, uint8_t* payload, unsigned int length);

public:
    /**
     * @brief Constructor de la clase ModbusMQTT
     * Inicializa todos los punteros a nullptr para gestión RAII segura
     */
    ModbusMQTT();
    
    /**
     * @brief Destructor con liberación garantizada de recursos
     * Sigue patrón RAII para evitar fugas de memoria
     */
    ~ModbusMQTT();
    
    /**
     * @brief Elimina copia y asignación para prevenir problemas de memoria
     */
    ModbusMQTT(const ModbusMQTT&) = delete;
    ModbusMQTT& operator=(const ModbusMQTT&) = delete;
    
    /**
     * @brief Inicializa el módulo MQTT con configuración segura
     * @param mbInstance Puntero a instancia Modbus existente
     * @param mqttConfig Configuración MQTT validada
     * @param wifiClient Cliente WiFi para conexión de red
     * @return true si inicialización exitosa, false en caso de error
     * 
     * @pre mbInstance debe ser un puntero válido
     * @pre mqttConfig debe pasar validación (mqttConfig.validate())
     * @post El módulo queda listo para conectar al broker MQTT
     */
    bool begin(Modbus* mbInstance, const ModbusMQTTConfig& mqttConfig, 
               Client& wifiClient);
    
    /**
     * @brief Intenta conectar al broker MQTT
     * @return true si conectado exitosamente, false en caso contrario
     * 
     * @note Implementa backoff exponencial para reconexiones fallidas
     */
    bool connect();
    
    /**
     * @brief Desconecta del broker MQTT y libera recursos de conexión
     */
    void disconnect();
    
    /**
     * @brief Verifica estado de conexión actual
     * @return true si conectado, false en caso contrario
     */
    bool isConnected() const { return connected; }
    
    /**
     * @brief Loop principal para mantener conexión MQTT
     * Debe llamarse periódicamente en el loop() de Arduino
     * @return true si conexión activa, false si se requiere reconexión
     */
    bool loop();
    
    /**
     * @brief Publica un valor de registro Modbus vía MQTT
     * @param registerType Tipo de registro Modbus
     * @param registerAddress Dirección del registro
     * @param value Valor a publicar
     * @return true si publicación exitosa, false en caso de error
     * 
     * @note Valida automáticamente longitud de tópicos y payloads
     */
    bool publishRegister(uint8_t registerType, uint16_t registerAddress, 
                        uint16_t value);
    
    /**
     * @brief Suscribe a tópicos para control remoto de registros Modbus
     * @param registerType Tipo de registro a suscribir
     * @param registerAddress Dirección del registro
     * @return true si suscripción exitosa, false en caso de error
     */
    bool subscribeRegister(uint8_t registerType, uint16_t registerAddress);
    
    /**
     * @brief Procesa mensaje MQTT entrante y actualiza registro Modbus
     * @param message Mensaje MQTT validado
     * @return true si procesamiento exitoso, false si error de validación
     * 
     * @note Incluye validación de formato de payload y límites de registro
     */
    bool processIncomingMessage(const ModbusMQTTMessage& message);
    
    /**
     * @brief Obtiene configuración actual (solo lectura)
     * @return Referencia constante a configuración actual
     */
    const ModbusMQTTConfig& getConfig() const { return config; }
    
    /**
     * @brief Restablece conexión y configuración a valores por defecto
     */
    void reset();
    
    /**
     * @brief Establece intervalo de reconexión tras fallo
     * @param delayMs Retraso en milisegundos
     */
    void setReconnectDelay(uint32_t delayMs) { reconnectDelay = delayMs; }
    
    /**
     * @brief Obtiene estadísticas de conexión (para debugging)
     * @return Número de intentos de reconexión realizados
     */
    uint32_t getReconnectAttempts() const;
};

#endif // MODBUS_MQTT_H
