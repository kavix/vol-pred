import pandas as pd
from joblib import load
from pymongo import MongoClient
import os
import sys

import json

# Load configurations
sys.path.append(os.path.dirname(__file__))
try:
    from .ml_config import MONGO_URI, COLLECTION_NAME
except ImportError:
    from ml_config import MONGO_URI, COLLECTION_NAME

# Model constants
LAG_COUNT = 3
TARGETS = ['watt', 'temperature', 'humidity']


def resolve_collection_name(db, preferred: str) -> str:
    """Best-effort resolution of collection name across common casing/pluralization variants."""
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

    exact = [name for name in candidates if name in existing]
    if len(exact) == 1:
        return exact[0]
    if len(exact) > 1:
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

    preferred_lower = preferred.lower()
    for name in existing:
        if name.lower() == preferred_lower:
            return name
    for name in existing:
        if name.lower() == f"{preferred_lower}s":
            return name

    return preferred

def get_latest_data_points(uri, collection_name, target, count):
    """Fetches the last 'count' of the target variable from MongoDB."""
    client = None
    try:
        client = MongoClient(uri)
        db = client.get_database()
        resolved_name = resolve_collection_name(db, collection_name)
        collection = db[resolved_name]

        # Fetch the 'count' most recent records that actually contain this target field.
        data = list(
            collection.find(
                {target: {"$exists": True, "$ne": None}},
                {target: 1, 'time': 1},
            )
            .sort('time', -1)
            .limit(count)
        )

        if len(data) < count:
            return None

        # Query returns [T_current, T_current-1, T_current-2]
        latest_values = []
        for doc in data:
            try:
                latest_values.append(float(doc[target]))
            except (TypeError, ValueError, KeyError):
                return None

        features = {}
        for i in range(count):
            # i=0 -> Lag_1 -> latest_values[0]
            features[f'{target}_lag_{i+1}'] = latest_values[i]

        X_new = pd.DataFrame([features])

        # Ensure column order matches training
        X_new = X_new[[f'{target}_lag_{i}' for i in range(1, count + 1)]]

        return X_new

    except Exception as e:
        print(f"Database error: {e}", file=sys.stderr)
        return None
    finally:
        if client is not None:
            client.close()

def load_and_predict(model_filename, X_new):
    """Loads the trained model and makes a prediction."""
    try:
        model = load(model_filename)
        prediction = model.predict(X_new)
        return prediction[0]
    except FileNotFoundError:
        print(f"Model file {model_filename} not found. Run training first.", file=sys.stderr)
        return None
    except Exception as e:
        print(f"Prediction error: {e}", file=sys.stderr)
        return None


def predict_latest(uri: str = None, collection_name: str = None) -> dict:
    """Returns latest predictions for all targets as a JSON-serializable dict."""
    results = {}

    uri = uri or MONGO_URI
    collection_name = collection_name or COLLECTION_NAME

    for target in TARGETS:
        model_filename = os.path.join(os.path.dirname(__file__), f"model_{target}.joblib")
        X_new_features = get_latest_data_points(uri, collection_name, target, LAG_COUNT)

        if X_new_features is None:
            continue

        prediction = load_and_predict(model_filename, X_new_features)
        if prediction is None:
            continue

        results[target] = float(prediction)

    return results

if __name__ == '__main__':
    results = predict_latest()

    if results:
        print(json.dumps(results))
        sys.exit(0)

    sys.exit(1)
