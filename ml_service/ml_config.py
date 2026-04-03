import os
from dotenv import load_dotenv
import sys

# Add root directory to path to find the .env file
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(ROOT_DIR)
load_dotenv(dotenv_path=os.path.join(ROOT_DIR, '.env'))

# Configuration constants
MONGO_URI = os.getenv("MONGO_URI", "mongodb+srv://blacky:2419624196@voltura.vl2m5kl.mongodb.net/volData?retryWrites=true&w=majority")
COLLECTION_NAME = os.getenv("MONGO_COLLECTION", os.getenv("COLLECTION_NAME", "finalVolData"))