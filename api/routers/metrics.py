"""
Metrics Router
Node performance metrics and telemetry
"""

from datetime import datetime
from typing import Dict
from fastapi import APIRouter
from pydantic import BaseModel, Field

router = APIRouter()


class MetricsResponse(BaseModel):
    """Response model for node metrics"""
    node_metrics: Dict[str, float] = Field(..., description="Node performance metrics")
    system_metrics: Dict[str, float] = Field(..., description="System resource metrics")
    l2_metrics: Dict[str, int] = Field(..., description="Layer-2 specific metrics")
    timestamp: str = Field(..., description="Metrics timestamp (ISO 8601)")


def get_current_metrics() -> MetricsResponse:
    """
    Get current node metrics
    In production, this would query actual AILEE-Core metrics
    """
    import psutil
    import os
    
    # Get system metrics
    try:
        cpu_percent = psutil.cpu_percent(interval=0.1)
        memory = psutil.virtual_memory()
        disk = psutil.disk_usage('/')
        
        system_metrics = {
            "cpu_usage_percent": round(cpu_percent, 2),
            "memory_used_percent": round(memory.percent, 2),
            "memory_available_mb": round(memory.available / (1024 * 1024), 2),
            "disk_used_percent": round(disk.percent, 2),
            "disk_available_gb": round(disk.free / (1024 * 1024 * 1024), 2)
        }
    except Exception:
        # Fallback if psutil metrics fail
        system_metrics = {
            "cpu_usage_percent": 0.0,
            "memory_used_percent": 0.0,
            "memory_available_mb": 0.0,
            "disk_used_percent": 0.0,
            "disk_available_gb": 0.0
        }
    
    # Mock node metrics (would come from AILEE-Core)
    node_metrics = {
        "requests_per_second": 12.5,
        "avg_response_time_ms": 45.2,
        "trust_score_computations": 1234.0,
        "validations_performed": 567.0,
        "uptime_hours": 24.5
    }
    
    # Mock L2 metrics
    l2_metrics = {
        "current_block_height": 12345,
        "pending_transactions": 42,
        "anchors_created": 100,
        "state_updates": 5000
    }
    
    return MetricsResponse(
        node_metrics=node_metrics,
        system_metrics=system_metrics,
        l2_metrics=l2_metrics,
        timestamp=datetime.utcnow().isoformat() + "Z"
    )


@router.get("/metrics", response_model=MetricsResponse)
async def get_metrics():
    """
    Get Node Metrics
    
    Returns comprehensive node performance metrics including:
    - Node-specific metrics (requests, computations, validations)
    - System resource metrics (CPU, memory, disk)
    - Layer-2 metrics (blocks, transactions, anchors)
    
    This endpoint is:
    - Read-only (no state modification)
    - Safe (no side effects)
    - Lightweight (minimal overhead)
    
    Returns:
        Current node metrics with timestamp
    """
    return get_current_metrics()
