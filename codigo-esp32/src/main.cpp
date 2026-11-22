#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ==========================================
// 1. CONFIGURACIÓN WIFI
// ==========================================
const char* WIFI_SSID = "iphone de Steven"; 
const char* WIFI_PASSWORD = "943673812";

// ==========================================
// 2. CONFIGURACIÓN MQTT
// ==========================================
const char* MQTT_SERVER = "g11dab68.ala.us-east-1.emqxsl.com";
const int MQTT_PORT = 8883; 
const char* MQTT_USER = "tanque";  
const char* MQTT_PASSWORD = "1234"; 

const char* TOPIC_SENSOR = "esp32/casa01/sensor";
const char* TOPIC_COMANDO = "esp32/casa01/comando";

// ==========================================
// 3. CERTIFICADO CA
// ==========================================
const char* ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUyMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

WiFiClientSecure espClient;
PubSubClient mqtt(espClient);

// ==========================================
// 4. CONFIGURACIÓN DE PINES
// ==========================================
#define SDA_LCD 21
#define SCL_LCD 22
#define TRIG_PIN 26     
#define ECHO_PIN 25     
#define AGUA_AO 34      
#define BUZZER_PIN 23
#define SERVO_PIN 13   

// ==========================================
// 5. OBJETOS Y VARIABLES
// ==========================================
LiquidCrystal_I2C lcd(0x27, 16, 2); 
Servo valvulaServo;

float distancia = 0;
float distanciaAgua = 0;
int porcentajeTanque = 0;
int nivelAgua = 0;
bool valvulaAbierta = true; 

const float ALTURA_TANQUE_CM = 25.0; 

const int VALOR_SECO = 3200; 
const int VALOR_MOJADO = 1200; 

const int UMBRAL_CIERRE_HUMEDAD = 5;   
const int UMBRAL_CRUDO_MOJADO = 3100;   

unsigned long tiempoAnterior = 0;
bool alertaEnviada = false; 

// ==========================================
// 6. FUNCIONES MQTT
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
  String mensaje = "";
  for (unsigned int i = 0; i < length; i++) mensaje += (char)payload[i];

  if (String(topic) == TOPIC_COMANDO) {
    if (mensaje.indexOf("abrir") >= 0) {
      if (nivelAgua < UMBRAL_CIERRE_HUMEDAD) {
        valvulaServo.write(100); 
        valvulaAbierta = true;
        Serial.println("¡Válvula ABIERTA desde la App!");
      } else {
        Serial.println("Bloqueado: Humedad alta, no se puede abrir.");
      }
    } else if (mensaje.indexOf("cerrar") >= 0) {
      valvulaServo.write(0);   
      valvulaAbierta = false;
      Serial.println("¡Válvula CERRADA desde la App!");
    }
  }
}

void conectarWiFi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");
}

void conectarMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Conectando a MQTT...");
    String clientId = "ESP32_TANQUE_" + String(random(0xffff), HEX);
    
    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("Conectado!");
      mqtt.subscribe(TOPIC_COMANDO);
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(mqtt.state());
      Serial.println(" intentando en 2 segundos...");
      delay(2000);
    }
  }
}

// ==========================================
// 7. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  Wire.begin(SDA_LCD, SCL_LCD);
  lcd.init();
  lcd.backlight();
  
  valvulaServo.attach(SERVO_PIN);
  valvulaServo.write(100); // Arranca abierta
  
  conectarWiFi();
  
  espClient.setCACert(ROOT_CA); 
  
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
  
  lcd.setCursor(0, 0);
  lcd.print("Sistema Listo");
  delay(1000);
  lcd.clear();
}

// ==========================================
// 8. LOOP
// ==========================================
void loop() {
  if (!mqtt.connected()) conectarMQTT();
  mqtt.loop(); 

  // Leer Ultrasónico
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duracion = pulseIn(ECHO_PIN, HIGH, 30000);
  distancia = (duracion == 0) ? -1 : (duracion * 0.034 / 2.0);

  if (distancia > 0 && distancia <= ALTURA_TANQUE_CM) {
    distanciaAgua = ALTURA_TANQUE_CM - distancia;
    porcentajeTanque = (distanciaAgua / ALTURA_TANQUE_CM) * 100;
    if (porcentajeTanque > 100) porcentajeTanque = 100;
    if (porcentajeTanque < 0) porcentajeTanque = 0;
  } else {
    porcentajeTanque = 0;
  }

  // Leer Humedad
  long suma = 0;
  for (int i = 0; i < 10; i++) { suma += analogRead(AGUA_AO); delay(10); }
  int lecturaAgua = suma / 10;
  nivelAgua = map(lecturaAgua, VALOR_SECO, VALOR_MOJADO, 0, 100);
  nivelAgua = constrain(nivelAgua, 0, 100);

  // Mostrar en LCD
  lcd.setCursor(0, 0);
  lcd.print("Tanque: ");
  lcd.print(porcentajeTanque);
  lcd.print("%   ");

  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(nivelAgua);
  lcd.print("% ");
  lcd.print(valvulaAbierta ? "ABR" : "CER");

  // Mostrar en consola para depurar
  Serial.print("Humedad: ");
  Serial.print(nivelAgua);
  Serial.print("% | Crudo: ");
  Serial.print(lecturaAgua);
  Serial.print(" | Tanque: ");
  Serial.print(porcentajeTanque);
  Serial.println("%");

  // 🚨 CIERRE DE EMERGENCIA AL 5% 🚨
  if (nivelAgua >= UMBRAL_CIERRE_HUMEDAD || lecturaAgua < UMBRAL_CRUDO_MOJADO) {
    
    if (valvulaAbierta) {
      valvulaServo.write(0); 
      valvulaAbierta = false;
      Serial.println("¡CIERRE AUTOMÁTICO ACTIVADO!");
    }

    tone(BUZZER_PIN, 1000); delay(150);
    tone(BUZZER_PIN, 1500); delay(150);
    
    alertaEnviada = true;
  } else {
    noTone(BUZZER_PIN); 
    if (nivelAgua < 2) { 
      alertaEnviada = false;
    }
  }

  // Enviar JSON a Angular
  if (millis() - tiempoAnterior > 2000) {
    tiempoAnterior = millis();
    String json = "{";
    json += "\"distancia\":" + String(distancia, 1) + ",";
    json += "\"nivel\":" + String(nivelAgua) + ",";
    json += "\"tanque\":" + String(porcentajeTanque) + ",";
    json += "\"valvula\":\"" + String(valvulaAbierta ? "abierta" : "cerrada") + "\"";
    json += "}";
    mqtt.publish(TOPIC_SENSOR, json.c_str());
  }
}