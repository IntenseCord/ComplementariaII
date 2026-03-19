#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

//-------------------------Configuración------------------//
const char* ssid = "MSOSARES.R";
const char* password = "M433#sosa";
//--------------------------------------------------------//
const char* apiKey = "12I8DTICBK8BYVV5";
const char* server = "https://api.thingspeak.com/update";
//--------------------------------------------------------//

#define DHTPIN 4
#define DHTTYPE DHT22

const int ldrPin = 34;

DHT dht(DHTPIN, DHTTYPE);

int minLuz = 4095;  // Valor mínimo (más oscuro)
int maxLuz = 0;     // Valor máximo (más brillante)

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  dht.begin();

  //------------------------WiFI----------------------------//
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando al WiFi");
  
  int intentos = 0;

  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED){
    Serial.print("Red Wifi: ");
    Serial.println(WiFi.SSID());
    Serial.print("Ip asignada: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("\n No hubo conexión");
  }

  // ---------------- CALIBRACIÓN LDR ---------------------//
  Serial.println("Calibrando sensor de luz...");
  Serial.println("Expón el sensor a diferentes condiciones de luz");
  
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    int valor = analogRead(ldrPin);

    
    if (valor > minLuz) minLuz = valor;
    if (valor < maxLuz) maxLuz = valor;

    delay(10);
  }
  
  Serial.println("Calibración completada!");
  Serial.print("Rango LDR - Mínimo: ");
  Serial.print(minLuz);
  Serial.print(" | Máximo: ");
  Serial.println(maxLuz);
  Serial.println("----------------------------------------");
}

//------------------------------------------------------------//

void loop() {
  int valorLdr = analogRead(ldrPin);
  
  
  int porcentajeLuz = map(valorLdr, minLuz, maxLuz, 0, 100);
  porcentajeLuz = constrain(porcentajeLuz, 0, 100);
  
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();
  
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error leyendo DHT22");
    delay(1000);
    return;
  }
  
  Serial.print("Luz: ");
  Serial.print(porcentajeLuz);
  Serial.print("% | Temp: ");
  Serial.print(temperatura, 2);
  Serial.print(" C | Humedad: ");
  Serial.println(humedad, 2);

  // ---------------- ENVÍO A THINGSPEAK ----------------//

  // No se necesito incluir la libreria de Thingspeak, 
  //por que ya estamos haciendo una petición al servidor(HTTP GET)
  
  if (WiFi.status() == WL_CONNECTED){
    HTTPClient http;

    String url = String(server) + "?api_key=" + apiKey;
    url += "&field1=" + String(temperatura);
    url += "&field2=" + String(humedad);
    url += "&field3=" + String(porcentajeLuz);

    Serial.println("Enviando info a ThingSpeak.....");
    http.begin(url);

    int httpCode = http.GET();

    if (httpCode > 0){
      Serial.println("Envio correcto");
    } else {
      Serial.println("Error en el envio");
    }

    http.end();

  } else {
    Serial.println("WiFi desconectado....");
    WiFi.reconnect();
  }

  delay(20000); // mínimo 15s para ThingSpeak
}