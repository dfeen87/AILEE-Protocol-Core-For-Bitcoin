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

    Returns:
        dict: Health status with timestamp and C++ node availability
    """
    # Check if C++ node is available
    client = get_ailee_client()
    cpp_node_available = await client.health_check()

    return {
        "status": "healthy",
        "service": "ailee-core-api",
        "cpp_node_available": cpp_node_available,
        "timestamp": datetime.now(timezone.utc).isoformat(),
    }
