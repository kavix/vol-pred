# FastAPI + ML service container
FROM python:3.11-slim

WORKDIR /app

# Install Python dependencies first for better caching
COPY ml_service/requirements.txt ./ml_service/requirements.txt
RUN pip install --no-cache-dir -r ml_service/requirements.txt

# Copy the rest of the application code
COPY . .

EXPOSE 3000
ENV PORT=3000

CMD ["sh", "-c", "uvicorn app:app --host 0.0.0.0 --port ${PORT:-3000}"]
