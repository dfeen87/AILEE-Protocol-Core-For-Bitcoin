"""
Health Check Router
Simple health check endpoint for load balancers and monitoring
"""

from fastapi import APIRouter
from fastapi.responses import JSONResponse

router = APIRouter()


@router.get("/health")
async def health_check():
    """
    Health Check Endpoint

    Returns the health status of the service.
    Performs a lightweight database readiness probe so that Railway's
    postDeployCommand (and other load-balancer checks) only succeed when
    the full stack is operational.

    This endpoint is intentionally PUBLIC - no authentication required.
    Used by Railway, load balancers, and monitoring systems.

    Returns:
        200 {"status": "ok",      "database": "ok"}      – service is ready
        503 {"status": "degraded","database": "error"}   – database not ready
    """
    from api.database import get_db

    try:
        db = get_db()
        cursor = await db.execute("SELECT 1")
        await cursor.close()
        db_status = "ok"
    except Exception:
        db_status = "error"

    healthy = db_status == "ok"
    return JSONResponse(
        status_code=200 if healthy else 503,
        content={"status": "ok" if healthy else "degraded", "database": db_status},
    )
