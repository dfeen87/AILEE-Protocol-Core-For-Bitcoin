# Connecting the REST API to Real L2 Node

## Current Architecture

### What's Deployed Now (Fly.io)
```
┌─────────────────────────────────────┐
│  Python FastAPI Server              │
│  (Dockerfile)                       │
│                                     │
│  - Returns MOCK data                │
│  - No L2 connection                 │
│  - Web dashboard works              │
└─────────────────────────────────────┘
```

### What You Need for Real L2
```
┌──────────────────────┐       ┌─────────────────────────────┐
│  Python FastAPI      │       │  C++ AILEE-Core Node        │
│  REST API            │◄─────►│  (src/main.cpp)             │
│                      │  IPC  │                             │
│  - /status           │       │  - L2State tracking         │
│  - /metrics          │       │  - Bitcoin RPC client       │
│  - /l2/state         │       │  - Orchestration engine     │
│  - /l2/anchors       │       │  - WebServer (port 8080?)   │
└──────────────────────┘       └─────────────────────────────┘
```

## Option 1: Use the C++ Built-in WebServer

The C++ node already has a web server (`src/AILEEWebServer.cpp`). You could:

1. **Deploy the C++ node instead** (use `Dockerfile.node`)
2. **Remove the Python API** (or keep it as a proxy)
3. The C++ node serves real data directly

### To Deploy C++ Node on Fly.io:

Update `fly.toml`:
```toml
[build]
  dockerfile = "Dockerfile.node"  # Use C++ node instead
```

## Option 2: Connect Python API to C++ Node

Keep both systems but connect them via IPC/HTTP:

### Step 1: Modify Python API to Connect to C++ Node

Update `api/routers/l2.py`:
```python
import httpx  # or use requests

# Instead of generate_mock_state_root()
async def get_real_l2_state():
    """Query C++ node for real L2 state"""
    try:
        async with httpx.AsyncClient() as client:
            # Assuming C++ node runs on localhost:9090
            response = await client.get("http://localhost:9090/l2/state")
            return response.json()
    except Exception as e:
        logger.error(f"Failed to connect to C++ node: {e}")
        # Fallback to mock data or return error
        return None
```

### Step 2: Run Both Services

#### Local Development:
```bash
# Terminal 1: Start C++ node
./build/ailee_node

# Terminal 2: Start Python API
uvicorn api.main:app --host 0.0.0.0 --port 8000
```

#### Docker Compose (Recommended):
```yaml
# docker-compose.yml
version: '3.8'
services:
  ailee-node:
    build:
      context: .
      dockerfile: Dockerfile.node
    ports:
      - "9090:9090"
    volumes:
      - ./data:/data
  
  api-server:
    build:
      context: .
      dockerfile: Dockerfile
    ports:
      - "8080:8080"
    depends_on:
      - ailee-node
    environment:
      - AILEE_NODE_URL=http://ailee-node:9090
```

#### Fly.io Multi-Process:
```toml
# fly.toml
[processes]
  node = "./ailee_node"
  api = "uvicorn api.main:app --host 0.0.0.0 --port 8080"

[services]
  internal_port = 8080
  processes = ["api"]
```

## Option 3: Create Integration Layer

Create a new module `api/l2_client.py`:

```python
"""Client to communicate with C++ AILEE-Core node"""
import os
import httpx
import logging
from typing import Optional, Dict, Any

logger = logging.getLogger(__name__)

class AILEECoreClient:
    """Client for C++ AILEE-Core node"""
    
    def __init__(self):
        self.base_url = os.getenv("AILEE_NODE_URL", "http://localhost:9090")
        self.client = httpx.AsyncClient(timeout=5.0)
    
    async def get_l2_state(self) -> Optional[Dict[str, Any]]:
        """Get L2 state from C++ node"""
        try:
            response = await self.client.get(f"{self.base_url}/l2/state")
            response.raise_for_status()
            return response.json()
        except Exception as e:
            logger.error(f"Failed to get L2 state: {e}")
            return None
    
    async def get_metrics(self) -> Optional[Dict[str, Any]]:
        """Get metrics from C++ node"""
        try:
            response = await self.client.get(f"{self.base_url}/metrics")
            response.raise_for_status()
            return response.json()
        except Exception as e:
            logger.error(f"Failed to get metrics: {e}")
            return None
    
    async def close(self):
        await self.client.aclose()

# Singleton instance
_client: Optional[AILEECoreClient] = None

def get_ailee_client() -> AILEECoreClient:
    """Get or create AILEE Core client"""
    global _client
    if _client is None:
        _client = AILEECoreClient()
    return _client
```

Then update `api/routers/l2.py`:
```python
from api.l2_client import get_ailee_client

@router.get("/state", response_model=L2StateResponse)
async def get_l2_state():
    """Get Current Layer-2 State Snapshot"""
    client = get_ailee_client()
    
    # Try to get real data from C++ node
    real_state = await client.get_l2_state()
    
    if real_state:
        # Parse and return real data
        return L2StateResponse(**real_state)
    else:
        # Fallback to mock data (for development)
        logger.warning("Using mock L2 state - C++ node not available")
        state_root = generate_mock_state_root()
        snapshot = L2StateSnapshot(
            state_root=state_root,
            block_height=12345,
            total_transactions=567890,
            last_anchor_height=830000,
            timestamp=datetime.now(timezone.utc).isoformat()
        )
        return L2StateResponse(state=snapshot, health="mock_mode")
```

## What You Need to Do Now

### For Real L2 Production:

1. **Choose your architecture** (Option 1, 2, or 3 above)

2. **Build and test C++ node locally**:
   ```bash
   mkdir -p build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ./ailee_node
   ```

3. **Configure Bitcoin RPC connection**:
   - Set up Bitcoin Core node (or connect to existing)
   - Configure RPC credentials in `config/` files
   - Never commit real credentials!

4. **Update API to connect to C++ node** (using code above)

5. **Deploy both services** (use docker-compose or Fly.io multi-process)

6. **Monitor and verify** real L2 state is being tracked

### For Now (Development/Demo):

✅ **Your current setup is FINE for:**
- Demonstrating the UI/UX
- Testing API endpoints
- Showing the architecture
- Development and iteration

❌ **Your current setup CANNOT:**
- Track real Bitcoin L2 state
- Process actual transactions
- Create Bitcoin anchors
- Run orchestration tasks

## Summary

**Question: "Is my global node working correctly now to real L2?"**

**Answer: NO - You're currently only running the Python API with mock data.**

**To get real L2 functionality:**
1. Build the C++ AILEE-Core node (`Dockerfile.node`)
2. Connect it to a Bitcoin node
3. Connect the Python API to the C++ node (or use C++ node directly)
4. Deploy both services together

The fixes I made ensure the **web dashboard works correctly** when you eventually connect to real data, but right now it's displaying mock/fake data from the Python API.
