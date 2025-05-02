/*
Note: 30/04/2025
This is for ESP8266 nodemcu with 0,93" oled SSD1306
*/

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pines
#define DHTPIN D4
#define DHTTYPE DHT11
#define ONE_WIRE_BUS D3
#define LED_PIN D5

// Inicializar sensores
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);  // SDA=D2, SCL=D1

void setup() {
  Serial.begin(115200);
  dht.begin();
  ds18b20.begin();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED encendido

  Wire.begin(D2, D1);  // Inicia I2C

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Error al iniciar pantalla OLED"));
    while (true);  // Detener ejecución
  }

  display.clearDisplay();
  display.display();
}

void loop() {
  // Lectura sensores
  float tempDHT = dht.readTemperature();
  float humDHT = dht.readHumidity();

  ds18b20.requestTemperatures();
  float tempDS = ds18b20.getTempCByIndex(0);

  // Mostrar en OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.setTextSize(2);
  display.print("T1:");
  display.print(tempDHT, 1);
  display.print("C");

  display.setCursor(0, 20);
  display.setTextSize(2);
  display.print("H1:");
  display.print(humDHT, 0);
  display.print("%");

  display.setCursor(0, 40);
  display.setTextSize(2);
  display.print("DT2:");
  display.print(tempDS, 1);
  display.print("C");

  display.display();

  // Monitor serial
  Serial.print("T1: "); Serial.print(tempDHT); Serial.print(" °C, ");
  Serial.print("H1: "); Serial.print(humDHT); Serial.print(" %, ");
  Serial.print("DT2: "); Serial.print(tempDS); Serial.println(" °C");

  delay(2000);  // Actualizar cada 2 segundos
}
