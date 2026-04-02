# Use a base image that has both Node.js and Python
FROM python:3.11-slim

# Install Node.js
RUN apt-get update && apt-get install -y curl gnupg
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
RUN apt-get install -y nodejs

# Set working directory
WORKDIR /app

# Copy package files first (for caching)
COPY package*.json ./
COPY backend/package*.json ./backend/

# Install Node dependencies
RUN npm install
RUN cd backend && npm install

# Copy Python requirements and install
COPY ml_service/requirements.txt ./ml_service/requirements.txt
RUN pip install -r ml_service/requirements.txt

# Copy the rest of the application code
COPY . .

# Expose the port the app runs on
EXPOSE 3000

# Define environment variables
ENV PORT=3000
ENV PYTHON_EXECUTABLE=python3

# Start the application
CMD ["npm", "start"]
