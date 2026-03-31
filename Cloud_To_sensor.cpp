#include <ESP8266WiFi.h>
#include "DHT.h"

#define DHTPIN D4
#define DHTTYPE DHT11

const char* ssid = "gaurab";
const char* password = "gaurab123";

String apiKey = "R079BILCHH333PVL";
const char* server = "api.thingspeak.com";

WiFiClient client;
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT Error!");
    delay(2000);
    return;
  }

  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print(" °C  Humidity: ");
  Serial.println(hum);

  if (client.connect(server, 80)) {
    String url = "/update?api_key=" + apiKey +
                 "&field1=" + String(temp) +
                 "&field2=" + String(hum);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("Data sent to ThingSpeak!");
  } else {
    Serial.println("Connection to ThingSpeak failed!");
  }

  client.stop();

  delay(15000); // ThingSpeak requires 15 sec delay
}
