# ESP32 Energy Monitor

A compact ESP32-based energy monitor that reads voltage, current and environmental sensors, displays values on an SSD1306 OLED and sends JSON payloads to a HTTP server (local or remote).

This project uses:
- EmonLib for AC voltage/current measurements
- DHTesp for temperature/humidity
- Adafruit SSD1306 + GFX for OLED display
- WiFi + HTTPClient to POST data to your server

Quick features
- Vrms, Irms, real power (W), apparent power, power factor
- Temperature and humidity from DHT11 (configurable)
- OLED realtime display (128×32)
- JSON POST to configurable server IP/port/path

Required libraries
- EmonLib
- DHTesp
- Adafruit SSD1306
- Adafruit GFX
- ESP32 Arduino core (board package)

Hardware / pinout (as used in esp.ino)
- ESP32 (generic)
- OLED I2C: SDA = GPIO 22, SCL = GPIO 23 (address 0x3C)
- DHT sensor: DHTPIN = GPIO 33 (DHT11 by default)
- Voltage sensor input: VOLTAGE_PIN = GPIO 34 (analog)
- Current sensor input: CURRENT_PIN = GPIO 35 (analog, CT sensor)
- USB power for ESP32

Wiring notes
- Connect CT (current transformer) and burden resistor per CT datasheet.
- Use proper resistive divider / isolation for mains voltage sensing hardware — unsafe wiring can kill. If unsure, stop and consult an electrician.
- Connect DHT data pin with a pull-up (4.7–10k) if needed.

Calibration
- In code you will find emon1.voltage(VOLTAGE_PIN, VCAL, PHASE) and emon1.current(CURRENT_PIN, ICAL).
- Use a reference meter to tune:
  - VCAL: adjust until Vrms matches reference.
  - ICAL: adjust until Irms matches reference.
  - PHASE: adjust small phaseShift until power readings align.
- Typical starting values in the sketch: VCAL = 148.3, ICAL = 1160.0, PHASE = 1.7 — these are example values and likely need tuning.

JSON payload sent to server
- Example:
  {"volt":230.5,"amps":1.234,"watt":284.12,"temperature":29.4,"humidity":65.2}
- Fields:
  - volt: Vrms (V)
  - amps: Irms (A)
  - watt: real power (W)
  - temperature: °C
  - humidity: %

Server configuration
- Set serverIP, serverPort and serverPath in esp.ino to point to your collector endpoint.
- Endpoint should accept POST with JSON application/json.

OLED layout (128×32)
- Line 1: IP or connection status
- Line 2: V:xxx.x  A:xx.xxx
- Line 3: P:xxx.xx W
- Line 4: T:xx.x H:xx% HTP:<HTTP code>

Build & flash
1. Install libraries listed above from Library Manager.
2. Select your ESP32 board in Arduino IDE / PlatformIO.
3. Compile and upload esp.ino.
4. Open Serial Monitor at 115200 for logs.

Troubleshooting
- No OLED output: check I2C wiring and address; try scanning I2C bus.
- DHT shows NaN: try longer delay or replace sensor; DHT11 is slow.
- Power readings wrong: recalibrate VCAL/ICAL/PHASE and confirm CT wiring.
- WiFi fails: check SSID/password and ensure server is reachable from ESP network.
- JSON not received: verify server IP/port/path and inspect HTTP response code printed on OLED and serial.

Security & safety
- This project interacts with mains AC — if you're not experienced with mains wiring do not attempt measurements; use an isolated/proper sensor module.
- Secure the server endpoint if deploying outside a trusted network.

License
- MIT — reuse and adapt freely. Give credit and be careful with mains wiring.

Contributing
- PRs and issue reports welcome. Add improved calibration helpers, averaging, data buffering or MQTT support.

Enjoy building — calibrate carefully and stay safe!
