const express = require('express');
const mongoose = require('mongoose');
const bodyParser = require('body-parser');
const cors = require('cors');
const { spawn } = require('child_process');
const path = require('path');
const app = express();

app.use(cors());
app.use(bodyParser.json());

// MongoDB connection
const mongoUri = 'mongodb+srv://blacky:2419624196@voltura.vl2m5kl.mongodb.net/volData?retryWrites=true&w=majority';
mongoose.set('strictQuery', false);

// Schema (kept because you requested to store one dataset)
const SensorSchema = new mongoose.Schema({
  volt: Number,
  current1: Number,
  current2: Number,
  current3: Number,
  power1: Number,
  power2: Number,
  power3: Number,
  total_power: Number,
  watt: Number, // Kept for ML compatibility (mapped from total_power)
  temperature: Number,
  humidity: Number,
  time: { type: Date, default: Date.now }
});

const Sensor = mongoose.model("finalVolData", SensorSchema);

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

// API endpoint to get the latest data
app.get('/current', async (req, res) => {
  try {
    const latest = await Sensor.findOne().sort({ time: -1 });
    if (!latest) {
      return res.status(404).json({ error: 'No data found' });
    }
    res.json(latest);
  } catch (err) {
    console.error('Error fetching data:', err);
    res.status(500).json({ error: err.message });
  }
});

// API endpoint to receive data from ESP
app.post('/send', async (req, res) => {
  try {
    const {
      volt,
      current1, current2, current3,
      power1, power2, power3,
      total_power,
      temperature, humidity
    } = req.body;

    const docObj = {};

    if (typeof volt !== 'undefined') docObj.volt = volt;

    if (typeof current1 !== 'undefined') docObj.current1 = current1;
    if (typeof current2 !== 'undefined') docObj.current2 = current2;
    if (typeof current3 !== 'undefined') docObj.current3 = current3;

    if (typeof power1 !== 'undefined') docObj.power1 = power1;
    if (typeof power2 !== 'undefined') docObj.power2 = power2;
    if (typeof power3 !== 'undefined') docObj.power3 = power3;

    if (typeof total_power !== 'undefined') {
      docObj.total_power = total_power;
      docObj.watt = total_power; // Map to watt for ML compatibility
    }

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

// API endpoint to get historical data
app.get('/history', async (req, res) => {
  try {
    const { start, end, limit } = req.query;
    const query = {};

    if (start || end) {
      query.time = {};
      if (start) query.time.$gte = new Date(start);
      if (end) query.time.$lte = new Date(end);
    }

    const limitVal = parseInt(limit) || 100;
    // Sort by time descending (newest first)
    const data = await Sensor.find(query).sort({ time: -1 }).limit(limitVal);
    res.json(data);
  } catch (err) {
    console.error('Error fetching history:', err);
    res.status(500).json({ error: err.message });
  }
});

// --- NEW API Endpoint for Prediction ---

// Use environment variable for Python path (Docker) or fallback to local venv
const PYTHON_EXECUTABLE = process.env.PYTHON_EXECUTABLE || path.join(__dirname, '..', 'venv', 'bin', 'python3');
const PREDICT_SCRIPT = path.join(__dirname, '..', 'ml_service', 'predict_future.py');

app.get('/predict', (req, res) => {
  // Spawn the Python process to run the prediction script
  const python = spawn(PYTHON_EXECUTABLE, [PREDICT_SCRIPT]);

  let predictionOutput = '';
  let errorOutput = '';

  // Capture standard output (the predicted numerical value)
  python.stdout.on('data', (data) => {
    predictionOutput += data.toString().trim();
  });

  // Capture standard error (for debugging)
  python.stderr.on('data', (data) => {
    errorOutput += data.toString();
  });

  // Handle process completion
  python.on('close', (code) => {
    if (code === 0) {
      // Prediction succeeded
      try {
        const predictions = JSON.parse(predictionOutput);
        return res.json({
          success: true,
          predictions: predictions,
          units: {
            watt: 'Watts',
            temperature: 'Celsius',
            humidity: '%'
          },
          model: 'Ridge Regression (Time Series)'
        });
      } catch (e) {
        console.error('Failed to parse Python output:', predictionOutput);
        return res.status(500).json({ success: false, error: 'Invalid output from ML script' });
      }
    }

    // If code != 0 or output was invalid
    console.error(`Prediction script failed with code ${code}. Error: ${errorOutput}`);
    return res.status(500).json({
      success: false,
      error: 'ML prediction failed.',
      details: errorOutput.substring(0, 200) // Truncate long errors
    });
  });

  // Handle spawn errors (e.g., python executable path is wrong)
  python.on('error', (err) => {
    console.error('Failed to spawn Python process:', err);
    res.status(500).json({ success: false, error: 'Internal server error running ML script.' });
  });
});

// API endpoint to get ML predictions
app.get('/predict', (req, res) => {
  // Resolve the path to the Python executable and script relative to the project root
  const pythonExecutable = process.env.PYTHON_EXECUTABLE || 'python3'; // Use 'python3' as a default
  const scriptPath = path.resolve(__dirname, '..', 'ml_service', 'predict_future.py');

  console.log(`Executing prediction script: ${pythonExecutable} ${scriptPath}`);

  const pythonProcess = spawn(pythonExecutable, [scriptPath]);

  let dataBuffer = '';
  let errorBuffer = '';

  pythonProcess.stdout.on('data', (data) => {
    dataBuffer += data.toString();
  });

  pythonProcess.stderr.on('data', (data) => {
    errorBuffer += data.toString();
  });

  pythonProcess.on('close', (code) => {
    console.log(`Prediction script exited with code ${code}`);

    if (code !== 0) {
      console.error('Prediction script stderr:', errorBuffer);
      // Check for a common error: script not found
      if (errorBuffer.includes('No such file or directory')) {
        return res.status(500).json({
          error: 'Prediction script not found.',
          details: `The server could not find the script at the expected path: ${scriptPath}. Please ensure the file exists and paths are correct.`
        });
      }
      return res.status(500).json({
        error: 'Failed to execute prediction script.',
        details: errorBuffer
      });
    }

    try {
      const predictions = JSON.parse(dataBuffer);
      res.json(predictions);
    } catch (parseError) {
      console.error('Error parsing prediction output:', parseError);
      console.error('Raw output from script:', dataBuffer);
      res.status(500).json({
        error: 'Failed to parse prediction output.',
        details: dataBuffer
      });
    }
  });

  pythonProcess.on('error', (err) => {
    console.error('Failed to start prediction script process:', err);
    res.status(500).json({
      error: 'Failed to start the prediction script process.',
      details: err.message
    });
  });
});

const port = process.env.PORT || 3000;
app.listen(port, '0.0.0.0', () => {
  console.log(`Server listening on 0.0.0.0:${port}`);
  mongoose.connect(mongoUri)
    .then(() => console.log('MongoDB connected successfully.'))
    .catch(err => console.error('MongoDB connection error:', err));
});
