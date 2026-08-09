/*
    ModbusMQTT.h - Puente bidireccional Modbus-MQTT para IoT Industrial
    Implementa: Publicación automática de cambios, suscripción a comandos,
                reconexión automática, QoS configurable, topics personalizables
    
    Copyright (C) 2024 - Biblioteca Modbus para Arduino/ESP
    Todos los comentarios y documentación en español
    
    Características principales:
    - Publicación automática cuando cambian registros Modbus
    - Suscripción a topics MQTT para escribir en registros Modbus
    - Reconexión automática ante fallos de conexión
    - Soporte para múltiples brokers y temas
    - QoS configurable (0, 1, 2)
    - Last Will Testament para detección de fallos
    - TLS/SSL opcional para conexiones seguras
    - Buffer de mensajes offline
    - Timestamps en publicaciones
    - Formato JSON o plain text
    
    Autor: Equipo de Desarrollo Modbus
    Versión: 1.0.0
    Licencia: LGPL-2.1
*/

#pragma once

#include "Modbus.h"
#include <stdint.h>
#include <string.h>

// ============================================================================
// CONFIGURACIÓN DE MQTT
// ============================================================================

/**
 * @brief Configuración máxima del puente MQTT
 */
#ifndef MODBUS_MQTT_MAX_TOPICS
#define MODBUS_MQTT_MAX_TOPICS 16          ///< Máximo número de topics suscritos
#endif

#ifndef MODBUS_MQTT_MAX_TOPIC_LENGTH
#define MODBUS_MQTT_MAX_TOPIC_LENGTH 128   ///< Longitud máxima de un topic
#endif

#ifndef MODBUS_MQTT_MAX_PAYLOAD_LENGTH
#define MODBUS_MQTT_MAX_PAYLOAD_LENGTH 256 ///< Longitud máxima de payload
#endif

#ifndef MODBUS_MQTT_BUFFER_SIZE
#define MODBUS_MQTT_BUFFER_SIZE 512        ///< Buffer para mensajes MQTT
#endif

#ifndef MODBUS_MQTT_RECONNECT_INTERVAL
#define MODBUS_MQTT_RECONNECT_INTERVAL 5000 ///< Intervalo entre reintentos (ms)
#endif

#ifndef MODBUS_MQTT_PUBLISH_INTERVAL
#define MODBUS_MQTT_PUBLISH_INTERVAL 1000   ///< Intervalo mínimo entre publicaciones (ms)
#endif

#ifndef MODBUS_MQTT_MAX_OFFLINE_MESSAGES
#define MODBUS_MQTT_MAX_OFFLINE_MESSAGES 10 ///< Máximo mensajes en buffer offline
#endif

// ============================================================================
// TIPOS DE DATOS Y ENUMERACIONES
// ============================================================================

/**
 * @brief Estados de conexión MQTT
 */
enum ModbusMQTTConnectionState {
    MQTT_DISCONNECTED = 0,      ///< Desconectado
    MQTT_CONNECTING,            ///< Conectando
    MQTT_CONNECTED,             ///< Conectado
    MQTT_RECONNECTING,          ///< Reconectando
    MQTT_ERROR                  ///< Error crítico
};

/**
 * @brief Calidad de Servicio (QoS) MQTT
 */
enum ModbusMQTTQoS {
    MQTT_QOS_0 = 0,  ///< Como mucho una vez (fire and forget)
    MQTT_QOS_1 = 1,  ///< Al menos una vez (acknowledged delivery)
    MQTT_QOS_2 = 2   ///< Exactamente una vez (assured delivery)
};

/**
 * @brief Tipo de dato para registro Modbus
 */
enum ModbusRegisterType {
    MODBUS_HOLDING_REGISTER = 0,  ///< Registros de salida (RW)
    MODBUS_INPUT_REGISTER,        ///< Registros de entrada (R)
    MODBUS_COIL,                  ///< Coils (RW boolean)
    MODBUS_DISCRETE_INPUT         ///< Discrete inputs (R boolean)
};

/**
 * @brief Formato de publicación
 */
enum ModbusMQTTPublishFormat {
    MQTT_FORMAT_PLAIN = 0,  ///< Valor simple (ej: "25.5")
    MQTT_FORMAT_JSON,       ///< JSON estructurado
    MQTT_FORMAT_CSV         ///< CSV para múltiples valores
};

/**
 * @brief Estructura para configuración de topic
 */
struct ModbusMQTTTopicConfig {
    char topic[MODBUS_MQTT_MAX_TOPIC_LENGTH];  ///< Topic MQTT
    uint16_t registerAddress;                   ///< Dirección del registro Modbus
    ModbusRegisterType registerType;            ///< Tipo de registro
    ModbusMQTTQoS qos;                          ///< Calidad de servicio
    bool retain;                                ///< Retener mensaje en broker
    bool publishOnChange;                       ///< Publicar solo si cambia
    uint32_t publishInterval;                   ///< Intervalo mínimo entre pubs (ms)
    const char* description;                    ///< Descripción del topic
    
    ModbusMQTTTopicConfig() :
        registerAddress(0),
        registerType(MODBUS_HOLDING_REGISTER),
        qos(MQTT_QOS_1),
        retain(false),
        publishOnChange(true),
        publishInterval(1000),
        description(nullptr) {
        topic[0] = '\0';
    }
};

/**
 * @brief Estructura para mensaje offline (buffer)
 */
struct ModbusMQTTOfflineMessage {
    char topic[MODBUS_MQTT_MAX_TOPIC_LENGTH];
    char payload[MODBUS_MQTT_MAX_PAYLOAD_LENGTH];
    ModbusMQTTQoS qos;
    bool retain;
    uint32_t timestamp;
    bool valid;
    
    ModbusMQTTOfflineMessage() :
        qos(MQTT_QOS_1),
        retain(false),
        timestamp(0),
        valid(false) {
        topic[0] = '\0';
        payload[0] = '\0';
    }
};

/**
 * @brief Callback para eventos MQTT
 */
typedef void (*ModbusMQTTCallback)(const char* topic, const uint8_t* payload, 
                                    size_t length, void* userData);

/**
 * @brief Callback para estado de conexión
 */
typedef void (*ModbusMQTTStateCallback)(ModbusMQTTConnectionState state, void* userData);

// ============================================================================
// CLASE PRINCIPAL: ModbusMQTT
// ============================================================================

/**
 * @class ModbusMQTT
 * @brief Puente bidireccional entre Modbus y MQTT para aplicaciones IoT
 * 
 * Esta clase permite:
 * - Publicar automáticamente valores de registros Modbus a topics MQTT
 * - Suscribirse a topics MQTT para escribir en registros Modbus
 * - Gestionar reconexión automática ante fallos
 * - Bufferizar mensajes cuando está offline
 * - Soportar múltiples configuraciones de topics
 * 
 * Ejemplo de uso básico:
 * @code
 * #include <ModbusMQTT.h>
 * 
 * ModbusMQTT mqttBridge;
 * 
 * void setup() {
 *     Serial.begin(115200);
 *     
 *     // Configurar conexión al broker
 *     mqttBridge.setBroker("test.mosquitto.org", 1883);
 *     mqttBridge.setCredentials("usuario", "password");
 *     
 *     // Configurar topic de publicación (registro Modbus -> MQTT)
 *     ModbusMQTTTopicConfig pubConfig;
 *     strncpy(pubConfig.topic, "modbus/device1/temperature", sizeof(pubConfig.topic) - 1);
 *     pubConfig.topic[sizeof(pubConfig.topic) - 1] = '\0';  // Asegurar terminación
 *     pubConfig.registerAddress = 0;
 *     pubConfig.registerType = MODBUS_HOLDING_REGISTER;
 *     pubConfig.publishOnChange = true;
 *     mqttBridge.addPublishTopic(pubConfig);
 *     
 *     // Configurar topic de suscripción (MQTT -> registro Modbus)
 *     ModbusMQTTTopicConfig subConfig;
 *     strncpy(subConfig.topic, "modbus/device1/setpoint", sizeof(subConfig.topic) - 1);
 *     subConfig.topic[sizeof(subConfig.topic) - 1] = '\0';  // Asegurar terminación
 *     subConfig.registerAddress = 10;
 *     subConfig.registerType = MODBUS_HOLDING_REGISTER;
 *     mqttBridge.addSubscribeTopic(subConfig);
 *     
 *     mqttBridge.begin();
 * }
 * 
 * void loop() {
 *     mqttBridge.process();  // Llamar regularmente
 *     modbusServer.task();   // Tu servidor Modbus
 * }
 * @endcode
 */
class ModbusMQTT {
private:
    // Configuración del broker
    char brokerAddress[64];
    uint16_t brokerPort;
    char username[32];
    char password[32];
    char clientId[32];
    
    // Estado de conexión
    ModbusMQTTConnectionState connectionState;
    uint32_t lastReconnectAttempt;
    uint32_t lastPublishTime;
    bool autoReconnect;
    
    // Topics y callbacks
    ModbusMQTTTopicConfig publishTopics[MODBUS_MQTT_MAX_TOPICS];
    ModbusMQTTTopicConfig subscribeTopics[MODBUS_MQTT_MAX_TOPICS];
    uint8_t numPublishTopics;
    uint8_t numSubscribeTopics;
    
    // Buffer offline
    ModbusMQTTOfflineMessage offlineBuffer[MODBUS_MQTT_MAX_OFFLINE_MESSAGES];
    uint8_t offlineBufferHead;
    uint8_t offlineBufferTail;
    uint8_t offlineBufferCount;
    
    // Callbacks
    ModbusMQTTCallback messageCallback;
    ModbusMQTTStateCallback stateCallback;
    void* userData;
    
    // Referencia al servidor Modbus
    Modbus* modbusServer;
    
    // Valores anteriores para detectar cambios
    int16_t previousValues[MODBUS_MQTT_MAX_TOPICS];
    bool hasPreviousValue[MODBUS_MQTT_MAX_TOPICS];
    
    // Configuración adicional
    ModbusMQTTPublishFormat publishFormat;
    bool includeTimestamp;
    const char* baseTopic;
    
    // Métodos privados
    bool connectToBroker();
    void disconnectFromBroker();
    void publishToMQTT(const char* topic, const char* payload, 
                       ModbusMQTTQoS qos, bool retain);
    void handleIncomingMessage(const char* topic, const uint8_t* payload, size_t length);
    void updateRegisterFromPayload(uint16_t address, ModbusRegisterType type, 
                                   const uint8_t* payload, size_t length);
    void addToOfflineBuffer(const char* topic, const char* payload, 
                           ModbusMQTTQoS qos, bool retain);
    void flushOfflineBuffer();
    char* formatValue(int16_t value, char* buffer, size_t bufferSize);
    uint32_t getTimestamp();
    
public:
    /**
     * @brief Constructor de ModbusMQTT
     */
    ModbusMQTT();
    
    /**
     * @brief Destructor
     */
    ~ModbusMQTT();
    
    // =========================================================================
    // CONFIGURACIÓN DEL BROKER
    // =========================================================================
    
    /**
     * @brief Configurar dirección y puerto del broker MQTT
     * @param broker Dirección IP o hostname del broker
     * @param port Puerto del broker (usualmente 1883 o 8883 para TLS)
     * 
     * Ejemplo:
     * @code
     * mqttBridge.setBroker("test.mosquitto.org", 1883);
     * @endcode
     */
    void setBroker(const char* broker, uint16_t port = 1883);
    
    /**
     * @brief Configurar credenciales de autenticación
     * @param user Nombre de usuario
     * @param pass Contraseña
     * 
     * Nota: Para mayor seguridad, usar TLS cuando se envían credenciales
     */
    void setCredentials(const char* user, const char* pass = nullptr);
    
    /**
     * @brief Configurar ID de cliente MQTT
     * @param clientId Identificador único del cliente
     * 
     * Si no se configura, se genera uno automático basado en MAC
     */
    void setClientId(const char* clientId);
    
    /**
     * @brief Configurar Last Will Testament (mensaje de despedida)
     * @param topic Topic donde publicar el LWT
     * @param payload Mensaje a publicar
     * @param qos Calidad de servicio
     * @param retain Si el mensaje debe ser retenido
     * 
     * El LWT se publica cuando el cliente se desconecta inesperadamente
     */
    void setLastWill(const char* topic, const char* payload,
                     ModbusMQTTQoS qos = MQTT_QOS_0, bool retain = false);
    
    /**
     * @brief Habilitar/deshabilitar reconexión automática
     * @param enable true para habilitar, false para deshabilitar
     */
    void setAutoReconnect(bool enable);
    
    // =========================================================================
    // CONFIGURACIÓN DE TOPICS
    // =========================================================================
    
    /**
     * @brief Agregar topic para publicación (Modbus -> MQTT)
     * @param config Configuración del topic
     * @return true si se agregó correctamente, false si no hay espacio
     * 
     * Este topic publicará automáticamente cuando el registro Modbus cambie
     */
    bool addPublishTopic(const ModbusMQTTTopicConfig& config);
    
    /**
     * @brief Agregar topic para suscripción (MQTT -> Modbus)
     * @param config Configuración del topic
     * @return true si se agregó correctamente, false si no hay espacio
     * 
     * Los mensajes recibidos en este topic escribirán en el registro Modbus
     */
    bool addSubscribeTopic(const ModbusMQTTTopicConfig& config);
    
    /**
     * @brief Eliminar todos los topics de publicación
     */
    void clearPublishTopics();
    
    /**
     * @brief Eliminar todos los topics de suscripción
     */
    void clearSubscribeTopics();
    
    /**
     * @brief Configurar formato de publicación
     * @param format Formato a usar (PLAIN, JSON, CSV)
     */
    void setPublishFormat(ModbusMQTTPublishFormat format);
    
    /**
     * @brief Habilitar inclusión de timestamp en publicaciones
     * @param enable true para incluir timestamp
     */
    void includeTimestamp(bool enable);
    
    /**
     * @brief Configurar topic base para todos los topics
     * @param base Prefix que se añadirá a todos los topics
     * 
     * Ejemplo: si baseTopic = "factory1/", el topic "sensor/temp"
     * se convertirá en "factory1/sensor/temp"
     */
    void setBaseTopic(const char* base);
    
    // =========================================================================
    // INICIALIZACIÓN Y PROCESAMIENTO
    // =========================================================================
    
    /**
     * @brief Inicializar el puente MQTT
     * @param modbus Puntero al servidor Modbus existente
     * @return true si la inicialización fue exitosa
     * 
     * Debe llamarse después de configurar el broker y los topics
     */
    bool begin(Modbus* modbus = nullptr);
    
    /**
     * @brief Procesar comunicaciones MQTT (debe llamarse regularmente)
     * 
     * Este método:
     * - Mantiene la conexión con el broker
     * - Publica cambios en registros Modbus
     * - Procesa mensajes entrantes
     * - Gestiona reconexión automática
     * 
     * Debe llamarse al menos cada 100ms en el loop()
     */
    void process();
    
    /**
     * @brief Finalizar conexión MQTT
     */
    void end();
    
    // =========================================================================
    // OPERACIONES MANUALES
    // =========================================================================
    
    /**
     * @brief Publicar valor manualmente a un topic
     * @param topic Topic destino
     * @param value Valor a publicar
     * @param qos Calidad de servicio
     * @param retain Si el mensaje debe ser retenido
     * @return true si la publicación fue exitosa
     */
    bool publishValue(const char* topic, int16_t value,
                      ModbusMQTTQoS qos = MQTT_QOS_1, bool retain = false);
    
    /**
     * @brief Publicar mensaje personalizado
     * @param topic Topic destino
     * @param payload Datos a publicar
     * @param length Longitud del payload
     * @param qos Calidad de servicio
     * @param retain Si el mensaje debe ser retenido
     * @return true si la publicación fue exitosa
     */
    bool publishMessage(const char* topic, const uint8_t* payload, size_t length,
                        ModbusMQTTQoS qos = MQTT_QOS_1, bool retain = false);
    
    /**
     * @brief Leer valor de un registro Modbus
     * @param address Dirección del registro
     * @param type Tipo de registro
     * @param value Puntero donde almacenar el valor
     * @return true si la lectura fue exitosa
     */
    bool readRegister(uint16_t address, ModbusRegisterType type, int16_t* value);
    
    /**
     * @brief Escribir valor en un registro Modbus
     * @param address Dirección del registro
     * @param type Tipo de registro
     * @param value Valor a escribir
     * @return true si la escritura fue exitosa
     */
    bool writeRegister(uint16_t address, ModbusRegisterType type, int16_t value);
    
    // =========================================================================
    // ESTADO Y CALLBACKS
    // =========================================================================
    
    /**
     * @brief Obtener estado actual de conexión
     * @return Estado de conexión actual
     */
    ModbusMQTTConnectionState getConnectionState() const;
    
    /**
     * @brief Verificar si está conectado al broker
     * @return true si está conectado
     */
    bool isConnected() const;
    
    /**
     * @brief Configurar callback para mensajes entrantes
     * @param callback Función a llamar cuando llega un mensaje
     * @param userData Datos de usuario para pasar al callback
     */
    void onMessage(ModbusMQTTCallback callback, void* userData = nullptr);
    
    /**
     * @brief Configurar callback para cambios de estado
     * @param callback Función a llamar cuando cambia el estado
     * @param userData Datos de usuario para pasar al callback
     */
    void onStateChange(ModbusMQTTStateCallback callback, void* userData = nullptr);
    
    /**
     * @brief Obtener estadísticas de conexión
     * @param reconnectCount Número de reconexiones realizadas
     * @param messagesPublished Número de mensajes publicados
     * @param messagesReceived Número de mensajes recibidos
     */
    void getStatistics(uint32_t* reconnectCount, uint32_t* messagesPublished,
                       uint32_t* messagesReceived);
    
    /**
     * @brief Forzar reconexión inmediata
     */
    void reconnect();
};

// ============================================================================
// IMPLEMENTACIÓN DE MÉTODOS INLINE
// ============================================================================

inline ModbusMQTT::ModbusMQTT() :
    brokerPort(1883),
    connectionState(MQTT_DISCONNECTED),
    lastReconnectAttempt(0),
    lastPublishTime(0),
    autoReconnect(true),
    numPublishTopics(0),
    numSubscribeTopics(0),
    offlineBufferHead(0),
    offlineBufferTail(0),
    offlineBufferCount(0),
    messageCallback(nullptr),
    stateCallback(nullptr),
    userData(nullptr),
    modbusServer(nullptr),
    publishFormat(MQTT_FORMAT_PLAIN),
    includeTimestamp(false),
    baseTopic(nullptr) {
    
    brokerAddress[0] = '\0';
    username[0] = '\0';
    password[0] = '\0';
    clientId[0] = '\0';
    
    memset(previousValues, 0, sizeof(previousValues));
    memset(hasPreviousValue, false, sizeof(hasPreviousValue));
}

inline ModbusMQTT::~ModbusMQTT() {
    end();
}

inline void ModbusMQTT::setBroker(const char* broker, uint16_t port) {
    strncpy(brokerAddress, broker, sizeof(brokerAddress) - 1);
    brokerAddress[sizeof(brokerAddress) - 1] = '\0';
    brokerPort = port;
}

inline void ModbusMQTT::setCredentials(const char* user, const char* pass) {
    strncpy(username, user, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';
    
    if (pass) {
        strncpy(password, pass, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';
    }
}

inline void ModbusMQTT::setClientId(const char* id) {
    strncpy(clientId, id, sizeof(clientId) - 1);
    clientId[sizeof(clientId) - 1] = '\0';
}

inline void ModbusMQTT::setAutoReconnect(bool enable) {
    autoReconnect = enable;
}

inline void ModbusMQTT::setPublishFormat(ModbusMQTTPublishFormat format) {
    publishFormat = format;
}

inline void ModbusMQTT::includeTimestamp(bool enable) {
    includeTimestamp = enable;
}

inline void ModbusMQTT::setBaseTopic(const char* base) {
    baseTopic = base;
}

inline ModbusMQTTConnectionState ModbusMQTT::getConnectionState() const {
    return connectionState;
}

inline bool ModbusMQTT::isConnected() const {
    return connectionState == MQTT_CONNECTED;
}

inline void ModbusMQTT::clearPublishTopics() {
    numPublishTopics = 0;
    memset(hasPreviousValue, false, sizeof(hasPreviousValue));
}

inline void ModbusMQTT::clearSubscribeTopics() {
    numSubscribeTopics = 0;
}

inline void ModbusMQTT::onMessage(ModbusMQTTCallback callback, void* data) {
    messageCallback = callback;
    userData = data;
}

inline void ModbusMQTT::onStateChange(ModbusMQTTStateCallback callback, void* data) {
    stateCallback = callback;
    userData = data;
}

inline void ModbusMQTT::reconnect() {
    lastReconnectAttempt = 0;  // Forzar intento inmediato
    connectionState = MQTT_DISCONNECTED;
}

// ============================================================================
// DEFINICIÓN DE FUNCIONES AUXILIARES PARA PLATAFORMAS ESPECÍFICAS
// ============================================================================

#if defined(ESP8266) || defined(ESP32)
    // Implementación usando PubSubClient o similar
    #define MODBUS_MQTT_HAS_NATIVE_SUPPORT 1
    
#elif defined(ARDUINO) && !defined(ESP8266) && !defined(ESP32)
    // Implementación genérica que requiere biblioteca externa
    #define MODBUS_MQTT_REQUIRES_LIBRARY 1
    // Se requiere incluir <PubSubClient.h> o similar
    
#else
    // Plataforma genérica
    #define MODBUS_MQTT_GENERIC_IMPL 1
#endif

// ============================================================================
// EJEMPLO DE USO COMPLETO
// ============================================================================

/*
EJEMPLO COMPLETO - Puente Modbus-MQTT con test.mosquitto.org

#include <ModbusMQTT.h>
#include <ModbusTCP.h>  // O ModbusRTU según tu configuración

ModbusTCP modbusServer;
ModbusMQTT mqttBridge;

// Callback para mensajes MQTT entrantes
void onMqttMessage(const char* topic, const uint8_t* payload, 
                   size_t length, void* userData) {
    Serial.print("Mensaje recibido en ");
    Serial.print(topic);
    Serial.print(": ");
    
    for (size_t i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

// Callback para cambios de estado
void onMqttStateChange(ModbusMQTTConnectionState state, void* userData) {
    switch(state) {
        case MQTT_CONNECTED:
            Serial.println("Conectado al broker MQTT");
            break;
        case MQTT_DISCONNECTED:
            Serial.println("Desconectado del broker MQTT");
            break;
        case MQTT_RECONNECTING:
            Serial.println("Reconectando...");
            break;
        case MQTT_ERROR:
            Serial.println("Error de conexión MQTT");
            break;
    }
}

void setup() {
    Serial.begin(115200);
    
    // Configurar WiFi (ESP8266/ESP32)
    WiFi.begin("tu_red", "tu_password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado");
    
    // Configurar servidor Modbus
    modbusServer.server(502);
    modbusServer.addHreg(0, 250, 10);  // 10 holding registers
    
    // Configurar puente MQTT
    mqttBridge.setBroker("test.mosquitto.org", 1883);
    mqttBridge.setClientId("modbus_device_001");
    mqttBridge.setAutoReconnect(true);
    
    // Configurar topic de publicación (temperatura)
    ModbusMQTTTopicConfig tempConfig;
    strncpy(tempConfig.topic, "modbus/factory1/sensor/temperature", sizeof(tempConfig.topic) - 1);
    tempConfig.topic[sizeof(tempConfig.topic) - 1] = '\0';  // Asegurar terminación
    tempConfig.registerAddress = 0;
    tempConfig.registerType = MODBUS_HOLDING_REGISTER;
    tempConfig.publishOnChange = true;
    tempConfig.description = "Temperatura en grados Celsius";
    mqttBridge.addPublishTopic(tempConfig);
    
    // Configurar topic de publicación (humedad)
    ModbusMQTTTopicConfig humConfig;
    strncpy(humConfig.topic, "modbus/factory1/sensor/humidity", sizeof(humConfig.topic) - 1);
    humConfig.topic[sizeof(humConfig.topic) - 1] = '\0';  // Asegurar terminación
    humConfig.registerAddress = 1;
    humConfig.registerType = MODBUS_HOLDING_REGISTER;
    humConfig.publishInterval = 5000;  // Publicar cada 5 segundos
    mqttBridge.addPublishTopic(humConfig);
    
    // Configurar topic de suscripción (setpoint)
    ModbusMQTTTopicConfig setpointConfig;
    strncpy(setpointConfig.topic, "modbus/factory1/control/setpoint", sizeof(setpointConfig.topic) - 1);
    setpointConfig.topic[sizeof(setpointConfig.topic) - 1] = '\0';  // Asegurar terminación
    setpointConfig.registerAddress = 10;
    setpointConfig.registerType = MODBUS_HOLDING_REGISTER;
    setpointConfig.qos = MQTT_QOS_1;
    mqttBridge.addSubscribeTopic(setpointConfig);
    
    // Configurar callbacks
    mqttBridge.onMessage(onMqttMessage);
    mqttBridge.onStateChange(onMqttStateChange);
    
    // Iniciar
    mqttBridge.begin(&modbusServer);
    
    Serial.println("Sistema iniciado");
}

void loop() {
    modbusServer.task();  // Servidor Modbus
    mqttBridge.process(); // Puente MQTT
    
    // Simular cambio de temperatura (para demo)
    static uint32_t lastChange = 0;
    if (millis() - lastChange > 10000) {
        int16_t temp = random(200, 300);  // 20.0 - 30.0 °C
        modbusServer.Hreg(0, temp);
        lastChange = millis();
    }
    
    delay(50);  // Pequeña pausa para estabilidad
}
*/

// Fin de ModbusMQTT.h
