#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

//---------------- WIFI ----------------//
const char* ssid = "CANO SOSA";
const char* password = "39215260";

//---------------- MQTT ----------------//
const char* mqtt_server = "192.168.1.16"; 
WiFiClient espClient;
PubSubClient client(espClient);

//---------------- SENSORES ----------------//
#define DHTPIN 4
#define DHTTYPE DHT22
const int ldrPin = 34;

DHT dht(DHTPIN, DHTTYPE);

//---------------- VARIABLES ----------------//
int minLuz = 4095;
int maxLuz = 0;

float temperatura = 0;
float humedad = 0;
int porcentajeLuz = 0;

//---------------- TIMERS ----------------//
unsigned long tSensores = 0;
unsigned long tEnvio = 0;
unsigned long tLED = 0;

//---------------- ESTADOS ----------------//
enum Estado {
  NORMAL,
  ALERTA_TEMP,
  ALERTA_LUZ,
  ALERTA_DOBLE
};

Estado estadoActual = NORMAL;

//---------------- HISTÉRESIS ----------------//
float tempAlta = 30;
float tempBaja = 28;

//---------------- WIFI ----------------//
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.println(WiFi.localIP());
}

//---------------- MQTT ----------------//
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT...");

    if (client.connect("ESP32Client")) {
      Serial.println("Conectado ✔");
    } else {
      Serial.print("Error, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando...");
      delay(2000);
    }
  }
}

//---------------- SETUP ----------------//
void setup() {
  Serial.begin(115200);
  dht.begin();
  analogReadResolution(12);

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // -------- CALIBRACIÓN --------
  Serial.println("Calibrando LDR...");
  unsigned long start = millis();

  while (millis() - start < 5000) {
    int valor = analogRead(ldrPin);

    if (valor > minLuz) minLuz = valor; 
    if (valor < maxLuz) maxLuz = valor;
  }

  Serial.println("Calibración lista");
}

//---------------- LECTURA SENSORES ----------------//
void leerSensores() {
  int valorLdr = analogRead(ldrPin);

  porcentajeLuz = map(valorLdr, minLuz, maxLuz, 0, 100);
  porcentajeLuz = constrain(porcentajeLuz, 0, 100);

  temperatura = dht.readTemperature();
  humedad = dht.readHumidity();

  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error DHT");
    return;
  }

  Serial.print("Temp: "); Serial.print(temperatura);
  Serial.print(" | Hum: "); Serial.print(humedad);
  Serial.print(" | Luz: "); Serial.println(porcentajeLuz);
}

//---------------- MAQUINA DE ESTADOS ----------------//
void actualizarEstado() {

  switch (estadoActual) {

    case NORMAL:
      if (temperatura >= tempAlta && porcentajeLuz <= 20)
        estadoActual = ALERTA_DOBLE;
      else if (temperatura >= tempAlta)
        estadoActual = ALERTA_TEMP;
      else if (porcentajeLuz <= 20)
        estadoActual = ALERTA_LUZ;
      break;

    case ALERTA_TEMP:
      if (temperatura <= tempBaja)
        estadoActual = NORMAL;
      break;

    case ALERTA_LUZ:
      if (porcentajeLuz > 30)
        estadoActual = NORMAL;
      break;

    case ALERTA_DOBLE:
      if (temperatura <= tempBaja && porcentajeLuz > 30)
        estadoActual = NORMAL;
      break;
  }
}

//---------------- ENVÍO MQTT ----------------//
void enviarDatos() {

  String payload = "{";
  payload += "\"temperatura\": " + String(temperatura) + ",";
  payload += "\"humedad\": " + String(humedad) + ",";
  payload += "\"luz\": " + String(porcentajeLuz);
  payload += "}";

  client.publish("sensor/datos", payload.c_str());

  Serial.println("Datos enviados MQTT:");
  Serial.println(payload);
}

//---------------- LOOP PRINCIPAL ----------------//
void loop() {

  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long ahora = millis();

  // -------- TAREA 1: Sensores (2s) --------
  if (ahora - tSensores >= 2000) {
    tSensores = ahora;
    leerSensores();
    actualizarEstado();
  }

  // -------- TAREA 2: Envío (20s) --------
  if (ahora - tEnvio >= 20000) {
    tEnvio = ahora;
    enviarDatos();
  }

  // -------- TAREA 3: LEDs (100ms) --------
  if (ahora - tLED >= 100) {
    tLED = ahora;

    Serial.print("Estado: ");
    Serial.println(estadoActual);
  }
}