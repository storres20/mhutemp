/* 09/06/2025 - Standalone ESP8266 v3 with DHT22 + DS18B20 */
/* Calibration formulas from certificate TC-14504-2025 applied */

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// === Pin definitions ===
#define DHTPIN D4
#define DHTTYPE DHT22
#define ONE_WIRE_BUS D3

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

unsigned long previousMillis = 0;
const long interval = 2000;

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);
  display.begin(0x3C, true);
  display.clearDisplay();
  display.display();

  dht.begin();
  sensors.begin();

  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 20);
  display.println("Connecting...");
  display.display();
  delay(3000);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    sensors.requestTemperatures();
    float raw_dsTemp = sensors.getTempCByIndex(0);
    float raw_dhtTemp = dht.readTemperature();
    float raw_dhtHum = dht.readHumidity();

    // Corrección DS18B20
    float dsTemp_corr = (raw_dsTemp == -127.00) ? 0 : raw_dsTemp - 0.1;

    // Corrección temperatura DHT22 (cuadrática)
    float dhtTemp_corr = isnan(raw_dhtTemp) ? 0 :
      (0.0019 * raw_dhtTemp * raw_dhtTemp) + (0.9144 * raw_dhtTemp) + 0.1345;

    // Corrección humedad DHT22 (lineal)
    float dhtHum_corr = isnan(raw_dhtHum) ? 0 : (1.230 * raw_dhtHum) - 20.55;

    if (isnan(raw_dhtTemp)) Serial.println("Error reading DHT22 temperature!");
    if (isnan(raw_dhtHum)) Serial.println("Error reading DHT22 humidity!");
    if (raw_dsTemp == -127.00) Serial.println("Error reading DS18B20!");

    // Mostrar en pantalla
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    display.setCursor(0, 0);
    display.println("Standalone Mode");

    display.setCursor(0, 15);
    display.print("Temp(IN): ");
    display.print(dhtTemp_corr, 2);
    display.print(" C");

    display.setCursor(0, 25);
    display.print("Hum(IN): ");
    display.print(dhtHum_corr, 2);
    display.print(" %");

    display.setCursor(0, 35);
    display.print("Temp(OUT): ");
    display.print(dsTemp_corr, 2);
    display.print(" C");

    display.display();

    // Serial debug
    Serial.printf("T.IN: %.2f°C, H.IN: %.2f%%, T.OUT: %.2f°C\n", dhtTemp_corr, dhtHum_corr, dsTemp_corr);
  }
}
