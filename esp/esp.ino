#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EmonLib.h>
#include <DHTesp.h>

// Wi-Fi credentials
const char* ssid     = "BlackY Pixel 7";
const char* password = "12345678";

// Server address (fill these)
const char* serverIP   = "10.170.182.10"; // <-- set this to your PC's IP visible to ESP
const int   serverPort = 3000;
const char* serverPath = "/send";

// OLED config (SSD1306 128x32 I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Sensor objects & measured values
EnergyMonitor emon1;
DHTesp dht;

float Vrms = 0;
float Irms = 0;
float watt = 0;
float tempC = 0;
float humidity = 0;

// add more detailed energy variables
float current_Vrms = 0;
float current_Irms = 0;      // in Amps
float current_realP = 0;     // real power (W)
float current_appP = 0;      // apparent power (VA)
float current_pf = 1.0;      // power factor

// Pin definitions
#define SDA_PIN 22
#define SCL_PIN 23
#define DHTPIN 33
#define VOLTAGE_PIN 34
#define CURRENT_PIN 35

String buildServerURL() {
	// ...build full URL from parts...
	return String("http://") + serverIP + ":" + String(serverPort) + serverPath;
}

void connectWiFi() {
	// ...connect with simple retry loop...
	if (WiFi.status() == WL_CONNECTED) return;

	WiFi.begin(ssid, password);
	Serial.print("Connecting to WiFi");
	unsigned long start = millis();
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		// timeout after 15s and try again in next loop
		if (millis() - start > 15000) break;
	}
	if (WiFi.status() == WL_CONNECTED) {
		Serial.println("\nWiFi connected!");
		Serial.print("IP: ");
		Serial.println(WiFi.localIP());
	} else {
		Serial.println("\nWiFi not connected (will retry).");
	}
}

void setup() {
	Serial.begin(115200);
	delay(100);

	// seed RNG (optional)
	randomSeed((unsigned long)millis() ^ (unsigned long)ESP.getEfuseMac());

	// init I2C on specified SDA/SCL pins for your board
	Wire.begin(SDA_PIN, SCL_PIN); // SDA=22, SCL=23

	// init OLED
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C address
		Serial.println("SSD1306 allocation failed");
	} else {
		display.clearDisplay();
		display.setTextSize(1);
		display.setTextColor(SSD1306_WHITE);
		display.setCursor(0, 0);
		display.println("OLED init OK");
		display.display();
	}

	// DHT Init
	dht.setup(DHTPIN, DHTesp::DHT11);

	// EmonLib Init (calibration values - adjust to your sensors)
	emon1.voltage(VOLTAGE_PIN, 148.3, 1.7);  // VCAL, PhaseShift
	emon1.current(CURRENT_PIN, 1160.0);      // ICAL

	displayMsg("Energy Monitor", "Starting...");

	connectWiFi();
}

void loop() {
	// ensure WiFi
	if (WiFi.status() != WL_CONNECTED) {
		connectWiFi();
	}

	// Read real sensor values
	// calcVI(sample_count, timeout_ms) - adjust if you need faster/slower
	emon1.calcVI(20, 2000);

	// Use EmonLib outputs for accurate measurements
	current_Vrms   = emon1.Vrms;
	// convert to Amps as you requested (if your calibration returns mA)
	current_Irms   = emon1.Irms / 1000.0;
	current_realP  = emon1.realPower;
	current_appP   = emon1.apparentPower;
	current_pf     = emon1.powerFactor;

	// set commonly used vars for backward compatibility / display
	Vrms = current_Vrms;
	Irms = current_Irms;
	watt = Vrms * Irms;;

	// Read DHT
	tempC = dht.getTemperature();
	humidity = dht.getHumidity();

	// Build and send JSON when connected
	int httpResponseCode = -1;
	String jsonData;
	if (WiFi.status() == WL_CONNECTED) {
		HTTPClient http;
		String url = buildServerURL();

		http.begin(url);
		http.addHeader("Content-Type", "application/json");

		// include real power and PF in JSON
		jsonData = String("{\"volt\":") + String(Vrms, 1) +
				   String(",\"amps\":") + String(Irms, 3) +
				   String(",\"watt\":") + String(watt, 2) +
				   String(",\"temperature\":") + String(tempC, 1) +
				   String(",\"humidity\":") + String(humidity, 1) +
				   String("}");

		httpResponseCode = http.POST(jsonData);

		Serial.print("POST ");
		Serial.print(url);
		Serial.print(" -> ");
		Serial.println(httpResponseCode);

		Serial.println(jsonData);

		http.end();
	} else {
		Serial.println("Skipping send: not connected to WiFi.");
	}

	// Update OLED with connection + send info (compact for 128x32)
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);

	display.setCursor(0, 0);
	if (WiFi.status() == WL_CONNECTED) {
		display.print("IP:");
		display.println(WiFi.localIP());
	} else {
		display.println("Not connected");
	}

	// line 2: show volt and amps
	display.setCursor(0, 8);
	display.print("V:");
	display.print(Vrms, 1);
	display.print(" A:");
	display.print(Irms, 3);

	// line 3: show real power and PF
	display.setCursor(0, 16);
	display.print("P:");
	display.print(watt, 2);


	// optional line 4: humidity and HTTP code
	display.setCursor(0, 24);
	display.print("T:");
	display.print(tempC, 1);
	display.print(" H:");
	display.print(humidity, 0);
	display.print("% HTP:");
	display.print(httpResponseCode);

	display.display();

	delay(1000); // wait 5s between readings
}

// small helper used during startup
void displayMsg(String l1, String l2) {
	display.clearDisplay();
	display.setCursor(0, 0);
	display.println(l1);
	display.setCursor(0, 12);
	display.println(l2);
	display.display();
}
