/*
    Biblioteca Modbus para Arduino
    Protocolo Modbus TLS
    Copyright (C) 2014 André Sarmento Barbosa
                  2017-2021 Alexander Emelianov (a.m.emelianov@gmail.com)
    
    @file ModbusTLS.h
    @brief Implementación segura del protocolo Modbus sobre TLS/SSL
*/
#pragma once
#if !defined(ESP8266) && !defined(ESP32)
#error Unsupported architecture
#endif
#include <WiFiClientSecure.h>
#if defined(ESP8266)
#include <WiFiServerSecure.h>
#else
// Just emty stub
class WiFiServerSecure {
public:
    WiFiServerSecure(uint16_t){}
    WiFiClientSecure available(){}
    void begin();
    inline WiFiClientSecure accept() {
        return available();
    }
};
#endif
#include "ModbusTCPTemplate.h"
#include "ModbusAPI.h"

/**
 * @brief Clase ModbusTLS para comunicación Modbus TCP segura (TLS/SSL)
 * @details Implementa comunicación Modbus sobre TLS para ESP8266/ESP32
 *          CORRECCIÓN CRÍTICA: Todos los objetos BearSSL usan stack en lugar de heap
 */
class ModbusTLS : public ModbusAPI<ModbusTCPTemplate<WiFiServerSecure, WiFiClientSecure>> {
    private:
    /**
     * @brief Establece conexión segura con un cliente
     * @param ip Dirección IP del slave
     * @param port Puerto de conexión
     * @param client_cert Certificado del cliente (opcional)
     * @param client_private_key Clave privada del cliente (opcional)
     * @return Índice del cliente o -1 si falla
     */
    int8_t _connect(IPAddress ip, uint16_t port, const char* client_cert = nullptr, const char* client_private_key = nullptr) {
            int8_t p = getFreeClient();
            if (p < 0)
                    return p;
            tcpclient[p] = new WiFiClientSecure();
        BIT_CLEAR(tcpServerConnection, p);
        #if defined(ESP8266)
        // CORRECCIÓN CRÍTICA: Usar objetos en stack en lugar de heap para evitar fugas de memoria
        // Los objetos BearSSL se destruyen automáticamente al salir del scope
        BearSSL::X509List clientCertList(client_cert);
        BearSSL::PrivateKey clientPrivKey(client_private_key);
        tcpclient[p]->setClientRSACert(&clientCertList, &clientPrivKey);
        tcpclient[p]->setBufferSizes(512, 512);
        #else
        tcpclient[p]->setCertificate(client_cert);
        tcpclient[p]->setPrivateKey(client_private_key);
        #endif
        return p;
    }
#if defined(MODBUSIP_USE_DNS)
    static IPAddress resolver (const char* host) {
        IPAddress remote_addr;
        if (WiFi.hostByName(host, remote_addr))
                return remote_addr;
        return IPADDR_NONE;
    }
#endif
    public:
    ModbusTLS() : ModbusAPI() {
        defaultPort = MODBUSTLS_PORT;
#if defined(MODBUSIP_USE_DNS)
        resolve = resolver;
#endif
    }
    #if defined(ESP8266)
    /**
     * @brief Inicia servidor TLS seguro
     * @param port Puerto del servidor
     * @param server_cert Certificado del servidor (opcional)
     * @param server_private_key Clave privada del servidor (opcional)
     * @param ca_cert Autoridad certificadora para validar clientes (opcional)
     */
    void server(uint16_t port, const char* server_cert = nullptr, const char* server_private_key = nullptr, const char* ca_cert = nullptr) {
        serverPort = port;
            tcpserver = new WiFiServerSecure(serverPort);
        // CORRECCIÓN CRÍTICA: Usar objetos en stack para evitar fugas de memoria
        BearSSL::X509List serverCertList(server_cert);
        BearSSL::PrivateKey serverPrivKey(server_private_key);
        tcpserver->setRSACert(&serverCertList, &serverPrivKey);
        if (ca_cert) {
            BearSSL::X509List trustedCA(ca_cert);
            tcpserver->setClientTrustAnchor(&trustedCA);
        }
        //tcpserver->setBufferSizes(512, 512);
            tcpserver->begin();
    }

    /**
     * @brief Conecta con clave pública conocida
     * @param ip Dirección IP del slave
     * @param port Puerto de conexión
     * @param client_cert Certificado del cliente (opcional)
     * @param client_private_key Clave privada del cliente (opcional)
     * @param key Clave pública conocida del servidor
     * @return true si conecta exitosamente
     */
    bool connectWithKnownKey(IPAddress ip, uint16_t port, const char* client_cert = nullptr, const char* client_private_key = nullptr, const char* key = nullptr) {
        if(getSlave(ip) >= 0)
                    return true;
        int8_t p = _connect(ip, port, client_cert, client_private_key);
        // CORRECCIÓN CRÍTICA: Usar objeto en stack para evitar fuga de memoria
        BearSSL::PublicKey clientPublicKey(key);
        tcpclient[p]->setKnownKey(&clientPublicKey);
        return tcpclient[p]->connect(ip, port);
    }

    #endif
#if defined(MODBUSIP_USE_DNS)
    bool connect(String host, uint16_t port, const char* client_cert = nullptr, const char* client_private_key = nullptr, const char* ca_cert = nullptr) {
        return connect(resolver(host.c_str()), port, client_cert, client_private_key, ca_cert);
    }
    bool connect(const char* host, uint16_t port, const char* client_cert = nullptr, const char* client_private_key = nullptr, const char* ca_cert = nullptr) {
        return connect(resolver(host), port, client_cert, client_private_key, ca_cert);
    }
#endif
    /**
     * @brief Conecta como cliente TLS con validación de certificado
     * @param ip Dirección IP del slave
     * @param port Puerto de conexión
     * @param client_cert Certificado del cliente (opcional)
     * @param client_private_key Clave privada del cliente (opcional)
     * @param ca_cert Autoridad certificadora para validar el servidor (opcional)
     * @return true si conecta exitosamente
     */
    bool connect(IPAddress ip, uint16_t port, const char* client_cert = nullptr, const char* client_private_key = nullptr, const char* ca_cert = nullptr) {
        if (!ip)
            return false;
        if(getSlave(ip) >= 0)
                    return false;
        int8_t p = _connect(ip, port, client_cert, client_private_key);
        if (p < 0)
            return false;
        #if defined(ESP8266)
        if (ca_cert) {
            // CORRECCIÓN CRÍTICA: Usar objeto en stack para evitar fuga de memoria
            BearSSL::X509List trustedCA(ca_cert);
            tcpclient[p]->setTrustAnchors(&trustedCA);
        } else {
            tcpclient[p]->setInsecure();
        }
        #else
        if (ca_cert) {
            tcpclient[p]->setCACert(ca_cert);
        }
        #endif
        //return tcpclient[p]->connect(ip, port);
        if (!tcpclient[p]->connect(ip, port))
            return false;
        return true;
    }
};
