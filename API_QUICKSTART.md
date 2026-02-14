# AILEE-Core REST API - Quick Start Guide

## 🚀 Local Development

### Run Locally (Python)
```bash
# Install dependencies
pip install -r requirements.txt

# Run the API
uvicorn api.main:app --host 0.0.0.0 --port 8080 --reload

# Access the API
curl http://localhost:8080/health

# View documentation
open http://localhost:8080/docs
```

### Run with Docker
```bash
# Build the image
docker build -t ailee-core-api .

# Run the container
docker run -p 8080:8080 ailee-core-api

# Test the API
curl http://localhost:8080/health
```

## ☁️ Deploy to Fly.io

### First-time Setup
```bash
# Install Fly CLI
curl -L https://fly.io/install.sh | sh

# Login to Fly.io
fly auth login

# Launch the app (first time)
fly launch

# Set secrets
fly secrets set AILEE_NODE_ID=my-node-1
fly secrets set AILEE_JWT_SECRET=$(openssl rand -base64 32)
```

### Deploy Updates
```bash
# Deploy changes
fly deploy

# Check status
fly status

# View logs
fly logs

# Open in browser
fly open
```

### Scale
```bash
# Scale instances
fly scale count 2

# Scale memory
fly scale memory 512
```

## 🔧 Configuration

Copy `.env.example` to `.env` and customize:

```bash
AILEE_NODE_ID=my-node-1
AILEE_ENV=production
AILEE_LOG_LEVEL=info
AILEE_JWT_ENABLED=false
```

## 📊 API Endpoints

- `GET /` - API information
- `GET /health` - Health check
- `GET /status` - Node status
- `POST /trust/score` - Compute trust score
- `POST /trust/validate` - Validate output
- `GET /l2/state` - L2 state snapshot
- `GET /l2/anchors` - Anchor history
- `GET /metrics` - Node metrics
- `GET /docs` - OpenAPI documentation

## 🧪 Testing

```bash
# Health check
curl http://localhost:8080/health

# Node status
curl http://localhost:8080/status

# Trust score
curl -X POST http://localhost:8080/trust/score \
  -H "Content-Type: application/json" \
  -d '{"input_data": "test"}'

# L2 state
curl http://localhost:8080/l2/state

# Metrics
curl http://localhost:8080/metrics
```

## 📚 Documentation

- OpenAPI/Swagger: `http://localhost:8080/docs`
- ReDoc: `http://localhost:8080/redoc`
- OpenAPI JSON: `http://localhost:8080/openapi.json`

## 🔒 Security

- JWT auth disabled by default for easy testing
- Enable in production: `AILEE_JWT_ENABLED=true`
- Rate limiting: 100 requests per 60 seconds
- Non-root Docker user
- Health check endpoint for monitoring

## 💡 Tips

1. Use environment variables for configuration
2. Enable JWT auth in production
3. Monitor health check endpoint
4. Review logs regularly
5. Start with minimal resources (256MB RAM)
6. Scale as needed based on traffic
