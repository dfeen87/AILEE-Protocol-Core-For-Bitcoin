"""
Health Check Router
Simple health check endpoint for load balancers and monitoring
"""

from fastapi import APIRouter
from datetime import datetime

router = APIRouter()


@router.get("/health")
async def health_check():
    """
    Health Check Endpoint
    
    Returns the health status of the service.
    This endpoint is designed for load balancers and monitoring systems.
    
    Returns:
        dict: Health status with timestamp
    """
    return {
        "status": "healthy",
        "service": "ailee-core-api",
        "timestamp": datetime.utcnow().isoformat() + "Z"
    }
