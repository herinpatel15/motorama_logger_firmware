#include "arduino_secrets.h"

#include <WiFiManager.h>
#include <strings_en.h>
#include <wm_consts_en.h>
#include <wm_strings_en.h>
#include <wm_strings_es.h>

#include "thingProperties.h"
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include "Pzem.h"
#include "Temperature_humidity.h"
#include "I2C_Communication.h"
#include "Rtc.h"
#include "Sd_card.h"

const char* AP_SSID = "EnergyMonitor_AP";
const char* AP_PASSWORD = "12345678";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer webServer(80);

String ssid;
String password;

unsigned long lastTime_log = 0;
unsigned long lastOperationalCheck = 0;
unsigned long operationalSeconds = 0;
TaskHandle_t rpmTaskHandle = NULL;
float temp_cfm = 0;
float inlet_area = 96.87;

// HTML WiFi Config Page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Energy Monitor WiFi Setup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; background: #f0f0f0; padding: 20px; }
    .container { max-width: 400px; margin: auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
    h1 { text-align: center; }
    .input-group { margin-bottom: 20px; }
    label { display: block; margin-bottom: 5px; }
    input, button { width: 100%; padding: 10px; border-radius: 4px; }
    button { background: #4CAF50; color: white; border: none; }
    button:hover { background: #45a049; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Energy Monitor Setup</h1>
    <form action="/save" method="POST">
      <div class="input-group">
        <label for="ssid">WiFi Network Name:</label>
        <input type="text" id="ssid" name="ssid" required>
      </div>
      <div class="input-group">
        <label for="password">WiFi Password:</label>
        <input type="password" id="password" name="password" required>
      </div>
      <button type="submit">Connect</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

void handleRoot() {
  webServer.send(200, "text/html", index_html);
}

void handleSave() {
  if (webServer.method() != HTTP_POST) {
    webServer.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  ssid = webServer.arg("ssid");
  password = webServer.arg("password");

  webServer.send(200, "text/html", "Credentials saved. Connecting...");

  delay(2000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts++ < 20) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    saveCredentials();
  } else {
    Serial.println("\nConnection failed. Restarting AP mode.");
    startAPMode();
  }
}

void saveCredentials() {
  File file = SPIFFS.open("/wifi_cred.txt", "w");
  if (!file) {
    Serial.println("Failed to write credentials");
    return;
  }
  file.println(ssid);
  file.println(password);
  file.close();
}

void loadCredentials() {
  File file = SPIFFS.open("/wifi_cred.txt", "r");
  if (!file) {
    Serial.println("No saved credentials");
    return;
  }
  ssid = file.readStringUntil('\n');
  password = file.readStringUntil('\n');
  ssid.trim();
  password.trim();
  file.close();
}

void saveArea(float area) {
  File file = SPIFFS.open("/Area.txt", "w");
  if (file) {
    file.println(area);
    file.close();
  }
}

void loadarea() {
  File file = SPIFFS.open("/Area.txt", "r");
  if (file) {
    String areaStr = file.readStringUntil('\n');
    area = areaStr.toFloat();
    inlet_area = area;
    file.close();
  }
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  dnsServer.start(DNS_PORT, "*", apIP);

  webServer.on("/", handleRoot);
  webServer.on("/save", handleSave);
  webServer.begin();

  Serial.println("AP mode started");
  Serial.print("AP IP: ");
  Serial.println(apIP);
}

void rpmCalculationTask(void* pvParameters) {
  const TickType_t delayTime = 1000 / portTICK_PERIOD_MS;
  while (true) {
    readSensorData();
    vTaskDelay(delayTime);
  }
}

void setup() {
  Serial.begin(115200);
  setupUART();

  Wire.begin();

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS failed");
    return;
  }

  loadCredentials();
  loadarea();

  if (ssid.length() > 0 && password.length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts++ < 20) {
      delay(500);
      Serial.print(".");
    }
  }

  if (WiFi.status() != WL_CONNECTED) startAPMode();

  setupRTC();
  setupSD();
  createOrOpenFile();
  AH10_init();

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  xTaskCreatePinnedToCore(
    rpmCalculationTask,
    "RPMTask",
    10000,
    NULL,
    1,
    &rpmTaskHandle,
    0);

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  DateTime nowStart = rtc.now();
  Serial.printf("START TIME: %02d:%02d:%02d %02d/%02d/%04d\n",
                nowStart.hour(), nowStart.minute(), nowStart.second(),
                nowStart.day(), nowStart.month(), nowStart.year());
  Serial.printf("START ENERGY: %d\n", totalEnergy);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    dnsServer.processNextRequest();
    webServer.handleClient();
    return;
  }

  DateTime now = rtc.now();
  ArduinoCloud.update();

  readSensorData();  // uart data

  rpm = getReceivedRPM();
  windspeed = getReceivedWindSpeed();

  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  pF = pzem.pf();

  temp_cfm = windspeed * 51;
  air_Flow_Volume = windspeed * inlet_area;

  readAHT10(&temperature, &humidity);

  // Serial.printf("RPM: %d\tWindSpeed: %.2f m/s\tVoltage: %.2f V\tCurrent: %.2f A\tPower: %.2f W\tEnergy: %.2f Wh\tPF: %.2f\tTemperature: %.2f °C\tHumidity: %.2f %%\tCFM: %.2f\n",
  //             rpm, windspeed, voltage, current, power, energy, pF, temperature, humidity, air_Flow_Volume);


  // Serial.printf("WindSpeed: %.2f\tCFM: %.2f\n", windspeed, air_Flow_Volume);

  if (millis() - lastOperationalCheck >= 1000) {
    if (current > 0.1) {
      operationalSeconds++;
      if (operationalSeconds >= 60) {
        operatioanl_time++;
        operationalSeconds = 0;
      }
    }
    lastOperationalCheck = millis();
  }

  if (millis() - lastLogTime >= 200) {
    lastLogTime = millis();
  }

  if ((now.minute() == 59) && now.second() < 3) {
    delay(1000);
    pzem.resetEnergy();
    Serial.printf("END ENERGY: %d\n", totalEnergy);
    totalEnergy = 0;
    operatioanl_time = 0;
    // DateTime nowStart = rtc.now();
    Serial.printf("END TIME: %02d:%02d:%02d %02d/%02d/%04d\n",
                  now.hour(), now.minute(), now.second(),
                  now.day(), now.month(), now.year());
    createOrOpenFile();
  }

  // if (now.hour() == 23 && now.minute() == 59 && now.second() >= 57) {
  //   delay(1000);
  //   pzem.resetEnergy();
  //   totalEnergy = 0;
  //   operatioanl_time = 0;
  //   createOrOpenFile();
  // }

  // Serial.printf("Data send :)");
}

void onAreaChange() {
  inlet_area = area;
  Serial.print("Updated area: ");
  Serial.println(inlet_area);
  saveArea(inlet_area);
}
