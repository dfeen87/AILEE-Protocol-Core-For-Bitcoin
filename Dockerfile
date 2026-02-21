# Production Dockerfile - AILEE-Core with C++ Node + Python API
FROM ubuntu:22.04 AS cpp-builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake git wget \
    libssl-dev libcurl4-openssl-dev libzmq3-dev \
    libjsoncpp-dev libyaml-cpp-dev librocksdb-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy the full repo
COPY . .

# Build the C++ node (no econ stub needed - you deleted that folder)
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF && \
    make -j$(nproc) ailee_node

# =======================
# Python runtime image
# =======================
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install Python 3.11 and runtime dependencies
# Note: Using Ubuntu 22.04 for both build and runtime stages ensures
# binary compatibility for shared libraries (libjsoncpp25, libyaml-cpp0.7, librocksdb6.11)
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3.11 python3-pip python3.11-venv \
    curl libssl3 libcurl4 libzmq5 libjsoncpp25 \
    libyaml-cpp0.7 librocksdb6.11 libstdc++6 procps \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Set python3.11 as default python
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.11 1 && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3.11 1

WORKDIR /app

# Install Python dependencies
COPY --chown=root:root requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

# Setup user and directories
RUN useradd -m -u 1000 ailee && \
    mkdir -p /app/logs /data && \
    chown -R ailee:ailee /app/logs /data

# Copy application code
COPY --chown=ailee:ailee api/ ./api/
COPY --chown=ailee:ailee web/ ./web/

# Copy C++ node and config from build stage
COPY --from=cpp-builder --chown=ailee:ailee /build/build/ailee_node ./ailee_node
COPY --from=cpp-builder --chown=ailee:ailee /build/config ./config

RUN chmod +x ./ailee_node

# Start script: runs as root (no USER directive) so that it can always write
# to /data even when Railway (or another platform) mounts a volume there at
# runtime with root ownership, overriding the build-time chown.
RUN echo '#!/bin/bash\n\
set -e\n\
# Ensure /data directory exists and is writable.\n\
# Railway mounts volumes as root at runtime, so we must run as root here.\n\
mkdir -p /data\n\
\n\
# Pin the C++ node to port 8181 so it never conflicts with the Railway-assigned\n\
# PORT variable (Railway typically assigns 8080 or a dynamic port for HTTP traffic).\n\
export AILEE_WEB_SERVER_PORT=8181\n\
echo "Starting C++ node on :${AILEE_WEB_SERVER_PORT}..."\n\
./ailee_node > /app/logs/cpp-node.log 2>&1 &\n\
CPP_PID=$!\n\
echo "C++ node PID: $CPP_PID"\n\
sleep 3\n\
if ! kill -0 $CPP_PID 2>/dev/null; then\n\
    echo "WARNING: C++ node failed to start - running in standalone (API-only) mode"\n\
    echo "--- C++ Node Log ---"\n\
    cat /app/logs/cpp-node.log\n\
else\n\
    echo "C++ node started on :${AILEE_WEB_SERVER_PORT}"\n\
fi\n\
\n\
# Python API: bind to PORT provided by Railway; default to 8000 if unset.\n\
API_PORT=${PORT:-8000}\n\
echo "Starting Python API on :${API_PORT} (PORT=${PORT:-unset})..."\n\
export AILEE_PORT=${API_PORT}\n\
export AILEE_NODE_URL="http://localhost:${AILEE_WEB_SERVER_PORT}"\n\
exec uvicorn api.main:app --host 0.0.0.0 --port ${API_PORT} --log-level info' > /app/start.sh && \
    chmod +x /app/start.sh
EXPOSE 8000

# start-period=60s: allows the C++ node (3s sleep) + Python API (db init, etc.) to fully start.
# retries=5 with interval=30s gives 2.5 minutes of retry before marking unhealthy.
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=5 \
    CMD curl -f http://localhost:${PORT:-8000}/health || exit 1

CMD ["/app/start.sh"]
