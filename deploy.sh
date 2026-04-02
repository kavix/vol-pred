#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Configuration ---
# The name of the Docker image to be built.
IMAGE_NAME="vol-pred-app"
# The name of the container to be run.
CONTAINER_NAME="vol-pred-container"
# The port to expose on the host machine.
HOST_PORT=3000
# The port the application runs on inside the container.
CONTAINER_PORT=3000

# --- Deployment Steps ---

echo "🚀 Starting deployment of $IMAGE_NAME..."

# 1. Build the Docker image
echo "Building Docker image: $IMAGE_NAME..."
docker build -t $IMAGE_NAME .
echo "✅ Docker image built successfully."

# 2. Stop and remove any existing container
# Check if a container with the same name is running or exists
if [ $(docker ps -a -q -f name=^/${CONTAINER_NAME}$) ]; then
    echo "Stopping and removing existing container: $CONTAINER_NAME..."
    docker stop $CONTAINER_NAME
    docker rm $CONTAINER_NAME
    echo "✅ Existing container stopped and removed."
fi

# 3. Run the new Docker container
echo "Running new container: $CONTAINER_NAME..."
docker run -d -p $HOST_PORT:$CONTAINER_PORT --name $CONTAINER_NAME $IMAGE_NAME
echo "✅ Container $CONTAINER_NAME is running on port $HOST_PORT."

echo "🎉 Deployment complete!"