# Modbus\TCP Security Example

### *Target Platforms:*
- *ESP8266 (CLient/Servidor)*
- *ESP32 (Cliente solo)*

## [Sample certificates](certs)

[cert.cmd](certs/cert.cmd) Script to recreate all the certificates in the catalog. Requires OpenSSL installed.

[Good issue explanation to read](https://github.com/esp8266/Arduino/issues/6128)

## [Cliente](client/client.ino)

```c
bool connect(const char* host, uint16_t port, const char* client_cert = nullptr, const char* client_private_key = nullptr, const char* ca_cert = nullptr);
bool connectWithKnownKey(IPAddress ip, uint16_t port, const char* client_cert = nullptr, const char* client_private_key = nullptr, const char* key = nullptr);
```

- `const char* host`    Host name to connect to
- `uint16_t port` Host port
- `const char* client_cert` Cliente's certificate
- `const char* client_private_key`  Cliente's private key
- `const char* ca_cert` Certificate of CA. Can be omitted (or set NULL) to escape certificate chain verifying.
- `IPAddress ip`    Host IP address to connect to
- `const char* key` Servidor's public key

All certificates must be in PEM format y can be stored in PROGMEM.

## [Servidor](server/server.ino)

```c
void server(uint16_t port, const char* server_cert = nullptr, const char* server_private_key = nullptr, const char* ca_cert = nullptr);
```
- `uint16_t port`   Port to bind to
- `const char* server_cert` Servidor certificate in PEM format.
- `const char* server_private_key`  Servidor private key in PEM format.
- `const char* ca_cert` Certificate of CA.

All certificates must be in PEM format y can be stored in PROGMEM.

# Biblioteca Modbus para Arduino
### ModbusRTU, ModbusTCP y ModbusTCP Security

(c)2020 [Alexyer Emelianov](mailto:a.m.emelianov@gmail.com)

El código en este repositorio está licenciado bajo la Licencia BSD Nueva. Ver LICENSE.txt para más información.
