#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EmonLib.h>
#include <DHTesp.h>

// Server URL (NO PORT)
const char* serverURL = "http://3.108.238.200/send";

// OLED config (SSD1306 128x32 I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Sensor objects
EnergyMonitor emon1;
DHTesp dht;

// Measurements
float Vrms = 0;
float Irms = 0;
float watt = 0;
float tempC = 0;
float humidity = 0;

// Advanced values
float current_Vrms = 0;
float current_Irms = 0;
float current_realP = 0;
float current_appP = 0;
float current_pf = 1.0;

// Pin definitions
#define SDA_PIN 22
#define SCL_PIN 23
#define DHTPIN 33
#define VOLTAGE_PIN 34
#define CURRENT_PIN 35

// Warm-up configuration
#define WARMUP_READINGS 4
#define WARMUP_DELAY_MS 500
bool isWarmedUp = false;
int warmupCounter = 0;

// WiFi Manager
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

const char* apSSID = "EnergyMonitor-Setup";
const char* apPassword = "12345678";
bool isAPMode = false;
bool shouldRestart = false;

String savedSSID = "";
String savedPassword = "";

// HTML Pages
const char HTML_HEAD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta charset="UTF-8">
  <title>Energy Monitor Setup</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 500px;
      width: 100%;
      padding: 40px 30px;
      animation: slideUp 0.5s ease;
    }
    @keyframes slideUp {
      from { opacity: 0; transform: translateY(30px); }
      to { opacity: 1; transform: translateY(0); }
    }
    h1 {
      color: #667eea;
      font-size: 28px;
      margin-bottom: 10px;
      text-align: center;
    }
    .subtitle {
      color: #666;
      text-align: center;
      margin-bottom: 30px;
      font-size: 14px;
    }
    .status {
      background: #f0f4ff;
      border-left: 4px solid #667eea;
      padding: 15px;
      border-radius: 8px;
      margin-bottom: 25px;
    }
    .status-label {
      font-size: 12px;
      color: #666;
      text-transform: uppercase;
      letter-spacing: 1px;
      margin-bottom: 5px;
    }
    .status-value {
      font-size: 16px;
      color: #333;
      font-weight: 600;
    }
    .network-list {
      margin-bottom: 25px;
    }
    .network-item {
      background: #f8f9fa;
      padding: 15px;
      margin-bottom: 10px;
      border-radius: 10px;
      cursor: pointer;
      transition: all 0.3s;
      border: 2px solid transparent;
    }
    .network-item:hover {
      background: #e9ecef;
      border-color: #667eea;
      transform: translateX(5px);
    }
    .network-name {
      font-weight: 600;
      color: #333;
      margin-bottom: 5px;
    }
    .network-signal {
      font-size: 12px;
      color: #666;
    }
    .form-group {
      margin-bottom: 20px;
    }
    label {
      display: block;
      margin-bottom: 8px;
      color: #333;
      font-weight: 500;
      font-size: 14px;
    }
    input[type="text"], input[type="password"] {
      width: 100%;
      padding: 12px 15px;
      border: 2px solid #e0e0e0;
      border-radius: 10px;
      font-size: 16px;
      transition: all 0.3s;
    }
    input:focus {
      outline: none;
      border-color: #667eea;
      box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
    }
    .btn {
      width: 100%;
      padding: 15px;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s;
      margin-bottom: 10px;
    }
    .btn-primary {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    .btn-primary:hover {
      transform: translateY(-2px);
      box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
    }
    .btn-danger {
      background: #ff4757;
      color: white;
    }
    .btn-danger:hover {
      background: #e84118;
      transform: translateY(-2px);
    }
    .btn-secondary {
      background: #f1f3f5;
      color: #333;
    }
    .btn-secondary:hover {
      background: #e9ecef;
    }
    .message {
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 20px;
      text-align: center;
    }
    .success { background: #d4edda; color: #155724; }
    .error { background: #f8d7da; color: #721c24; }
    .loading {
      display: inline-block;
      width: 20px;
      height: 20px;
      border: 3px solid rgba(255,255,255,.3);
      border-radius: 50%;
      border-top-color: white;
      animation: spin 1s ease-in-out infinite;
    }
    @keyframes spin {
      to { transform: rotate(360deg); }
    }
  </style>
</head>
<body>
<div class="container">
)rawliteral";

const char HTML_FOOT[] PROGMEM = R"rawliteral(
</div>
</body>
</html>
)rawliteral";

void loadCredentials() {
  preferences.begin("wifi", false);
  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("password", "");
  preferences.end();
}

void saveCredentials(String ssid, String password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}

void clearCredentials() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
  savedSSID = "";
  savedPassword = "";
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  isAPMode = true;
  
  displayMsg("Setup Mode", WiFi.softAPIP().toString().c_str());
  
  Serial.println("\n=== AP Mode Started ===");
  Serial.print("SSID: ");
  Serial.println(apSSID);
  Serial.print("Password: ");
  Serial.println(apPassword);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
}

void handleRoot() {
  String html = FPSTR(HTML_HEAD);
  
  html += "<h1>⚡ Energy Monitor</h1>";
  html += "<div class='subtitle'>WiFi Configuration</div>";
  
  if (savedSSID != "") {
    html += "<div class='status'>";
    html += "<div class='status-label'>Currently Saved</div>";
    html += "<div class='status-value'>" + savedSSID + "</div>";
    html += "</div>";
  }
  
  html += "<h3 style='margin-bottom: 15px; color: #333;'>Available Networks</h3>";
  html += "<div class='network-list' id='networks'>";
  
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    String signal = rssi > -50 ? "Excellent" : rssi > -60 ? "Good" : rssi > -70 ? "Fair" : "Weak";
    
    html += "<div class='network-item' onclick='selectNetwork(\"" + ssid + "\")'>";
    html += "<div class='network-name'>📶 " + ssid + "</div>";
    html += "<div class='network-signal'>" + signal + " (" + String(rssi) + " dBm)</div>";
    html += "</div>";
  }
  
  html += "</div>";
  
  html += "<form action='/connect' method='POST'>";
  html += "<div class='form-group'>";
  html += "<label>WiFi Network</label>";
  html += "<input type='text' name='ssid' id='ssid' placeholder='Enter WiFi name' required>";
  html += "</div>";
  html += "<div class='form-group'>";
  html += "<label>Password</label>";
  html += "<input type='password' name='password' placeholder='Enter password' required>";
  html += "</div>";
  html += "<button type='submit' class='btn btn-primary'>Connect to WiFi</button>";
  html += "</form>";
  
  if (savedSSID != "") {
    html += "<form action='/forget' method='POST'>";
    html += "<button type='submit' class='btn btn-danger'>Forget WiFi</button>";
    html += "</form>";
  }
  
  html += "<script>";
  html += "function selectNetwork(ssid) { document.getElementById('ssid').value = ssid; }";
  html += "</script>";
  
  html += FPSTR(HTML_FOOT);
  server.send(200, "text/html", html);
}

void handleConnect() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  
  saveCredentials(ssid, password);
  
  String html = FPSTR(HTML_HEAD);
  html += "<h1>⚡ Connecting...</h1>";
  html += "<div class='message success'>";
  html += "<div class='loading'></div><br><br>";
  html += "Connecting to <strong>" + ssid + "</strong><br>";
  html += "Device will restart in 3 seconds...";
  html += "</div>";
  html += FPSTR(HTML_FOOT);
  
  server.send(200, "text/html", html);
  
  delay(3000);
  shouldRestart = true;
}

void handleForget() {
  clearCredentials();
  
  String html = FPSTR(HTML_HEAD);
  html += "<h1>✅ WiFi Forgotten</h1>";
  html += "<div class='message success'>";
  html += "WiFi credentials have been cleared.<br>";
  html += "Device will restart in 3 seconds...";
  html += "</div>";
  html += FPSTR(HTML_FOOT);
  
  server.send(200, "text/html", html);
  
  delay(3000);
  shouldRestart = true;
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/forget", HTTP_POST, handleForget);
  
  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });
  
  server.begin();
}

bool connectToWiFi() {
  if (savedSSID == "") return false;
  
  displayMsg("Connecting...", savedSSID.c_str());
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  
  Serial.print("Connecting to ");
  Serial.println(savedSSID);
  
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    displayMsg("Connected!", WiFi.localIP().toString().c_str());
    delay(2000);
    return true;
  } else {
    Serial.println("\nConnection failed!");
    return false;
  }
}

void performWarmup() {
  displayMsg("Warming Up...", String(warmupCounter) + "/" + String(WARMUP_READINGS));
  
  emon1.calcVI(20, 2000);
  dht.getTemperature();
  dht.getHumidity();
  
  Serial.print("Warm-up reading ");
  Serial.print(warmupCounter + 1);
  Serial.print("/");
  Serial.print(WARMUP_READINGS);
  Serial.print(" - V: ");
  Serial.print(emon1.Vrms);
  Serial.print("V, I: ");
  Serial.print(emon1.Irms / 1000.0);
  Serial.println("A (discarded)");
  
  warmupCounter++;
  
  if (warmupCounter >= WARMUP_READINGS) {
    isWarmedUp = true;
    Serial.println("=== Warm-up complete! Starting normal operation ===");
    displayMsg("Ready!", "Starting...");
    delay(1000);
  }
  
  delay(WARMUP_DELAY_MS);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("OLED Ready");
    display.display();
  }

  dht.setup(DHTPIN, DHTesp::DHT11);
  emon1.voltage(VOLTAGE_PIN, 148.3, 1.7);
  emon1.current(CURRENT_PIN, 1160.0);

  displayMsg("Energy Monitor", "Initializing...");
  delay(1000);

  loadCredentials();
  
  if (!connectToWiFi()) {
    startAPMode();
    setupWebServer();
  } else {
    Serial.println("\n=== Starting sensor warm-up phase ===");
  }
}

void loop() {
  if (shouldRestart) {
    ESP.restart();
  }
  
  if (isAPMode) {
    dnsServer.processNextRequest();
    server.handleClient();
    return;
  }
  
  if (!isWarmedUp) {
    performWarmup();
    return;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    displayMsg("WiFi Lost!", "Reconnecting...");
    if (!connectToWiFi()) {
      startAPMode();
      setupWebServer();
    }
    return;
  }

  emon1.calcVI(20, 2000);

  current_Vrms   = emon1.Vrms;
  current_Irms   = emon1.Irms / 1000.0;
  current_realP  = emon1.realPower;
  current_appP   = emon1.apparentPower;
  current_pf     = emon1.powerFactor;

  Vrms = current_Vrms;
  Irms = current_Irms;
  watt = Vrms * Irms;

  tempC = dht.getTemperature();
  humidity = dht.getHumidity();

  int httpResponseCode = -1;
  String jsonData;

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    jsonData = String("{\"volt\":") + String(Vrms, 1) +
               String(",\"amps\":") + String(Irms, 3) +
               String(",\"watt\":") + String(watt, 2) +
               String(",\"temperature\":") + String(tempC, 1) +
               String(",\"humidity\":") + String(humidity, 1) +
               String("}");

    httpResponseCode = http.POST(jsonData);

    Serial.print("POST ");
    Serial.print(serverURL);
    Serial.print(" -> ");
    Serial.println(httpResponseCode);
    Serial.println(jsonData);

    http.end();
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("IP:");
  display.println(WiFi.localIP());

  display.setCursor(0, 8);
  display.print("V:");
  display.print(Vrms, 1);
  display.print(" A:");
  display.print(Irms, 3);

  display.setCursor(0, 16);
  display.print("P:");
  display.print(watt, 2);

  display.setCursor(0, 24);
  display.print("T:");
  display.print(tempC, 1);
  display.print(" H:");
  display.print(humidity, 0);
  display.print("% R:");
  display.print(httpResponseCode);

  display.display();

  delay(1000);
}

void displayMsg(String l1, String l2) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(l1);
  display.setCursor(0, 12);
  display.println(l2);
  display.display();
}