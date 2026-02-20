#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);   // SDA, SCL

  if (!bmp.begin()) {
    Serial.println("BMP180 not found");
    while (1);
  }

  Serial.println("BMP180 initialized");
}

void loop() {

  Serial.print("Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" °C");

  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");

  Serial.print("Altitude = ");
  Serial.print(bmp.readAltitude());
  Serial.println(" meters");

  Serial.println("--------------------");

  delay(2000);
}
