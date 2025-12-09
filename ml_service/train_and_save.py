import pandas as pd
from pymongo import MongoClient
from sklearn.linear_model import Ridge
from joblib import dump
import os
import sys

# Load configurations
sys.path.append(os.path.dirname(__file__))
try:
    from .ml_config import MONGO_URI, COLLECTION_NAME
except ImportError:
    from ml_config import MONGO_URI, COLLECTION_NAME

# Model constants
LAG_COUNT = 3
TARGETS = ['watt', 'temperature', 'humidity']

def fetch_all_data_from_mongo(uri, collection_name):
    """Fetches all data from MongoDB into a time-series indexed DataFrame."""
    try:
        client = MongoClient(uri)
        # Parse DB name from URI if needed, or use default from connection string
        db = client.get_database()
        collection = db[collection_name] 
        
        # Check if collection exists/has data
        if collection.count_documents({}) == 0:
             print(f"Warning: Collection '{collection_name}' is empty.", file=sys.stderr)
             return pd.DataFrame()

        data = list(collection.find({}, {'_id': 0, '__v': 0}))
        df = pd.DataFrame(data)
        if 'time' in df.columns:
            df['time'] = pd.to_datetime(df['time'])
            return df.set_index('time').sort_index()
        else:
            print("Error: 'time' column not found in data.", file=sys.stderr)
            return pd.DataFrame()
            
    except Exception as e:
        print(f"Error fetching data: {e}", file=sys.stderr)
        return pd.DataFrame()

def train_and_save_model(df, target, lag_count, model_filename):
    """Trains the Ridge Regression model and saves it."""
    if df.empty:
        print("DataFrame is empty. Skipping training.", file=sys.stderr)
        return False
        
    # Feature Engineering (Lagging)
    df_ts = df[[target]].copy()
    for i in range(1, lag_count + 1):
        df_ts[f'{target}_lag_{i}'] = df_ts[target].shift(i)
        
    df_ts.dropna(inplace=True)
    
    if df_ts.empty:
        print("Not enough data to create lags. Skipping training.", file=sys.stderr)
        return False

    X = df_ts.drop(columns=[target])
    y = df_ts[target]
    
    model = Ridge(alpha=1.0)
    model.fit(X, y)
    
    # Save the trained model
    dump(model, model_filename)
    print(f"SUCCESS: Model trained and saved to {model_filename}. R2 on all data: {model.score(X, y):.4f}")
    return True

if __name__ == '__main__':
    print(f"Connecting to {MONGO_URI} collection: {COLLECTION_NAME}")
    all_data_df = fetch_all_data_from_mongo(MONGO_URI, COLLECTION_NAME)
    
    for target in TARGETS:
        model_filename = os.path.join(os.path.dirname(__file__), f"model_{target}.joblib")
        print(f"Training model for {target}...")
        train_and_save_model(all_data_df, target, LAG_COUNT, model_filename)
