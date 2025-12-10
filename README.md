# Energy Monitor System

ESP32-based energy monitoring with cloud storage and easy WiFi setup.

## Features
- Real-time voltage, current, power, temperature & humidity monitoring
- OLED display for live readings
- WiFi configuration portal (no code editing needed!)
- Cloud data storage (MongoDB + Vercel)

## Hardware Required
- ESP32 DevKit
- Voltage sensor (ZMPT101B) → Pin 34
- Current sensor (SCT-013) → Pin 35
- DHT11 sensor → Pin 33
- SSD1306 OLED (I2C) → SDA: Pin 22, SCL: Pin 23

## Quick Setup

### 1. Cloud Setup
1. Create free MongoDB Atlas cluster at [mongodb.com](https://www.mongodb.com/cloud/atlas)
2. Get connection string: `mongodb+srv://user:pass@cluster.mongodb.net/dbname`
3. Deploy to Vercel: `vercel --prod`
4. Add `MONGO_URI` environment variable in Vercel dashboard
5. Note your Vercel URL (e.g., `your-app.vercel.app`)

### 2. ESP32 Setup
1. Install Arduino libraries:
   - EmonLib
   - DHTesp  
   - Adafruit_GFX
   - Adafruit_SSD1306

2. Open `esp/esp.ino` and update server URL:
   ```cpp
   const char* serverURL = "http://your-app.vercel.app/send";
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
- Verify server URL matches your Vercel deployment
- Check HTTP response code on OLED (last line shows "R:200" if OK)

## File Structure
```
├── api/send.js          # Vercel serverless function
├── backend/server.js    # Local server (optional)
├── esp/esp.ino         # ESP32 firmware
└── vercel.json         # Vercel config
```

## Default Settings
- Setup Hotspot: `EnergyMonitor-Setup` / `12345678`
- Portal Address: `192.168.4.1`
- Data interval: 5 second
- Warm-up readings: 4 (2 seconds)

## License
MIT