/*
    Biblioteca Modbus para Arduino
    Protocolo Modbus RTU
    Copyright (C) 2014 André Sarmento Barbosa
                  2017-2021 Alexander Emelianov (a.m.emelianov@gmail.com)
    
    @file ModbusRTU.h
    @brief Implementación del protocolo Modbus RTU sobre comunicación serial
*/
#pragma once
#include "ModbusAPI.h"

// Soporte para mutex en ESP32 para operaciones multi-hilo
#if defined(ESP32) && defined(MODBUS_THREAD_SAFE)
#include <mutex>
#endif

class ModbusRTUTemplate : public Modbus {
    protected:
        Stream* _port;
        int16_t   _txEnablePin = -1;
#if defined(MODBUSRTU_REDE)
        int16_t   _rxPin = -1;
#endif
		bool _direct = true;	// Transmit control logic (true=txEnableDirect, false=inverse)
		uint32_t _t;	// inter-Frame Delay in uS
#if defined(MODBUSRTU_FLUSH_DELAY)
		uint32_t _t1;	// char send time
#endif
		uint32_t t = 0;		// time since last Data byte arrived
		bool isMaster = false;
		uint8_t  _slaveId;
		uint32_t _timestamp = 0;
		cbTransaction _cb = nullptr;
		uint8_t* _data = nullptr;
		uint8_t* _sentFrame = nullptr;
		TAddress _sentReg = COIL(0);
		uint16_t maxRegs = MODBUS_MAX_WORDS;
		uint8_t address = 0;
		
		// Phase 2 Security: Security configuration and rate limiting
		SecurityConfig_t _securityConfig = SECURITY_CONFIG_DEFAULT;
		RateLimiter_t _rateLimiter = {0, 0, 0};
		
		// Phase 3 Performance: Buffer Pool and performance statistics
		BufferPoolConfig_t _bufferPoolConfig = BUFFER_POOL_CONFIG_DEFAULT;
		PerformanceStats_t _perfStats = {0, 0, 0, 0, 0, 0};
		uint8_t* _bufferPool[MODBUS_BUFFER_POOL_SIZE] = {nullptr};
		bool _bufferPoolAvailable[MODBUS_BUFFER_POOL_SIZE] = {true};
		uint8_t _poolIndex = 0;
		
		// Tarea 2.3: Dynamic timeout calculation based on baudrate
		uint32_t _currentBaudrate = 0;
		uint32_t _timeoutBase = 0;
		uint32_t _charTime = 0;
		bool _autoTimeoutEnabled = true;

#if defined(ESP32) && defined(MODBUS_THREAD_SAFE)
		std::mutex _taskMutex;  // Mutex para proteger operaciones en multi-hilo
#endif

		uint16_t send(uint8_t slaveId, TAddress startreg, cbTransaction cb, uint8_t unit = MODBUSIP_UNIT, uint8_t* data = nullptr, bool waitResponse = true);
		// Prepare and send ModbusRTU Frame. _frame Buffer and _len should be filled with Modbus Data
		// slaveId - Slave id
		// startreg - first local Register to save returned Data to (meaningless for Write to Slave operations)
		// cb - transaction Callback function
		// Data - if not null use Buffer to save returned Data instead of local registers
		bool rawSend(uint8_t slaveId, uint8_t* frame, uint8_t len);
		bool cleanup(); 	// Free clients if not connected and remove timeout transactions and transaction with processed events
		uint16_t crc16(uint8_t address, uint8_t* frame, uint8_t pdulen);
		uint16_t crc16_alt(uint8_t address, uint8_t* frame, uint8_t pduLen);
		
		// Fase 3: Gestión de Buffer Pool
		uint8_t* allocateBuffer(uint16_t size);
		void freeBuffer(uint8_t* buffer);
		void initBufferPool();
    public:
		void setBaudrate(uint32_t baud = -1);
		uint32_t calculateMinimumInterFrameTime(uint32_t baud, uint8_t char_bits = 11);
		void setInterFrameTime(uint32_t t_us);
		uint32_t charSendTime(uint32_t baud, uint8_t char_bits = 11);
		template <class T>
		bool begin(T* port, int16_t txEnablePin = -1, bool txEnableDirect = true);
#if defined(MODBUSRTU_REDE)
		template <class T>
		bool begin(T* port, int16_t txEnablePin, int16_t rxEnablePin, bool txEnableDirect);
#endif
		bool begin(Stream* port, int16_t txEnablePin = -1, bool txEnableDirect = true);
        void task();
		void client() { isMaster = true; };
		inline void master() {client();}
		void server(uint8_t serverId) {_slaveId = serverId;};
		inline void slave(uint8_t slaveId) {server(slaveId);}
		uint8_t server() { return _slaveId; }
		inline uint8_t slave() { return server(); }
		uint32_t eventSource() override {return address;}
		
		// Phase 2 Security: Security configuration API
	public:
		void setSecurityConfig(const SecurityConfig_t& config) {_securityConfig = config;}
		SecurityConfig_t getSecurityConfig() const {return _securityConfig;}
		void enableSecurityLogging(bool enable) {_securityConfig.enableLogging = enable;}
		void setSecurityLogCallback(cbSecurityLog callback) {_securityConfig.logCallback = callback;}
		void enableStrictValidation(bool enable) {_securityConfig.enableStrictValidation = enable;}
		void enableDoSProtection(bool enable) {_securityConfig.enableDoSProtection = enable;}
		void enableRateLimiting(bool enable) {_securityConfig.enableRateLimiting = enable;}
		RateLimiter_t getRateLimiterStats() const {return _rateLimiter;}
		
		// Phase 3 Performance: Performance optimization API
	public:
		void initBufferPool();
		void setBufferPoolConfig(const BufferPoolConfig_t& config) {_bufferPoolConfig = config;}
		BufferPoolConfig_t getBufferPoolConfig() const {return _bufferPoolConfig;}
		PerformanceStats_t getPerformanceStats() const {return _perfStats;}
		void resetPerformanceStats() {_perfStats = {0, 0, 0, 0, 0, 0};}
		void enableBufferPool(bool enable) {_bufferPoolConfig.enableBufferPool = enable;}
		
		// Tarea 2.3: Dynamic timeout management API
		/**
		 * @brief Habilita o deshabilita el cálculo automático de timeouts basado en baudrate
		 * @param enable true para habilitar, false para deshabilitar
		 */
		void enableAutoTimeout(bool enable) {_autoTimeoutEnabled = enable;}
		
		/**
		 * @brief Configura el baudrate y calcula automáticamente los timeouts
		 * @param baud Baudrate deseado (1200-921600)
		 * @return true si exitoso, false si baudrate inválido
		 */
		bool setBaudrate(uint32_t baud);
		
		/**
		 * @brief Obtiene el baudrate actual configurado
		 * @return Baudrate actual en bps
		 */
		uint32_t getCurrentBaudrate() const {return _currentBaudrate;}
		
		/**
		 * @brief Calcula el tiempo de inter-frame mínimo según especificación Modbus
		 * @param baud Baudrate del puerto serial
		 * @param char_bits Tamaño de carácter (default 11 bits según spec Modbus)
		 * @return Tiempo en microsegundos
		 */
		uint32_t calculateMinimumInterFrameTime(uint32_t baud, uint8_t char_bits = 11);
		
		/**
		 * @brief Establece manualmente el tiempo de inter-frame
		 * @param t_us Tiempo en microsegundos
		 */
		void setInterFrameTime(uint32_t t_us);
		
		/**
		 * @brief Calcula el tiempo de transmisión de un carácter
		 * @param baud Baudrate del puerto
		 * @param char_bits Tamaño de carácter en bits
		 * @return Tiempo en microsegundos
		 */
		uint32_t charSendTime(uint32_t baud, uint8_t char_bits = 11);
		
		/**
		 * @brief Obtiene el timeout actual configurado
		 * @return Timeout en microsegundos
		 */
		uint32_t getTimeout() const {return _timeoutBase;}
		
		/**
		 * @brief Configura timeout personalizado para comunicación
		 * @param timeout_us Timeout en microsegundos
		 */
		void setTimeout(uint32_t timeout_us) {_timeoutBase = timeout_us; _autoTimeoutEnabled = false;}
};

template <class T>
bool ModbusRTUTemplate::begin(T* port, int16_t txEnablePin, bool txEnableDirect) {
    uint32_t baud = 0;
    #if defined(ESP32) || defined(ESP8266) // baudRate() only available on ESP32+ESP8266
    baud = port->baudRate();
    #else
    baud = 9600;
    #endif
    
    // Tarea 2.3: Configurar baudrate y calcular timeouts dinámicamente
    _currentBaudrate = baud;
    setInterFrameTime(calculateMinimumInterFrameTime(baud));
    
#if defined(MODBUSRTU_FLUSH_DELAY)
	_t1 = charSendTime(baud);
#endif
    _port = port;
    if (txEnablePin >= 0) {
	    _txEnablePin = txEnablePin;
		_direct = txEnableDirect;
        pinMode(_txEnablePin, OUTPUT);
        digitalWrite(_txEnablePin, _direct?LOW:HIGH);
    }
    return true;
}
#if defined(MODBUSRTU_REDE)
template <class T>
bool ModbusRTUTemplate::begin(T* port, int16_t txEnablePin, int16_t rxEnablePin, bool txEnableDirect) {
	begin(port, txEnablePin, txEnableDirect);
	if (rxEnablePin > 0) {
		_rxPin = rxEnablePin;
        pinMode(_rxPin, OUTPUT);
        digitalWrite(_rxPin, _direct?LOW:HIGH);
	}
	return true;
}
#endif
class ModbusRTU : public ModbusAPI<ModbusRTUTemplate> {};
