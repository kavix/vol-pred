# Use a base image that has both Node.js and Python
FROM python:3.11-slim

# Install Node.js
RUN apt-get update && apt-get install -y curl gnupg && \
    curl -fsSL https://deb.nodesource.com/setup_20.x | bash - && \
    apt-get install -y nodejs

# Set working directory
WORKDIR /app

# Install Python dependencies first for better caching
COPY ml_service/requirements.txt ./ml_service/requirements.txt
RUN pip install --no-cache-dir -r ml_service/requirements.txt

# Install Node.js dependencies
COPY package.json ./
COPY backend/package.json ./backend/
# In case lock files exist
COPY package-lock.json ./
COPY backend/package-lock.json ./backend/
RUN npm install
RUN npm install --prefix backend

# Copy the rest of the application code
COPY . .

# Expose the port the app runs on
EXPOSE 3000

# Define environment variables
ENV PORT=3000
ENV PYTHON_EXECUTABLE=python3

# Start the application
CMD ["npm", "start"]
CMD ["node", "server.js"]
