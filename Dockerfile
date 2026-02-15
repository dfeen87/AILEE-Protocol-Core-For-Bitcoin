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
FROM python:3.11-slim

# Install minimal runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl libssl3t64 libcurl4t64 libzmq5 libjsoncpp26 \
    libyaml-cpp0.8 librocksdb9.10 libstdc++6 procps \
    ca-certificates \
    && chmod 644 /etc/ssl/certs/ca-certificates.crt \
    && rm -rf /var/lib/apt/lists/*

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

# FIXED: Start script with proper port configuration
RUN echo '#!/bin/bash\n\
set -e\n\
echo "Starting C++ node on :8080..."\n\
./ailee_node > /app/logs/cpp-node.log 2>&1 &\n\
CPP_PID=$!\n\
echo "C++ node PID: $CPP_PID"\n\
sleep 5\n\
if ! kill -0 $CPP_PID 2>/dev/null; then\n\
    echo "ERROR: C++ node failed to start!"\n\
    echo "--- C++ Node Log ---"\n\
    cat /app/logs/cpp-node.log\n\
    exit 1\n\
fi\n\
echo "C++ node started successfully"\n\
echo "Starting Python API on :8000..."\n\
export PORT=8000\n\
export AILEE_NODE_URL="http://localhost:8080"\n\
exec uvicorn api.main:app --host 0.0.0.0 --port 8000 --log-level info' > /app/start.sh && \
    chmod +x /app/start.sh && chown ailee:ailee /app/start.sh

USER ailee
EXPOSE 8000 8080

HEALTHCHECK --interval=30s --timeout=10s --start-period=20s --retries=3 \
    CMD curl -f http://localhost:8000/health || exit 1

CMD ["/app/start.sh"]
