# Energy Monitor System

ESP32-based energy monitoring with cloud storage and easy WiFi setup.

## Features
- Real-time voltage, current, power, temperature & humidity monitoring
- OLED display for live readings
- WiFi configuration portal (no code editing needed!)
- Cloud data storage (MongoDB + FastAPI)

## Hardware Required
- ESP32 DevKit
- Voltage sensor (ZMPT101B) → Pin 34
- Current sensor (SCT-013) → Pin 35
- DHT11 sensor → Pin 33
- SSD1306 OLED (I2C) → SDA: Pin 22, SCL: Pin 23

## Quick Setup

### Backend API (FastAPI)
The backend is now a **FastAPI** service (default `http://localhost:3000`).

**Local run:**
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r ml_service/requirements.txt
uvicorn app:app --host 0.0.0.0 --port 3000 --reload
```

**Environment variables:**
- `MONGO_URI` (recommended) – MongoDB Atlas connection string
- `MONGO_COLLECTION` (optional) – override collection name if needed

### 1. Cloud Setup
1. Create free MongoDB Atlas cluster at [mongodb.com](https://www.mongodb.com/cloud/atlas)
2. Get connection string: `mongodb+srv://user:pass@cluster.mongodb.net/dbname`
3. Deploy the FastAPI backend (Docker recommended)
4. Set `MONGO_URI` in your deployment environment

See `EC2_Deployment.md` for a Docker + EC2 walkthrough.

### 2. ESP32 Setup
1. Install Arduino libraries:
   - EmonLib
   - DHTesp  
   - Adafruit_GFX
   - Adafruit_SSD1306

2. Open `esp/esp.ino` and update server URL:
   ```cpp
   const char* serverURL = "http://<your-server-ip>:3000/send";
   ```

3. Upload to ESP32

### 3. WiFi Configuration
**First Time:**
1. Power on ESP32
2. Connect to WiFi: `EnergyMonitor-Setup` (password: `12345678`)
3. Portal opens automatically (or visit `192.168.4.1`)
4. Select your WiFi network and enter password
5. Done! Auto-reconnects on every restart

**Change WiFi:**
1. Connect to `EnergyMonitor-Setup` hotspot again
2. Click "Forget WiFi" button
3. Connect to new network

## How It Works
- Device starts in "Setup Mode" if no WiFi saved
- Creates hotspot with web portal
- You configure WiFi through beautiful interface
- Credentials saved automatically
- 2-second sensor warm-up on boot (eliminates spikes)
- Sends data to cloud every second

## Troubleshooting

**Can't connect to setup hotspot?**
- Wait 15 seconds after power-on
- Check OLED shows "Setup Mode"

**Portal doesn't open?**
- Manually go to `192.168.4.1`

**WiFi won't connect?**
- Verify 2.4GHz network (ESP32 doesn't support 5GHz)
- Check password is correct

**Readings look wrong?**
- Wait for 2-second warm-up to complete
- Check sensor connections

**Data not saving?**
- Verify `serverURL` in `esp/esp.ino` matches your backend deployment
- Check HTTP response code on OLED (last line shows "R:200" if OK)

## File Structure
```
├── app.py               # FastAPI backend (current)
├── ml_service/          # ML training + prediction code
├── esp/esp.ino          # ESP32 firmware
└── Dockerfile           # Container build
```

## Default Settings
- Setup Hotspot: `EnergyMonitor-Setup` / `12345678`
- Portal Address: `192.168.4.1`
- Data interval: 5 second
- Warm-up readings: 4 (2 seconds)

## License
MIT