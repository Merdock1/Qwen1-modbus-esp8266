/*
  Ejemplo: Comunicación Modbus ASCII
  TAREA 3.2: SOPORTE MODBUS ASCII
  
  Este ejemplo demuestra cómo usar la biblioteca ModbusASCII para comunicación
  serial en formato ASCII hexadecimal, una alternativa más legible al formato RTU binario.
  
  Características demostradas:
  - Configuración básica de esclavo Modbus ASCII
  - Registro de coils, discrete inputs, input registers y holding registers
  - Parsing automático de tramas ASCII
  - Validación LRC (Longitudinal Redundancy Check)
  
  Hardware requerido:
  - Arduino/ESP8266/ESP32
  - Convertidor RS485 (MAX485, SP3485, etc.) si se usa comunicación diferencial
  
  Conexiones típicas RS485:
  - MAX485 RO -> RX del microcontrolador
  - MAX485 DI -> TX del microcontrolador
  - MAX485 DE+RE -> Pin de control (opcional, GPIO según placa)
  - MAX485 A/B -> Bus RS485 diferencial
  
  Formato de trama ASCII:
  :[Address][Function][Data][LRC]\r\n
  Ejemplo: :010300000001F9\r\n
           :  = Inicio (0x3A)
           01 = Dirección esclavo
           03 = Función (Read Holding Registers)
           0000 = Registro inicial
           0001 = Cantidad de registros
           F9 = LRC
           \r\n = Fin de línea
  
  Diferencias clave vs RTU:
  - ASCII: Más lento (~2x), más legible, debuggable con terminal serial
  - RTU: Más rápido, más eficiente, requiere analyzer para debug
  
  Escrito por: Modbus Library Team
  Fecha: 2024
  Licencia: BSD New License
*/

#include <ModbusASCII.h>

// Configurar puerto serial y pines RS485
#define SERIAL_BAUD 9600
#define RS485_DE_PIN -1  // -1 si no usa control de dirección (USB-TTL directo)

// Crear instancia ModbusASCII
ModbusASCII mb;

// IDs de registro para el ejemplo
#define COIL_START    0
#define DISCRETE_START 0
#define IREG_START    0
#define HREG_START    0

// Cantidad de registros
#define NUM_COILS     10
#define NUM_DISCRETE  10
#define NUM_IREGS     10
#define NUM_HREGS     10

// Variables de datos
bool coils[NUM_COILS];
bool discretes[NUM_DISCRETE];
uint16_t iregs[NUM_IREGS];
uint16_t hregs[NUM_HREGS];

// Callback para logging de eventos Modbus
void modbusLog(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  Serial.print("Evento Modbus: 0x");
  Serial.print(event, HEX);
  Serial.print(" Transacción: ");
  Serial.println(transactionId);
}

void setup() {
  // Inicializar comunicación serial
  Serial.begin(115200);
  while (!Serial) {
    ; // Esperar conexión serial (necesario para algunas placas)
  }
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("Ejemplo Modbus ASCII - Esclavo");
  Serial.println("========================================");
  Serial.println();
  
  // Inicializar puerto serial para Modbus
  Serial1.begin(SERIAL_BAUD, SERIAL_8N1);
  
  // Inicializar biblioteca ModbusASCII
  mb.begin(&Serial1, RS485_DE_PIN);
  
  // Configurar como esclavo con ID 1
  mb.slave(1);
  
  Serial.println("Configurando registros Modbus...");
  
  // Agregar Coils (salidas digitales escribibles)
  for (int i = 0; i < NUM_COILS; i++) {
    mb.addCoil(COIL_START + i, false);
    coils[i] = false;
  }
  Serial.printf("  - %d Coils agregados (dirección %d-%d)\n", 
                NUM_COILS, COIL_START, COIL_START + NUM_COILS - 1);
  
  // Agregar Discrete Inputs (entradas digitales solo lectura)
  for (int i = 0; i < NUM_DISCRETE; i++) {
    mb.addIsts(DISCRETE_START + i, (i % 2 == 0));  // Alternar true/false
    discretes[i] = (i % 2 == 0);
  }
  Serial.printf("  - %d Discrete Inputs agregados (dirección %d-%d)\n", 
                NUM_DISCRETE, DISCRETE_START, DISCRETE_START + NUM_DISCRETE - 1);
  
  // Agregar Input Registers (entradas analógicas solo lectura)
  for (int i = 0; i < NUM_IREGS; i++) {
    mb.addIreg(IREG_START + i, i * 100);  // Valores: 0, 100, 200, ...
    iregs[i] = i * 100;
  }
  Serial.printf("  - %d Input Registers agregados (dirección %d-%d)\n", 
                NUM_IREGS, IREG_START, IREG_START + NUM_IREGS - 1);
  
  // Agregar Holding Registers (registros escribibles)
  for (int i = 0; i < NUM_HREGS; i++) {
    mb.addHreg(HREG_START + i, i * 10);  // Valores: 0, 10, 20, ...
    hregs[i] = i * 10;
  }
  Serial.printf("  - %d Holding Registers agregados (dirección %d-%d)\n", 
                NUM_HREGS, HREG_START, HREG_START + NUM_HREGS - 1);
  
  Serial.println();
  Serial.println("Configuración completada!");
  Serial.println();
  Serial.println("Esperando comandos Modbus ASCII...");
  Serial.println();
  Serial.println("Funciones soportadas:");
  Serial.println("  0x01 - Read Coils");
  Serial.println("  0x02 - Read Discrete Inputs");
  Serial.println("  0x03 - Read Holding Registers");
  Serial.println("  0x04 - Read Input Registers");
  Serial.println("  0x05 - Write Single Coil");
  Serial.println("  0x06 - Write Single Register");
  Serial.println("  0x0F - Write Multiple Coils");
  Serial.println("  0x10 - Write Multiple Registers");
  Serial.println();
  Serial.println("Ejemplo de comando (desde maestro):");
  Serial.println("  :010300000001F9\\r\\n  <- Leer 1 Holding Register desde addr 0");
  Serial.println("  Respuesta esperada:");
  Serial.println("  :0103020000F9\\r\\n  <- Valor del register 0 es 0x0000");
  Serial.println();
}

void loop() {
  // Procesar comunicación Modbus (debe llamarse frecuentemente)
  mb.task();
  
  // Ejemplo: Actualizar valor de Input Register 0 con tiempo transcurrido
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    
    // Actualizar Input Register 0 con segundos transcurridos
    uint16_t seconds = millis() / 1000;
    mb.Ireg(IREG_START, seconds % 65536);  // Limitar a 16 bits
    
    // Toggle Coil 0 cada 2 segundos para demostración
    static bool coilState = false;
    if ((seconds % 2) == 0 && !coilState) {
      coilState = true;
      mb.Coil(COIL_START, true);
    } else if ((seconds % 2) == 1 && coilState) {
      coilState = false;
      mb.Coil(COIL_START, false);
    }
  }
  
  // Pequeño delay para evitar busy-waiting
  delay(1);
}

/*
  PRUEBA CON QMODMASTER O COMGRABBER:
  
  1. Conectar convertidor USB-RS485 a la PC
  2. Abrir QModMaster (o similar)
  3. Configurar:
     - Modo: ASCII (no RTU!)
     - Puerto: COMx (el asignado al USB-RS485)
     - Baudrate: 9600
     - Data: 8 bits
     - Parity: None
     - Stop: 1 bit
     - Slave ID: 1
  
  4. Probar lecturas:
     - FC03 Read Holding Registers: Address 0, Count 10
     - FC04 Read Input Registers: Address 0, Count 10
     - FC01 Read Coils: Address 0, Count 10
     - FC02 Read Discrete Inputs: Address 0, Count 10
  
  5. Probar escrituras:
     - FC05 Write Single Coil: Address 0, Value ON/OFF
     - FC06 Write Single Register: Address 0, Value 1234
     - FC15 Write Multiple Coils: Address 0, Count 5
     - FC16 Write Multiple Registers: Address 0, Count 5
  
  MONITORING CON TERMINAL SERIAL:
  
  Para ver las tramas ASCII en texto plano:
  1. Abrir Monitor Serial Arduino IDE (115200 baud)
  2. Usar otro programa terminal (Putty, screen) en el puerto RS485
  3. Las tramas serán visibles como texto:
     :010300000001F9\r\n
     :0103020000F9\r\n
  
  SOLUCIÓN DE PROBLEMAS:
  
  Problema: No hay respuesta
  - Verificar conexiones RS485 (A/B invertidos?)
  - Verificar baudrate coincide (9600)
  - Verificar Slave ID es 1
  - Asegurar que mb.task() se llama en loop()
  
  Problema: Errores LRC
  - Verificar que el maestro usa modo ASCII (no RTU)
  - Verificar configuración 8N1 (8 datos, sin paridad, 1 stop)
  - Ruido eléctrico en bus RS485 (usar terminación 120Ω)
  
  Problema: Timeouts
  - Aumentar timeout en maestro (modo ASCII es más lento)
  - Verificar que task() se llama suficientemente rápido
  
  RECURSOS ADICIONALES:
  
  - Documentación completa: docs/API_ES.md
  - Especificación Modbus ASCII: modbus.org/docs/Modbus_ASCII.pdf
  - Herramientas de test: qmodmaster.sourceforge.net
*/
