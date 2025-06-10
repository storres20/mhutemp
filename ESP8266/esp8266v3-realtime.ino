/* MHUTEMP - ESP8266 v3 WebSocket Sensor
 * 09/06/2025 - Version with corrected calibration, OLED, and Serial monitor logging
 */

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

#define DHTPIN D4
#define DHTTYPE DHT22
#define ONE_WIRE_BUS D3
#define LED_PIN D5
#define RESET_PIN D6

#define EEPROM_SIZE 32
#define USERNAME_ADDR 0

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
const unsigned long pingInterval = 30000;

void showOledMessage(const char* line1, const char* line2 = "", const char* line3 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  if (strlen(line2) > 0) display.setCursor(0, 20), display.println(line2);
  if (strlen(line3) > 0) display.setCursor(0, 40), display.println(line3);
  display.display();
}

void displayDataOnOLED(float tempCorr, float humCorr, float dsCorr) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.print("User: ");
  display.print(username);

  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timeBuf[6];
  strftime(timeBuf, sizeof(timeBuf), "%H:%M", timeinfo);
  display.setCursor(128 - 6 * strlen(timeBuf), 0);
  display.print(timeBuf);

  display.setCursor(0, 15);
  display.printf("Temp.IN: %.2f C", tempCorr);

  display.setCursor(0, 30);
  display.printf("Hum.IN: %.2f %%", humCorr);

  display.setCursor(0, 45);
  display.printf("Temp.OUT: %.2f C", dsCorr);

  display.display();
}

void webSocketMessage(WebsocketsMessage message) {
  Serial.printf("%s - 🔔 WebSocket Message: %s\n", getDateTimeStringUTC().c_str(), message.data().c_str());
}

void webSocketEvent(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.printf("%s - ✅ WebSocket Connected\n", getDateTimeStringUTC().c_str());
    wsConnected = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.printf("%s - ❌ WebSocket Disconnected\n", getDateTimeStringUTC().c_str());
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

  if (WiFi.SSID() == "") {
    showOledMessage("No WiFi saved", "SSID: ESP8266-Setup", "Password: password123");
    delay(2000);
  } else {
    for (int i = 15; i > 0; i--) {
      char line2[20];
      sprintf(line2, "Connecting in %ds", i);
      showOledMessage("Waiting for WiFi...", line2);
      delay(1000);
      if (WiFi.status() == WL_CONNECTED) break;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    showOledMessage("WiFiManager Portal", "SSID: ESP8266-Setup", "Password: password123");
    delay(2000);
  }

  if (isUsernameStored()) getUsername(), custom_username.setValue(username, 16);
  else custom_username.setValue("", 16);

  wifiManager.addParameter(&custom_username);
  wifiManager.autoConnect("ESP8266-Setup", "password123");

  strcpy(username, custom_username.getValue());
  toLowerCase(username);
  if (strlen(username) > 0) saveUsername(username);
  else if (isUsernameStored()) getUsername();
  else strcpy(username, "guest");

  digitalWrite(LED_PIN, HIGH);
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  waitForNTPTime();

  showOledMessage("Connecting...");

  webSocket.onMessage(webSocketMessage);
  webSocket.onEvent(webSocketEvent);
  webSocket.setInsecure();
  reconnectWebSocket();
}

void loop() {
  if (resetTriggered) {
    Serial.printf("%s - 🔄 Resetting WiFi...\n", getDateTimeStringUTC().c_str());
    showOledMessage("Resetting WiFi...", "SSID: ESP8266-Setup", "Password: password123");
    wifiManager.resetSettings();
    delay(2000);
    ESP.restart();
  }

  webSocket.poll();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("%s - ⚠️  WiFi desconectado. Iniciando reconexión...\n", getDateTimeStringUTC().c_str());
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 5) {
      WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
      Serial.printf("%s - 🔁 Reintentando WiFi (%d/5)...\n", getDateTimeStringUTC().c_str(), retries + 1);
      delay(5000);
      retries++;
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.printf("%s - ❌ No se pudo reconectar. Reiniciando...\n", getDateTimeStringUTC().c_str());
      delay(2000);
      ESP.restart();
    } else {
      Serial.printf("%s - ✅ Reconexión WiFi exitosa.\n", getDateTimeStringUTC().c_str());
    }
  }

  if (!wsConnected && millis() - lastReconnectAttempt > 10000) {
    Serial.printf("%s - 🔌 Intentando reconexión WebSocket...\n", getDateTimeStringUTC().c_str());
    reconnectWebSocket();
    lastReconnectAttempt = millis();
  }

  if (millis() - lastPingTime > pingInterval && wsConnected) {
    Serial.printf("%s - 📶 Enviando ping...\n", getDateTimeStringUTC().c_str());
    webSocket.ping();
    lastPingTime = millis();
  }

  if (millis() - lastOledCheck >= oledCheckInterval) {
    lastOledCheck = millis();
    display.display();
    if (display.getWriteError()) {
      Serial.printf("%s - 🖥️ OLED error, reinicializando...\n", getDateTimeStringUTC().c_str());
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
    float dsRaw = sensors.getTempCByIndex(0);
    float tRaw = dht.readTemperature();
    float hRaw = dht.readHumidity();

    if (isnan(tRaw)) Serial.println("❌ Error: Temp.IN"), tRaw = 0;
    if (isnan(hRaw)) Serial.println("❌ Error: Hum.IN"), hRaw = 0;
    if (dsRaw == -127.00) Serial.println("❌ Error: Temp.OUT"), dsRaw = 0;

    float correctedDSTemp = dsRaw - 0.1;
    float correctedTemp = 0.0019 * tRaw * tRaw + 0.9144 * tRaw + 0.1345;
    float correctedHum  = 1.230 * hRaw - 20.55;

    displayDataOnOLED(correctedTemp, correctedHum, correctedDSTemp);

    String datetime = getDateTimeStringUTC();
    String jsonPayload = "{\"username\":\"" + String(username) + "\", "
                         "\"temperature\":" + String(correctedTemp) + ", "
                         "\"humidity\":" + String(correctedHum) + ", "
                         "\"dsTemperature\":" + String(correctedDSTemp) + ", "
                         "\"datetime\":\"" + datetime + "\"}";

    if (wsConnected) webSocket.send(jsonPayload);

    Serial.printf("%s - 📤 Enviado | Temp.IN: %.2f°C | Hum.IN: %.2f%% | Temp.OUT: %.2f°C\n",
      datetime.c_str(), correctedTemp, correctedHum, correctedDSTemp);
  }
}

void reconnectWebSocket() {
  String wsUrl = "wss://" + String(serverIP) + "/";
  Serial.printf("%s - 🔗 Conectando WebSocket: %s\n", getDateTimeStringUTC().c_str(), wsUrl.c_str());
  webSocket.connect(wsUrl);
}

String getDateTimeStringUTC() {
  time_t now = time(nullptr);
  struct tm* timeinfo = gmtime(&now);
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", timeinfo);
  return String(buffer) + "Z";
}

void toLowerCase(char* str) {
  for (int i = 0; str[i]; i++) str[i] = tolower(str[i]);
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
  for (int i = 0; i < 15; i++) EEPROM.write(USERNAME_ADDR + i, input[i]);
  EEPROM.write(USERNAME_ADDR + 15, '\0');
  EEPROM.commit();
}

void waitForNTPTime() {
  while (time(nullptr) <= 100000) {
    Serial.println("⏳ Esperando sincronización NTP...");
    delay(500);
  }
  Serial.println("⏰ Tiempo NTP sincronizado.");
}
