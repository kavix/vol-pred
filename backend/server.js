require('dotenv').config();
const express = require('express');
const mongoose = require('mongoose');
const bodyParser = require('body-parser');
const cors = require('cors');
const app = express();

app.use(cors());
app.use(bodyParser.json());

// MongoDB connection
const mongoUri = process.env.MONGO_URI;
if (!mongoUri) {
    console.error("❌ MONGO_URI missing in .env");
    process.exit(1);
}
mongoose.set('strictQuery', false);

// Schema (kept because you requested to store one dataset)
const SensorSchema = new mongoose.Schema({
    volt: Number,
    amps: Number,
    watt: Number,
    temperature: Number,
    humidity: Number,
    time: { type: Date, default: Date.now }
});

const Sensor = mongoose.model("Sensor", SensorSchema);

// simple request logger for debugging
app.use((req, res, next) => {
  console.log(`[${new Date().toISOString()}] ${req.method} ${req.path} from ${req.ip}`);
  if (req.method === 'POST') console.log('  body:', req.body);
  next();
});

// health endpoint
app.get('/health', (req, res) => {
  res.json({ status: 'ok', time: new Date().toISOString() });
});

// API endpoint to receive data from ESP
app.post('/send', async (req, res) => {
  try {
    const { value1, value2, value3, volt, amps, watt, temperature, humidity } = req.body;
    const docObj = {};

    // map legacy names to new fields
    if (typeof volt !== 'undefined') docObj.volt = volt;
    else if (typeof value1 !== 'undefined') docObj.volt = value1;

    if (typeof amps !== 'undefined') docObj.amps = amps;
    else if (typeof value2 !== 'undefined') docObj.amps = value2;

    if (typeof watt !== 'undefined') docObj.watt = watt;
    else if (typeof value3 !== 'undefined') docObj.watt = value3;

    if (typeof temperature !== 'undefined') docObj.temperature = temperature;
    if (typeof humidity !== 'undefined') docObj.humidity = humidity;

    if (Object.keys(docObj).length === 0) {
      console.log('No valid fields in incoming body:', req.body);
      return res.status(400).json({ error: 'No valid sensor fields in body' });
    }

    const data = new Sensor(docObj);
    const saved = await data.save();
    console.log('Saved document id:', saved._id.toString());
    res.status(201).json({ message: 'Data saved', id: saved._id, data: docObj });
  } catch (err) {
    console.error('Error saving data:', err);
    res.status(500).json({ error: err.message });
  }
});

// Connect and optionally insert ONE sample dataset if collection empty
mongoose.connect(mongoUri)
  .then(async () => {
    console.log('Connected to MongoDB');

    const count = await Sensor.countDocuments().catch(() => 0);
    if (!count) {
      const sampleData = new Sensor({
        volt: 201.5,
        amps: 1.23,
        watt: 283.5,
        temperature: 20.4,
        humidity: 60.2
      });
      await sampleData.save();
      console.log('Inserted ONE sample dataset:', sampleData);
    } else {
      console.log('Collection already contains documents, skipping sample insert.');
    }
  })
  .catch(err => {
    console.error('MongoDB connection error:', err.message);
    process.exit(1);
  });

const port = process.env.PORT || 3000;
// listen on all interfaces so other devices can reach this server
app.listen(port, '0.0.0.0', () => {
    console.log(`Server running on port ${port} (listening on 0.0.0.0)`);
});
