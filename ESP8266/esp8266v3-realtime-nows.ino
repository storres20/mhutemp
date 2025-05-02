/* 26/04/2025 - Version Standalone Sensor */
/* No WiFi - No WebSocket - Only OLED display with DHT11 + DS18B20 */

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// **Pin definitions**
#define DHTPIN D4
#define DHTTYPE DHT11
#define ONE_WIRE_BUS D3  // DS18B20 connected here

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

unsigned long previousMillis = 0;
const long interval = 2000; // Update every 2 seconds

void setup() {
  Serial.begin(115200);

  Wire.begin(D2, D1);
  display.begin(0x3C, true);
  display.clearDisplay();
  display.display();

  dht.begin();
  sensors.begin();

  // Show "Connecting..." at startup
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 20);
  display.println("Connecting...");
  display.display();
  delay(3000);  // Show for 3 seconds
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    sensors.requestTemperatures();
    float dsTemperature = sensors.getTempCByIndex(0);
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature)) {
      Serial.println("Error reading DHT11 temperature!");
      temperature = 0;
    }
    if (isnan(humidity)) {
      Serial.println("Error reading DHT11 humidity!");
      humidity = 0;
    }
    if (dsTemperature == -127.00) {
      Serial.println("Error reading DS18B20!");
      dsTemperature = 0;
    }

    // Show measurements on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    display.setCursor(0, 0);
    display.println("Standalone Mode");

    display.setCursor(0, 15);
    display.print("Temp: ");
    display.print(temperature);
    display.print(" C");

    display.setCursor(0, 25);
    display.print("Humidity: ");
    display.print(humidity);
    display.print(" %");

    display.setCursor(0, 35);
    display.print("DS18B20: ");
    display.print(dsTemperature);
    display.print(" C");

    display.display();

    // Also print to Serial Monitor
    Serial.printf("Temp: %.2f°C, Humidity: %.2f%%, DS18B20: %.2f°C\n", temperature, humidity, dsTemperature);
  }
}
