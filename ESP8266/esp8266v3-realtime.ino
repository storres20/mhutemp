/* 26/04/2025 - This version is for ESP8266 v3 */
/* The issue related with initialization between OLED and ds18b20 temperature sensor has fixed */
/* Now added OLED messages for WiFi events */

#include <ESP8266WiFi.h>
#include <ArduinoWebsockets.h>
using namespace websockets;
#include <WiFiManager.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <time.h>

// **Pin definitions**
#define DHTPIN D4
#define DHTTYPE DHT11
#define ONE_WIRE_BUS D3  // DS18B20 connected here
#define LED_PIN D5
#define RESET_PIN D6

// **EEPROM settings**
#define EEPROM_SIZE 32
#define USERNAME_ADDR 0

// **WebSocket Server**
const char* serverIP = "bio-data-production.up.railway.app";

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

WiFiManager wifiManager;
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire);

char username[16];
WiFiManagerParameter custom_username("username", "Enter username", "", 16);
WebsocketsClient webSocket;

volatile bool resetTriggered = false;
unsigned long previousMillis = 0;
const long interval = 2000;

unsigned long lastReconnectAttempt = 0;
bool wsConnected = false;

unsigned long lastOledCheck = 0;
const long oledCheckInterval = 5000;

unsigned long lastPingTime = 0;
const unsigned long pingInterval = 30000;  // 30s ping keep-alive

// **New: Show messages on OLED**
void showOledMessage(const char* line1, const char* line2 = "", const char* line3 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  if (strlen(line2) > 0) {
    display.setCursor(0, 20);
    display.println(line2);
  }
  if (strlen(line3) > 0) {
    display.setCursor(0, 40);
    display.println(line3);
  }
  display.display();
}

// **WebSocket Events**
void webSocketMessage(WebsocketsMessage message) {
  Serial.printf("WebSocket Message Received: %s\n", message.data().c_str());
}

void webSocketEvent(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("✅ WebSocket Connected");
    wsConnected = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("❌ WebSocket Disconnected");
    wsConnected = false;
  }
}

void ICACHE_RAM_ATTR resetWiFiSettings() {
  resetTriggered = true;
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(RESET_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_PIN), resetWiFiSettings, FALLING);

  Wire.begin(D2, D1);
  display.begin(0x3C, true);
  display.clearDisplay();
  display.display();

  dht.begin();
  sensors.begin();

// Show waiting WiFi message with countdown
if (WiFi.SSID() == "") {
  // No WiFi credentials saved
  showOledMessage("No WiFi saved", "SSID: ESP8266-Setup", "Password: password123");
  delay(2000);
} else {
  // Countdown for WiFi connection attempt
  for (int i = 15; i > 0; i--) {
    char line2[20];
    sprintf(line2, "Connecting in %ds", i);
    showOledMessage("Waiting for WiFi...", line2);
    delay(1000);
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
  }
}

// If not connected, show WiFiManager portal info
if (WiFi.status() != WL_CONNECTED) {
  showOledMessage("WiFiManager Portal", "SSID: ESP8266-Setup", "Password: password123");
  delay(2000);
}

// Start WiFiManager
if (isUsernameStored()) {
  getUsername();
  custom_username.setValue(username, 16); // Rellena campo con username guardado
} else {
  custom_username.setValue("", 16); // Campo vacío si no hay username
}

wifiManager.addParameter(&custom_username);

wifiManager.autoConnect("ESP8266-Setup", "password123");

  if (WiFi.SSID() == "") {
    // No WiFi credentials saved
    showOledMessage("No WiFi saved", "SSID: ESP8266-Setup", "Password: password123");
    delay(2000);
  } else if (WiFi.status() != WL_CONNECTED) {
    // Opened WiFiManager portal
    showOledMessage("WiFiManager Portal", "SSID: ESP8266-Setup", "Password: password123");
    delay(2000);
  }

  Serial.println("✅ Connected to Wi-Fi.");
  Serial.print("📡 IP Address: ");
  Serial.println(WiFi.localIP());

  strcpy(username, custom_username.getValue());
  toLowerCase(username);

  if (strlen(username) > 0) {
    saveUsername(username);
  } else if (isUsernameStored()) {
    getUsername();
  } else {
    strcpy(username, "guest");
  }

  digitalWrite(LED_PIN, HIGH);

  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  // Show message
  showOledMessage("Connecting...");

  waitForNTPTime();

  webSocket.onMessage(webSocketMessage);
  webSocket.onEvent(webSocketEvent);
  webSocket.setInsecure();
  reconnectWebSocket();
}

void loop() {
  if (resetTriggered) {
    Serial.println("🔄 Resetting WiFi...");
    showOledMessage("Resetting WiFi...", "SSID: ESP8266-Setup", "Password: password123");
    wifiManager.resetSettings();
    delay(2000);
    ESP.restart();
  }

  webSocket.poll();

  if (!wsConnected && millis() - lastReconnectAttempt > 10000) {
    Serial.println("🔁 Attempting WebSocket reconnect...");
    reconnectWebSocket();
    lastReconnectAttempt = millis();
  }

  if (millis() - lastPingTime > pingInterval && wsConnected) {
    Serial.println("📶 Sending ping to keep connection alive");
    webSocket.ping();
    lastPingTime = millis();
  }

  if (millis() - lastOledCheck >= oledCheckInterval) {
    lastOledCheck = millis();
    display.display();  // Test write
    if (display.getWriteError()) {
      Serial.println("🖥️ OLED might be off, reinitializing...");
      Wire.begin(D2, D1);
      display.begin(0x3C, true);
      display.clearDisplay();
      display.display();
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    sensors.requestTemperatures();
    float dsTemperature = sensors.getTempCByIndex(0);
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature)) {
      Serial.println("❌ Error: DHT Temperature reading failed!");
      temperature = 0;
    }
    if (isnan(humidity)) {
      Serial.println("❌ Error: DHT Humidity reading failed!");
      humidity = 0;
    }
    if (dsTemperature == -127.00) {
      Serial.println("❌ Error: DS18B20 sensor not detected!");
      dsTemperature = 0;
    }

    displayDataOnOLED(temperature, humidity, dsTemperature);

    String datetime = getDateTimeString();
    String jsonPayload = "{\"username\":\"" + String(username) + "\", "
                         "\"temperature\":" + String(temperature) + ", "
                         "\"humidity\":" + String(humidity) + ", "
                         "\"dsTemperature\":" + String(dsTemperature) + ", "
                         "\"datetime\":\"" + datetime + "\"}";

    if (wsConnected) {
      webSocket.send(jsonPayload);
    }

    Serial.printf("🌡️ Temp: %.2f°C, Humidity: %.2f%%, DS18B20: %.2f°C\n", temperature, humidity, dsTemperature);
  }
}

void reconnectWebSocket() {
  String wsUrl = "wss://" + String(serverIP) + "/";
  Serial.print("🔗 Connecting to WebSocket: ");
  Serial.println(wsUrl);
  webSocket.connect(wsUrl);
}

void toLowerCase(char* str) {
  for (int i = 0; str[i]; i++) {
    str[i] = tolower(str[i]);
  }
}

bool isUsernameStored() {
  return EEPROM.read(USERNAME_ADDR) != 0xFF;
}

void getUsername() {
  for (int i = 0; i < 15; i++) {
    char c = EEPROM.read(USERNAME_ADDR + i);
    if (c == '\0' || c == 0xFF) break;
    username[i] = c;
  }
  username[15] = '\0';
}

void saveUsername(char* input) {
  for (int i = 0; i < 15; i++) {
    EEPROM.write(USERNAME_ADDR + i, input[i]);
  }
  EEPROM.write(USERNAME_ADDR + 15, '\0');
  EEPROM.commit();
}

void waitForNTPTime() {
  while (time(nullptr) <= 100000) {
    Serial.println("⏳ Waiting for NTP time sync...");
    delay(500);
  }
  Serial.println("⏰ NTP time synchronized.");
}

String getDateTimeString() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", timeinfo);
  return String(buffer);
}

void displayDataOnOLED(float tempDHT, float humDHT, float tempDS) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.print("Welcome ");
  display.print(username);

  display.setCursor(0, 20);
  display.print("Temp: ");
  display.print(tempDHT);
  display.print(" C");

  display.setCursor(0, 30);
  display.print("Humidity: ");
  display.print(humDHT);
  display.print(" %");

  display.setCursor(0, 40);
  display.print("DS18B20: ");
  display.print(tempDS);
  display.print(" C");

  display.setCursor(0, 55);
  display.print("WS: ");
  display.print(wsConnected ? "Conn" : "Fail");

  display.display();
}
