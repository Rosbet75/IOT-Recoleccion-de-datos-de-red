# 📡 Proyecto IoT con ESP32, Docker y SQL

Este proyecto implementa una arquitectura **IoT completa**, donde un **ESP32 programado desde Arduino IDE** se conecta a la red, envía datos a un **backend desplegado con Docker Compose**, los almacena en una **base de datos MySQL** y posteriormente los datos pueden ser **visualizados mediante scripts en Python**.

El objetivo es demostrar un flujo realista de **captura → transmisión → persistencia → visualización de datos** usando tecnologías ampliamente utilizadas en entornos profesionales.

---

## 🧠 Arquitectura general
+-------------+ WiFi / HTTP +-------------------+
| ESP32 | ───────────────────▶ | Apache + PHP API |
| (Arduino) | | (Docker) |
+-------------+ +-------------------+
|
| mysqli
▼
+-------------------+
| MySQL |
| (Docker) |
+-------------------+
|
▼
+-------------------+
| Python |
| Data Analysis |
+-------------------+

---

## 🔌 Dispositivo IoT (ESP32)

- Programado usando **Arduino IDE**
- Conectado a una red WiFi
- Envía datos periódicamente (lecturas de sensores o datos simulados)
- Comunicación mediante **HTTP** hacia el backend

### Tecnologías
- ESP32 / ESP32-S3
- Arduino Framework
- WiFiClient / HTTPClient

---

## 🐳 Backend con Docker Compose

La infraestructura del backend se levanta completamente usando **Docker Compose**, permitiendo reproducibilidad y aislamiento del entorno.

### Servicios incluidos
- **Apache + PHP 8.2** → API para recibir datos del ESP32
- **MySQL 8.0** → Persistencia de datos
- **phpMyAdmin** → Administración visual de la base de datos

---
