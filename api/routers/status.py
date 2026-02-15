"""
Status Router
Node metadata, version, uptime, and configuration information
"""

import time
from datetime import datetime, timezone
from fastapi import APIRouter

from api.config import settings
from api.l2_client import get_ailee_client

router = APIRouter()


@router.get("/status")
async def get_status():
    """
    Node Status Endpoint
    
    Returns comprehensive node metadata from the C++ AILEE-Core node
    
    Returns:
        dict: Node status and metadata
    """
    client = get_ailee_client()
    
    # Try to get real status from C++ node
    cpp_status = await client.get_status()
    
    if cpp_status:
        # Return real status from C++ node
        return cpp_status
    
    # Fallback: C++ node not available, return Python API status only
    from api.main import get_startup_time
    startup_time = get_startup_time()
    
    uptime_seconds = 0
    if startup_time:
        uptime_seconds = time.time() - startup_time
    
    return {
        "node": {
            "id": settings.node_id,
            "version": settings.app_version,
            "environment": settings.env,
            "uptime_seconds": round(uptime_seconds, 2),
            "mode": "api_only"  # Indicates C++ node not connected
        },
        "config": {
            "jwt_enabled": settings.jwt_enabled,
            "cors_enabled": settings.cors_enabled,
            "rate_limit_enabled": settings.rate_limit_enabled,
            "log_level": settings.log_level
        },
        "api": {
            "name": settings.app_name,
            "description": settings.app_description,
            "host": settings.host,
            "port": settings.port
        },
        "warning": "C++ AILEE-Core node not available - API running in standalone mode",
        "timestamp": datetime.now(timezone.utc).isoformat()
    }
