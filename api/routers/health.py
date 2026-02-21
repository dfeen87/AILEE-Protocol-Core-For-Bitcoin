"""
Health Check Router
Simple health check endpoint for load balancers and monitoring
"""

from fastapi import APIRouter

router = APIRouter()


@router.get("/health")
async def health_check():
    """
    Health Check Endpoint

    Returns the health status of the service.
    This endpoint is intentionally PUBLIC - no authentication required.
    Used by Railway, load balancers, and monitoring systems.

    Returns:
        dict: Simple health status
    """
    return {"status": "ok"}
