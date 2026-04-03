# API Endpoints

Base URL (local default): `http://localhost:3000`

The **current backend** is FastAPI in `app.py`.
A **legacy Express backend** still exists in `backend/server.js` (same paths) but is not used by the Dockerfile.

## Environment variables

- `PORT` (default: `3000`)
- `MONGO_URI` (recommended) – MongoDB connection string
- `MONGO_COLLECTION` (optional) – MongoDB collection name (default comes from `ml_service/ml_config.py`)
- `MONGO_TIMEOUT_MS` (optional) – Mongo connection timeout in ms (default: `5000`)

---

## FastAPI (current) — `app.py`

### GET /health

**Description:** Health check.

**Response (200)**
```json
{ "status": "ok", "time": "2026-04-03T10:11:12.123Z" }
```

**Example**
```bash
curl -s http://localhost:3000/health
```

---

### GET /current

**Description:** Returns the latest sensor document (sorted by `time` descending).

**Response (200)**
- Returns the stored fields (e.g. `volt`, `current1..3`, `power1..3`, `total_power`, `watt`, `temperature`, `humidity`, `time`) plus:
  - `id` / `_id` (string)
  - `watt` is derived from `total_power` if missing
  - `amps` is derived by summing `current1 + current2 + current3` when present

**Response (404)**
```json
{ "error": "No data found" }
```

**Example**
```bash
curl -s http://localhost:3000/current
```

---

### POST /send

**Description:** Inserts a new sensor document. Any subset of fields is accepted.

**Request body (JSON)**
```json
{
  "volt": 230.5,
  "current1": 0.21,
  "current2": 0.18,
  "current3": 0.25,
  "power1": 48.0,
  "power2": 41.4,
  "power3": 57.2,
  "total_power": 146.6,
  "temperature": 28.1,
  "humidity": 62.0
}
```

**Notes**
- If `total_power` is provided, the API also stores `watt = total_power` (for ML compatibility).
- The server adds `time` automatically.

**Response (201)**
```json
{
  "message": "Data saved",
  "id": "<mongo_object_id>",
  "data": {
    "volt": 230.5,
    "current1": 0.21,
    "current2": 0.18,
    "current3": 0.25,
    "power1": 48.0,
    "power2": 41.4,
    "power3": 57.2,
    "total_power": 146.6,
    "temperature": 28.1,
    "humidity": 62.0,
    "watt": 146.6
  }
}
```

**Response (400)**
```json
{ "error": "No valid sensor fields in body" }
```

**Example**
```bash
curl -s -X POST http://localhost:3000/send \
  -H 'Content-Type: application/json' \
  -d '{"volt":230.5,"current1":0.21,"current2":0.18,"current3":0.25,"total_power":146.6,"temperature":28.1,"humidity":62.0}'
```

---

### GET /history

**Description:** Returns historical sensor documents.

**Query parameters**
- `start` (optional) – ISO date/time (e.g. `2026-04-03` or `2026-04-03T10:00:00Z`)
- `end` (optional) – ISO date/time
- `limit` (optional, default `100`, max `1000`) – maximum number of records

**Response (200)**
- An array of sensor documents (newest first).

**Example**
```bash
curl -s 'http://localhost:3000/history?limit=10'
curl -s 'http://localhost:3000/history?start=2026-04-01T00:00:00Z&end=2026-04-03T23:59:59Z&limit=100'
```

---

### GET /predict

**Description:** Returns ML predictions based on the latest stored data.

**Response (200)**
```json
{
  "success": true,
  "predictions": {
    "watt": 123.45,
    "temperature": 27.89,
    "humidity": 61.23
  },
  "units": {
    "watt": "Watts",
    "temperature": "Celsius",
    "humidity": "%"
  },
  "model": "Ridge Regression (Time Series)"
}
```

**Response (500)**
```json
{ "success": false, "error": "ML prediction failed.", "details": "..." }
```

---

## FastAPI built-in docs (available when running `app.py`)

- GET `/docs` – Swagger UI
- GET `/redoc` – ReDoc UI
- GET `/openapi.json` – OpenAPI schema

---

## Legacy Express API — `backend/server.js` (not used by Docker)

The legacy Express server defines the same paths:
- GET `/health`
- GET `/current`
- POST `/send`
- GET `/history`
- GET `/predict`

Note: `backend/server.js` currently contains **two** `GET /predict` handlers; in Express the later one will override the earlier one.
