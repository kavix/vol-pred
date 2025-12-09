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

def get_latest_data_points(uri, collection_name, target, count):
    """Fetches the last 'count' of the target variable from MongoDB."""
    try:
        client = MongoClient(uri)
        db = client.get_database()
        collection = db[collection_name]
        
        # Fetch the 'count' most recent records (sorted descending by time)
        data = list(collection.find({}, {target: 1, 'time': 1}).sort('time', -1).limit(count))
        
        if len(data) < count: return None 

        # Extract values and reverse to match lagged feature order (Lag_3, Lag_2, Lag_1)
        # Data comes in [Latest, Previous, Pre-Previous]
        # We need [Pre-Previous, Previous, Latest] to match Lag_3, Lag_2, Lag_1 logic?
        # Wait, if we predict t using t-1, t-2, t-3.
        # The model was trained with:
        # Target: t
        # Features: Lag_1 (t-1), Lag_2 (t-2), Lag_3 (t-3)
        # So for prediction of T_future, we need T_current (as Lag_1), T_current-1 (as Lag_2), T_current-2 (as Lag_3).
        # The query returns [T_current, T_current-1, T_current-2].
        # So latest_values[0] is T_current (Lag_1).
        # latest_values[1] is T_current-1 (Lag_2).
        # latest_values[2] is T_current-2 (Lag_3).
        
        # The training script does:
        # df_ts[f'{target}_lag_{i}'] = df_ts[target].shift(i)
        # So Lag_1 is the previous row.
        
        # If we have [T_current, T_current-1, T_current-2]
        # We want to map them to columns: watt_lag_1, watt_lag_2, watt_lag_3
        # watt_lag_1 = T_current
        # watt_lag_2 = T_current-1
        # watt_lag_3 = T_current-2
        
        latest_values = [doc[target] for doc in data] # [T_current, T_current-1, T_current-2]
        
        # Create DataFrame with correct column names
        # Columns should be ['watt_lag_1', 'watt_lag_2', 'watt_lag_3']
        # Values should be [T_current, T_current-1, T_current-2]
        
        features = {}
        for i in range(count):
            # i=0 -> Lag_1 -> latest_values[0]
            features[f'{target}_lag_{i+1}'] = latest_values[i]
            
        X_new = pd.DataFrame([features])
        
        # Ensure column order matches training (usually pandas sorts or keeps insertion order, but sklearn cares about order if dataframe)
        # The training script:
        # for i in range(1, lag_count + 1): df_ts[f'{target}_lag_{i}'] ...
        # So columns are watt_lag_1, watt_lag_2, watt_lag_3
        
        X_new = X_new[[f'{target}_lag_{i}' for i in range(1, count + 1)]]
        
        return X_new
        
    except Exception as e:
        print(f"Database error: {e}", file=sys.stderr)
        return None

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

if __name__ == '__main__':
    results = {}
    
    for target in TARGETS:
        model_filename = os.path.join(os.path.dirname(__file__), f"model_{target}.joblib")
        X_new_features = get_latest_data_points(MONGO_URI, COLLECTION_NAME, target, LAG_COUNT)
        
        if X_new_features is not None:
            prediction = load_and_predict(model_filename, X_new_features)
            if prediction is not None:
                results[target] = prediction
    
    if results:
        print(json.dumps(results))
        sys.exit(0)
    else:
        sys.exit(1)
