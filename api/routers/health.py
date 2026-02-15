"""
Health Check Router
Simple health check endpoint for load balancers and monitoring
"""

from datetime import datetime, timezone

from fastapi import APIRouter

from api.l2_client import get_ailee_client

router = APIRouter()


@router.get("/health")
async def health_check():
    """
    Health Check Endpoint

    Returns the health status of the service.
    This endpoint is designed for load balancers and monitoring systems.
    It also checks the availability of the C++ AILEE-Core node.

    The API can operate in standalone mode when the C++ node is unavailable,
    so the status field reflects the API's health rather than the C++ node's status.
    Use cpp_node_available to check if the C++ node is reachable.

    Returns:
        dict: Health status with timestamp and C++ node availability
    """
    # Check if C++ node is available
    # Wrap in try-except to ensure we always return a valid response
    client = get_ailee_client()
    try:
        cpp_node_available = await client.health_check()
    except Exception:
        # If health check raises an exception, treat node as unavailable
        cpp_node_available = False

    return {
        "status": "healthy",
        "service": "ailee-core-api",
        "cpp_node_available": cpp_node_available,
        "timestamp": datetime.now(timezone.utc).isoformat(),
    }
