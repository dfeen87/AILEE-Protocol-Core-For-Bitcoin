"""
Status Router
Node metadata, version, uptime, and configuration information
"""

import time
from datetime import datetime
from fastapi import APIRouter

from api.config import settings

router = APIRouter()


@router.get("/status")
async def get_status():
    """
    Node Status Endpoint
    
    Returns comprehensive node metadata including:
    - Version information
    - Uptime statistics
    - Configuration summary
    - Environment details
    
    This endpoint provides read-only access to node state.
    
    Returns:
        dict: Node status and metadata
    """
    # Calculate uptime (would need to be tracked from main.py startup)
    # For now, we'll use a placeholder
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
            "uptime_seconds": round(uptime_seconds, 2)
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
        "timestamp": datetime.utcnow().isoformat() + "Z"
    }
