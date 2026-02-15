# Fly.io Deployment Fixes

## Problem Statement
The AILEE Protocol Core application at https://ailee-protocol-core-for-bitcoin.fly.dev/ was not working due to missing deployment configuration.

## Root Causes Identified

1. **Missing Dockerfile reference in fly.toml**: The `[build]` section was empty, so Fly.io didn't know how to build the application.

2. **Library version mismatch**: The Dockerfile used different OS bases for build (Ubuntu 22.04) and runtime (Debian-based python:3.11-slim), causing shared library incompatibility (libjsoncpp, libyaml-cpp, librocksdb).

3. **Incorrect environment variable**: The startup script used `PORT=8000` but the FastAPI config expects `AILEE_PORT=8000` due to the `AILEE_` prefix in pydantic-settings.

4. **Outdated documentation**: Documentation showed incorrect port numbers (8080 instead of 8000).

## Changes Made

### 1. fly.toml
```toml
[build]
  dockerfile = "Dockerfile"
```
Added the Dockerfile reference so Fly.io knows to use Docker for builds.

### 2. Dockerfile
- Changed runtime base from `python:3.11-slim` to `ubuntu:22.04`
- Added Python 3.11 installation in runtime stage
- Fixed environment variable from `PORT` to `AILEE_PORT`
- Added documentation comment explaining library compatibility requirements

### 3. Documentation
- Updated README.md with correct port (8000)
- Updated API_QUICKSTART.md with correct port (8000)
- Added web dashboard access instructions

## How to Deploy

### First Time Setup
```bash
# Install Fly CLI if not already installed
curl -L https://fly.io/install.sh | sh

# Login to Fly.io
fly auth login

# Deploy (uses existing fly.toml configuration)
fly deploy
```

### Subsequent Deployments
```bash
# Deploy updates
fly deploy

# View deployment status
fly status

# View application logs
fly logs

# Open in browser
fly open
```

## Testing Locally

```bash
# Build Docker image
docker build -t ailee-core-api .

# Run container
docker run -p 8000:8000 ailee-core-api

# Test health endpoint
curl http://localhost:8000/health

# Access web dashboard
open http://localhost:8000/
```

## Expected Behavior

Once deployed, the application should:
- ✅ Respond to health checks at `/health`
- ✅ Serve the web dashboard at `/`
- ✅ Provide API documentation at `/docs`
- ✅ Return status information at `/status`
- ✅ Expose all Layer-2 Bitcoin protocol endpoints

## Architecture

The deployed application consists of:
1. **C++ Core Node**: Runs the AILEE Layer-2 protocol logic (completes and exits)
2. **Python FastAPI Server**: Provides REST API and web dashboard on port 8000
3. **Web Dashboard**: Static HTML/CSS/JS served at the root endpoint

## Security Notes

- The application runs in production mode by default
- CORS is enabled for web access
- Rate limiting is active (100 requests per 60 seconds)
- JWT authentication is disabled by default (can be enabled via secrets)
- All secrets should be set via `fly secrets set` command, never committed to code

## Troubleshooting

If the deployment fails:

1. **Check build logs**: `fly logs`
2. **Verify Dockerfile builds locally**: `docker build -t test .`
3. **Check health endpoint**: The health check endpoint must respond within 10 seconds
4. **Verify port configuration**: Ensure internal_port in fly.toml matches EXPOSE in Dockerfile (8000)

## Next Steps

After deployment, you may want to:
- Set up custom domain: `fly certs add yourdomain.com`
- Configure persistent storage: `fly volumes create ailee_data --size 10`
- Scale up instances: `fly scale count 2`
- Increase memory: `fly scale memory 2048`
- Set secrets for Bitcoin RPC: `fly secrets set BITCOIN_RPC_USER=... BITCOIN_RPC_PASSWORD=...`

## Resources

- [Fly.io Documentation](https://fly.io/docs/)
- [AILEE Protocol Documentation](./README.md)
- [API Quick Start Guide](./API_QUICKSTART.md)
- [Production Deployment Guide](./docs/PRODUCTION_DEPLOYMENT.md)
