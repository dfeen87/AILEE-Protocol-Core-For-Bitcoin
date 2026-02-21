#!/bin/bash
set -e

# Diagnostics
echo "--- Post-Deploy Diagnostics ---"
ulimit -a
free -m
df -h
echo "-----------------------------"

# Start application in background
echo "Starting application for health check..."
export PORT=${PORT:-8080}
# run in background
/app/start.sh &
APP_PID=$!

echo "Application started with PID $APP_PID. Waiting for health..."

# Poll for health
RETRIES=30
for ((i=1; i<=RETRIES; i++)); do
    # curl -f fails on 404/500. -s silent.
    # We suppress output of curl, but not the error if any?
    if curl -sf http://localhost:$PORT/health > /dev/null; then
        echo "✅ Health check passed!"
        # We need to cleanup.
        # Since we didn't use exec, we can kill the background job.
        kill $APP_PID || true
        exit 0
    fi
    echo "Health check attempt $i/$RETRIES..."
    sleep 2
done

echo "❌ Health check failed."
kill $APP_PID || true
exit 1
