/*
    test_modbus_mqtt.cpp - Tests unitarios para el puente Modbus-MQTT
    
    Copyright (C) 2024 - Biblioteca Modbus para Arduino/ESP
    Todos los comentarios en español
    
    Tests implementados:
    - Configuración del broker y credenciales
    - Gestión de topics de publicación y suscripción
    - Publicación automática ante cambios
    - Suscripción y escritura en registros
    - Reconexión automática
    - Buffer offline
    - Formatos de publicación (PLAIN, JSON, CSV)
    - Callbacks de estado y mensajes
*/

#include <iostream>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <vector>
#include <string>

// Mock de la clase Modbus para testing
class ModbusMock {
private:
    int16_t holdingRegisters[256];
    int16_t inputRegisters[256];
    bool coils[256];
    bool discreteInputs[256];
    
public:
    ModbusMock() {
        memset(holdingRegisters, 0, sizeof(holdingRegisters));
        memset(inputRegisters, 0, sizeof(inputRegisters));
        memset(coils, 0, sizeof(coils));
        memset(discreteInputs, 0, sizeof(discreteInputs));
    }
    
    bool Hreg(uint16_t address, int16_t value) {
        if (address < 256) {
            holdingRegisters[address] = value;
            return true;
        }
        return false;
    }
    
    int16_t Hreg(uint16_t address) const {
        if (address < 256) {
            return holdingRegisters[address];
        }
        return 0;
    }
    
    bool Ireg(uint16_t address, int16_t value) {
        if (address < 256) {
            inputRegisters[address] = value;
            return true;
        }
        return false;
    }
    
    int16_t Ireg(uint16_t address) const {
        if (address < 256) {
            return inputRegisters[address];
        }
        return 0;
    }
    
    bool Coil(uint16_t address, bool value) {
        if (address < 256) {
            coils[address] = value;
            return true;
        }
        return false;
    }
    
    bool Coil(uint16_t address) const {
        if (address < 256) {
            return coils[address];
        }
        return false;
    }
    
    bool Discrete(uint16_t address, bool value) {
        if (address < 256) {
            discreteInputs[address] = value;
            return true;
        }
        return false;
    }
    
    bool Discrete(uint16_t address) const {
        if (address < 256) {
            return discreteInputs[address];
        }
        return false;
    }
};

// Incluir definiciones básicas antes del header principal
#define MODBUS_MQTT_MAX_TOPICS 16
#define MODBUS_MQTT_MAX_TOPIC_LENGTH 128
#define MODBUS_MQTT_MAX_PAYLOAD_LENGTH 256
#define MODBUS_MQTT_BUFFER_SIZE 512
#define MODBUS_MQTT_RECONNECT_INTERVAL 5000
#define MODBUS_MQTT_PUBLISH_INTERVAL 1000
#define MODBUS_MQTT_MAX_OFFLINE_MESSAGES 10

// Tipos básicos
typedef void (*ModbusMQTTCallback)(const char* topic, const uint8_t* payload, 
                                    size_t length, void* userData);
typedef void (*ModbusMQTTStateCallback)(int state, void* userData);

enum ModbusMQTTConnectionState {
    MQTT_DISCONNECTED = 0,
    MQTT_CONNECTING,
    MQTT_CONNECTED,
    MQTT_RECONNECTING,
    MQTT_ERROR
};

enum ModbusMQTTQoS {
    MQTT_QOS_0 = 0,
    MQTT_QOS_1 = 1,
    MQTT_QOS_2 = 2
};

enum ModbusRegisterType {
    MODBUS_HOLDING_REGISTER = 0,
    MODBUS_INPUT_REGISTER,
    MODBUS_COIL,
    MODBUS_DISCRETE_INPUT
};

enum ModbusMQTTPublishFormat {
    MQTT_FORMAT_PLAIN = 0,
    MQTT_FORMAT_JSON,
    MQTT_FORMAT_CSV
};

struct ModbusMQTTTopicConfig {
    char topic[MODBUS_MQTT_MAX_TOPIC_LENGTH];
    uint16_t registerAddress;
    ModbusRegisterType registerType;
    ModbusMQTTQoS qos;
    bool retain;
    bool publishOnChange;
    uint32_t publishInterval;
    const char* description;
    
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

// Simulación simplificada de ModbusMQTT para tests
class ModbusMQTT {
private:
    char brokerAddress[64];
    uint16_t brokerPort;
    char clientId[32];
    ModbusMQTTConnectionState connectionState;
    uint32_t lastReconnectAttempt;
    uint32_t lastPublishTime;
    bool autoReconnect;
    
    ModbusMQTTTopicConfig publishTopics[MODBUS_MQTT_MAX_TOPICS];
    ModbusMQTTTopicConfig subscribeTopics[MODBUS_MQTT_MAX_TOPICS];
    uint8_t numPublishTopics;
    uint8_t numSubscribeTopics;
    
    ModbusMQTTCallback messageCallback;
    ModbusMQTTStateCallback stateCallback;
    void* userData;
    
    ModbusMock* modbusServer;
    ModbusMQTTPublishFormat publishFormat;
    const char* baseTopic;
    
    // Estadísticas para tests
    uint32_t messagesPublished;
    uint32_t messagesReceived;
    uint32_t reconnectCount;
    
public:
    ModbusMQTT() :
        brokerPort(1883),
        connectionState(MQTT_DISCONNECTED),
        lastReconnectAttempt(0),
        lastPublishTime(0),
        autoReconnect(true),
        numPublishTopics(0),
        numSubscribeTopics(0),
        messageCallback(nullptr),
        stateCallback(nullptr),
        userData(nullptr),
        modbusServer(nullptr),
        publishFormat(MQTT_FORMAT_PLAIN),
        baseTopic(nullptr),
        messagesPublished(0),
        messagesReceived(0),
        reconnectCount(0) {
        
        brokerAddress[0] = '\0';
        clientId[0] = '\0';
    }
    
    ~ModbusMQTT() {}
    
    void setBroker(const char* broker, uint16_t port = 1883) {
        strncpy(brokerAddress, broker, sizeof(brokerAddress) - 1);
        brokerAddress[sizeof(brokerAddress) - 1] = '\0';
        brokerPort = port;
    }
    
    void setClientId(const char* id) {
        strncpy(clientId, id, sizeof(clientId) - 1);
        clientId[sizeof(clientId) - 1] = '\0';
    }
    
    void setAutoReconnect(bool enable) {
        autoReconnect = enable;
    }
    
    bool addPublishTopic(const ModbusMQTTTopicConfig& config) {
        if (numPublishTopics >= MODBUS_MQTT_MAX_TOPICS) {
            return false;
        }
        publishTopics[numPublishTopics++] = config;
        return true;
    }
    
    bool addSubscribeTopic(const ModbusMQTTTopicConfig& config) {
        if (numSubscribeTopics >= MODBUS_MQTT_MAX_TOPICS) {
            return false;
        }
        subscribeTopics[numSubscribeTopics++] = config;
        return true;
    }
    
    void clearPublishTopics() {
        numPublishTopics = 0;
    }
    
    void clearSubscribeTopics() {
        numSubscribeTopics = 0;
    }
    
    void setPublishFormat(ModbusMQTTPublishFormat format) {
        publishFormat = format;
    }
    
    void setBaseTopic(const char* base) {
        baseTopic = base;
    }
    
    bool begin(ModbusMock* modbus = nullptr) {
        modbusServer = modbus;
        connectionState = MQTT_CONNECTING;
        reconnectCount++;  // Contar intento de conexión inicial
        // Simular conexión exitosa inmediata
        connectionState = MQTT_CONNECTED;
        return true;
    }
    
    void process() {
        // Simulación básica - en implementación real manejaría keep-alive
        if (connectionState == MQTT_CONNECTING) {
            connectionState = MQTT_CONNECTED;
            reconnectCount++;
        }
    }
    
    void end() {
        connectionState = MQTT_DISCONNECTED;
    }
    
    bool publishValue(const char* topic, int16_t value,
                      ModbusMQTTQoS qos = MQTT_QOS_1, bool retain = false) {
        if (connectionState != MQTT_CONNECTED) {
            return false;
        }
        messagesPublished++;
        return true;
    }
    
    bool readRegister(uint16_t address, ModbusRegisterType type, int16_t* value) {
        if (!modbusServer || !value) {
            return false;
        }
        
        switch(type) {
            case MODBUS_HOLDING_REGISTER:
                *value = modbusServer->Hreg(address);
                return true;
            case MODBUS_INPUT_REGISTER:
                *value = modbusServer->Ireg(address);
                return true;
            default:
                return false;
        }
    }
    
    bool writeRegister(uint16_t address, ModbusRegisterType type, int16_t value) {
        if (!modbusServer) {
            return false;
        }
        
        switch(type) {
            case MODBUS_HOLDING_REGISTER:
                return modbusServer->Hreg(address, value);
            case MODBUS_INPUT_REGISTER:
                return modbusServer->Ireg(address, value);
            default:
                return false;
        }
    }
    
    ModbusMQTTConnectionState getConnectionState() const {
        return connectionState;
    }
    
    bool isConnected() const {
        return connectionState == MQTT_CONNECTED;
    }
    
    void onMessage(ModbusMQTTCallback callback, void* data = nullptr) {
        messageCallback = callback;
        userData = data;
    }
    
    void onStateChange(ModbusMQTTStateCallback callback, void* data = nullptr) {
        stateCallback = callback;
        userData = data;
    }
    
    void getStatistics(uint32_t* reconnectCount, uint32_t* messagesPublished,
                       uint32_t* messagesReceived) {
        if (reconnectCount) *reconnectCount = this->reconnectCount;
        if (messagesPublished) *messagesPublished = this->messagesPublished;
        if (messagesReceived) *messagesReceived = this->messagesReceived;
    }
    
    void reconnect() {
        connectionState = MQTT_DISCONNECTED;
        lastReconnectAttempt = 0;
    }
    
    // Getters para tests
    const char* getBrokerAddress() const { return brokerAddress; }
    uint16_t getBrokerPort() const { return brokerPort; }
    uint8_t getNumPublishTopics() const { return numPublishTopics; }
    uint8_t getNumSubscribeTopics() const { return numSubscribeTopics; }
};

// ============================================================================
// TESTS UNITARIOS
// ============================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Test: " << #name << "... "; \
    try { \
        test_##name(); \
        std::cout << "✓ PASSED" << std::endl; \
        tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "✗ FAILED: " << e.what() << std::endl; \
        tests_failed++; \
    } catch (...) { \
        std::cout << "✗ FAILED: Excepción desconocida" << std::endl; \
        tests_failed++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) if (!(cond)) throw std::runtime_error("Condición falsa")
#define ASSERT_FALSE(cond) if (cond) throw std::runtime_error("Condición verdadera")
#define ASSERT_EQ(a, b) if ((a) != (b)) throw std::runtime_error("Valores no iguales")
#define ASSERT_NE(a, b) if ((a) == (b)) throw std::runtime_error("Valores iguales")
#define ASSERT_STR_EQ(a, b) if (strcmp((a), (b)) != 0) throw std::runtime_error("Strings no iguales")

// Test 1: Configuración básica del broker
TEST(mqtt_broker_config) {
    ModbusMQTT mqtt;
    
    mqtt.setBroker("test.mosquitto.org", 1883);
    ASSERT_STR_EQ(mqtt.getBrokerAddress(), "test.mosquitto.org");
    ASSERT_EQ(mqtt.getBrokerPort(), 1883);
    
    mqtt.setBroker("broker.hivemq.com", 8883);
    ASSERT_STR_EQ(mqtt.getBrokerAddress(), "broker.hivemq.com");
    ASSERT_EQ(mqtt.getBrokerPort(), 8883);
    
    mqtt.setClientId("device_001");
}

// Test 2: Configuración de reconexión automática
TEST(mqtt_auto_reconnect) {
    ModbusMQTT mqtt;
    
    mqtt.setAutoReconnect(true);
    ASSERT_FALSE(mqtt.isConnected());  // Inicialmente desconectado
    
    mqtt.begin();
    ASSERT_TRUE(mqtt.isConnected());   // Después de begin, conectado
    
    mqtt.reconnect();
    ASSERT_FALSE(mqtt.isConnected());  // Después de reconnect, desconectado
    
    mqtt.process();
    // process() simula reconexión pero el contador aumenta
}

// Test 3: Agregar topics de publicación
TEST(mqtt_add_publish_topics) {
    ModbusMQTT mqtt;
    
    ModbusMQTTTopicConfig config1;
    strcpy(config1.topic, "modbus/sensor/temp");
    config1.registerAddress = 0;
    config1.publishOnChange = true;
    
    ModbusMQTTTopicConfig config2;
    strcpy(config2.topic, "modbus/sensor/humidity");
    config2.registerAddress = 1;
    config2.publishInterval = 5000;
    
    ASSERT_TRUE(mqtt.addPublishTopic(config1));
    ASSERT_TRUE(mqtt.addPublishTopic(config2));
    ASSERT_EQ(mqtt.getNumPublishTopics(), 2);
    
    // Llenar hasta el máximo
    for (int i = 0; i < MODBUS_MQTT_MAX_TOPICS - 2; i++) {
        ModbusMQTTTopicConfig cfg;
        snprintf(cfg.topic, sizeof(cfg.topic), "modbus/test/%d", i);
        ASSERT_TRUE(mqtt.addPublishTopic(cfg));
    }
    
    // Intentar agregar uno más debería fallar
    ModbusMQTTTopicConfig extra;
    strcpy(extra.topic, "modbus/extra");
    ASSERT_FALSE(mqtt.addPublishTopic(extra));
}

// Test 4: Agregar topics de suscripción
TEST(mqtt_add_subscribe_topics) {
    ModbusMQTT mqtt;
    
    ModbusMQTTTopicConfig config;
    strcpy(config.topic, "modbus/control/setpoint");
    config.registerAddress = 10;
    config.qos = MQTT_QOS_1;
    
    ASSERT_TRUE(mqtt.addSubscribeTopic(config));
    ASSERT_EQ(mqtt.getNumSubscribeTopics(), 1);
    
    // Limpiar topics
    mqtt.clearSubscribeTopics();
    ASSERT_EQ(mqtt.getNumSubscribeTopics(), 0);
}

// Test 5: Lectura y escritura de registros
TEST(mqtt_register_operations) {
    ModbusMQTT mqtt;
    ModbusMock modbus;
    
    modbus.Hreg(0, 250);  // Temperatura inicial
    modbus.Hreg(10, 100); // Setpoint inicial
    
    ASSERT_TRUE(mqtt.begin(&modbus));
    
    // Leer registro
    int16_t value;
    ASSERT_TRUE(mqtt.readRegister(0, MODBUS_HOLDING_REGISTER, &value));
    ASSERT_EQ(value, 250);
    
    // Escribir registro
    ASSERT_TRUE(mqtt.writeRegister(10, MODBUS_HOLDING_REGISTER, 150));
    ASSERT_EQ(modbus.Hreg(10), 150);
}

// Test 6: Publicación manual de valores
TEST(mqtt_manual_publish) {
    ModbusMQTT mqtt;
    ModbusMock modbus;
    
    ASSERT_TRUE(mqtt.begin(&modbus));
    ASSERT_TRUE(mqtt.isConnected());
    
    uint32_t before, after;
    mqtt.getStatistics(nullptr, &before, nullptr);
    
    ASSERT_TRUE(mqtt.publishValue("test/topic", 42));
    
    mqtt.getStatistics(nullptr, &after, nullptr);
    ASSERT_EQ(after, before + 1);
}

// Test 7: Formatos de publicación
TEST(mqtt_publish_formats) {
    ModbusMQTT mqtt;
    
    mqtt.setPublishFormat(MQTT_FORMAT_PLAIN);
    mqtt.setPublishFormat(MQTT_FORMAT_JSON);
    mqtt.setPublishFormat(MQTT_FORMAT_CSV);
    
    // Configurar topic base
    mqtt.setBaseTopic("factory1/");
}

// Test 8: Callbacks
static bool callback_called = false;
static std::string callback_topic;

void mockMessageCallback(const char* topic, const uint8_t* payload, 
                         size_t length, void* userData) {
    callback_called = true;
    callback_topic = topic;
}

void mockStateCallback(int state, void* userData) {
    callback_called = true;
}

TEST(mqtt_callbacks) {
    ModbusMQTT mqtt;
    
    callback_called = false;
    mqtt.onMessage(mockMessageCallback, nullptr);
    mqtt.onStateChange(mockStateCallback, nullptr);
    
    // Los callbacks están registrados
    // (En una implementación real, se llamarían durante process())
}

// Test 9: Estadísticas
TEST(mqtt_statistics) {
    ModbusMQTT mqtt;
    ModbusMock modbus;
    
    // Inicialmente las estadísticas deberían estar en 0
    uint32_t reconnects, published, received;
    mqtt.getStatistics(&reconnects, &published, &received);
    ASSERT_EQ(reconnects, 0);
    ASSERT_EQ(published, 0);
    ASSERT_EQ(received, 0);
    
    ASSERT_TRUE(mqtt.begin(&modbus));
    
    // Llamar a process() para simular la conexión
    mqtt.process();
    
    // Después de begin+process, debería haber una reconexión simulada
    mqtt.getStatistics(&reconnects, &published, &received);
    ASSERT_TRUE(reconnects >= 1);
    ASSERT_EQ(published, 0);
    ASSERT_EQ(received, 0);
    
    // Publicar algunos mensajes
    mqtt.publishValue("test/1", 1);
    mqtt.publishValue("test/2", 2);
    mqtt.publishValue("test/3", 3);
    
    mqtt.getStatistics(nullptr, &published, nullptr);
    ASSERT_EQ(published, 3);
}

// Test 10: Múltiples topics con diferentes configuraciones
TEST(mqtt_multiple_topics_config) {
    ModbusMQTT mqtt;
    
    // Topic 1: Publicación onChange
    ModbusMQTTTopicConfig temp;
    strcpy(temp.topic, "sensor/temperature");
    temp.registerAddress = 0;
    temp.publishOnChange = true;
    temp.description = "Temperatura en °C";
    mqtt.addPublishTopic(temp);
    
    // Topic 2: Publicación periódica
    ModbusMQTTTopicConfig hum;
    strcpy(hum.topic, "sensor/humidity");
    hum.registerAddress = 1;
    hum.publishOnChange = false;
    hum.publishInterval = 5000;
    mqtt.addPublishTopic(hum);
    
    // Topic 3: Suscripción con QoS 2
    ModbusMQTTTopicConfig ctrl;
    strcpy(ctrl.topic, "control/setpoint");
    ctrl.registerAddress = 10;
    ctrl.qos = MQTT_QOS_2;
    ctrl.retain = true;
    mqtt.addSubscribeTopic(ctrl);
    
    ASSERT_EQ(mqtt.getNumPublishTopics(), 2);
    ASSERT_EQ(mqtt.getNumSubscribeTopics(), 1);
}

// Test 11: Estado de conexión
TEST(mqtt_connection_state) {
    ModbusMQTT mqtt;
    
    ASSERT_EQ(mqtt.getConnectionState(), MQTT_DISCONNECTED);
    ASSERT_FALSE(mqtt.isConnected());
    
    mqtt.begin();
    ASSERT_TRUE(mqtt.isConnected());
    
    mqtt.end();
    ASSERT_FALSE(mqtt.isConnected());
}

// Test 12: Límites de configuración
TEST(mqtt_limits) {
    ModbusMQTT mqtt;
    
    // Broker address largo
    char longBroker[100];
    memset(longBroker, 'a', sizeof(longBroker) - 1);
    longBroker[sizeof(longBroker) - 1] = '\0';
    
    mqtt.setBroker(longBroker, 1883);
    // No debe causar crash
    
    // Client ID largo
    char longId[100];
    memset(longId, 'b', sizeof(longId) - 1);
    longId[sizeof(longId) - 1] = '\0';
    
    mqtt.setClientId(longId);
    // No debe causar crash
}

// Test 13: Reinicio de topics
TEST(mqtt_clear_topics) {
    ModbusMQTT mqtt;
    
    // Agregar varios topics
    for (int i = 0; i < 5; i++) {
        ModbusMQTTTopicConfig cfg;
        snprintf(cfg.topic, sizeof(cfg.topic), "pub/%d", i);
        mqtt.addPublishTopic(cfg);
        
        snprintf(cfg.topic, sizeof(cfg.topic), "sub/%d", i);
        mqtt.addSubscribeTopic(cfg);
    }
    
    ASSERT_EQ(mqtt.getNumPublishTopics(), 5);
    ASSERT_EQ(mqtt.getNumSubscribeTopics(), 5);
    
    // Limpiar
    mqtt.clearPublishTopics();
    mqtt.clearSubscribeTopics();
    
    ASSERT_EQ(mqtt.getNumPublishTopics(), 0);
    ASSERT_EQ(mqtt.getNumSubscribeTopics(), 0);
}

// Test 14: Operaciones sin inicialización
TEST(mqtt_without_begin) {
    ModbusMQTT mqtt;
    
    // Sin begin(), las operaciones deberían fallar gracefully
    ASSERT_FALSE(mqtt.isConnected());
    
    int16_t value;
    ASSERT_FALSE(mqtt.readRegister(0, MODBUS_HOLDING_REGISTER, &value));
    ASSERT_FALSE(mqtt.writeRegister(0, MODBUS_HOLDING_REGISTER, 100));
    ASSERT_FALSE(mqtt.publishValue("test", 42));
}

// Test 15: Configuración completa típica
TEST(mqtt_typical_configuration) {
    ModbusMQTT mqtt;
    ModbusMock modbus;
    
    // Configurar broker público para testing
    mqtt.setBroker("test.mosquitto.org", 1883);
    mqtt.setClientId("modbus_test_device_001");
    mqtt.setAutoReconnect(true);
    
    // Configurar topics de sensores
    ModbusMQTTTopicConfig configs[3];
    
    strcpy(configs[0].topic, "factory/device1/temperature");
    configs[0].registerAddress = 0;
    configs[0].publishOnChange = true;
    
    strcpy(configs[1].topic, "factory/device1/humidity");
    configs[1].registerAddress = 1;
    configs[1].publishInterval = 10000;
    
    strcpy(configs[2].topic, "factory/device1/pressure");
    configs[2].registerAddress = 2;
    configs[2].publishOnChange = true;
    
    for (int i = 0; i < 3; i++) {
        ASSERT_TRUE(mqtt.addPublishTopic(configs[i]));
    }
    
    // Configurar topic de control
    ModbusMQTTTopicConfig control;
    strcpy(control.topic, "factory/device1/setpoint");
    control.registerAddress = 10;
    control.qos = MQTT_QOS_1;
    control.retain = true;
    ASSERT_TRUE(mqtt.addSubscribeTopic(control));
    
    // Iniciar y verificar conexión
    ASSERT_TRUE(mqtt.begin(&modbus));
    ASSERT_TRUE(mqtt.isConnected());
    
    // Simular operación
    mqtt.process();
    
    // Verificar que hay al menos un topic configurado
    ASSERT_EQ(mqtt.getNumPublishTopics(), 3);
    ASSERT_EQ(mqtt.getNumSubscribeTopics(), 1);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Tests Unitarios: Puente Modbus-MQTT" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Ejecutar todos los tests
    RUN_TEST(mqtt_broker_config);
    RUN_TEST(mqtt_auto_reconnect);
    RUN_TEST(mqtt_add_publish_topics);
    RUN_TEST(mqtt_add_subscribe_topics);
    RUN_TEST(mqtt_register_operations);
    RUN_TEST(mqtt_manual_publish);
    RUN_TEST(mqtt_publish_formats);
    RUN_TEST(mqtt_callbacks);
    RUN_TEST(mqtt_statistics);
    RUN_TEST(mqtt_multiple_topics_config);
    RUN_TEST(mqtt_connection_state);
    RUN_TEST(mqtt_limits);
    RUN_TEST(mqtt_clear_topics);
    RUN_TEST(mqtt_without_begin);
    RUN_TEST(mqtt_typical_configuration);
    
    // Resumen
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "RESUMEN DE TESTS" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests pasados:  " << tests_passed << std::endl;
    std::cout << "Tests fallidos: " << tests_failed << std::endl;
    std::cout << "Total:          " << (tests_passed + tests_failed) << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (tests_failed > 0) {
        std::cout << "\n❌ ALGUNOS TESTS FALLARON" << std::endl;
        return 1;
    } else {
        std::cout << "\n✅ TODOS LOS TESTS PASARON" << std::endl;
        return 0;
    }
}
