# 💧 EcoTanque IoT - TheHouseFit

Sistema de monitoreo de nivel de agua y cierre automático de emergencia por detección de fugas.

##  Descripción
El proyecto monitorea un tanque de agua mediante un sensor ultrasónico. Si el agua llega a la base, un sensor de humedad activa una válvula de emergencia (servomotor) para evitar desbordamientos. Los datos se envían por MQTT a un Dashboard Web (Angular) para su monitoreo y control en tiempo real.

##  Hardware Requerido
* **ESP32:** Microcontrolador principal
* **HC-SR04:** Ultrasónico (Pin `TRIG`: 26, `ECHO`: 25)
* **YL-69 / FC-28:** Humedad (Pin `AO`: 34)
* **Servo SG90:** Válvula (Pin Señal: 13)
* **LCD 16x2 I2C:** Pantalla (`SDA`: 21, `SCL`: 22)
* **Buzzer:** Alarma (Pin: 23)

##  Software y Tecnologías
* **PlatformIO (C++):** Lógica del ESP32, sensores y control del servo.
* **Angular (TypeScript/HTML/CSS):** Dashboard interactivo en tiempo real.
* **EMQX Cloud (MQTT):** Broker de comunicación (Puerto 8883, TLS).

##  Funcionamiento Clave
1. **Medición:** El ultrasónico calcula el % de llenado del tanque. Se muestra en LCD y se envía a la Web.
2. **Cierre de Emergencia:** Si el sensor de humedad detecta agua ($\ge 5\%$), el servo gira a $0^\circ$ bloqueando el paso, suena el buzzer y se envía la alerta al Dashboard.
3. **Control Remoto:** Desde la página Angular, el usuario puede abrir o cerrar la válvula manualmente (bloqueado si hay emergencia activa).

##  Solución de Problemas Comunes
* **El ESP32 se reinicia o no conecta:** El servomotor consume demasiada energía. Desconecta el cable rojo (`VCC`) del servo del ESP32 y aliméntalo con una fuente externa de 5V (uniendo las tierras).
* **El sensor de humedad no cierra:** Ajustar las variables `VALOR_SECO` y `VALOR_MOJADO` en el código según la lectura real del monitor serie.
* **No conecta a MQTT:** Verificar usuario, contraseña y el certificado CA en el código.