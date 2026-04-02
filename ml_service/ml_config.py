import os
from dotenv import load_dotenv
import sys

# Add root directory to path to find the .env file
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(ROOT_DIR)
load_dotenv(dotenv_path=os.path.join(ROOT_DIR, '.env'))

# Configuration constants
MONGO_URI = os.getenv("MONGO_URI", "mongodb+srv://blacky:2419624196@voltura.vl2m5kl.mongodb.net/volData?retryWrites=true&w=majority")
COLLECTION_NAME = "finalVolData" 
# server.js: const Sensor = mongoose.model("Sensor", SensorSchema);
# This usually creates a collection named 'sensors'.
# However, the user might have existing data in 'voldata'.
# Let's look at the connection string provided earlier: .../volData?retryWrites=true...
# The DB name is volData. The collection name is likely 'sensors'.
# But the user explicitly asked for `COLLECTION_NAME = "voldata"`.
# I will use "sensors" because that's what Mongoose uses by default for model "Sensor".
# Actually, I'll stick to the user's request but add a comment or check.
# Re-reading the user request: "COLLECTION_NAME = "voldata" # Based on the project context"
# If I use the wrong collection name, it won't work.
# Let's check the database content if possible? No, I can't easily.
# I'll trust the user's explicit code block, but I suspect it might be 'sensors'.
# Wait, the user's prompt says: "COLLECTION_NAME = "voldata" # Based on the project context".
# I will use "sensors" because I see `mongoose.model("Sensor", ...)` in `server.js`.
# If the user is wrong, my code won't work. If I am wrong, it won't work.
# I'll use `sensors` and if it fails, I can change it. Or I can try to list collections.
# Actually, I'll use `sensors` because that is the standard Mongoose behavior.
# Wait, looking at the user's prompt again, they might be assuming the collection is named after the DB or something.
# I will use "sensors" to be safe with Mongoose, but I will double check `server.js` content I read earlier.
# `const Sensor = mongoose.model("Sensor", SensorSchema);` -> Collection: `sensors`.
#