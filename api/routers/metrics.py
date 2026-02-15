"""
Metrics Router
Node performance metrics and telemetry with Prometheus support
"""

from datetime import datetime, timezone
from typing import Dict
from fastapi import APIRouter
from fastapi.responses import PlainTextResponse
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
        timestamp=datetime.now(timezone.utc).isoformat()
    )


def get_prometheus_metrics() -> str:
    """
    Generate Prometheus-compatible metrics
    In production, this would use the C++ PrometheusExporter
    """
    import psutil
    import time
    
    # Get current metrics
    metrics = get_current_metrics()
    
    # Build Prometheus text format
    lines = []
    
    # System metrics
    lines.append("# HELP ailee_cpu_usage_percent CPU usage percentage")
    lines.append("# TYPE ailee_cpu_usage_percent gauge")
    lines.append(f"ailee_cpu_usage_percent {metrics.system_metrics['cpu_usage_percent']}")
    lines.append("")
    
    lines.append("# HELP ailee_memory_usage_percent Memory usage percentage")
    lines.append("# TYPE ailee_memory_usage_percent gauge")
    lines.append(f"ailee_memory_usage_percent {metrics.system_metrics['memory_used_percent']}")
    lines.append("")
    
    lines.append("# HELP ailee_memory_available_bytes Available memory in bytes")
    lines.append("# TYPE ailee_memory_available_bytes gauge")
    lines.append(f"ailee_memory_available_bytes {metrics.system_metrics['memory_available_mb'] * 1024 * 1024}")
    lines.append("")
    
    # Node metrics
    lines.append("# HELP ailee_requests_per_second Request rate")
    lines.append("# TYPE ailee_requests_per_second gauge")
    lines.append(f"ailee_requests_per_second {metrics.node_metrics['requests_per_second']}")
    lines.append("")
    
    lines.append("# HELP ailee_response_time_milliseconds Average response time")
    lines.append("# TYPE ailee_response_time_milliseconds gauge")
    lines.append(f"ailee_response_time_milliseconds {metrics.node_metrics['avg_response_time_ms']}")
    lines.append("")
    
    lines.append("# HELP ailee_trust_computations_total Total trust score computations")
    lines.append("# TYPE ailee_trust_computations_total counter")
    lines.append(f"ailee_trust_computations_total {metrics.node_metrics['trust_score_computations']}")
    lines.append("")
    
    lines.append("# HELP ailee_validations_total Total validations performed")
    lines.append("# TYPE ailee_validations_total counter")
    lines.append(f"ailee_validations_total {metrics.node_metrics['validations_performed']}")
    lines.append("")
    
    # L2 metrics
    lines.append("# HELP ailee_block_height Current block height")
    lines.append("# TYPE ailee_block_height gauge")
    lines.append(f"ailee_block_height {metrics.l2_metrics['current_block_height']}")
    lines.append("")
    
    lines.append("# HELP ailee_pending_transactions Pending transactions")
    lines.append("# TYPE ailee_pending_transactions gauge")
    lines.append(f"ailee_pending_transactions {metrics.l2_metrics['pending_transactions']}")
    lines.append("")
    
    lines.append("# HELP ailee_anchors_created_total Total anchors created")
    lines.append("# TYPE ailee_anchors_created_total counter")
    lines.append(f"ailee_anchors_created_total {metrics.l2_metrics['anchors_created']}")
    lines.append("")
    
    lines.append("# HELP ailee_state_updates_total Total state updates")
    lines.append("# TYPE ailee_state_updates_total counter")
    lines.append(f"ailee_state_updates_total {metrics.l2_metrics['state_updates']}")
    lines.append("")
    
    return "\n".join(lines)


@router.get("/metrics", response_model=MetricsResponse)
async def get_metrics():
    """
    Get Node Metrics (JSON format)
    
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


@router.get("/metrics/prometheus", response_class=PlainTextResponse)
async def get_prometheus_metrics_endpoint():
    """
    Get Node Metrics (Prometheus format)
    
    Returns metrics in Prometheus text exposition format for scraping.
    This endpoint is compatible with Prometheus monitoring systems.
    
    In production, this would call the C++ PrometheusExporter to get
    real-time metrics from the AILEE-Core node.
    
    Returns:
        Metrics in Prometheus text format
    """
    return get_prometheus_metrics()
