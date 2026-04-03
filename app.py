import os
import sys
from datetime import datetime, timezone
from typing import Any, Dict, Optional

import certifi
from fastapi import FastAPI, HTTPException, Query, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from pymongo import MongoClient


# Prefer ML service config so API + ML stay consistent
try:
    from ml_service.ml_config import MONGO_URI as _DEFAULT_MONGO_URI
    from ml_service.ml_config import COLLECTION_NAME as _DEFAULT_COLLECTION_NAME
except Exception:
    _DEFAULT_MONGO_URI = "mongodb+srv://blacky:2419624196@voltura.vl2m5kl.mongodb.net/volData?retryWrites=true&w=majority"
    _DEFAULT_COLLECTION_NAME = "finalVolData"


MONGO_URI = os.getenv("MONGO_URI", _DEFAULT_MONGO_URI)
# Allow overriding collection name to match existing deployments
MONGO_COLLECTION = os.getenv("MONGO_COLLECTION", _DEFAULT_COLLECTION_NAME)


class SensorIn(BaseModel):
    volt: Optional[float] = None

    current1: Optional[float] = None
    current2: Optional[float] = None
    current3: Optional[float] = None

    power1: Optional[float] = None
    power2: Optional[float] = None
    power3: Optional[float] = None

    total_power: Optional[float] = None

    temperature: Optional[float] = None
    humidity: Optional[float] = None


app = FastAPI(title="vol-pred API", version="1.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


_mongo_client: Optional[MongoClient] = None
_collection = None


@app.exception_handler(HTTPException)
async def _http_exception_handler(_request: Request, exc: HTTPException):
    # Express-style error shape: { error: "..." }
    if isinstance(exc.detail, dict):
        content = exc.detail
    else:
        content = {"error": str(exc.detail)}
    return JSONResponse(status_code=exc.status_code, content=content)


def _iso_z(dt: datetime) -> str:
    if dt.tzinfo is None:
        dt = dt.replace(tzinfo=timezone.utc)
    else:
        dt = dt.astimezone(timezone.utc)
    return dt.isoformat().replace("+00:00", "Z")


def _serialize_doc(doc: Dict[str, Any]) -> Dict[str, Any]:
    out: Dict[str, Any] = {}

    for key, value in doc.items():
        if key == "__v":
            continue
        if key == "_id":
            out["_id"] = str(value)
            out["id"] = str(value)
            continue
        if key == "time" and isinstance(value, datetime):
            out["time"] = _iso_z(value)
            continue
        out[key] = value

    # Derived compatibility fields (used by docs / typical dashboards)
    if "watt" not in out and "total_power" in out:
        out["watt"] = out.get("total_power")

    current_values = [out.get("current1"), out.get("current2"), out.get("current3")]
    if any(v is not None for v in current_values):
        amps = 0.0
        for v in current_values:
            if v is None:
                continue
            try:
                amps += float(v)
            except (TypeError, ValueError):
                pass
        out.setdefault("amps", amps)

    return out


def _parse_iso_datetime(value: str) -> datetime:
    # Accept values like `2026-04-03`, `2026-04-03T10:11:12`, `...Z`
    try:
        cleaned = value.strip().replace("Z", "+00:00")
        dt = datetime.fromisoformat(cleaned)
    except ValueError:
        raise HTTPException(status_code=400, detail={"error": f"Invalid datetime: {value}"})

    # Store/query as naive UTC datetimes (Mongo stores UTC)
    if dt.tzinfo is not None:
        dt = dt.astimezone(timezone.utc).replace(tzinfo=None)
    return dt


def _resolve_collection_name(db, preferred: str) -> str:
    # Try to find the existing collection name across common Mongoose/PyMongo variants.
    try:
        existing = db.list_collection_names()
    except Exception:
        return preferred

    candidates = []
    for name in [
        preferred,
        preferred.lower(),
        f"{preferred}s",
        f"{preferred.lower()}s",
    ]:
        if name not in candidates:
            candidates.append(name)

    # Exact matches first
    exact = [name for name in candidates if name in existing]
    if len(exact) == 1:
        return exact[0]
    if len(exact) > 1:
        # Pick the one that actually has documents
        best_name = exact[0]
        best_count = -1
        for name in exact:
            try:
                count = db[name].estimated_document_count()
            except Exception:
                count = 0
            if count > best_count:
                best_count = count
                best_name = name
        return best_name

    # Case-insensitive fallback
    preferred_lower = preferred.lower()
    for name in existing:
        if name.lower() == preferred_lower:
            return name
    for name in existing:
        if name.lower() == f"{preferred_lower}s":
            return name

    return preferred


@app.on_event("startup")
def _startup() -> None:
    global _mongo_client, _collection

    timeout_ms = int(os.getenv("MONGO_TIMEOUT_MS", "5000"))
    _mongo_client = MongoClient(
        MONGO_URI,
        tlsCAFile=certifi.where(),
        serverSelectionTimeoutMS=timeout_ms,
    )
    db = _mongo_client.get_database()

    # Try to resolve existing collection name, but don't block startup on DB connectivity.
    collection_name = MONGO_COLLECTION
    try:
        collection_name = _resolve_collection_name(db, MONGO_COLLECTION)
    except Exception as e:
        print(f"MongoDB collection resolution warning: {e}", file=sys.stderr)

    _collection = db[collection_name]
    try:
        print(f"MongoDB ready: db={db.name} collection={collection_name}")
    except Exception:
        pass


@app.on_event("shutdown")
def _shutdown() -> None:
    global _mongo_client
    if _mongo_client is not None:
        _mongo_client.close()
        _mongo_client = None


@app.middleware("http")
async def _log_requests(request: Request, call_next):
    start = datetime.now(timezone.utc)
    response = await call_next(request)
    duration_ms = int((datetime.now(timezone.utc) - start).total_seconds() * 1000)
    print(
        f"[{_iso_z(start)}] {request.method} {request.url.path} "
        f"{response.status_code} {duration_ms}ms from {request.client.host if request.client else 'unknown'}"
    )
    return response


@app.get("/health")
def health() -> Dict[str, Any]:
    return {"status": "ok", "time": _iso_z(datetime.now(timezone.utc))}


@app.get("/current")
def current() -> Dict[str, Any]:
    if _collection is None:
        raise HTTPException(status_code=500, detail={"error": "Database not initialized"})

    try:
        latest = _collection.find_one(sort=[("time", -1)])
    except Exception as e:
        raise HTTPException(status_code=500, detail={"error": str(e)})

    if not latest:
        raise HTTPException(status_code=404, detail={"error": "No data found"})

    return _serialize_doc(latest)


@app.post("/send", status_code=201)
def send(payload: SensorIn) -> Dict[str, Any]:
    if _collection is None:
        raise HTTPException(status_code=500, detail={"error": "Database not initialized"})

    doc_obj: Dict[str, Any] = {}
    body = payload.model_dump(exclude_none=True)

    # Copy known fields
    for key in [
        "volt",
        "current1",
        "current2",
        "current3",
        "power1",
        "power2",
        "power3",
        "total_power",
        "temperature",
        "humidity",
    ]:
        if key in body:
            doc_obj[key] = body[key]

    if "total_power" in doc_obj:
        doc_obj["watt"] = doc_obj["total_power"]

    if not doc_obj:
        raise HTTPException(status_code=400, detail={"error": "No valid sensor fields in body"})

    # Match Express behavior: auto timestamp
    doc_obj["time"] = datetime.utcnow()

    try:
        result = _collection.insert_one(doc_obj)
    except Exception as e:
        raise HTTPException(status_code=500, detail={"error": str(e)})

    return {
        "message": "Data saved",
        "id": str(result.inserted_id),
        "data": {k: v for k, v in doc_obj.items() if k != "time"},
    }


@app.get("/history")
def history(
    start: Optional[str] = Query(default=None),
    end: Optional[str] = Query(default=None),
    limit: int = Query(default=100, ge=1, le=1000),
) -> Any:
    if _collection is None:
        raise HTTPException(status_code=500, detail={"error": "Database not initialized"})

    query: Dict[str, Any] = {}

    if start or end:
        time_query: Dict[str, Any] = {}
        if start:
            time_query["$gte"] = _parse_iso_datetime(start)
        if end:
            time_query["$lte"] = _parse_iso_datetime(end)
        query["time"] = time_query

    try:
        cursor = _collection.find(query).sort("time", -1).limit(limit)
        return [_serialize_doc(doc) for doc in cursor]
    except Exception as e:
        raise HTTPException(status_code=500, detail={"error": str(e)})


@app.get("/predict")
def predict() -> Dict[str, Any]:
    try:
        from ml_service.predict_future import predict_latest

        predictions = predict_latest()
    except Exception as e:
        raise HTTPException(
            status_code=500,
            detail={
                "success": False,
                "error": "ML prediction failed.",
                "details": str(e)[:200],
            },
        )

    if not predictions:
        raise HTTPException(
            status_code=500,
            detail={"success": False, "error": "ML prediction failed."},
        )

    return {
        "success": True,
        "predictions": predictions,
        "units": {"watt": "Watts", "temperature": "Celsius", "humidity": "%"},
        "model": "Ridge Regression (Time Series)",
    }
