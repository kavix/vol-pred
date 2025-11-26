#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

	// seed RNG
	randomSeed((unsigned long)millis() ^ (unsigned long)ESP.getEfuseMac());

	// init I2C on specified SDA/SCL pins for your board
	Wire.begin(22, 23); // SDA=22, SCL=23

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

	connectWiFi();
}

void loop() {
	// ensure WiFi
	if (WiFi.status() != WL_CONNECTED) {
		connectWiFi();
	}

	// generate requested readings:
	// volt: float 200.0 .. 240.0 (one decimal)
	float volt = random(2000, 2401) / 10.0; // 200.0 .. 240.0

	// amps: float 0.20 .. 1.50 (two decimals) — adjust range if needed
	float amps = random(20, 151) / 100.0; // 0.20 .. 1.50

	// watt: volt * amps (one decimal)
	float watt = volt * amps;

	// temperature: 20.0 .. 35.0 (one decimal)
	float temperature = random(200, 351) / 10.0; // 20.0 .. 35.0

	// humidity: 30.0 .. 80.0 (one decimal)
	float humidity = random(300, 801) / 10.0; // 30.0 .. 80.0

	int httpResponseCode = -1;
	String jsonData;
	if (WiFi.status() == WL_CONNECTED) {
		HTTPClient http;
		String url = buildServerURL();

		http.begin(url);
		http.addHeader("Content-Type", "application/json");

		// build JSON: {"volt":200.5,"amps":1.23,"watt":283.5,"temperature":29.4,"humidity":65.2}
		jsonData = String("{\"volt\":") + String(volt, 1) +
				   String(",\"amps\":") + String(amps, 2) +
				   String(",\"watt\":") + String(watt, 1) +
				   String(",\"temperature\":") + String(temperature, 1) +
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
	display.print(volt, 1);
	display.print(" A:");
	display.print(amps, 2);

	// line 3: show watt and temp
	display.setCursor(0, 16);
	display.print("W:");
	display.print(watt, 1);
	display.print(" T:");
	display.print(temperature, 1);

	// optional line 4: humidity and HTTP code
	display.setCursor(0, 24);
	display.print("H:");
	display.print(humidity, 1);
	display.print(" HTP:");
	display.print(httpResponseCode);

	display.display();

	delay(5000); // wait 5s between readings
}
